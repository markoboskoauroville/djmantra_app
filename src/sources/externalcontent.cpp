#include "sources/externalcontent.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QAtomicInt>
#include <QSet>

#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#include <unistd.h>
#endif

#include "util/logger.h"

namespace djmantra {

namespace {

const mixxx::Logger kLogger("ExternalContent");

QMutex s_mutex;
std::shared_ptr<ExternalContentProvider> s_pProvider;
QString s_mirrorDirectory;
QAtomicInt s_linkCounter;

std::shared_ptr<ExternalContentProvider> provider() {
    const auto locker = QMutexLocker(&s_mutex);
    return s_pProvider;
}

bool hasSupportedSuffix(const QString& path, const QStringList& suffixes) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    return !suffix.isEmpty() && suffixes.contains(suffix);
}

/// Sparse file with the size and modification time of the original.
bool writePlaceholder(const QString& path, qint64 size, qint64 modifiedMs) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::ReadWrite)) {
        return false;
    }
    if (file.size() != size && !file.resize(size)) {
        return false;
    }
    return file.setFileTime(QDateTime::fromMSecsSinceEpoch(modifiedMs),
            QFileDevice::FileModificationTime);
}

} // namespace

void ExternalContent::setProvider(
        std::shared_ptr<ExternalContentProvider> pProvider, const QString& mirrorDirectory) {
    const auto locker = QMutexLocker(&s_mutex);
    s_pProvider = std::move(pProvider);
    s_mirrorDirectory = mirrorDirectory.isEmpty() ? QString() : QDir::cleanPath(mirrorDirectory);
    if (!s_mirrorDirectory.isEmpty()) {
        QDir().mkpath(s_mirrorDirectory);
    }
}

bool ExternalContent::isEnabled() {
    const auto locker = QMutexLocker(&s_mutex);
    return s_pProvider && !s_mirrorDirectory.isEmpty();
}

QString ExternalContent::mirrorDirectory() {
    const auto locker = QMutexLocker(&s_mutex);
    return s_mirrorDirectory;
}

QString ExternalContent::volumeDirectory(const QString& volumeId) {
    const QString mirror = mirrorDirectory();
    return mirror.isEmpty() ? QString() : mirror + QChar('/') + volumeId;
}

bool ExternalContent::splitPlaceholder(
        const QString& path, QString* pVolumeId, QString* pRelativePath) {
    const QString mirror = mirrorDirectory();
    if (mirror.isEmpty()) {
        return false;
    }
    const QString clean = QDir::cleanPath(path);
    if (!clean.startsWith(mirror + QChar('/'))) {
        return false;
    }
    const QString rest = clean.mid(mirror.size() + 1);
    const qsizetype slash = rest.indexOf(QChar('/'));
    if (slash <= 0 || slash == rest.size() - 1) {
        return false;
    }
    if (pVolumeId) {
        *pVolumeId = rest.left(slash);
    }
    if (pRelativePath) {
        *pRelativePath = rest.mid(slash + 1);
    }
    return true;
}

bool ExternalContent::isPlaceholder(const QString& path) {
    return splitPlaceholder(path, nullptr, nullptr);
}

QList<ExternalContentProvider::Volume> ExternalContent::volumes() {
    const auto pProvider = provider();
    return pProvider ? pProvider->volumes() : QList<ExternalContentProvider::Volume>();
}

bool ExternalContent::isVolumeMounted(const QString& volumeId) {
    const auto all = volumes();
    for (const auto& volume : all) {
        if (volume.id == volumeId) {
            return volume.mounted;
        }
    }
    return false;
}

QString ExternalContent::volumeIdOfDirectory(const QString& directory) {
    const QString mirror = mirrorDirectory();
    if (mirror.isEmpty()) {
        return {};
    }
    const QString clean = QDir::cleanPath(directory);
    if (!clean.startsWith(mirror + QChar('/'))) {
        return {};
    }
    const QString rest = clean.mid(mirror.size() + 1);
    const QString id = rest.section(QChar('/'), 0, 0);
    return id.startsWith(QChar('.')) ? QString() : id;
}

