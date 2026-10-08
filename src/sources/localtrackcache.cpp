#include "sources/localtrackcache.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QStorageInfo>
#include <QThread>
#include <QWaitCondition>
#include <QtGlobal>
#include <QDeadlineTimer>
#include <limits>
#include <algorithm>
#include <map>
#include <memory>

#include "sources/externalcontent.h"
#include "util/logger.h"

namespace djmantra {

namespace {

const mixxx::Logger kLogger("LocalTrackCache");

constexpr qint64 kCopyBlockSize = 1024 * 1024;
const QString kPartSuffix = QStringLiteral(".part");
const QString kSourceInfoSuffix = QStringLiteral(".src");

QMutex s_settingsMutex;
LocalTrackCache::Settings s_settings{QString(), LocalTrackCache::Mode::Off};

// One lock per cached file, so that two decks loading the same file wait for
// one copy, while different files are copied in parallel.
QMutex s_fileLocksMutex;
std::map<QString, std::weak_ptr<QMutex>> s_fileLocks;

// Eviction and size accounting
QMutex s_directoryMutex;

std::shared_ptr<QMutex> fileLock(const QString& key) {
    const auto locker = QMutexLocker(&s_fileLocksMutex);
    auto& weak = s_fileLocks[key];
    auto pLock = weak.lock();
    if (!pLock) {
        pLock = std::make_shared<QMutex>();
        weak = pLock;
    }
    // Drop entries that nobody holds any more
    for (auto it = s_fileLocks.begin(); it != s_fileLocks.end();) {
        if (it->second.expired()) {
            it = s_fileLocks.erase(it);
        } else {
            ++it;
        }
    }
    return pLock;
}

QString normalizedPath(const QString& path) {
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool startsWithPrefix(const QString& path, const QString& prefix) {
    if (prefix.isEmpty()) {
        return false;
    }
    QString dir = QDir::cleanPath(prefix);
    if (!dir.endsWith(QChar('/'))) {
        dir += QChar('/');
    }
    return path.startsWith(dir);
}

struct SourceInfo {
    qint64 size = -1;
    qint64 modifiedMs = -1;
    QString path;
};

SourceInfo sourceInfoOf(const QFileInfo& fileInfo) {
    return SourceInfo{fileInfo.size(),
            fileInfo.lastModified().toMSecsSinceEpoch(),
            fileInfo.absoluteFilePath()};
}

bool readSourceInfo(const QString& copy, SourceInfo* pInfo) {
    QFile file(copy + kSourceInfoSuffix);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QChar('\n'));
    if (lines.size() < 3) {
        return false;
    }
    bool sizeOk = false;
    bool timeOk = false;
    pInfo->size = lines[0].toLongLong(&sizeOk);
    pInfo->modifiedMs = lines[1].toLongLong(&timeOk);
    pInfo->path = lines[2];
    return sizeOk && timeOk;
}

bool writeSourceInfo(const QString& copy, const SourceInfo& info) {
    QFile file(copy + kSourceInfoSuffix);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    const QString text = QStringLiteral("%1\n%2\n%3\n")
                                 .arg(info.size)
                                 .arg(info.modifiedMs)
                                 .arg(info.path);
    return file.write(text.toUtf8()) > 0;
}

/// Mark a copy as just used, for the least-recently-used eviction.
void touch(const QString& copy) {
    QFile file(copy);
    if (file.open(QIODevice::ReadWrite)) {
        file.setFileTime(QDateTime::currentDateTime(), QFileDevice::FileModificationTime);
    }
}

void removeCopy(const QString& copy) {
    QFile::remove(copy);
    QFile::remove(copy + kSourceInfoSuffix);
}

/// A copy file in the cache directory (not a .part or .src file).
bool isCopyFile(const QFileInfo& fileInfo) {
    const QString name = fileInfo.fileName();
    return fileInfo.isFile() && !name.endsWith(kPartSuffix) &&
            !name.endsWith(kSourceInfoSuffix);
}

qint64 usedBytesLocked(const QString& directory) {
    qint64 total = 0;
    const auto entries = QDir(directory).entryInfoList(QDir::Files);
    for (const auto& entry : entries) {
        if (isCopyFile(entry)) {
            total += entry.size();
        }
    }
    return total;
}

/// Delete the oldest copies until `incomingBytes` fits.
/// Copies that are open for reading stay readable until closed (unlinked
/// files on Linux/Android), so a playing deck is never affected.
bool makeRoom(const LocalTrackCache::Settings& settings,
        qint64 incomingBytes,
        const QString& keepCopy) {
    const auto locker = QMutexLocker(&s_directoryMutex);
    QDir dir(settings.directory);
    // Leftovers of interrupted copies
    const auto parts = dir.entryInfoList({QStringLiteral("*") + kPartSuffix}, QDir::Files);
    for (const auto& part : parts) {
        if (part.absoluteFilePath() != keepCopy + kPartSuffix &&
                part.lastModified().secsTo(QDateTime::currentDateTime()) > 3600) {
            QFile::remove(part.absoluteFilePath());
        }
    }
    auto entries = dir.entryInfoList(QDir::Files, QDir::Time | QDir::Reversed);
    entries.erase(std::remove_if(entries.begin(),
                          entries.end(),
                          [](const QFileInfo& entry) { return !isCopyFile(entry); }),
            entries.end());
    qint64 used = 0;
    for (const auto& entry : std::as_const(entries)) {
        used += entry.size();
    }
    auto freeBytes = [&]() {
        QStorageInfo storage(settings.directory);
        storage.refresh();
        return storage.isValid() ? storage.bytesAvailable() : std::numeric_limits<qint64>::max();
    };
    // Oldest first (QDir::Time sorts newest first, Reversed flips it)
    for (const auto& entry : std::as_const(entries)) {
        const bool fitsQuota = used + incomingBytes <= settings.maxBytes;
        const bool fitsDisk = freeBytes() - incomingBytes >= settings.reserveBytes;
        if (fitsQuota && fitsDisk) {
            return true;
        }
        if (entry.absoluteFilePath() == keepCopy) {
            continue;
        }
        kLogger.info() << "Removing least recently used copy" << entry.fileName();
        used -= entry.size();
        removeCopy(entry.absoluteFilePath());
    }
    return used + incomingBytes <= settings.maxBytes &&
            freeBytes() - incomingBytes >= settings.reserveBytes;
}

bool waitFor(const std::function<bool()>& condition,
        const LocalTrackCache::Settings& settings,
        const std::function<bool()>& isCancelled) {
    QDeadlineTimer deadline(settings.reconnectTimeout);
    while (!condition()) {
        if (deadline.hasExpired() || (isCancelled && isCancelled())) {
            return false;
        }
        QThread::msleep(static_cast<unsigned long>(settings.pollInterval.count()));
    }
    return true;
}

bool sameSource(const SourceInfo& a, const SourceInfo& b) {
    return a.size == b.size && a.modifiedMs == b.modifiedMs;
}

} // namespace

QStringList LocalTrackCache::defaultRemovablePrefixes() {
#if defined(Q_OS_ANDROID)
    // USB drives and SD cards: /storage/<volume id>/
    return {QStringLiteral("/storage/"), QStringLiteral("/mnt/media_rw/")};
#elif defined(Q_OS_MACOS)
    return {QStringLiteral("/Volumes/")};
#elif defined(Q_OS_WIN)
    return {};
#else
    return {QStringLiteral("/media/"), QStringLiteral("/run/media/"), QStringLiteral("/mnt/")};
#endif
}

QStringList LocalTrackCache::defaultExcludedPrefixes() {
#if defined(Q_OS_ANDROID)
    // Internal shared storage
    return {QStringLiteral("/storage/emulated/"), QStringLiteral("/storage/self/")};
#else
    return {};
#endif
}

void LocalTrackCache::configure(const Settings& settings) {
    Settings newSettings = settings;
    if (!newSettings.directory.isEmpty()) {
        newSettings.directory = normalizedPath(newSettings.directory);
        if (!QDir().mkpath(newSettings.directory)) {
            kLogger.warning() << "Cannot create cache directory" << newSettings.directory;
            newSettings.mode = Mode::Off;
        }
    }
    const auto locker = QMutexLocker(&s_settingsMutex);
    s_settings = newSettings;
    kLogger.info() << "Mode" << static_cast<int>(newSettings.mode)
                   << "directory" << newSettings.directory
                   << "max MB" << newSettings.maxBytes / (1024 * 1024);
}

LocalTrackCache::Settings LocalTrackCache::settings() {
    const auto locker = QMutexLocker(&s_settingsMutex);
    return s_settings;
}

bool LocalTrackCache::isEnabled() {
    const Settings current = settings();
    return current.mode != Mode::Off && !current.directory.isEmpty();
}

bool LocalTrackCache::isDriveReachable(const QString& driveRoot) {
    const QString volumeId = ExternalContent::volumeIdOfDirectory(driveRoot);
    if (!volumeId.isEmpty()) {
        // Placeholders of a drive read through the Storage Access Framework
        return ExternalContent::isVolumeMounted(volumeId);
    }
    return QFileInfo(driveRoot).isDir();
}

QString LocalTrackCache::driveRoot(const QString& path) {
    const Settings current = settings();
    const QString clean = normalizedPath(path);
    {
        // Placeholders: the drive is <mirror>/<volume id>
        QString volumeId;
        if (ExternalContent::splitPlaceholder(clean, &volumeId, nullptr)) {
            return ExternalContent::volumeDirectory(volumeId);
        }
    }
    for (const auto& excluded : std::as_const(current.excludedPrefixes)) {
        if (startsWithPrefix(clean, excluded)) {
            return {};
        }
    }
    for (const auto& prefix : std::as_const(current.removablePrefixes)) {
        if (!startsWithPrefix(clean, prefix)) {
            continue;
        }
        QString base = QDir::cleanPath(prefix);
        if (!base.endsWith(QChar('/'))) {
            base += QChar('/');
        }
        const QString rest = clean.mid(base.size());
        const qsizetype slash = rest.indexOf(QChar('/'));
        if (slash <= 0) {
            return {}; // the drive directory itself, or a file directly in the prefix
        }
        const QString first = base + rest.left(slash);
        // Linux mounts drives at /media/<user>/<label> (and /run/media/...):
        // a directory there that is not a mount point is a user directory.
        if (base.endsWith(QStringLiteral("media/")) && QFileInfo(first).isDir() &&
                QStorageInfo(first).rootPath() != first) {
            const QString afterUser = rest.mid(slash + 1);
            const qsizetype secondSlash = afterUser.indexOf(QChar('/'));
            if (secondSlash <= 0) {
                return {};
            }
            return first + QChar('/') + afterUser.left(secondSlash);
        }
        return first;
    }
    return {};
}

bool LocalTrackCache::isOnUnreachableDrive(
        const QString& path, const QStringList& libraryRoots) {
    const QString clean = normalizedPath(path);
    for (const auto& root : libraryRoots) {
        const QString cleanRoot = normalizedPath(root);
        if ((clean == cleanRoot || startsWithPrefix(clean, cleanRoot)) &&
                !QFileInfo(cleanRoot).isDir()) {
            return true;
        }
    }
    const QString drive = driveRoot(clean);
    return !drive.isEmpty() && !isDriveReachable(drive);
}

bool LocalTrackCache::shouldCache(const QString& path) {
    const Settings current = settings();
    if (current.mode == Mode::Off || current.directory.isEmpty() || path.isEmpty()) {
        return false;
    }
    const QString clean = normalizedPath(path);
    if (startsWithPrefix(clean, current.directory)) {
        return false; // already a copy
    }
    if (current.mode == Mode::All) {
        return true;
    }
    return !driveRoot(clean).isEmpty();
}

QString LocalTrackCache::copyPath(const QString& path) {
    const Settings current = settings();
    if (current.directory.isEmpty()) {
        return {};
    }
    const QString clean = normalizedPath(path);
    const QByteArray hash =
            QCryptographicHash::hash(clean.toUtf8(), QCryptographicHash::Sha1).toHex();
    // Keep the extension: the decoder is chosen by it
    const QString suffix = QFileInfo(clean).suffix().toLower();
    QString name = QString::fromLatin1(hash);
    if (!suffix.isEmpty()) {
        name += QChar('.') + suffix;
    }
    return QDir(current.directory).filePath(name);
}

QString LocalTrackCache::readPath(const QString& path) {
    if (!shouldCache(path)) {
        return path;
    }
    const QString copy = copyPath(path);
    SourceInfo cached;
    if (!QFileInfo::exists(copy) || !readSourceInfo(copy, &cached)) {
        return path;
    }
    const QFileInfo source(path);
    if (source.exists() && !sameSource(sourceInfoOf(source), cached)) {
        return path; // the original was changed: the copy is stale
    }
    // Unchanged, or the drive is not reachable right now
    return copy;
}

LocalTrackCache::Result LocalTrackCache::ensureCached(
        const QString& path, const std::function<bool()>& isCancelled) {
    if (!shouldCache(path)) {
        return Result{path, false, QString()};
    }
    const Settings current = settings();
    const QString copy = copyPath(path);
    const auto pLock = fileLock(copy);
    const auto locker = QMutexLocker(pLock.get());

    SourceInfo cached;
    const bool haveCopy = QFileInfo::exists(copy) && readSourceInfo(copy, &cached);

    const QString drive = driveRoot(path);
    // The file can be read: it exists and its drive is there (a placeholder
    // always exists, its drive is there when the volume is mounted).
    const auto available = [&] {
        return QFileInfo::exists(path) && (drive.isEmpty() || isDriveReachable(drive));
    };
    QFileInfo source(path);
    if (!available()) {
        if (haveCopy) {
            kLogger.info() << "Drive not reachable, using the local copy of" << path;
            touch(copy);
            return Result{copy, true, QString()};
        }
        // A drive that dropped out for a moment: wait for it. A missing file
        // on a drive that is there fails at once.
        const bool driveMissing = !drive.isEmpty() && !isDriveReachable(drive);
        if (!driveMissing || !waitFor(available, current, isCancelled)) {
            return Result{path, false, QStringLiteral("File not found")};
        }
        source = QFileInfo(path);
    }
    const SourceInfo sourceInfo = sourceInfoOf(source);
    if (haveCopy && sameSource(sourceInfo, cached)) {
        touch(copy);
        return Result{copy, true, QString()};
    }
    if (haveCopy) {
        removeCopy(copy); // stale
    }
    if (sourceInfo.size > current.maxBytes) {
        kLogger.warning() << "File is larger than the cache, playing it from the drive:" << path;
        return Result{path, false, QStringLiteral("File is larger than the cache")};
    }
    if (!makeRoom(current, sourceInfo.size, copy)) {
        kLogger.warning() << "Not enough internal storage to copy" << path;
        return Result{path, false, QStringLiteral("Not enough internal storage")};
    }

    const QString part = copy + kPartSuffix;
    QFile target(part);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return Result{path, false, target.errorString()};
    }
    // Placeholders are read through their content reader
    std::unique_ptr<ExternalContent::Reader> pReader;
    QFile input;
    qint64 copied = 0;
    QByteArray buffer;
    buffer.resize(kCopyBlockSize);
    int reconnects = 0;
    while (copied < sourceInfo.size) {
        if (isCancelled && isCancelled()) {
            target.close();
            QFile::remove(part);
            return Result{path, false, QStringLiteral("Cancelled")};
        }
        if (!input.isOpen()) {
            pReader = std::make_unique<ExternalContent::Reader>(path);
            input.setFileName(pReader->path());
            if (pReader->path().isEmpty() || !input.open(QIODevice::ReadOnly) ||
                    !input.seek(copied)) {
                input.close();
                pReader.reset();
                // The drive dropped out: wait until the same file is back
                const bool back = waitFor(
                        [&] {
                            return available() &&
                                    sameSource(sourceInfoOf(QFileInfo(path)), sourceInfo);
                        },
                        current,
                        isCancelled);
                if (!back) {
                    target.close();
                    QFile::remove(part);
                    kLogger.warning() << "Drive did not come back, copy failed:" << path;
                    return Result{path, false, QStringLiteral("Drive not reachable")};
                }
                ++reconnects;
                kLogger.info() << "Drive is back, continuing the copy at" << copied
                               << "bytes:" << path;
                continue;
            }
        }
        const qint64 wanted = std::min(kCopyBlockSize, sourceInfo.size - copied);
        const qint64 read = input.read(buffer.data(), wanted);
        if (read <= 0) {
            // Read error or unexpected end: reopen (after the drive is back)
            input.close();
            pReader.reset();
            if (!available()) {
                continue; // the wait happens when reopening
            }
            const QFileInfo again(path);
            if (!sameSource(sourceInfoOf(again), sourceInfo)) {
                target.close();
                QFile::remove(part);
                return Result{path, false, QStringLiteral("File changed while copying")};
            }
            if (++reconnects > 100) {
                target.close();
                QFile::remove(part);
                return Result{path, false, QStringLiteral("Read error")};
            }
            QThread::msleep(static_cast<unsigned long>(current.pollInterval.count()));
            continue;
        }
        if (target.write(buffer.constData(), read) != read) {
            const QString error = target.errorString();
            target.close();
            QFile::remove(part);
            return Result{path, false, error};
        }
        copied += read;
    }
    input.close();
    pReader.reset();
    if (!target.flush()) {
        target.close();
        QFile::remove(part);
        return Result{path, false, QStringLiteral("Write error")};
    }
    target.close();
    QFile::remove(copy);
    if (!QFile::rename(part, copy) || !writeSourceInfo(copy, sourceInfo)) {
        removeCopy(copy);
        QFile::remove(part);
        return Result{path, false, QStringLiteral("Cannot store the copy")};
    }
    touch(copy);
    kLogger.info() << "Copied" << sourceInfo.size / 1024 << "KiB to internal storage"
                   << (reconnects > 0 ? QStringLiteral("(drive reconnected %1x)").arg(reconnects)
                                      : QString())
                   << path;
    return Result{copy, true, QString()};
}

qint64 LocalTrackCache::usedBytes() {
    const Settings current = settings();
    if (current.directory.isEmpty()) {
        return 0;
    }
    const auto locker = QMutexLocker(&s_directoryMutex);
    return usedBytesLocked(current.directory);
}

void LocalTrackCache::clear() {
    const Settings current = settings();
    if (current.directory.isEmpty()) {
        return;
    }
    const auto locker = QMutexLocker(&s_directoryMutex);
    const auto entries = QDir(current.directory).entryInfoList(QDir::Files);
    for (const auto& entry : entries) {
        QFile::remove(entry.absoluteFilePath());
    }
}

} // namespace djmantra
