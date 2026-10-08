#include "library/externaldrives.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QStorageInfo>

#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QJniObject>
#endif

#include "library/library.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "moc_externaldrives.cpp"
#include "sources/androidstorage.h"
#include "sources/externalcontent.h"
#include "sources/localtrackcache.h"
#include "sources/soundsourceproxy.h"
#include "util/logger.h"
#include "util/volumewatcher.h"

namespace djmantra {

namespace {

const mixxx::Logger kLogger("ExternalDrives");

const QString kGroup = QStringLiteral("[DJMantra]");
const ConfigKey kCacheModeKey(kGroup, QStringLiteral("LocalCacheMode"));
const ConfigKey kCacheMaxMbKey(kGroup, QStringLiteral("LocalCacheMaxMB"));
const ConfigKey kCacheDirKey(kGroup, QStringLiteral("LocalCacheDir"));
const ConfigKey kAskToAddKey(kGroup, QStringLiteral("AskToAddDrives"));
const ConfigKey kIgnoredDrivesKey(kGroup, QStringLiteral("IgnoredDrives"));

// Wait for the drive to settle (mounting, file system check) before scanning
constexpr int kRescanDelayMillis = 3000;

bool isOnDrive(const QString& path, const QString& driveRoot) {
    const QString clean = QDir::cleanPath(path);
    return clean == driveRoot || clean.startsWith(driveRoot + QChar('/'));
}

QString driveName(const QString& driveRoot) {
    if (!ExternalContent::volumeIdOfDirectory(driveRoot).isEmpty()) {
        return ExternalContent::displayPath(driveRoot);
    }
    const QStorageInfo storage(driveRoot);
    if (storage.isValid() && !storage.displayName().isEmpty() &&
            storage.rootPath() == driveRoot) {
        return storage.displayName();
    }
    return QFileInfo(driveRoot).fileName();
}

/// Drives read through the Storage Access Framework (Android): every mounted
/// removable volume, as its placeholder directory.
QStringList listContentDrives() {
    QStringList drives;
    const auto volumes = ExternalContent::volumes();
    for (const auto& volume : volumes) {
        if (volume.mounted) {
            drives.append(ExternalContent::volumeDirectory(volume.id));
        }
    }
    drives.sort();
    return drives;
}

bool isGranted(const QString& volumeId) {
    const auto volumes = ExternalContent::volumes();
    for (const auto& volume : volumes) {
        if (volume.id == volumeId) {
            return volume.granted;
        }
    }
    return false;
}

/// Bring the placeholders of a drive up to date; true if anything changed.
bool syncDrive(const QString& volumeId) {
    const auto result = ExternalContent::syncVolume(
            volumeId, SoundSourceProxy::getSupportedFileSuffixes());
    return result.ok && (result.added + result.updated + result.removed) > 0;
}

} // namespace

ExternalDrives::ExternalDrives(QObject* pParent,
        UserSettingsPointer pConfig,
        TrackCollectionManager* pTrackCollectionManager,
        Library* pLibrary)
        : QObject(pParent),
          m_pConfig(std::move(pConfig)),
          m_pTrackCollectionManager(pTrackCollectionManager),
          m_pLibrary(pLibrary),
          m_pWatcher(ExternalContent::isEnabled()
                          ? new VolumeWatcher(this, &listContentDrives)
                          : new VolumeWatcher(this)) {
    configureCache(m_pConfig);

    m_rescanTimer.setSingleShot(true);
    m_rescanTimer.setInterval(kRescanDelayMillis);
    connect(&m_rescanTimer, &QTimer::timeout, this, [this] {
        if (m_pTrackCollectionManager) {
            kLogger.info() << "Rescanning the library for the drive that came back";
            m_pTrackCollectionManager->startLibraryScan();
        }
    });
    connect(m_pWatcher, &VolumeWatcher::driveAttached, this, &ExternalDrives::slotDriveAttached);
    connect(m_pWatcher, &VolumeWatcher::driveDetached, this, &ExternalDrives::slotDriveDetached);
    m_pWatcher->start();

    // Drives granted earlier: files may have changed while the app was closed
    bool changed = false;
    const QStringList drives = m_pWatcher->drives();
    for (const auto& drive : drives) {
        const QString volumeId = ExternalContent::volumeIdOfDirectory(drive);
        if (!volumeId.isEmpty() && isGranted(volumeId)) {
            changed = syncDrive(volumeId) || changed;
        }
    }
    if (changed) {
        m_rescanTimer.start();
    }
}

ExternalDrives::~ExternalDrives() = default;

// static
void ExternalDrives::configureCache(const UserSettingsPointer& pConfig) {
    LocalTrackCache::Settings settings;
    const int mode = pConfig->getValue(kCacheModeKey, 1);
    settings.mode = mode <= 0 ? LocalTrackCache::Mode::Off
            : mode == 1       ? LocalTrackCache::Mode::RemovableOnly
                              : LocalTrackCache::Mode::All;
    settings.maxBytes = qint64{pConfig->getValue(kCacheMaxMbKey, 8192)} * 1024 * 1024;
    QString dir = pConfig->getValue(kCacheDirKey, QString());
    if (dir.isEmpty()) {
#if defined(Q_OS_ANDROID)
        // Internal app storage: not cleared by the system when space runs low
        dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
        dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#endif
        dir = QDir(dir).filePath(QStringLiteral("track-cache"));
    }
    settings.directory = dir;
    LocalTrackCache::configure(settings);

#if defined(Q_OS_ANDROID)
    // USB sticks are read through the Storage Access Framework
    if (!ExternalContent::isEnabled()) {
        ExternalContent::setProvider(AndroidStorage::createProvider(),
                QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                        .filePath(QStringLiteral("drives")));
    }
#endif
}

// static
void ExternalDrives::chooseMusicFolder(const QString& volumeId,
        std::function<void(const QString&)> callback) {
    if (!ExternalContent::isEnabled()) {
        const QString folder = QFileDialog::getExistingDirectory(nullptr,
                tr("Add music folder"),
                QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
        callback(folder);
        return;
    }
    AndroidStorage::pickFolder(volumeId, [callback](const QString& folder) {
        const QString volume = ExternalContent::volumeIdOfDirectory(folder);
        if (!volume.isEmpty()) {
            // Placeholders first: the library folder must exist to be added
            syncDrive(volume);
            QDir().mkpath(folder);
        }
        callback(folder);
    });
}

bool ExternalDrives::hasLibraryFolderOn(const QString& driveRoot) const {
    if (!m_pTrackCollectionManager) {
        return false;
    }
    const auto rootDirs = m_pTrackCollectionManager->internalCollection()->loadRootDirs();
    for (const auto& rootDir : rootDirs) {
        // A library folder on the drive, or the drive inside a library folder
        if (isOnDrive(rootDir.location(), driveRoot) ||
                isOnDrive(driveRoot, QDir::cleanPath(rootDir.location()))) {
            return true;
        }
    }
    return false;
}

void ExternalDrives::slotDriveAttached(const QString& driveRoot) {
    if (m_pLibrary) {
        m_pLibrary->slotExternalDrivesChanged(); // online marks
    }
    const QString volumeId = ExternalContent::volumeIdOfDirectory(driveRoot);
    if (!volumeId.isEmpty() && isGranted(volumeId)) {
        syncDrive(volumeId);
    }
    if (hasLibraryFolderOn(driveRoot)) {
        // Tracks on it were kept while it was away; check for new files
        m_rescanTimer.start();
        return;
    }
    if (m_pConfig->getValue(kAskToAddKey, 1) == 0) {
        return;
    }
    const QStringList ignored = m_pConfig->getValue(kIgnoredDrivesKey, QString())
                                        .split(QChar(','), Qt::SkipEmptyParts);
    if (ignored.contains(QFileInfo(driveRoot).fileName())) {
        return;
    }
    offerToAdd(driveRoot);
}

void ExternalDrives::slotDriveDetached(const QString& driveRoot) {
    // Nothing to do: loaded tracks play from their internal copies, and
    // tracks on the drive stay in the library until it comes back.
    kLogger.info() << "Drive removed, tracks on it are kept:" << driveRoot;
    if (m_pLibrary) {
        m_pLibrary->slotExternalDrivesChanged(); // offline marks
    }
}

void ExternalDrives::offerToAdd(const QString& driveRoot) {
    auto* pBox = new QMessageBox(QMessageBox::Question,
            tr("New drive"),
            tr("A drive was connected: %1\n\nAdd its music to the library?")
                    .arg(driveName(driveRoot)),
            QMessageBox::NoButton,
            nullptr);
    pBox->setAttribute(Qt::WA_DeleteOnClose);
    QPushButton* pAdd = pBox->addButton(tr("Add to library"), QMessageBox::AcceptRole);
    QPushButton* pNever = pBox->addButton(tr("Not this drive"), QMessageBox::RejectRole);
    pBox->addButton(tr("Not now"), QMessageBox::DestructiveRole);
    connect(pBox, &QMessageBox::finished, this, [this, pBox, pAdd, pNever, driveRoot] {
        if (pBox->clickedButton() == pAdd) {
            const QString volumeId = ExternalContent::volumeIdOfDirectory(driveRoot);
            if (!volumeId.isEmpty()) {
                // Android: the user grants the drive in the system picker
                chooseMusicFolder(volumeId, [this](const QString& folder) {
                    if (!folder.isEmpty() && m_pLibrary && m_pLibrary->requestAddDir(folder) &&
                            m_pTrackCollectionManager) {
                        m_pTrackCollectionManager->startLibraryScan();
                    }
                });
                return;
            }
            if (m_pLibrary && m_pLibrary->requestAddDir(driveRoot) && m_pTrackCollectionManager) {
                m_pTrackCollectionManager->startLibraryScan();
            }
        } else if (pBox->clickedButton() == pNever) {
            QStringList ignored = m_pConfig->getValue(kIgnoredDrivesKey, QString())
                                          .split(QChar(','), Qt::SkipEmptyParts);
            ignored.append(QFileInfo(driveRoot).fileName());
            m_pConfig->setValue(kIgnoredDrivesKey, ignored.join(QChar(',')));
        }
    });
    pBox->open();
}

// static
void ExternalDrives::requestAllFilesAccess() {
#if defined(Q_OS_ANDROID)
    const jboolean granted = QJniObject::callStaticMethod<jboolean>(
            "android/os/Environment", "isExternalStorageManager", "()Z");
    if (granted) {
        return;
    }
    QMessageBox::information(nullptr,
            tr("Access to USB drives"),
            tr("To play music from USB sticks, USB disks and SD cards, DJ Mantra "
               "needs \"All files access\".\n\nOn the next screen, turn on "
               "\"Allow access to manage all files\", then come back."));
    const QJniObject action = QJniObject::fromString(
            QStringLiteral("android.settings.MANAGE_APP_ALL_FILES_ACCESS_PERMISSION"));
    QJniObject intent("android/content/Intent", "(Ljava/lang/String;)V", action.object<jstring>());
    const QJniObject packageUri = QJniObject::callStaticObjectMethod("android/net/Uri",
            "parse",
            "(Ljava/lang/String;)Landroid/net/Uri;",
            QJniObject::fromString(QStringLiteral("package:com.djmantra.app")).object<jstring>());
    intent.callObjectMethod("setData",
            "(Landroid/net/Uri;)Landroid/content/Intent;",
            packageUri.object());
    constexpr jint kFlagActivityNewTask = 0x10000000;
    intent.callObjectMethod("addFlags", "(I)Landroid/content/Intent;", kFlagActivityNewTask);
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    context.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", intent.object());
#endif
}

} // namespace djmantra
