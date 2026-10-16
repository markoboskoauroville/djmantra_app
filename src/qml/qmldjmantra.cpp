#include "qml/qmldjmantra.h"

#include <algorithm>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QPointer>
#include <QTimer>
#include <QtDebug>
#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUrl>
#include <thread>
#include <functional>
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#include <unistd.h>
#endif
#include <algorithm>
#include <memory>
#include <vector>
#include <cmath>

#if defined(Q_OS_ANDROID)
#include <QJniEnvironment>
#include <QPermissions>
#include <QJniObject>
#endif

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "controllers/midi/androidmidicontroller.h"
#include "engine/controls/sectiondetector.h"
#include "library/library.h"
#include "library/trackcollectionmanager.h"
#include "mixer/playerinfo.h"
#include "soundio/soundmanager.h"
#include "soundio/soundmanagerconfig.h"
#if defined(Q_OS_ANDROID)
// The Android output's host API name (sounddeviceoboe.cpp, in mixxx-lib):
// this library is built without __OBOE__ and Oboe's headers (round 22: the
// router listed only "Phone default", the outputs were compiled out)
extern const QString kOboeHostApi;
#endif
#include "qml/asyncimageprovider.h"
#include "qml/cqlink.h"
#include "qml/qmleffectsmanagerproxy.h"
#include "effects/effectsmanager.h"
#include "effects/effectslot.h"
#include "effects/chains/quickeffectchain.h"
#include "effects/backends/effectsbackendmanager.h"
#include "qml/gputime.h"
#include "qml/qmllibraryproxy.h"
#include "moc_qmldjmantra.cpp"
#include "sources/soundsourceproxy.h"
#include "track/keyutils.h"
#include "track/trackref.h"
#include "track/track.h"
#include "waveform/waveform.h"
#include "util/duration.h"
#include "widget/djmantra/camelot.h"
#include "widget/djmantra/wdeckcounter.h"

namespace mixxx {
namespace qml {

namespace {

djmantra::camelot::Code codeOf(double keyValue) {
    return djmantra::camelot::fromKey(KeyUtils::keyFromNumericValue(keyValue));
}

} // namespace

// V84 (Marko 16.10.2026: "all documents, especially charts which are captured by jpegs or text, end
// up in documents, dj mantra folder"): the app's documents (BeatStats charts / captures / pages, the
// download queues, the room corrections) live in Documents/DJ Mantra/<sub>; the music stays in Music
// (songs, playlists, Suno, recordings). BeatStatsActivity.java writes to the same place.
static QString djmantraDocs(const QString& sub) {
#if defined(Q_OS_ANDROID)
    const QString root = QStringLiteral("/sdcard/Documents/DJ Mantra");
#else
    const QString root = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
            QStringLiteral("/DJ Mantra");
#endif
    return sub.isEmpty() ? root : root + QChar('/') + sub;
}

namespace {

// Once: what V83 and before wrote to Music/djmantra_app/<sub> moves to Documents/DJ Mantra/<sub>
// (same storage: a rename; a file already there is kept and the old one left in place)
void moveDocumentsOnce() {
#if defined(Q_OS_ANDROID)
    const QString oldRoot = QStringLiteral("/sdcard/Music/djmantra_app");
#else
    const QString oldRoot = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
            QStringLiteral("/djmantra_app");
#endif
    for (const QString& sub : {QStringLiteral("BeatStats"),
                 QStringLiteral("Download_Queue"),
                 QStringLiteral("Room_Corrections")}) {
        const QString from = oldRoot + QChar('/') + sub;
        if (!QDir(from).exists()) {
            continue;
        }
        const QString to = djmantraDocs(sub);
        int moved = 0;
        QDirIterator it(from, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString file = it.next();
            const QString target = to + file.mid(from.size());
            QDir().mkpath(QFileInfo(target).absolutePath());
            if (!QFileInfo::exists(target) && QFile::rename(file, target)) {
                ++moved;
            }
        }
        // the old folder goes when it is empty (deepest folders first)
        QDirIterator dirs(from, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        QStringList folders;
        while (dirs.hasNext()) {
            folders << dirs.next();
        }
        std::sort(folders.begin(), folders.end(), [](const QString& a, const QString& b) {
            return a.size() > b.size();
        });
        for (const QString& folder : std::as_const(folders)) {
            QDir().rmdir(folder);
        }
        QDir().rmdir(from);
        qInfo().noquote() << "Documents:" << moved << "files" << from << "->" << to;
    }
}

} // namespace

namespace {
// The controller's bridge to the Qt Quick interface (Marko, 12.10.2026: the
// Mix Ultra's BROWSE opens the song list, its pad mode buttons choose the tabs)
std::vector<std::unique_ptr<ControlObject>> s_controllerControls;
} // namespace

// static
void QmlDJMantra::registerUserSettings(UserSettingsPointer pConfig) {
    s_controllerControls.clear();
    if (pConfig) {
        for (const char* key : {"browse_move", "browse_press", "browse_back", "load", "page", "stop_deck",
                     // V77: the Mix Ultra's FX pads (Effects.qml)
                     "fx_pad", "fx_pad_off", "fx_active_1", "fx_active_2",
                     // V77: SHIFT + FILTER = the effect's depth (pop-up), and the pickup ghosts
                     "fx_edit", "fx_depth_1", "fx_depth_2"}) {
            s_controllerControls.push_back(std::make_unique<ControlObject>(
                    ConfigKey(QStringLiteral("[DJMantra]"), QString::fromLatin1(key))));
        }
        // ghost_<deck>_<code>: a controller knob's position + 1 while it has not picked up its
        // value (0: none); codes 1..6 EQ parameter1..6, 7 FILTER, 8 the effect's DEPTH
        for (int deck = 1; deck <= 2; ++deck) {
            for (int code = 1; code <= 8; ++code) {
                s_controllerControls.push_back(std::make_unique<ControlObject>(
                        ConfigKey(QStringLiteral("[DJMantra]"),
                                QStringLiteral("ghost_%1_%2").arg(deck).arg(code))));
            }
        }
        // V8: the analysis options of Settings, read by the engine (sections) and
        // the analyzer (beats), remembered in the user settings
        for (const auto& [key, fallback] : std::initializer_list<std::pair<const char*, double>>{
                     {"section_algorithm", 0},
                     {"section_sensitivity", 3},
                     {"beat_grid_mode", 0},
                     {"beat_on_kick", 1},
                     {"stop_time", 0},
                     // the Mix Ultra's link for the screen's badge (0 none, 1 USB, 2 Bluetooth),
                     // set by AndroidMidiController; made here, before the QML binds to it
                     {"controller_link", 0}}) {
            const ConfigKey configKey(QStringLiteral("[DJMantra]"), QString::fromLatin1(key));
            auto pControl = std::make_unique<ControlObject>(configKey);
            pControl->set(pConfig->getValue(configKey, fallback));
            if (qstrcmp(key, "controller_link") == 0) {
                // the controller may have connected already
                pControl->set(androidMidiControllerLink());
            }
            s_controllerControls.push_back(std::move(pControl));
        }
    }
    s_pUserSettings = std::move(pConfig);
}

QmlDJMantra::QmlDJMantra(UserSettingsPointer pConfig, QObject* pParent)
        : QObject(pParent),
          m_pConfig(std::move(pConfig)) {
    hideBars();
    moveDocumentsOnce();
    // the CQ mixer: back to the address it had; the delay's tempo follows the decks
    m_pCq = new djmantra::CqLink(this);
    connect(m_pCq, &djmantra::CqLink::statusChanged, this, &QmlDJMantra::cqChanged);
    const QString cqHost = setting(QStringLiteral("cq_ip"), QString()).toString();
    if (!cqHost.isEmpty()) {
        m_pCq->connectTo(cqHost);
    }
    auto* pTempo = new QTimer(this);
    connect(pTempo, &QTimer::timeout, this, &QmlDJMantra::cqFollowTempo);
    pTempo->start(1000);
#if defined(Q_OS_ANDROID)
    auto* pTimer = new QTimer(this);
    connect(pTimer, &QTimer::timeout, this, &QmlDJMantra::followInsets);
    pTimer->start(300);
    // Full screen (round 18): the bars hide again when the app comes back
    // from another one, and on any touch while they are shown
    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive) {
            hideBars();
        }
    });
    qApp->installEventFilter(this);
    // A song without a genre: looked up online (round 18)
    connect(&PlayerInfo::instance(),
            &PlayerInfo::trackChanged,
            this,
            [this](const QString& group, TrackPointer pNewTrack, TrackPointer) {
                // the decks only (r41 V70: the room test signal on [Sampler9] was looked up too)
                if (group.startsWith(QStringLiteral("[Channel"))) {
                    lookUpGenre(group, pNewTrack);
                }
                // a new song starts fresh (Marko 14.10.2026: "on loading next song, you need to
                // reset all EQs and all neural filtering ... without inheritance of the old
                // settings"): the bands and the stem levels at 0 dB, no kills, the FILTER off
                if (pNewTrack && (group == QStringLiteral("[Channel1]") ||
                                         group == QStringLiteral("[Channel2]"))) {
                    const QString eq = QStringLiteral("[EqualizerRack1_") + group +
                            QStringLiteral("_Effect1]");
                    for (int i = 1; i <= 6; ++i) {
                        setControl(eq, QStringLiteral("parameter%1").arg(i), 1.0);
                    }
                    for (int i = 1; i <= 3; ++i) {
                        setControl(eq, QStringLiteral("button_parameter%1").arg(i), 0.0);
                    }
                    setControl(QStringLiteral("[QuickEffectRack1_") + group + QChar(']'),
                            QStringLiteral("super1"),
                            0.5);
                    qInfo().noquote() << "New song on" << group << ": EQ, Neural Mix and FILTER reset";
                }
            });
#endif
}

bool QmlDJMantra::eventFilter(QObject* pObject, QEvent* pEvent) {
#if defined(Q_OS_ANDROID)
    if ((pEvent->type() == QEvent::TouchBegin || pEvent->type() == QEvent::MouseButtonPress) &&
            pObject->isWindowType() &&
            QJniObject::callStaticMethod<jboolean>(
                    "com/djmantra/app/WindowBridge", "visible", "()Z")) {
        hideBars();
    }
#endif
    return QObject::eventFilter(pObject, pEvent);
}

void QmlDJMantra::lookUpGenre(const QString& group, TrackPointer pTrack) {
#if defined(Q_OS_ANDROID)
    if (!pTrack) {
        return;
    }
    // round 24: genres saved before the lookup asked for English came in
    // Croatian ("Klasična glazba", "Elektronička glazba"): looked up again
    const QString saved = pTrack->getGenre().trimmed();
    const bool croatian = saved.contains(QStringLiteral("glazba"), Qt::CaseInsensitive);
    if (!saved.isEmpty() && !croatian) {
        return;
    }
    const QString title = pTrack->getTitle().trimmed();
    const QString artist = pTrack->getArtist().trimmed();
    const QString location = pTrack->getLocation();
    if (title.isEmpty() || m_genreAsked.contains(location)) {
        return;
    }
    m_genreAsked.insert(location);
    qInfo() << "Quick: looking up the genre of" << group << artist << "-" << title;
    QPointer<QmlDJMantra> pThis(this);
    std::thread([pThis, pTrack, artist, title, saved] {
        const QJniObject result = QJniObject::callStaticObjectMethod(
                "com/djmantra/app/GenreLookup",
                "lookup",
                "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
                QJniObject::fromString(artist).object<jstring>(),
                QJniObject::fromString(title).object<jstring>());
        const QString genre = result.isValid() ? result.toString().trimmed() : QString();
        if (genre.isEmpty() || !pThis) {
            return;
        }
        QMetaObject::invokeMethod(
                pThis.data(),
                [pTrack, genre, saved] {
                    if (pTrack->getGenre().trimmed() == saved) {
                        pTrack->updateGenre(genre);
                        qInfo() << "Quick: genre" << pTrack->getTitle() << ":" << genre;
                    }
                },
                Qt::QueuedConnection);
    }).detach();
#else
    Q_UNUSED(group);
    Q_UNUSED(pTrack);
#endif
}

