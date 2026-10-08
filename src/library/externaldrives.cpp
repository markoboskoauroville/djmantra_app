#include "library/externaldrives.h"

#include <QDir>
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
#include "sources/localtrackcache.h"
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
    const QStorageInfo storage(driveRoot);
    if (storage.isValid() && !storage.displayName().isEmpty() &&
            storage.rootPath() == driveRoot) {
        return storage.displayName();
    }
    return QFileInfo(driveRoot).fileName();
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
          m_pWatcher(new VolumeWatcher(this)) {
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
