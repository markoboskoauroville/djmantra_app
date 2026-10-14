#include "qml/qmldjmantra.h"

#include <QDir>
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
#include <thread>
#if defined(Q_OS_LINUX) || defined(Q_OS_ANDROID)
#include <unistd.h>
#endif
#include <algorithm>
#include <memory>
#include <vector>
#include <cmath>

#if defined(Q_OS_ANDROID)
#include <QJniEnvironment>
#include <QJniObject>
#endif

#include "control/controlobject.h"
#include "control/controlproxy.h"
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

namespace {
// The controller's bridge to the Qt Quick interface (Marko, 12.10.2026: the
// Mix Ultra's BROWSE opens the song list, its pad mode buttons choose the tabs)
std::vector<std::unique_ptr<ControlObject>> s_controllerControls;
} // namespace

// static
void QmlDJMantra::registerUserSettings(UserSettingsPointer pConfig) {
    s_controllerControls.clear();
    if (pConfig) {
        for (const char* key : {"browse_move", "browse_press", "browse_back", "load", "page", "stop_deck"}) {
            s_controllerControls.push_back(std::make_unique<ControlObject>(
                    ConfigKey(QStringLiteral("[DJMantra]"), QString::fromLatin1(key))));
        }
        // V8: the analysis options of Settings, read by the engine (sections) and
        // the analyzer (beats), remembered in the user settings
        for (const auto& [key, fallback] : std::initializer_list<std::pair<const char*, double>>{
                     {"section_algorithm", 0},
                     {"section_sensitivity", 3},
                     {"beat_grid_mode", 0},
                     {"beat_on_kick", 1},
                     {"stop_time", 0}}) {
            const ConfigKey configKey(QStringLiteral("[DJMantra]"), QString::fromLatin1(key));
            auto pControl = std::make_unique<ControlObject>(configKey);
            pControl->set(pConfig->getValue(configKey, fallback));
            s_controllerControls.push_back(std::move(pControl));
        }
    }
    s_pUserSettings = std::move(pConfig);
}

QmlDJMantra::QmlDJMantra(UserSettingsPointer pConfig, QObject* pParent)
        : QObject(pParent),
          m_pConfig(std::move(pConfig)) {
    hideBars();
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
                lookUpGenre(group, pNewTrack);
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
    paths << QStringLiteral("/sdcard/Music") << QStringLiteral("/sdcard/Download");
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

QVariantList QmlDJMantra::listFolder(const QString& folder) const {
    QVariantList folders;
    QVariantList songs;
    const QDir dir(folder);
    const auto entries = dir.entryInfoList(
            QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable,
            QDir::Name | QDir::IgnoreCase);
    for (const QFileInfo& info : entries) {
        if (info.fileName().startsWith(QChar('.'))) {
            continue;
        }
        QVariantMap entry;
        entry[QStringLiteral("name")] = info.isDir() ? info.fileName() : info.completeBaseName();
        entry[QStringLiteral("path")] = info.absoluteFilePath();
        entry[QStringLiteral("isFolder")] = info.isDir();
        if (info.isDir()) {
            folders << entry;
        } else if (SoundSourceProxy::isFileNameSupported(info.fileName())) {
            songs << entry;
        }
    }
    return folders + songs;
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

QString QmlDJMantra::startRecording(const QString& format, const QString& name) {
    if (!m_pConfig) {
        return QString();
    }
    if (ControlObject::get(ConfigKey(QStringLiteral("[Recording]"), QStringLiteral("status"))) > 0) {
        return QString();
    }
#if defined(Q_OS_ANDROID)
    const QString dir = QStringLiteral("/sdcard/Music/DJ Mantra Recordings");
#else
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) +
            QStringLiteral("/DJ Mantra Recordings");
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