// static
QmlDJMantra* QmlDJMantra::create(QQmlEngine* pQmlEngine, QJSEngine*) {
    auto* pInstance = new QmlDJMantra(s_pUserSettings);
    QJSEngine::setObjectOwnership(pInstance, QJSEngine::JavaScriptOwnership);
    Q_UNUSED(pQmlEngine);
    return pInstance;
}

QString QmlDJMantra::version() const {
#ifdef DJMANTRA_VERSION
    return QStringLiteral(DJMANTRA_VERSION);
#else
    return QCoreApplication::applicationVersion();
#endif
}

void QmlDJMantra::followInsets() {
#if defined(Q_OS_ANDROID)
    // One pass of the bars watch (round 14: its own loop never ran)
    QJniObject::callStaticMethod<void>("com/djmantra/app/WindowBridge",
            "poll",
            "(Landroid/content/Context;)V",
            QNativeInterface::QAndroidApplication::context().object());
    const QJniObject array = QJniObject::callStaticObjectMethod(
            "com/djmantra/app/WindowBridge", "insets", "()[I");
    if (!array.isValid()) {
        return;
    }
    QJniEnvironment env;
    const auto jArray = static_cast<jintArray>(array.object());
    if (env->GetArrayLength(jArray) != 4) {
        return;
    }
    jint px[4];
    env->GetIntArrayRegion(jArray, 0, 4, px);
    const QScreen* pScreen = QGuiApplication::primaryScreen();
    const qreal ratio = pScreen ? pScreen->devicePixelRatio() : 1.0;
    bool changed = false;
    for (int i = 0; i < 4; ++i) {
        const int dp = qRound(px[i] / ratio);
        if (dp != m_insets[i]) {
            m_insets[i] = dp;
            changed = true;
        }
    }
    const int corner = qRound(QJniObject::callStaticMethod<jint>(
                                      "com/djmantra/app/WindowBridge", "cornerRadius", "()I") /
            ratio);
    if (corner != m_cornerRadius) {
        m_cornerRadius = corner;
        changed = true;
    }
    if (changed) {
        qInfo() << "Quick: insets" << m_insets[0] << m_insets[1] << m_insets[2] << m_insets[3]
                << "corner" << m_cornerRadius;
        emit insetsChanged();
    }
#endif
}

void QmlDJMantra::showBars() {
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/djmantra/app/WindowBridge",
            "show",
            "(Landroid/content/Context;)V",
            QNativeInterface::QAndroidApplication::context().object());
#endif
}

namespace {

QByteArray readSmall(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.read(256);
}

// The GPU load in % from a driver file, -1 if this one does not tell.
// Adreno: "45 %" or "busy total"; Mali / MediaTek / Pixel: a number first.
int gpuLoadFrom(const QString& path) {
    const QByteArray text = readSmall(path).trimmed();
    if (text.isEmpty()) {
        return -1;
    }
    static const QRegularExpression kNumbers(QStringLiteral("\\d+"));
    QList<qint64> numbers;
    auto it = kNumbers.globalMatch(QString::fromLatin1(text));
    while (it.hasNext() && numbers.size() < 2) {
        numbers.append(it.next().captured().toLongLong());
    }
    if (numbers.isEmpty()) {
        return -1;
    }
    if (path.endsWith(QStringLiteral("/gpubusy"))) {
        return numbers.size() == 2 && numbers[1] > 0
                ? static_cast<int>(100 * numbers[0] / numbers[1])
                : 0;
    }
    return numbers[0] >= 0 && numbers[0] <= 100 ? static_cast<int>(numbers[0]) : -1;
}

} // namespace

QVariantMap QmlDJMantra::usage() {
    QVariantMap result;
    int cpu = -1;
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
    // /proc/self/stat: utime and stime, the 12th and 13th fields after ")"
    const QByteArray stat = readSmall(QStringLiteral("/proc/self/stat"));
    const int close = stat.lastIndexOf(')');
    if (close > 0) {
        const QList<QByteArray> fields = stat.mid(close + 2).split(' ');
        if (fields.size() > 12) {
            const qint64 ticks = fields[11].toLongLong() + fields[12].toLongLong();
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            const long hz = sysconf(_SC_CLK_TCK);
            const long cores = std::max(1L, sysconf(_SC_NPROCESSORS_CONF));
            if (m_lastCpuTicks >= 0 && now > m_lastCpuAtMs && hz > 0) {
                const double busyMs = 1000.0 * (ticks - m_lastCpuTicks) / hz;
                cpu = static_cast<int>(std::lround(
                        100.0 * busyMs / ((now - m_lastCpuAtMs) * static_cast<double>(cores))));
            }
            m_lastCpuTicks = ticks;
            m_lastCpuAtMs = now;
        }
    }
    // The GPU: the first driver file this phone lets the app read
    if (!m_gpuProbed) {
        m_gpuProbed = true;
        QStringList candidates = {
                QStringLiteral("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage"),
                QStringLiteral("/sys/class/kgsl/kgsl-3d0/gpubusy"),
                QStringLiteral("/sys/kernel/ged/hal/gpu_utilization"),
                QStringLiteral("/proc/mtk_mali/utilization"),
                QStringLiteral("/sys/kernel/gpu/gpu_busy"),
                QStringLiteral("/sys/class/misc/mali0/device/utilization"),
                QStringLiteral("/sys/devices/platform/mali.0/utilization"),
        };
        // Pixel (Tensor, Mali): /sys/devices/platform/<address>.mali/utilization
        QStringList pixel;
        const QDir platform(QStringLiteral("/sys/devices/platform"));
        for (const auto& name : platform.entryList({QStringLiteral("*.mali")}, QDir::Dirs)) {
            pixel << platform.filePath(name + QStringLiteral("/utilization"))
                  << platform.filePath(name + QStringLiteral("/gpu_busy"));
        }
        candidates = pixel + candidates;
        for (const auto& path : candidates) {
            if (gpuLoadFrom(path) >= 0) {
                m_gpuSource = path;
                break;
            }
        }
        qInfo() << "Quick: GPU load from"
                << (m_gpuSource.isEmpty() ? QStringLiteral("nowhere (the phone does not tell)")
                                          : m_gpuSource);
    }
#endif
    result[QStringLiteral("cpu")] = cpu;
    int gpu = m_gpuSource.isEmpty() ? -1 : gpuLoadFrom(m_gpuSource);
    if (gpu < 0) {
        // no driver file for the app: the app's own frames, measured by the GPU
        // (qml/gputime.h): busy time / wall time since the last look
        const long long busy = djmantra::gputime::busyMicroseconds();
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (busy >= 0 && m_lastGpuBusyUs >= 0 && now > m_lastGpuAtMs) {
            gpu = static_cast<int>(std::lround(std::clamp(
                    (busy - m_lastGpuBusyUs) / (10.0 * (now - m_lastGpuAtMs)), 0.0, 100.0)));
        }
        m_lastGpuBusyUs = busy;
        m_lastGpuAtMs = now;
    }
    result[QStringLiteral("gpu")] = gpu;
    result[QStringLiteral("gpuSource")] = !m_gpuSource.isEmpty() ? QStringLiteral("driver")
            : gpu >= 0 ? (djmantra::gputime::fromGpuTimestamps() ? QStringLiteral("app frames (GPU)")
                                                                  : QStringLiteral("app frames (render thread)"))
                                                                  : QString();
    return result;
}

namespace {

const QString kDefaultOutputName = QStringLiteral("android-default-output");
const QString kRoutedPrefix = QStringLiteral("android-out:");

QString keyOfDevice(const SoundDeviceId& id) {
    return id.name.startsWith(kRoutedPrefix) ? id.name.mid(kRoutedPrefix.size()) : QString();
}

} // namespace

QVariantList QmlDJMantra::audioOutputs() const {
    QVariantList result;
    QVariantMap phone;
    phone[QStringLiteral("key")] = QString();
    phone[QStringLiteral("label")] = tr("Phone default (follows plugging in)");
    phone[QStringLiteral("channels")] = 2;
    result.append(phone);
#if defined(Q_OS_ANDROID)
    // AudioRouter.outputs(): "key|label|channels" per output
    const QJniObject array = QJniObject::callStaticObjectMethod(
            "com/djmantra/app/AudioRouter",
            "outputs",
            "(Landroid/content/Context;)[Ljava/lang/String;",
            QNativeInterface::QAndroidApplication::context().object());
    QJniEnvironment env;
    if (env.checkAndClearExceptions() || !array.isValid()) {
        qWarning() << "Quick: the audio outputs could not be read";
        return result;
    }
    const auto jArray = static_cast<jobjectArray>(array.object());
    const jsize count = env->GetArrayLength(jArray);
    for (jsize i = 0; i < count; ++i) {
        const auto jItem = static_cast<jstring>(env->GetObjectArrayElement(jArray, i));
        const QString item = QJniObject(jItem).toString();
        env->DeleteLocalRef(jItem);
        const QStringList parts = item.split(QChar('|'));
        qInfo().noquote() << "Quick: audio output" << item;
        if (parts.size() < 2) {
            continue;
        }
        const int channels = parts.size() >= 3 ? std::max(2, parts[2].toInt()) : 2;
        QVariantMap entry;
        entry[QStringLiteral("key")] = parts[0];
        entry[QStringLiteral("label")] = parts[1];
        entry[QStringLiteral("channels")] = channels - channels % 2;
        result.append(entry);
    }
#endif
    return result;
}

int QmlDJMantra::audioOutputsVersion() const {
#if defined(Q_OS_ANDROID)
    return QJniObject::callStaticMethod<jint>("com/djmantra/app/AudioRouter", "version", "()I");
#else
    return 0;
#endif
}

QVariantMap QmlDJMantra::audioRoute() const {
    QVariantMap result;
    result[QStringLiteral("master")] = QString();
    result[QStringLiteral("headphones")] = QStringLiteral("none");
    result[QStringLiteral("masterPair")] = 0;
    result[QStringLiteral("headphonesPair")] = 0;
    if (!s_pSoundManager) {
        return result;
    }
    const auto outputs = s_pSoundManager->getConfig().getOutputs();
    for (auto it = outputs.cbegin(); it != outputs.cend(); ++it) {
        const int pair = static_cast<int>(it.value().getChannelGroup().getChannelBase()) / 2;
        if (it.value().getType() == AudioPathType::Main) {
            result[QStringLiteral("master")] = keyOfDevice(it.key());
            result[QStringLiteral("masterPair")] = pair;
        } else if (it.value().getType() == AudioPathType::Headphones) {
            result[QStringLiteral("headphones")] = keyOfDevice(it.key());
            result[QStringLiteral("headphonesPair")] = pair;
        }
    }
    return result;
}

