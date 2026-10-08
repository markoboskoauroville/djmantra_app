#pragma once

#include <QObject>
#include <QPointer>
#include <QTimer>

#include "preferences/usersettings.h"

class Library;
class TrackCollectionManager;

namespace djmantra {

class VolumeWatcher;

/// DJ Mantra: music on external drives (USB sticks, USB disks, SD cards).
///
/// - Sets up LocalTrackCache, so decks play tracks from internal copies.
/// - Watches drives: when a drive with library folders comes back, the
///   library is rescanned; for a new drive the user is asked once whether to
///   add it to the library.
/// - Android: asks for "All files access", which reading USB drives needs.
///
/// Settings ([DJMantra] in mixxx.cfg):
///   LocalCacheMode   0 off, 1 external drives only (default), 2 all files
///   LocalCacheMaxMB  size limit of the copies (default 8192)
///   LocalCacheDir    where the copies are kept (default: app data / cache)
///   AskToAddDrives   1 (default): offer new drives for the library
///   IgnoredDrives    drives the user did not want to add (comma separated)
class ExternalDrives : public QObject {
    Q_OBJECT
  public:
    ExternalDrives(QObject* pParent,
            UserSettingsPointer pConfig,
            TrackCollectionManager* pTrackCollectionManager,
            Library* pLibrary);
    ~ExternalDrives() override;

    /// Apply the cache settings. Called at construction; safe to call again.
    static void configureCache(const UserSettingsPointer& pConfig);

    /// Android: make sure the app may read files on USB drives. Shows an
    /// explanation and opens the system setting if that is not allowed yet.
    static void requestAllFilesAccess();

  private slots:
    void slotDriveAttached(const QString& driveRoot);
    void slotDriveDetached(const QString& driveRoot);

  private:
    bool hasLibraryFolderOn(const QString& driveRoot) const;
    void offerToAdd(const QString& driveRoot);

    UserSettingsPointer m_pConfig;
    QPointer<TrackCollectionManager> m_pTrackCollectionManager;
    QPointer<Library> m_pLibrary;
    VolumeWatcher* m_pWatcher;
    QTimer m_rescanTimer;
};

} // namespace djmantra
