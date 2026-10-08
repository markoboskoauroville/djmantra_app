#include "library/onlinestatus.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QHash>
#include <QMutex>

#include "sources/localtrackcache.h"

namespace djmantra {

namespace {

constexpr qint64 kDriveCheckMillis = 1000;

struct DriveState {
    bool reachable = true;
    qint64 checkedAt = -1; // never
};

QMutex s_mutex;
// folder -> drive root ("" when not on an external drive)
QHash<QString, QString> s_driveOfFolder;
QHash<QString, DriveState> s_drives;
QElapsedTimer s_clock;

} // namespace

bool OnlineStatus::isDriveOffline(const QString& location) {
    if (location.isEmpty()) {
        return false;
    }
    const qsizetype slash = location.lastIndexOf(QChar('/'));
    const QString folder = slash > 0 ? location.left(slash) : location;
    const auto locker = QMutexLocker(&s_mutex);
    if (!s_clock.isValid()) {
        s_clock.start();
    }
    auto driveIt = s_driveOfFolder.constFind(folder);
    if (driveIt == s_driveOfFolder.constEnd()) {
        driveIt = s_driveOfFolder.insert(folder, LocalTrackCache::driveRoot(location));
    }
    const QString& drive = driveIt.value();
    if (drive.isEmpty()) {
        return false;
    }
    DriveState& state = s_drives[drive];
    const qint64 now = s_clock.elapsed();
    if (state.checkedAt < 0 || now - state.checkedAt > kDriveCheckMillis) {
        state.reachable = LocalTrackCache::isDriveReachable(drive);
        state.checkedAt = now;
    }
    return !state.reachable;
}

void OnlineStatus::invalidate() {
    const auto locker = QMutexLocker(&s_mutex);
    s_drives.clear();
    s_driveOfFolder.clear();
}

} // namespace djmantra