QString QmlDJMantra::setAudioRoute(const QString& master,
        int masterPair,
        const QString& headphones,
        int headphonesPair) {
    if (!s_pSoundManager) {
        return tr("No sound");
    }
    // the outputs plugged in since the start are known only after a new look
    s_pSoundManager->clearAndQueryDevices();
#if defined(Q_OS_ANDROID)
    const auto devices = s_pSoundManager->getDeviceList(kOboeHostApi, true, false);
#else
    const auto devices = s_pSoundManager->getDeviceList(
            s_pSoundManager->getConfig().getAPI(), true, false);
#endif
    const auto findDevice = [&devices](const QString& key) -> SoundDevicePointer {
        const QString name = key.isEmpty() ? kDefaultOutputName : kRoutedPrefix + key;
        for (const auto& pDevice : devices) {
            if (pDevice->getDeviceId().name == name) {
                return pDevice;
            }
        }
        return nullptr;
    };
    const auto pMaster = findDevice(master);
    if (!pMaster) {
        return tr("The master output is not connected");
    }
    SoundManagerConfig config = s_pSoundManager->getConfig();
    config.clearOutputs();
    // a stereo pair of the output: pair 0 = channels 1-2, 1 = 3-4 ...
    const auto pairFits = [](const SoundDevicePointer& pDevice, int pair) {
        return pair >= 0 && 2 * pair + 2 <= static_cast<int>(pDevice->getNumOutputChannels().value());
    };
    if (!pairFits(pMaster, masterPair)) {
        return tr("The master output has no channels %1-%2").arg(2 * masterPair + 1).arg(2 * masterPair + 2);
    }
    config.addOutput(pMaster->getDeviceId(),
            AudioOutput(AudioPathType::Main,
                    static_cast<unsigned char>(2 * masterPair),
                    mixxx::audio::ChannelCount::stereo(),
                    0));
    const bool separate = headphones != QStringLiteral("none") &&
            (headphones != master || headphonesPair != masterPair);
    if (separate) {
        const auto pHeadphones = findDevice(headphones);
        if (!pHeadphones) {
            return tr("The headphones output is not connected");
        }
        if (!pairFits(pHeadphones, headphonesPair)) {
            return tr("The headphones output has no channels %1-%2")
                    .arg(2 * headphonesPair + 1)
                    .arg(2 * headphonesPair + 2);
        }
        config.addOutput(pHeadphones->getDeviceId(),
                AudioOutput(AudioPathType::Headphones,
                        static_cast<unsigned char>(2 * headphonesPair),
                        mixxx::audio::ChannelCount::stereo(),
                        0));
    }
    const SoundDeviceStatus status = s_pSoundManager->setConfig(config);
    qInfo().noquote() << "Quick: audio route master ->"
                      << (master.isEmpty() ? QStringLiteral("phone default") : master)
                      << QStringLiteral("channels %1-%2").arg(2 * masterPair + 1).arg(2 * masterPair + 2);
    qInfo().noquote() << "Quick: audio route headphones ->"
                      << (!separate ? QStringLiteral("none")
                                    : (headphones.isEmpty() ? QStringLiteral("phone default")
                                                            : headphones) +
                                      QStringLiteral(" channels %1-%2")
                                              .arg(2 * headphonesPair + 1)
                                              .arg(2 * headphonesPair + 2));
    if (status != SoundDeviceStatus::Ok) {
        return tr("The outputs could not be opened: %1").arg(s_pSoundManager->getLastErrorMessage(status));
    }
    return QString();
}

QVariant QmlDJMantra::setting(const QString& name, const QVariant& fallback) const {
    if (!m_pConfig) {
        return fallback;
    }
    const ConfigKey key(QStringLiteral("[DJMantra]"), name);
    if (!m_pConfig->exists(key)) {
        return fallback;
    }
    const QString value = m_pConfig->getValueString(key);
    if (fallback.typeId() == QMetaType::Bool) {
        return value == QStringLiteral("1") || value == QStringLiteral("true");
    }
    return value;
}

void QmlDJMantra::setSetting(const QString& name, const QVariant& value) {
    if (!m_pConfig) {
        return;
    }
    const ConfigKey key(QStringLiteral("[DJMantra]"), name);
    m_pConfig->setValue(key,
            value.typeId() == QMetaType::Bool ? QString(value.toBool() ? "1" : "0")
                                              : value.toString());
    m_pConfig->save();
    // an option the engine reads (sections, beats): it follows at once
    if (ControlObject* pControl = ControlObject::getControl(key, ControlFlag::AllowMissingOrInvalid)) {
        pControl->set(value.toDouble());
    }
    qInfo().noquote() << "Settings:" << name << "=" << value.toString();
}

bool QmlDJMantra::barsVisible() const {
#if defined(Q_OS_ANDROID)
    return QJniObject::callStaticMethod<jboolean>("com/djmantra/app/WindowBridge", "visible", "()Z");
#else
    return false;
#endif
}

QVariantList QmlDJMantra::sections(const QString& group) const {
    QVariantList result;
    for (const double fraction : djmantra::publishedSections(group)) {
        result.append(fraction);
    }
    return result;
}

void QmlDJMantra::hideBars() {
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/djmantra/app/WindowBridge",
            "immersive",
            "(Landroid/content/Context;)V",
            QNativeInterface::QAndroidApplication::context().object());
#endif
}

QString QmlDJMantra::camelot(double keyValue) const {
    const auto code = codeOf(keyValue);
    return code.isValid() ? djmantra::camelot::text(code) : QString();
}

QString QmlDJMantra::musicalName(const QString& code) const {
    if (code.size() < 2) {
        return QString();
    }
    djmantra::camelot::Code c;
    c.number = code.left(code.size() - 1).toInt();
    c.major = code.endsWith(QChar('B'));
    return djmantra::camelot::musicalName(c);
}

QVariantMap QmlDJMantra::keyMatch(double fromKey, double toKey) const {
    const auto from = codeOf(fromKey);
    const auto to = codeOf(toKey);
    QString why;
    const auto match = djmantra::camelot::match(from, to, &why);
    QVariantMap result;
    result[QStringLiteral("text")] =
            djmantra::camelot::text(from) + QStringLiteral(" > ") + djmantra::camelot::text(to);
    result[QStringLiteral("why")] = match == djmantra::camelot::Match::None ? tr("key") : why;
    result[QStringLiteral("color")] = djmantra::camelot::color(match).name();
    return result;
}

int QmlDJMantra::keyMatchShift(double deckKey, double otherKey) const {
    return djmantra::camelot::matchShift(codeOf(deckKey), codeOf(otherKey));
}

QString QmlDJMantra::formatTime(double seconds) const {
    const bool negative = seconds < 0;
    const QString text = mixxx::Duration::formatTime(
            std::fabs(seconds), mixxx::Duration::Precision::SECONDS);
    return negative ? QChar('-') + text : text;
}

QString QmlDJMantra::barBeat(double beatNumber, double beatsPerBar) const {
    return WDeckCounter::barBeat(static_cast<int>(beatNumber), static_cast<int>(beatsPerBar));
}

QString QmlDJMantra::barsBeatsLeft(double beats, double beatsPerBar) const {
    return WDeckCounter::barsBeatsLeft(static_cast<int>(beats), static_cast<int>(beatsPerBar));
}

double QmlDJMantra::waveformLevel(const QString& group) const {
    // V9: the colour's inputs as djay's colour was fitted (src/waveform/djaycolor.h):
    // the bands divided by the song's loudest band (99th percentile of max(l, m, h));
    // 0 while the waveform is not analysed yet (QML asks again)
    const TrackPointer pTrack = PlayerInfo::instance().getTrackInfo(group);
    const ConstWaveformPointer pWaveform = pTrack ? pTrack->getWaveform() : ConstWaveformPointer();
    if (!pWaveform || pWaveform->getCompletion() < pWaveform->getDataSize()) {
        return 0.0;
    }
    const int size = pWaveform->getDataSize();
    std::vector<int> histogram(256, 0);
    for (int i = 0; i < size; ++i) {
        // max(low, mid, high), as the colour was fitted (djaycolor.h)
        histogram[std::max({pWaveform->getLow(i), pWaveform->getMid(i), pWaveform->getHigh(i)})]++;
    }
    int level = 255;
    int above = 0;
    while (level > 1 && above + histogram[level] <= size / 100) {
        above += histogram[level];
        --level;
    }
    return 255.0 / std::max(level, 8);
}

double QmlDJMantra::waveformGain(const QString& group, int length) const {
    Q_UNUSED(length);
    const TrackPointer pTrack = PlayerInfo::instance().getTrackInfo(group);
    if (!pTrack) {
        return 0.0;
    }
    const ConstWaveformPointer pWaveform = pTrack->getWaveform();
    if (!pWaveform || pWaveform->getCompletion() < pWaveform->getDataSize()) {
        return 0.0; // not analysed yet: QML asks again
    }
    // The GPU waveform's height is |(low, mid, high)| / sqrt(3), well below
    // the overall peak: scale its 95th percentile to 80 % of the lane
    // (round 14: data peaks ~110 of 255, the lanes 5-15 % full)
    const int size = pWaveform->getDataSize();
    std::vector<int> histogram(256, 0);
    for (int i = 0; i < size; ++i) {
        const double l = pWaveform->getLow(i);
        const double m = pWaveform->getMid(i);
        const double h = pWaveform->getHigh(i);
        const int magnitude = std::min(255, static_cast<int>(std::sqrt((l * l + m * m + h * h) / 3.0)));
        histogram[magnitude]++;
    }
    int level = 255;
    int above = 0;
    while (level > 0 && above + histogram[level] <= size / 20) {
        above += histogram[level];
        --level;
    }
    if (level < 4) {
        return 1.0; // silence
    }
    const double gain = std::clamp(0.8 * 255.0 / level, 1.0, 12.0);
    qInfo().noquote() << "Quick: waveform gain" << group << gain << "(95 % level" << level
                      << "of" << size << "points)";
    return gain;
}

bool QmlDJMantra::isPlaying(const QString& group) const {
    ControlProxy play(group, QStringLiteral("play"), nullptr, ControlFlag::NoAssertIfMissing);
    return play.valid() && play.get() > 0;
}

QString QmlDJMantra::freeDeck() const {
    const QString deck1 = QStringLiteral("[Channel1]");
    const QString deck2 = QStringLiteral("[Channel2]");
    const bool playing1 = isPlaying(deck1);
    const bool playing2 = isPlaying(deck2);
    QString deck;
    if (playing1 != playing2) {
        deck = playing1 ? deck2 : deck1;
    } else if (!playing1) {
        deck = m_lastLoaded == deck1 ? deck2 : deck1;
    } else {
        // Both play: the one heard less (volume and crossfader side)
        const auto get = [](const QString& group, const QString& item) {
            ControlProxy proxy(group, item, nullptr, ControlFlag::NoAssertIfMissing);
            return proxy.valid() ? proxy.get() : 0.0;
        };
        const double fader = get(QStringLiteral("[Master]"), QStringLiteral("crossfader"));
        const double level1 = get(deck1, QStringLiteral("volume")) * (fader <= 0 ? 1.0 : 1.0 - fader);
        const double level2 = get(deck2, QStringLiteral("volume")) * (fader >= 0 ? 1.0 : 1.0 + fader);
        deck = level1 <= level2 ? deck1 : deck2;
    }
    qInfo().noquote() << "Quick: free deck" << deck << "(playing" << playing1 << playing2 << ")";
    return deck;
}

void QmlDJMantra::noteLoaded(const QString& group) {
    m_lastLoaded = group;
}

QVariantList QmlDJMantra::musicRoots() const {
    QVariantList roots;
    QStringList paths;
#if defined(Q_OS_ANDROID)
    // (not QStandardPaths' MusicLocation: on Android that is a second "Music",
    // the app's own folder; round 14 listed Music twice)
    // Marko 14.10.2026: the app's own root first, Music/djmantra_app (one master folder;
    // underscores, no spaces):
    // Playlists/<name>/ songs + Playlists/<name>.m3u8, Recordings/; Music and Download stay
    QDir().mkpath(QStringLiteral("/sdcard/Music/djmantra_app"));
    paths << QStringLiteral("/sdcard/Music/djmantra_app") << QStringLiteral("/sdcard/Music")
          << QStringLiteral("/sdcard/Download");
#else
    paths << QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
#endif
    QStringList seen;
    for (const QString& path : std::as_const(paths)) {
        // /sdcard/Music and /storage/emulated/0/Music are one folder
        const QString clean = QFileInfo(path).canonicalFilePath();
        if (clean.isEmpty() || seen.contains(clean) || !QFileInfo(clean).isDir()) {
            continue;
        }
        seen << clean;
        QVariantMap entry;
        entry[QStringLiteral("name")] = QFileInfo(clean).fileName();
        entry[QStringLiteral("path")] = clean;
        entry[QStringLiteral("isFolder")] = true;
        roots << entry;
    }
    return roots;
}

