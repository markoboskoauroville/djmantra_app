#include "util/volumewatcher.h"

#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>

#include "moc_volumewatcher.cpp"
#include "sources/localtrackcache.h"
#include "util/logger.h"

namespace djmantra {

namespace {

const mixxx::Logger kLogger("VolumeWatcher");

} // namespace

VolumeWatcher::VolumeWatcher(QObject* pParent, ListDrivesFn listDrives, int intervalMillis)
        : QObject(pParent),
          m_listDrives(std::move(listDrives)) {
    m_timer.setInterval(intervalMillis);
    connect(&m_timer, &QTimer::timeout, this, &VolumeWatcher::poll);
}

void VolumeWatcher::start() {
    const QStringList present = m_listDrives();
    m_drives = QSet<QString>(present.begin(), present.end());
    m_started = true;
    kLogger.info() << "External drives:" << present;
    m_timer.start();
}

void VolumeWatcher::stop() {
    m_timer.stop();
    m_started = false;
}

QStringList VolumeWatcher::drives() const {
    QStringList list(m_drives.begin(), m_drives.end());
    list.sort();
    return list;
}

void VolumeWatcher::poll() {
    if (!m_started) {
        return;
    }
    const QStringList present = m_listDrives();
    const QSet<QString> now(present.begin(), present.end());
    QStringList attached;
    QStringList detached;
    for (const auto& drive : now) {
        if (!m_drives.contains(drive)) {
            attached.append(drive);
        }
    }
    for (const auto& drive : std::as_const(m_drives)) {
        if (!now.contains(drive)) {
            detached.append(drive);
        }
    }
    m_drives = now;
    attached.sort();
    detached.sort();
    for (const auto& drive : std::as_const(detached)) {
        kLogger.info() << "Drive removed:" << drive;
        emit driveDetached(drive);
    }
    for (const auto& drive : std::as_const(attached)) {
        kLogger.info() << "Drive attached:" << drive;
        emit driveAttached(drive);
    }
}

QStringList VolumeWatcher::listMountedDrives() {
    QSet<QString> drives;
    auto addIfDrive = [&drives](const QString& dir) {
        // A drive directory is the drive root of any file in it
        const QString root =
                LocalTrackCache::driveRoot(QDir(dir).filePath(QStringLiteral("x")));
        if (!root.isEmpty() && root == QDir::cleanPath(dir) && QFileInfo(root).isDir()) {
            drives.insert(root);
        }
    };
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const auto& volume : volumes) {
        if (volume.isValid() && volume.isReady()) {
            addIfDrive(volume.rootPath());
        }
    }
#if defined(Q_OS_ANDROID)
    // Apps cannot always see the mount table; /storage lists the volumes.
    const auto entries = QDir(QStringLiteral("/storage"))
                                 .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto& entry : entries) {
        addIfDrive(entry.absoluteFilePath());
    }
#endif
    QStringList list(drives.begin(), drives.end());
    list.sort();
    return list;
}

} // namespace djmantra
