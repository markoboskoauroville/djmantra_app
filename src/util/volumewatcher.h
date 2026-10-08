#pragma once

#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <functional>

namespace djmantra {

/// Notices external drives (USB stick, USB disk, SD card) being plugged in
/// and pulled out, by polling. Polling works the same on Android (where apps
/// get no mount events) and on desktop systems.
class VolumeWatcher : public QObject {
    Q_OBJECT
  public:
    using ListDrivesFn = std::function<QStringList()>;

    explicit VolumeWatcher(QObject* pParent = nullptr,
            ListDrivesFn listDrives = &VolumeWatcher::listMountedDrives,
            int intervalMillis = 2000);

    /// Starts polling; the drives present now are not reported as attached.
    void start();
    void stop();

    QStringList drives() const;

    /// Directories of the external drives that are mounted now, as
    /// LocalTrackCache::driveRoot() names them (e.g. /storage/1A2B-3C4D).
    static QStringList listMountedDrives();

  public slots:
    void poll();

  signals:
    void driveAttached(const QString& driveRoot);
    void driveDetached(const QString& driveRoot);

  private:
    ListDrivesFn m_listDrives;
    QTimer m_timer;
    QSet<QString> m_drives;
    bool m_started = false;
};

} // namespace djmantra