namespace {

QString playlistsFile() {
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/playlists.json");
}

QJsonArray readPlaylists() {
    QFile file(playlistsFile());
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonArray();
    }
    return QJsonDocument::fromJson(file.readAll()).array();
}

void writePlaylists(const QJsonArray& playlists) {
    QSaveFile file(playlistsFile());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(playlists).toJson(QJsonDocument::Compact));
        file.commit();
    }
}

int findPlaylist(const QJsonArray& playlists, const QString& name) {
    for (int i = 0; i < playlists.size(); ++i) {
        if (playlists[i].toObject().value(QStringLiteral("name")).toString() == name) {
            return i;
        }
    }
    return -1;
}

QStringList playlistsFolders() {
#if defined(Q_OS_ANDROID)
    // the app's root first (Marko 14.10.2026), then Music/Playlists
    return {QStringLiteral("/sdcard/Music/djmantra_app/Playlists"), QStringLiteral("/sdcard/Music/Playlists")};
#else
    const QString music = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    return {music + QStringLiteral("/djmantra_app/Playlists"), music + QStringLiteral("/Playlists")};
#endif
}

// M3U / M3U8 files in djmantra_app/Playlists (and Music/Playlists), each taken in once (by name)
void importM3uPlaylists() {
    QJsonArray playlists = readPlaylists();
    bool changed = false;
    QFileInfoList files;
    for (const QString& folder : playlistsFolders()) {
        files += QDir(folder).entryInfoList({QStringLiteral("*.m3u"), QStringLiteral("*.m3u8")}, QDir::Files);
    }
    for (const QFileInfo& info : files) {
        const QDir dir = info.absoluteDir();
        const QString name = info.completeBaseName();
        if (findPlaylist(playlists, name) >= 0) {
            continue;
        }
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }
        QJsonArray songs;
        while (!file.atEnd()) {
            QString line = QString::fromUtf8(file.readLine()).trimmed();
            if (line.isEmpty() || line.startsWith(QChar('#'))) {
                continue;
            }
            if (line.startsWith(QStringLiteral("file://"))) {
                line = QUrl(line).toLocalFile();
            }
            const QString path = QFileInfo(line).isRelative() ? dir.absoluteFilePath(line) : line;
            if (QFileInfo::exists(path)) {
                songs.append(QFileInfo(path).absoluteFilePath());
            }
        }
        QJsonObject playlist;
        playlist[QStringLiteral("name")] = name;
        playlist[QStringLiteral("created")] = static_cast<double>(info.lastModified().toMSecsSinceEpoch());
        playlist[QStringLiteral("songs")] = songs;
        playlists.append(playlist);
        changed = true;
        qInfo().noquote() << "Playlists: imported" << info.fileName() << songs.size() << "songs";
    }
    if (changed) {
        writePlaylists(playlists);
    }
}

} // namespace

namespace {

QString queueFile(const QString& service) {
    const QString dir = djmantraDocs(QStringLiteral("Download_Queue"));
    QDir().mkpath(dir);
    QString name = service;
    name.replace(QChar(' '), QChar('_'));
    return dir + QChar('/') + name + QStringLiteral(".txt");
}

QStringList queueLines(const QString& service) {
    QFile file(queueFile(service));
    QStringList lines;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!file.atEnd()) {
            const QString line = QString::fromUtf8(file.readLine()).trimmed();
            if (!line.isEmpty()) {
                lines << line;
            }
        }
    }
    return lines;
}

void writeQueue(const QString& service, const QStringList& lines) {
    QSaveFile file(queueFile(service));
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        for (const QString& line : lines) {
            file.write(line.toUtf8() + '\n');
        }
        file.commit();
    }
}

} // namespace

QString QmlDJMantra::searchLink(const QString& service,
        const QString& artist,
        const QString& title,
        const QString& isrc,
        const QString& tidalId) const {
    // the exact recording when its code is known (Marko 14.10.2026): Spotify by isrc:<ISRC>, TIDAL
    // by its track ID found by ISRC; YouTube Music has no ISRC search, so artist + title there
    const QString query = QString::fromUtf8(QUrl::toPercentEncoding((artist + QChar(' ') + title).simplified()));
    if (service.startsWith(QStringLiteral("Spotify"), Qt::CaseInsensitive)) {
        if (!isrc.isEmpty()) {
            return QStringLiteral("https://open.spotify.com/search/isrc%3A") + isrc;
        }
        return QStringLiteral("https://open.spotify.com/search/") + query;
    }
    if (service.startsWith(QStringLiteral("TIDAL"), Qt::CaseInsensitive)) {
        if (!tidalId.isEmpty()) {
            return QStringLiteral("https://tidal.com/browse/track/") + tidalId;
        }
        return QStringLiteral("https://tidal.com/search?q=") + query;
    }
    if (service.startsWith(QStringLiteral("YouTube Music"), Qt::CaseInsensitive)) {
        return QStringLiteral("https://music.youtube.com/search?q=") + query;
    }
    return QStringLiteral("https://www.youtube.com/results?search_query=") + query;
}

bool QmlDJMantra::addToDownloadQueue(const QString& service,
        const QString& artist,
        const QString& title,
        const QString& isrc,
        const QString& tidalId) {
    QStringList lines = queueLines(service);
    const QString line = artist.simplified() + QStringLiteral(" | ") + title.simplified() +
            QStringLiteral(" | ") + searchLink(service, artist, title, isrc, tidalId);
    if (lines.contains(line)) {
        return false;
    }
    lines << line;
    writeQueue(service, lines);
    qInfo().noquote() << "Download queue" << service << "+" << line;
    return true;
}

QVariantList QmlDJMantra::downloadQueue(const QString& service) const {
    QVariantList result;
    for (const QString& line : queueLines(service)) {
        const QStringList parts = line.split(QStringLiteral(" | "));
        QVariantMap entry;
        entry[QStringLiteral("artist")] = parts.value(0);
        entry[QStringLiteral("title")] = parts.value(1);
        entry[QStringLiteral("link")] = parts.value(2);
        result << entry;
    }
    return result;
}

bool QmlDJMantra::removeFromDownloadQueue(const QString& service, int index) {
    QStringList lines = queueLines(service);
    if (index < 0 || index >= lines.size()) {
        return false;
    }
    lines.removeAt(index);
    writeQueue(service, lines);
    return true;
}

QString QmlDJMantra::downloadQueueFile(const QString& service) const {
    return queueFile(service);
}

void QmlDJMantra::openBeatStats() {
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/djmantra/app/BeatStatsActivity",
            "open",
            "(Landroid/content/Context;)V",
            QNativeInterface::QAndroidApplication::context().object());
#endif
    qInfo() << "BeatStats: opened";
}

QVariantList QmlDJMantra::beatStatsCharts() const {
    QFile file(djmantraDocs(QStringLiteral("BeatStats/charts.json")));
    if (!file.open(QIODevice::ReadOnly)) {
        return QVariantList();
    }
    return QJsonDocument::fromJson(file.readAll()).array().toVariantList();
}

#if defined(Q_OS_ANDROID)
namespace {

// "Artist - Title (Mix).mp3" and "Title" / "Artist" compared without case, accents' marks,
// punctuation and extra spaces
QString matchKey(const QString& text) {
    QString key = text.normalized(QString::NormalizationForm_KD).toLower();
    key.remove(QRegularExpression(QStringLiteral("[\\p{Mn}]")));
    key.replace(QRegularExpression(QStringLiteral("[^a-z0-9]+")), QStringLiteral(" "));
    return key.simplified();
}

QString callTidal(const char* method, const char* signature, const QStringList& args = {}) {
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    QJniObject result;
    if (args.isEmpty()) {
        result = QJniObject::callStaticObjectMethod("com/djmantra/app/TidalBridge", method, signature,
                context.object());
    } else if (args.size() == 1) {
        result = QJniObject::callStaticObjectMethod("com/djmantra/app/TidalBridge", method, signature,
                context.object(), QJniObject::fromString(args[0]).object<jstring>());
    } else {
        result = QJniObject::callStaticObjectMethod("com/djmantra/app/TidalBridge", method, signature,
                context.object(), QJniObject::fromString(args[0]).object<jstring>(),
                QJniObject::fromString(args[1]).object<jstring>());
    }
    QJniEnvironment env;
    env.checkAndClearExceptions();
    return result.isValid() ? result.toString() : QString();
}

} // namespace
#endif

bool QmlDJMantra::tidalLoggedIn() const {
#if defined(Q_OS_ANDROID)
    return QJniObject::callStaticMethod<jboolean>("com/djmantra/app/TidalBridge", "loggedIn",
            "(Landroid/content/Context;)Z", QNativeInterface::QAndroidApplication::context().object());
#else
    return false;
#endif
}

void QmlDJMantra::tidalLogout() {
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/djmantra/app/TidalBridge", "logout",
            "(Landroid/content/Context;)V", QNativeInterface::QAndroidApplication::context().object());
#endif
    emit tidalResult(QStringLiteral("logout"), QString());
}

#if defined(Q_OS_ANDROID)
// a TIDAL call in the background; the answer comes back on the UI thread
static void runTidal(QmlDJMantra* self, const QString& action, std::function<QString()> work) {
    QPointer<QmlDJMantra> guard(self);
    std::thread([guard, action, work]() {
        const QString result = work();
        QMetaObject::invokeMethod(qApp, [guard, action, result]() {
            if (guard) {
                emit guard->tidalResult(action, result);
            }
        }, Qt::QueuedConnection);
    }).detach();
}
#endif

void QmlDJMantra::tidalLogin() {
#if defined(Q_OS_ANDROID)
    runTidal(this, QStringLiteral("login"), []() {
        return callTidal("login", "(Landroid/content/Context;)Ljava/lang/String;");
    });
#endif
}

void QmlDJMantra::tidalPlaylists() {
#if defined(Q_OS_ANDROID)
    runTidal(this, QStringLiteral("playlists"), []() {
        return callTidal("playlists", "(Landroid/content/Context;)Ljava/lang/String;");
    });
#endif
}

void QmlDJMantra::tidalImport(const QString& id, const QString& name) {
#if defined(Q_OS_ANDROID)
    QPointer<QmlDJMantra> guard(this);
    std::thread([guard, id, name]() {
        const QString json = callTidal("playlistTracks",
                "(Landroid/content/Context;Ljava/lang/String;)Ljava/lang/String;", {id});
        QMetaObject::invokeMethod(qApp, [guard, json, name]() {
            if (!guard) {
                return;
            }
            // the phone's songs by their file names, under Music
            QHash<QString, QString> files;
            QDirIterator it(QStringLiteral("/sdcard/Music"), QDir::Files | QDir::Readable,
                    QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString path = it.next();
                if (SoundSourceProxy::isFileNameSupported(path)) {
                    files.insert(matchKey(it.fileInfo().completeBaseName()), path);
                }
            }
            int matched = 0;
            int queued = 0;
            const QJsonArray tracks = QJsonDocument::fromJson(json.toUtf8()).array();
            for (const auto& value : tracks) {
                const QJsonObject track = value.toObject();
                const QString artist = track.value(QStringLiteral("artist")).toString();
                const QString title = track.value(QStringLiteral("title")).toString();
                const QString firstArtist = matchKey(artist.section(QChar(','), 0, 0));
                const QString titleKey = matchKey(title);
                QString found;
                for (auto i = files.constBegin(); i != files.constEnd() && found.isEmpty(); ++i) {
                    if (i.key().contains(titleKey) && i.key().contains(firstArtist)) {
                        found = i.value();
                    }
                }
                if (!found.isEmpty()) {
                    guard->addToPlaylist(name, found);
                    ++matched;
                } else {
                    guard->addToDownloadQueue(QStringLiteral("TIDAL"), artist, title);
                    ++queued;
                }
            }
            qInfo().noquote() << "TIDAL import" << name << ":" << matched << "on the phone," << queued << "queued";
            emit guard->tidalResult(QStringLiteral("import"),
                    QStringLiteral("%1|%2|%3").arg(name).arg(matched).arg(queued));
        }, Qt::QueuedConnection);
    }).detach();
#else
    Q_UNUSED(id);
    Q_UNUSED(name);
#endif
}