QString ExternalContent::displayPath(const QString& path) {
    const QString id = volumeIdOfDirectory(path);
    if (id.isEmpty()) {
        return path;
    }
    QString label = id;
    const auto all = volumes();
    for (const auto& volume : all) {
        if (volume.id == id && !volume.label.isEmpty()) {
            label = volume.label;
        }
    }
    const QString rest = QDir::cleanPath(path).mid(volumeDirectory(id).size());
    return label + rest;
}

ExternalContent::SyncResult ExternalContent::syncVolume(
        const QString& volumeId, const QStringList& fileSuffixes) {
    SyncResult result;
    const auto pProvider = provider();
    const QString root = volumeDirectory(volumeId);
    if (!pProvider || root.isEmpty() || volumeId.isEmpty() || volumeId.contains(QChar('/'))) {
        return result;
    }
    QList<ExternalContentProvider::Entry> entries;
    if (!pProvider->list(volumeId, &entries)) {
        kLogger.warning() << "Cannot list drive" << volumeId << "- placeholders kept";
        return result;
    }
    QSet<QString> wanted;
    for (const auto& entry : std::as_const(entries)) {
        if (entry.relativePath.isEmpty() || entry.relativePath.startsWith(QChar('/')) ||
                entry.relativePath.contains(QStringLiteral("..")) ||
                !hasSupportedSuffix(entry.relativePath, fileSuffixes)) {
            continue;
        }
        const QString path = QDir::cleanPath(root + QChar('/') + entry.relativePath);
        wanted.insert(path);
        const QFileInfo existing(path);
        const bool exists = existing.exists();
        if (exists && existing.size() == entry.size &&
                existing.lastModified().toMSecsSinceEpoch() == entry.modifiedMs) {
            continue;
        }
        if (!writePlaceholder(path, entry.size, entry.modifiedMs)) {
            kLogger.warning() << "Cannot write placeholder" << path;
            continue;
        }
        if (exists) {
            ++result.updated;
        } else {
            ++result.added;
        }
    }
    // Files that are gone from the drive
    QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = QDir::cleanPath(it.next());
        if (!wanted.contains(path)) {
            QFile::remove(path);
            ++result.removed;
        }
    }
    result.ok = true;
    kLogger.info() << "Drive" << volumeId << "placeholders: +" << result.added << "~"
                   << result.updated << "-" << result.removed;
    return result;
}

ExternalContent::Reader::Reader(const QString& path) {
    QString volumeId;
    QString relativePath;
    if (!splitPlaceholder(path, &volumeId, &relativePath)) {
        m_path = path;
        return;
    }
    m_isPlaceholder = true;
    const auto pProvider = provider();
    if (!pProvider) {
        return;
    }
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    m_fd = pProvider->openForReading(volumeId, relativePath);
    if (m_fd < 0) {
        return;
    }
    // Decoders and tag readers choose the file type by the extension, so the
    // descriptor is reached through a link that keeps it.
    const QString linkDir = mirrorDirectory() + QStringLiteral("/.fd");
    QDir().mkpath(linkDir);
    QString name = QString::number(s_linkCounter.fetchAndAddRelaxed(1));
    const QString suffix = QFileInfo(relativePath).suffix();
    if (!suffix.isEmpty()) {
        name += QChar('.') + suffix;
    }
    m_link = linkDir + QChar('/') + name;
    QFile::remove(m_link);
    if (QFile::link(QStringLiteral("/proc/self/fd/%1").arg(m_fd), m_link)) {
        m_path = m_link;
    } else {
        m_link.clear();
        m_path = QStringLiteral("/proc/self/fd/%1").arg(m_fd);
    }
#endif
}

ExternalContent::Reader::~Reader() {
    if (!m_link.isEmpty()) {
        QFile::remove(m_link);
    }
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    if (m_fd >= 0) {
        ::close(m_fd);
    }
#endif
}

} // namespace djmantra