void QmlDJMantra::tidalPush(const QString& name) {
#if defined(Q_OS_ANDROID)
    // the songs' artist and title from their tags (the library), read here on the UI thread
    QJsonArray songs;
    for (const QVariant& entry : listFolder(QStringLiteral("playlist:") + name)) {
        const QVariantMap info = songInfo(entry.toMap().value(QStringLiteral("path")).toString());
        QJsonObject song;
        song[QStringLiteral("artist")] = info.value(QStringLiteral("artist")).toString();
        song[QStringLiteral("title")] = info.value(QStringLiteral("title")).toString();
        song[QStringLiteral("isrc")] = QString();
        songs.append(song);
    }
    const QString json = QString::fromUtf8(QJsonDocument(songs).toJson(QJsonDocument::Compact));
    runTidal(this, QStringLiteral("push"), [name, json]() {
        return name + QChar('|') + callTidal("pushPlaylist",
                "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
                {name, json});
    });
#else
    Q_UNUSED(name);
#endif
}

namespace {

QString sunoDir() {
#if defined(Q_OS_ANDROID)
    return QStringLiteral("/sdcard/Music/djmantra_app/Suno");
#else
    return QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + QStringLiteral("/djmantra_app/Suno");
#endif
}

QJsonArray readSunoLinks() {
    QFile file(sunoDir() + QStringLiteral("/links.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonArray();
    }
    return QJsonDocument::fromJson(file.readAll()).array();
}

void writeSunoLinks(const QJsonArray& links) {
    QDir().mkpath(sunoDir());
    QSaveFile file(sunoDir() + QStringLiteral("/links.json"));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(links).toJson());
        file.commit();
    }
}

} // namespace

int QmlDJMantra::sunoAdd(const QString& name, const QString& text) {
    const QString playlist = name.trimmed().isEmpty() ? QStringLiteral("Suno") : name.trimmed();
    static const QRegularExpression linkPattern(
            QStringLiteral("https?://(?:www\\.|app\\.)?suno\\.(?:com|ai)/(?:song|s)/[A-Za-z0-9-]+"));
    QStringList urls;
    auto it = linkPattern.globalMatch(text);
    while (it.hasNext()) {
        const QString url = it.next().captured(0);
        if (!urls.contains(url)) {
            urls << url;
        }
    }
    if (urls.isEmpty()) {
        return 0;
    }
    // remembered at once (also those not saved yet: they are fetched again next time)
    QJsonArray links = readSunoLinks();
    for (const QString& url : std::as_const(urls)) {
        bool known = false;
        for (const auto& value : std::as_const(links)) {
            const QJsonObject link = value.toObject();
            if (link.value(QStringLiteral("url")).toString() == url &&
                    link.value(QStringLiteral("playlist")).toString() == playlist) {
                known = true;
            }
        }
        if (!known) {
            QJsonObject link;
            link[QStringLiteral("url")] = url;
            link[QStringLiteral("playlist")] = playlist;
            link[QStringLiteral("added")] = static_cast<double>(QDateTime::currentMSecsSinceEpoch());
            links.append(link);
        }
    }
    writeSunoLinks(links);
    qInfo().noquote() << "Suno:" << urls.size() << "links for" << playlist;
#if defined(Q_OS_ANDROID)
    QPointer<QmlDJMantra> guard(this);
    const QString dir = sunoDir();
    std::thread([guard, urls, playlist, dir]() {
        int added = 0;
        int failed = 0;
        int waiting = 0;
        for (const QString& url : urls) {
            const QString answer = QJniObject::callStaticObjectMethod("com/djmantra/app/SunoBridge",
                    "fetch",
                    "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
                    QJniObject::fromString(url).object<jstring>(),
                    QJniObject::fromString(dir).object<jstring>())
                                           .toString();
            const bool ok = !answer.startsWith(QStringLiteral("error"));
            const bool titleOnly = answer.startsWith(QStringLiteral("noaudio|"));
            titleOnly ? ++waiting : ok ? ++added : ++failed;
            QMetaObject::invokeMethod(qApp, [guard, url, playlist, answer, ok, titleOnly]() {
                if (!guard) {
                    return;
                }
                if (!ok) {
                    emit guard->sunoResult(QStringLiteral("error"), answer);
                    return;
                }
                if (titleOnly) {
                    // Suno keeps the audio for its logged-in owner: the title now, the song by
                    // GET (Suno's own download) or a file of that title
                    const QString title = answer.section(QChar('|'), 1);
                    QJsonArray links = readSunoLinks();
                    for (int i = 0; i < links.size(); ++i) {
                        QJsonObject link = links[i].toObject();
                        if (link.value(QStringLiteral("url")).toString() == url) {
                            link[QStringLiteral("title")] = title;
                            links[i] = link;
                        }
                    }
                    writeSunoLinks(links);
                    guard->sunoScan();
                    emit guard->sunoResult(QStringLiteral("title"), title);
                    return;
                }
                const QString path = answer.section(QChar('|'), 0, 0);
                const QString title = answer.section(QChar('|'), 1);
                guard->addToPlaylist(playlist, path);
                QJsonArray links = readSunoLinks();
                for (int i = 0; i < links.size(); ++i) {
                    QJsonObject link = links[i].toObject();
                    if (link.value(QStringLiteral("url")).toString() == url) {
                        link[QStringLiteral("title")] = title;
                        link[QStringLiteral("file")] = path;
                        links[i] = link;
                    }
                }
                writeSunoLinks(links);
                emit guard->sunoResult(QStringLiteral("song"), title);
            }, Qt::QueuedConnection);
        }
        QMetaObject::invokeMethod(qApp, [guard, added, failed, waiting]() {
            if (guard) {
                emit guard->sunoResult(QStringLiteral("done"),
                        QStringLiteral("%1|%2|%3").arg(added).arg(failed).arg(waiting));
            }
        }, Qt::QueuedConnection);
    }).detach();
#endif
    return urls.size();
}

void QmlDJMantra::sunoGet(const QString& url, const QString& title) {
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/djmantra/app/SunoActivity",
            "open",
            "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V",
            QNativeInterface::QAndroidApplication::context().object(),
            QJniObject::fromString(url).object<jstring>(),
            QJniObject::fromString(title).object<jstring>());
#else
    Q_UNUSED(url);
    Q_UNUSED(title);
#endif
}

int QmlDJMantra::sunoScan() {
    // a song's file: in djmantra_app/Suno or Download, its name holding the song's title
    // (letters and digits only, any case); one from Download is copied into djmantra_app/Suno
    const auto plain = [](const QString& text) {
        QString key = text.toLower();
        key.replace(QRegularExpression(QStringLiteral("[^\\p{L}\\p{N}]+")), QString());
        return key;
    };
    QList<QFileInfo> files;
    for (const QString& dir : {sunoDir(),
#if defined(Q_OS_ANDROID)
                 QStringLiteral("/sdcard/Download")
#else
                 QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
#endif
         }) {
        for (const QFileInfo& info : QDir(dir).entryInfoList(QDir::Files | QDir::Readable, QDir::Time)) {
            if (SoundSourceProxy::isFileNameSupported(info.fileName())) {
                files << info;
            }
        }
    }
    QJsonArray links = readSunoLinks();
    int found = 0;
    for (int i = 0; i < links.size(); ++i) {
        QJsonObject link = links[i].toObject();
        const QString title = link.value(QStringLiteral("title")).toString();
        const QString have = link.value(QStringLiteral("file")).toString();
        if (title.isEmpty() || (!have.isEmpty() && QFileInfo::exists(have))) {
            continue;
        }
        const QString key = plain(title);
        for (const QFileInfo& info : std::as_const(files)) {
            if (key.isEmpty() || !plain(info.completeBaseName()).contains(key)) {
                continue;
            }
            QString path = info.absoluteFilePath();
            if (info.absolutePath() != sunoDir()) {
                const QString copy = sunoDir() + QChar('/') + info.fileName();
                if (QFile::exists(copy) || QFile::copy(path, copy)) {
                    path = copy;
                }
            }
            link[QStringLiteral("file")] = path;
            links[i] = link;
            addToPlaylist(link.value(QStringLiteral("playlist")).toString(), path);
            qInfo().noquote() << "Suno: found" << title << "->" << path;
            ++found;
            break;
        }
    }
    if (found > 0) {
        writeSunoLinks(links);
    }
    return found;
}

QVariantList QmlDJMantra::sunoLinks() const {
    return readSunoLinks().toVariantList();
}

void QmlDJMantra::tidalSyncAll() {
#if defined(Q_OS_ANDROID)
    QList<QPair<QString, QString>> lists;
    for (const QVariant& root : playlistRoots()) {
        const QString name = root.toMap().value(QStringLiteral("name")).toString();
        QJsonArray songs;
        for (const QVariant& entry : listFolder(QStringLiteral("playlist:") + name)) {
            const QVariantMap info = songInfo(entry.toMap().value(QStringLiteral("path")).toString());
            QJsonObject song;
            song[QStringLiteral("artist")] = info.value(QStringLiteral("artist")).toString();
            song[QStringLiteral("title")] = info.value(QStringLiteral("title")).toString();
            song[QStringLiteral("isrc")] = QString();
            songs.append(song);
        }
        if (!songs.isEmpty()) {
            lists << qMakePair(name, QString::fromUtf8(QJsonDocument(songs).toJson(QJsonDocument::Compact)));
        }
    }
    QPointer<QmlDJMantra> guard(this);
    std::thread([guard, lists]() {
        for (const auto& list : lists) {
            const QString answer = list.first + QChar('|') + callTidal("pushPlaylist",
                    "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
                    {list.first, list.second});
            QMetaObject::invokeMethod(qApp, [guard, answer]() {
                if (guard) {
                    emit guard->tidalResult(QStringLiteral("push"), answer);
                }
            }, Qt::QueuedConnection);
        }
        const int count = static_cast<int>(lists.size());
        QMetaObject::invokeMethod(qApp, [guard, count]() {
            if (guard) {
                emit guard->tidalResult(QStringLiteral("sync"), QString::number(count));
            }
        }, Qt::QueuedConnection);
    }).detach();
#endif
}

QVariantList QmlDJMantra::tidalKeyFiles() const {
    QVariantList files;
#if defined(Q_OS_ANDROID)
    const QStringList dirs = {djmantraDocs(QStringLiteral("Keys")),
            QStringLiteral("/sdcard/Music/djmantra_app/Keys"),
            QStringLiteral("/sdcard/Download"),
            QStringLiteral("/sdcard/Documents")};
#else
    const QStringList dirs = {djmantraDocs(QStringLiteral("Keys")),
            QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
                    QStringLiteral("/djmantra_app/Keys"),
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)};
#endif
    for (const QString& dir : dirs) {
        const auto entries = QDir(dir).entryInfoList(
                {QStringLiteral("*.txt"), QStringLiteral("*.json"), QStringLiteral("*.env")},
                QDir::Files | QDir::Readable,
                QDir::Name | QDir::IgnoreCase);
        for (const QFileInfo& info : entries) {
            QVariantMap entry;
            entry[QStringLiteral("name")] = info.fileName();
            entry[QStringLiteral("path")] = info.absoluteFilePath();
            entry[QStringLiteral("folder")] = dir;
            files << entry;
        }
    }
    return files;
}

QString QmlDJMantra::tidalUseKeyFile(const QString& path) {
    // only the Client ID: a "client id" line, else the first 16-character code not on a line
    // naming the secret; nothing else from the file is kept or logged
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 65536) {
        return QString();
    }
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QChar('\n'));
    static const QRegularExpression labelled(QStringLiteral(
            "client[ _-]?id\\W{0,4}([A-Za-z0-9]{8,64})"), QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression code(QStringLiteral("\\b([A-Za-z0-9]{16})\\b"));
    QString id;
    for (const QString& line : lines) {
        const auto m = labelled.match(line);
        if (m.hasMatch()) {
            id = m.captured(1);
            break;
        }
    }
    for (int i = 0; id.isEmpty() && i < lines.size(); ++i) {
        if (lines[i].contains(QStringLiteral("secret"), Qt::CaseInsensitive)) {
            continue;
        }
        const auto m = code.match(lines[i]);
        if (m.hasMatch()) {
            id = m.captured(1);
        }
    }
    if (id.isEmpty()) {
        qInfo() << "TIDAL: no Client ID in the key file";
        return QString();
    }
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/djmantra/app/TidalBridge", "setClientId",
            "(Landroid/content/Context;Ljava/lang/String;)V",
            QNativeInterface::QAndroidApplication::context().object(),
            QJniObject::fromString(id).object<jstring>());
#endif
    qInfo() << "TIDAL: Client ID taken from the key file";
    emit tidalResult(QStringLiteral("logout"), QString());
    return id;
}

QString QmlDJMantra::tidalClientId() const {
#if defined(Q_OS_ANDROID)
    return QJniObject::callStaticObjectMethod("com/djmantra/app/TidalBridge", "clientId",
            "(Landroid/content/Context;)Ljava/lang/String;",
            QNativeInterface::QAndroidApplication::context().object())
            .toString();
#else
    return QStringLiteral("JjTAmDVohiV7GgOL");
#endif
}

QVariantList QmlDJMantra::playlistRoots() {
    importM3uPlaylists();
    QVariantList roots;
    QSet<QString> sunoPlaylists;
    for (const auto& link : readSunoLinks()) {
        sunoPlaylists.insert(link.toObject().value(QStringLiteral("playlist")).toString());
    }
    const QJsonArray playlists = readPlaylists();
    for (const auto& value : playlists) {
        const QJsonObject playlist = value.toObject();
        QVariantMap entry;
        const QString name = playlist.value(QStringLiteral("name")).toString();
        entry[QStringLiteral("name")] = name;
        entry[QStringLiteral("path")] = QStringLiteral("playlist:") + name;
        entry[QStringLiteral("isFolder")] = true;
        entry[QStringLiteral("created")] = playlist.value(QStringLiteral("created")).toDouble();
        // the songs still on the phone; a playlist without any is not shown (Marko 14.10.2026)
        int songs = 0;
        for (const auto& song : playlist.value(QStringLiteral("songs")).toArray()) {
            if (QFileInfo::exists(song.toString())) {
                ++songs;
            }
        }
        if (songs == 0) {
            continue;
        }
        entry[QStringLiteral("count")] = songs;
        if (sunoPlaylists.contains(name)) {
            entry[QStringLiteral("artist")] = QStringLiteral("SUNO PLAYLIST");
        }
        roots << entry;
    }
    return roots;
}

bool QmlDJMantra::addToPlaylist(const QString& name, const QString& path) {
    const QString clean = name.trimmed();
    if (clean.isEmpty() || path.isEmpty()) {
        return false;
    }
    QJsonArray playlists = readPlaylists();
    int i = findPlaylist(playlists, clean);
    if (i < 0) {
        QJsonObject playlist;
        playlist[QStringLiteral("name")] = clean;
        playlist[QStringLiteral("created")] = static_cast<double>(QDateTime::currentMSecsSinceEpoch());
        playlist[QStringLiteral("songs")] = QJsonArray();
        playlists.append(playlist);
        i = playlists.size() - 1;
    }
    QJsonObject playlist = playlists[i].toObject();
    QJsonArray songs = playlist.value(QStringLiteral("songs")).toArray();
    if (songs.contains(path)) {
        return false;
    }
    songs.append(path);
    playlist[QStringLiteral("songs")] = songs;
    playlists[i] = playlist;
    writePlaylists(playlists);
    qInfo().noquote() << "Playlists: added" << path << "to" << clean;
    return true;
}

bool QmlDJMantra::removeFromPlaylist(const QString& name, const QString& path) {
    QJsonArray playlists = readPlaylists();
    const int i = findPlaylist(playlists, name);
    if (i < 0) {
        return false;
    }
    QJsonObject playlist = playlists[i].toObject();
    QJsonArray songs = playlist.value(QStringLiteral("songs")).toArray();
    for (int k = 0; k < songs.size(); ++k) {
        if (songs[k].toString() == path) {
            songs.removeAt(k);
            playlist[QStringLiteral("songs")] = songs;
            playlists[i] = playlist;
            writePlaylists(playlists);
            qInfo().noquote() << "Playlists: removed" << path << "from" << name;
            return true;
        }
    }
    return false;
}

double QmlDJMantra::playlistLength(const QString& name) const {
    const QJsonArray playlists = readPlaylists();
    const int i = findPlaylist(playlists, name);
    if (i < 0) {
        return 0;
    }
    double total = 0;
    for (const auto& song : playlists[i].toObject().value(QStringLiteral("songs")).toArray()) {
        total += songInfo(song.toString()).value(QStringLiteral("duration")).toDouble();
    }
    return total;
}

// a folder with at least one song in it or in any folder inside it (Marko 14.10.2026: "I want
// only playlists which contain some songs"; the scan stops at the first song)
static bool hasSongs(const QString& dir) {
    QDirIterator it(dir, QDir::Files | QDir::Readable, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        if (!it.fileName().startsWith(QChar('.')) && SoundSourceProxy::isFileNameSupported(it.fileName())) {
            return true;
        }
    }
    return false;
}

QVariantList QmlDJMantra::listFolder(const QString& folder) const {
    QVariantList folders;
    QVariantList songs;
    if (folder.startsWith(QStringLiteral("playlist:"))) {
        // a playlist: its songs in its order (the ones still on the phone)
        const QJsonArray playlists = readPlaylists();
        const int i = findPlaylist(playlists, folder.mid(9));
        if (i >= 0) {
            for (const auto& song : playlists[i].toObject().value(QStringLiteral("songs")).toArray()) {
                const QFileInfo info(song.toString());
                if (!info.exists()) {
                    continue;
                }
                QVariantMap entry;
                entry[QStringLiteral("name")] = info.completeBaseName();
                entry[QStringLiteral("path")] = info.absoluteFilePath();
                entry[QStringLiteral("isFolder")] = false;
                songs << entry;
            }
        }
        return songs;
    }
    const QDir dir(folder);
    const auto entries = dir.entryInfoList(
            QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable,
            QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& info : entries) {
        // hidden; Room_Corrections holds the room test signals, not songs (r41 V70)
        if (info.fileName().startsWith(QChar('.')) ||
                (info.isDir() && info.fileName() == QStringLiteral("Room_Corrections"))) {
            continue;
        }
        QVariantMap entry;
        entry[QStringLiteral("name")] = info.isDir() ? info.fileName() : info.completeBaseName();
        entry[QStringLiteral("path")] = info.absoluteFilePath();
        entry[QStringLiteral("isFolder")] = info.isDir();
        if (info.isDir()) {
            // empty folders (no song anywhere inside) are not shown
            if (hasSongs(info.absoluteFilePath())) {
                folders << entry;
            }
        } else if (SoundSourceProxy::isFileNameSupported(info.fileName())) {
            songs << entry;
        }
    }
    return folders + songs;
}

QVariantList QmlDJMantra::findSongs(const QString& query) const {
    QVariantList result;
    const QStringList words = query.simplified().toLower().split(QChar(' '), Qt::SkipEmptyParts);
    if (words.isEmpty()) {
        return result;
    }
    const auto add = [&result](const QString& path, const QString& title, const QString& artist,
                             double seconds) {
        const QFileInfo info(path);
        QVariantMap entry;
        entry[QStringLiteral("name")] = info.completeBaseName();
        entry[QStringLiteral("path")] = path;
        entry[QStringLiteral("isFolder")] = false;
        entry[QStringLiteral("title")] = title.isEmpty() ? info.completeBaseName() : title;
        entry[QStringLiteral("artist")] = artist == QStringLiteral("<unknown>") ? QString() : artist;
        entry[QStringLiteral("duration")] = seconds;
        result << entry;
    };
#if defined(Q_OS_ANDROID)
    const QJniObject array = QJniObject::callStaticObjectMethod(
            "com/djmantra/app/StorageBridge",
            "findSongs",
            "(Landroid/content/Context;Ljava/lang/String;)[Ljava/lang/String;",
            QNativeInterface::QAndroidApplication::context().object(),
            QJniObject::fromString(query).object<jstring>());
    QJniEnvironment env;
    if (env.checkAndClearExceptions() || !array.isValid()) {
        qWarning() << "Find song: the media index could not be read";
        return result;
    }
    const auto jArray = static_cast<jobjectArray>(array.object());
    const jsize count = env->GetArrayLength(jArray);
    for (jsize i = 0; i < count; ++i) {
        const auto jItem = static_cast<jstring>(env->GetObjectArrayElement(jArray, i));
        const QStringList parts = QJniObject(jItem).toString().split(QChar('\t'));
        env->DeleteLocalRef(jItem);
        if (parts.size() >= 4 && QFileInfo::exists(parts[0]) &&
                SoundSourceProxy::isFileNameSupported(parts[0])) {
            add(parts[0], parts[1], parts[2], parts[3].toDouble() / 1000.0);
        }
    }
#else
    // desktop: the file names under the music folder
    QDirIterator it(QStandardPaths::writableLocation(QStandardPaths::MusicLocation),
            QDir::Files | QDir::Readable,
            QDirIterator::Subdirectories);
    while (it.hasNext() && result.size() < 300) {
        const QString path = it.next();
        const QString name = it.fileInfo().completeBaseName().toLower();
        if (!SoundSourceProxy::isFileNameSupported(path)) {
            continue;
        }
        bool all = true;
        for (const QString& word : words) {
            all = all && name.contains(word);
        }
        if (all) {
            add(path, QString(), QString(), 0);
        }
    }
#endif
    qInfo().noquote() << "Find song:" << query << "->" << result.size();
    return result;
}

QVariantMap QmlDJMantra::songInfo(const QString& path) const {
    QVariantMap info;
    const auto pLibrary = QmlLibraryProxy::library();
    if (!pLibrary) {
        return info;
    }
    const TrackPointer pTrack = pLibrary->trackCollectionManager()->getOrAddTrack(
            TrackRef::fromFilePath(path));
    if (!pTrack) {
        return info;
    }
    info[QStringLiteral("artist")] = pTrack->getArtist();
    info[QStringLiteral("title")] = pTrack->getTitle();
    info[QStringLiteral("bpm")] = pTrack->getBpm() > 0 ? pTrack->getBpm() : 0.0;
    info[QStringLiteral("duration")] = pTrack->getDuration(); // seconds (the playlist's total)
    const auto code = djmantra::camelot::fromKey(pTrack->getKey());
    info[QStringLiteral("key")] = code.isValid() ? djmantra::camelot::text(code) : QString();
    info[QStringLiteral("cover")] = pTrack->getCoverInfo().hasImage()
            ? AsyncImageProvider::trackLocationToCoverArtUrl(pTrack->getLocation()).toString()
            : QString();
    return info;
}

namespace {

// One line per analysed song for the comparison with rekordbox
// (testing/sections/REKORDBOX_COMPARE.md): the tester reads it with logcat
void logAnalysis(const TrackPointer& pTrack) {
    if (!pTrack) {
        return;
    }
    const auto pBeats = pTrack->getBeats();
    const auto sampleRate = pTrack->getSampleRate();
    double firstBeat = -1;
    if (pBeats && sampleRate.isValid() && pBeats->firstBeat().isValid()) {
        firstBeat = pBeats->firstBeat().value() / sampleRate.toDouble();
    }
    qInfo().noquote() << QStringLiteral("Analysis: %1\tbpm %2\tkey %3\tfirst_beat_s %4\tconst %5")
                                 .arg(QFileInfo(pTrack->getLocation()).fileName())
                                 .arg(pTrack->getBpm(), 0, 'f', 4)
                                 .arg(pTrack->getKeyText())
                                 .arg(firstBeat, 0, 'f', 4)
                                 .arg(pBeats && pBeats->hasConstantTempo() ? 1 : 0);
}

} // namespace

void QmlDJMantra::analyzeAhead(const QStringList& paths) {
    const auto pLibrary = QmlLibraryProxy::library();
    if (!pLibrary) {
        return;
    }
    QList<AnalyzerScheduledTrack> tracks;
    for (const auto& path : paths) {
        const TrackPointer pTrack = pLibrary->trackCollectionManager()->getOrAddTrack(
                TrackRef::fromFilePath(path));
        if (!pTrack || !pTrack->getId().isValid()) {
            continue;
        }
        const bool hasKey = djmantra::camelot::fromKey(pTrack->getKey()).isValid();
        if (pTrack->getBpm() > 0 && hasKey) {
            logAnalysis(pTrack); // analysed before
            continue;
        }
        tracks.append(AnalyzerScheduledTrack(pTrack->getId()));
    }
    scheduleAnalysis(tracks);
}

void QmlDJMantra::useDjmFilter() {
    // Marko 13.10.2026: the decks' FILTER (the quick effect knob, super1) is the
    // Pioneer DJM Color FX filter (djayfx.h DjmFilter), not Mixxx's own filter
    const auto pEffects = QmlEffectsManagerProxy::effectsManager();
    if (!pEffects) {
        return;
    }
    const EffectManifestPointer pManifest = pEffects->getBackendManager()->getManifest(
            QStringLiteral("org.djmantra.effects.djmfilter"), EffectBackendType::BuiltIn);
    if (!pManifest) {
        qWarning() << "DJM filter: not registered";
        return;
    }
    for (int deck = 1; deck <= 4; ++deck) {
        const QString group = QStringLiteral("[Channel%1]").arg(deck);
        const auto pChain = pEffects->getQuickEffectChain(group);
        if (!pChain) {
            continue;
        }
        const EffectSlotPointer pSlot = pChain->getEffectSlot(0);
        if (!pSlot || (pSlot->getManifest() && pSlot->getManifest()->id() == pManifest->id())) {
            continue;
        }
        pSlot->loadEffectWithDefaults(pManifest);
        qInfo().noquote() << "DJM filter: loaded on" << group;
    }
}

namespace {
const QString kRoomEqGroup = QStringLiteral("[OutputEffectRack_[Master]_Effect1]");
constexpr int kRoomBands = 31;

#if defined(Q_OS_ANDROID)
// the microphone, asked for once; `then` runs when it is allowed
void withMicrophone(QObject* pContext, std::function<void(bool)> then) {
    QMicrophonePermission microphone;
    if (qApp->checkPermission(microphone) == Qt::PermissionStatus::Granted) {
        then(true);
        return;
    }
    qApp->requestPermission(microphone, pContext, [then](const QPermission& permission) {
        then(permission.status() == Qt::PermissionStatus::Granted);
    });
}
#endif
} // namespace

void QmlDJMantra::useRoomEq() {
    const auto pEffects = QmlEffectsManagerProxy::effectsManager();
    if (!pEffects) {
        return;
    }
    const EffectManifestPointer pManifest = pEffects->getBackendManager()->getManifest(
            QStringLiteral("org.djmantra.effects.roomeq"), EffectBackendType::BuiltIn);
    const auto pChain = pEffects->getOutputEffectChain();
    if (!pManifest || !pChain) {
        qWarning() << "Room EQ: not available";
        return;
    }
    const EffectSlotPointer pSlot = pChain->getEffectSlot(0);
    if (pSlot && !(pSlot->getManifest() && pSlot->getManifest()->id() == pManifest->id())) {
        pSlot->loadEffectWithDefaults(pManifest);
        qInfo() << "Room EQ: loaded on the main output";
    }
    // the 31 bands as left (a V67 10-band curve is not taken over)
    const QStringList saved = setting(QStringLiteral("room_eq"), QString()).toString().split(QChar(','));
    for (int i = 0; i < kRoomBands; ++i) {
        setControl(kRoomEqGroup, QStringLiteral("parameter%1").arg(i + 1),
                saved.size() == kRoomBands ? saved[i].toDouble() : 0.0);
    }
    setRoomEqOn(setting(QStringLiteral("room_eq_on"), QStringLiteral("0")).toString() == QStringLiteral("1"));
}

void QmlDJMantra::setRoomEqOn(bool on) {
    setControl(kRoomEqGroup, QStringLiteral("enabled"), on ? 1.0 : 0.0);
    setSetting(QStringLiteral("room_eq_on"), on ? QStringLiteral("1") : QStringLiteral("0"));
}

void QmlDJMantra::setRoomBand(int band, double db) {
    if (band < 0 || band >= kRoomBands) {
        return;
    }
    setControl(kRoomEqGroup, QStringLiteral("parameter%1").arg(band + 1), std::clamp(db, -12.0, 12.0));
    QStringList values;
    for (const QVariant& v : roomBands()) {
        values << QString::number(v.toDouble(), 'f', 1);
    }
    setSetting(QStringLiteral("room_eq"), values.join(QChar(',')));
}

QVariantList QmlDJMantra::roomBands() const {
    QVariantList bands;
    for (int i = 0; i < kRoomBands; ++i) {
        ControlProxy proxy(ConfigKey(kRoomEqGroup, QStringLiteral("parameter%1").arg(i + 1)),
                nullptr,
                ControlFlag::NoWarnIfMissing);
        bands << (proxy.valid() ? proxy.get() : 0.0);
    }
    return bands;
}

static QString roomDir() {
    return djmantraDocs(QStringLiteral("Room_Corrections"));
}

QString QmlDJMantra::roomEqSignal(const QString& kind, double seconds) {
#if defined(Q_OS_ANDROID)
    return QJniObject::callStaticObjectMethod("com/djmantra/app/RoomEqBridge",
            "writeSignal",
            "(Ljava/lang/String;Ljava/lang/String;D)Ljava/lang/String;",
            QJniObject::fromString(roomDir()).object<jstring>(),
            QJniObject::fromString(kind).object<jstring>(),
            static_cast<jdouble>(seconds))
            .toString();
#else
    Q_UNUSED(kind);
    Q_UNUSED(seconds);
    return QString();
#endif
}

void QmlDJMantra::roomEqMeasure(const QString& kind, double seconds, int inputId) {
#if defined(Q_OS_ANDROID)
    QPointer<QmlDJMantra> guard(this);
    withMicrophone(this, [guard, kind, seconds, inputId](bool allowed) {
        if (!guard) {
            return;
        }
        if (!allowed) {
            emit guard->roomEqResult(QStringLiteral("measure"),
                    QStringLiteral("error: DJ Mantra may not use the microphone"));
            return;
        }
        // the recording starts now; QML starts the test signal on the mixer right after
        emit guard->roomEqResult(QStringLiteral("recording"), QString());
        std::thread([guard, kind, seconds, inputId]() {
            const QString result = QJniObject::callStaticObjectMethod("com/djmantra/app/RoomEqBridge",
                    "measure",
                    "(Landroid/content/Context;Ljava/lang/String;DI)Ljava/lang/String;",
                    QNativeInterface::QAndroidApplication::context().object(),
                    QJniObject::fromString(kind).object<jstring>(),
                    static_cast<jdouble>(seconds),
                    static_cast<jint>(inputId))
                                           .toString();
            QMetaObject::invokeMethod(qApp, [guard, result]() {
                if (guard) {
                    emit guard->roomEqResult(QStringLiteral("measure"), result);
                }
            }, Qt::QueuedConnection);
        }).detach();
    });
#else
    Q_UNUSED(kind);
    Q_UNUSED(seconds);
    Q_UNUSED(inputId);
    emit roomEqResult(QStringLiteral("measure"), QStringLiteral("error: only on the phone"));
#endif
}

QVariantList QmlDJMantra::roomEqInputs() const {
#if defined(Q_OS_ANDROID)
    const QString json = QJniObject::callStaticObjectMethod("com/djmantra/app/RoomEqBridge",
            "inputs",
            "(Landroid/content/Context;)Ljava/lang/String;",
            QNativeInterface::QAndroidApplication::context().object())
                                 .toString();
    return QJsonDocument::fromJson(json.toUtf8()).array().toVariantList();
#else
    QVariantMap phone;
    phone[QStringLiteral("id")] = 0;
    phone[QStringLiteral("name")] = QStringLiteral("Phone microphone");
    return {phone};
#endif
}

void QmlDJMantra::roomEqLive(bool on, int inputId) {
#if defined(Q_OS_ANDROID)
    const auto start = [on, inputId](bool allowed) {
        QJniObject::callStaticMethod<void>("com/djmantra/app/RoomEqBridge",
                "live",
                "(Landroid/content/Context;ZI)V",
                QNativeInterface::QAndroidApplication::context().object(),
                static_cast<jboolean>(on && allowed),
                static_cast<jint>(inputId));
    };
    if (on) {
        withMicrophone(this, start);
    } else {
        start(false);
    }
#else
    Q_UNUSED(on);
    Q_UNUSED(inputId);
#endif
}

static QString roomFile(const QString& name) {
    QString safe = name.trimmed();
    safe.replace(QRegularExpression(QStringLiteral("[/\\\\:*?\"<>|]")), QStringLiteral("_"));
    return roomDir() + QChar('/') + (safe.isEmpty() ? QStringLiteral("Room") : safe) + QStringLiteral(".json");
}

QString QmlDJMantra::roomsLoad() const {
    QJsonArray rooms;
    const auto files = QDir(roomDir()).entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& info : files) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject() && doc.object().contains(QStringLiteral("points"))) {
            rooms.append(doc.object());
        } else if (doc.isArray()) {
            // V68 kept all rooms in one rooms.json: taken over, one file each
            for (const auto& room : doc.array()) {
                rooms.append(room);
                const_cast<QmlDJMantra*>(this)->roomSave(
                        QString::fromUtf8(QJsonDocument(room.toObject()).toJson()));
            }
            QFile::remove(info.absoluteFilePath());
        }
    }
    return QString::fromUtf8(QJsonDocument(rooms).toJson(QJsonDocument::Compact));
}

QString QmlDJMantra::roomSave(const QString& json) {
    const QJsonObject room = QJsonDocument::fromJson(json.toUtf8()).object();
    const QString path = roomFile(room.value(QStringLiteral("name")).toString());
    QDir().mkpath(roomDir());
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(room).toJson(QJsonDocument::Indented));
        file.commit();
    }
    return path;
}

void QmlDJMantra::roomDelete(const QString& name) {
    QFile::remove(roomFile(name));
}

QString QmlDJMantra::roomEqLiveColumns() const {
#if defined(Q_OS_ANDROID)
    return QJniObject::callStaticObjectMethod("com/djmantra/app/RoomEqBridge",
            "liveColumns",
            "()Ljava/lang/String;")
            .toString();
#else
    return QString();
#endif
}

int QmlDJMantra::batteryPercent() const {
#if defined(Q_OS_ANDROID)
    return QJniObject::callStaticMethod<jint>("com/djmantra/app/WindowBridge",
            "battery",
            "(Landroid/content/Context;)I",
            QNativeInterface::QAndroidApplication::context().object());
#else
    return -1;
#endif
}

void QmlDJMantra::setAutoGain(bool on) {
    // Mixxx's ReplayGain stage applies each song's ratio (made peak-based by
    // AnalyzerEbur128 on Android): on = the song to -1 dBFS, no extra boost
    ControlObject::set(ConfigKey(QStringLiteral("[ReplayGain]"), QStringLiteral("ReplayGainEnabled")), on ? 1 : 0);
    ControlObject::set(ConfigKey(QStringLiteral("[ReplayGain]"), QStringLiteral("ReplayGainBoost")), 1);
    ControlObject::set(ConfigKey(QStringLiteral("[ReplayGain]"), QStringLiteral("DefaultBoost")), 1);
    setSetting(QStringLiteral("auto_gain"), on ? QStringLiteral("1") : QStringLiteral("0"));
    qInfo() << "Auto gain:" << (on ? "on" : "off");
}

void QmlDJMantra::setControllerLink(const QString& link) {
    setSetting(QStringLiteral("controller_link_mode"), link);
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/djmantra/app/MidiBridge",
            "setLink",
            "(Landroid/content/Context;Ljava/lang/String;)V",
            QNativeInterface::QAndroidApplication::context().object(),
            QJniObject::fromString(link).object<jstring>());
#endif
    qInfo() << "Controller link:" << link;
}

QString QmlDJMantra::startRecording(const QString& format, const QString& name) {
    if (!m_pConfig) {
        return QString();
    }
    if (ControlObject::get(ConfigKey(QStringLiteral("[Recording]"), QStringLiteral("status"))) > 0) {
        return QString();
    }
#if defined(Q_OS_ANDROID)
    // the app's root (Marko 14.10.2026): djmantra_app/Recordings
    const QString dir = QStringLiteral("/sdcard/Music/djmantra_app/Recordings");
#else
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
            QStringLiteral("/djmantra_app/Recordings");
#endif
    QDir().mkpath(dir);
    const ConfigKey encoding(QStringLiteral("[Recording]"), QStringLiteral("Encoding"));
    m_pConfig->set(ConfigKey(QStringLiteral("[Recording]"), QStringLiteral("Directory")), dir);
    // no characters a file name cannot have
    QString prefix = name.trimmed();
    prefix.replace(QRegularExpression(QStringLiteral("[/\\\\:*?\"<>|]")), QStringLiteral("-"));
    m_pConfig->set(ConfigKey(QStringLiteral("[Recording]"), QStringLiteral("NamePrefix")),
            prefix.isEmpty() ? QStringLiteral("DJ Mantra Mix") : prefix);
    m_pConfig->set(encoding,
            format.compare(QStringLiteral("WAV"), Qt::CaseInsensitive) == 0
                    ? QStringLiteral("WAV")
                    : QStringLiteral("FLAC"));
    // a push: RecordingManager toggles on the press
    setControl(QStringLiteral("[Recording]"), QStringLiteral("toggle_recording"), 1);
    setControl(QStringLiteral("[Recording]"), QStringLiteral("toggle_recording"), 0);
    const QString path = recordingPath();
    qInfo() << "Recording: started" << m_pConfig->getValueString(encoding) << path;
    return path;
}

void QmlDJMantra::stopRecording() {
    if (ControlObject::get(ConfigKey(QStringLiteral("[Recording]"), QStringLiteral("status"))) <= 0) {
        return;
    }
    const QString path = recordingPath();
    setControl(QStringLiteral("[Recording]"), QStringLiteral("toggle_recording"), 1);
    setControl(QStringLiteral("[Recording]"), QStringLiteral("toggle_recording"), 0);
    qInfo() << "Recording: stopped" << path << QFileInfo(path).size() << "bytes";
}

QString QmlDJMantra::recordingPath() const {
    return m_pConfig ? m_pConfig->getValueString(
                               ConfigKey(QStringLiteral("[Recording]"), QStringLiteral("Path")))
                     : QString();
}

void QmlDJMantra::stopAnalysis() {
    // Marko 13.10.2026: only the folder that is open is analysed; leaving it stops
    // the analysis (the next visit goes on from the songs not done yet)
    if (m_pAnalysis) {
        m_pAnalysis->stop();
        m_pAnalysis.reset();
        qInfo() << "Analysis: stopped (the folder was left)";
    }
    if (m_analysisLeft != 0) {
        m_analysisLeft = 0;
        emit analysisChanged();
    }
}

int QmlDJMantra::analyzeDecksAgain() {
    const auto pLibrary = QmlLibraryProxy::library();
    if (!pLibrary) {
        return 0;
    }
    QList<AnalyzerScheduledTrack> tracks;
    for (int deck = 1; deck <= 4; ++deck) {
        const QString group = QStringLiteral("[Channel%1]").arg(deck);
        const TrackPointer pTrack = PlayerInfo::instance().getTrackInfo(group);
        if (!pTrack || isPlaying(group) || pTrack->isBpmLocked()) {
            continue;
        }
        // without beats the analyzer runs again, with the options of Settings
        pTrack->trySetBeats(mixxx::BeatsPointer());
        tracks.append(AnalyzerScheduledTrack(pTrack->getId()));
        qInfo().noquote() << "Settings: analysing again" << group << pTrack->getLocation();
    }
    scheduleAnalysis(tracks);
    return static_cast<int>(tracks.size());
}

void QmlDJMantra::scheduleAnalysis(const QList<AnalyzerScheduledTrack>& tracks) {
    const auto pLibrary = QmlLibraryProxy::library();
    if (tracks.isEmpty() || !pLibrary) {
        return;
    }
    if (!m_pAnalysis) {
        // One thread, low priority, beats and key only (no waveform): the
        // decks' sound must not drop while it runs
        m_pAnalysis = pLibrary->createTrackAnalysisScheduler(1,
                static_cast<AnalyzerModeFlags>(
                        AnalyzerModeFlags::WithBeats | AnalyzerModeFlags::LowPriority));
        connect(m_pAnalysis.get(),
                &TrackAnalysisScheduler::progress,
                this,
                [this](AnalyzerProgress, int current, int total) {
                    const int left = std::max(0, total - current);
                    if (left != m_analysisLeft) {
                        m_analysisLeft = left;
                        emit analysisChanged();
                    }
                });
        connect(m_pAnalysis.get(),
                &TrackAnalysisScheduler::trackProgress,
                this,
                [](TrackId trackId, AnalyzerProgress progress) {
                    if (progress < kAnalyzerProgressDone) {
                        return;
                    }
                    if (const auto pLibrary = QmlLibraryProxy::library()) {
                        logAnalysis(pLibrary->trackCollectionManager()->getTrackById(trackId));
                    }
                });
        connect(m_pAnalysis.get(), &TrackAnalysisScheduler::finished, this, [this] {
            qInfo() << "Quick: analysis in advance finished";
            m_analysisLeft = 0;
            emit analysisChanged();
            // deleted later, not from inside its own signal
            QTimer::singleShot(0, this, [this] {
                if (m_analysisLeft == 0) {
                    m_pAnalysis.reset();
                }
            });
        });
    }
    const int scheduled = m_pAnalysis->scheduleTracks(tracks);
    m_pAnalysis->resume();
    m_analysisLeft += scheduled;
    emit analysisChanged();
    qInfo() << "Quick: analysing" << scheduled << "songs in advance (BPM, key)";
}

void QmlDJMantra::log(const QString& text) const {
    qInfo().noquote() << "Quick:" << text;
}

bool QmlDJMantra::setControl(const QString& group, const QString& key, double value) const {
    // Through a proxy, as a controller or QML does: ControlObject::set() names the
    // object itself as the sender, so its owner (an effect slot's "enabled", a unit's
    // routing) never hears of the change (round 38: Echo Out never ran)
    ControlProxy proxy(ConfigKey(group, key), nullptr, ControlFlag::NoWarnIfMissing);
    if (!proxy.valid()) {
        qWarning() << "setControl: no control" << group << key;
        return false;
    }
    proxy.set(value);
    return true;
}


// ---- the Allen & Heath CQ (qml/cqlink.h; testing/results/20261013_cq12t/midi.md) ----

QString QmlDJMantra::cqStatus() const {
    return m_pCq ? m_pCq->status() : QString();
}

bool QmlDJMantra::cqConnected() const {
    return m_pCq && m_pCq->isConnected();
}

void QmlDJMantra::cqConnect(const QString& host) {
    setSetting(QStringLiteral("cq_ip"), host.trimmed());
    m_cqTappedBpm = 0;
    m_pCq->connectTo(host);
}

void QmlDJMantra::cqDisconnect() {
    setSetting(QStringLiteral("cq_ip"), QString());
    m_pCq->disconnectFrom();
}

void QmlDJMantra::cqSetDelay(bool on) {
    // the CQ input the app plays into (1 = Ip1/2, USB 1-2) -> FX n send; FX n unmuted
    const int input = std::clamp(setting(QStringLiteral("cq_input"), QStringLiteral("1")).toInt(), 1, 16);
    const int fx = std::clamp(setting(QStringLiteral("cq_fx"), QStringLiteral("1")).toInt(), 1, 4);
    m_pCq->setMute(0x00, 0x50 + fx, false);
    m_pCq->setLevel(0x4C, 0x14 + (input - 1) * 4 + (fx - 1), on ? 0.0 : -100.0);
    m_cqDelayOn = on;
    qInfo().noquote() << "CQ: delay" << (on ? "on" : "off") << "input" << input << "FX" << fx;
    if (on) {
        cqTapNow();
    }
    emit cqChanged();
}

void QmlDJMantra::cqTapNow() {
    const int key = setting(QStringLiteral("cq_tap_key"), QStringLiteral("0")).toInt();
    const double bpm = onAirBpm();
    if (key >= 1 && key <= 3 && bpm > 0) {
        m_pCq->tapTempo(key, bpm);
        m_cqTappedBpm = bpm;
    }
}

double QmlDJMantra::onAirBpm() const {
    // the playing deck that is loudest in the mix (volume; the crossfader's side)
    double best = -1;
    double bpm = 0;
    const double crossfader = ControlObject::get(ConfigKey(QStringLiteral("[Master]"), QStringLiteral("crossfader")));
    for (int deck = 1; deck <= 2; ++deck) {
        const QString group = QStringLiteral("[Channel%1]").arg(deck);
        if (ControlObject::get(ConfigKey(group, QStringLiteral("play"))) <= 0) {
            continue;
        }
        const double side = deck == 1 ? 1 - std::max(0.0, crossfader) : 1 + std::min(0.0, crossfader);
        const double weight = ControlObject::get(ConfigKey(group, QStringLiteral("volume"))) * side;
        if (weight > best) {
            best = weight;
            bpm = ControlObject::get(ConfigKey(group, QStringLiteral("bpm")));
        }
    }
    return bpm;
}

void QmlDJMantra::cqFollowTempo() {
    // a new tempo held for 2 s is tapped in again (not every small wobble of a sync)
    if (!m_pCq || !m_pCq->isConnected() ||
            setting(QStringLiteral("cq_tap_key"), QStringLiteral("0")).toInt() == 0) {
        return;
    }
    const double bpm = onAirBpm();
    if (bpm <= 0 || std::fabs(bpm - m_cqTappedBpm) < 0.5) {
        m_cqStableTicks = 0;
        return;
    }
    if (std::fabs(bpm - m_cqPendingBpm) < 0.5) {
        if (++m_cqStableTicks >= 2) {
            m_cqStableTicks = 0;
            cqTapNow();
        }
    } else {
        m_cqPendingBpm = bpm;
        m_cqStableTicks = 0;
    }
}

} // namespace qml
} // namespace mixxx
