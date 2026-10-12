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
#include <vector>
#include <cmath>

#if defined(Q_OS_ANDROID)
#include <QJniEnvironment>
#include <QJniObject>
#endif

#include "control/controlproxy.h"
#include "engine/controls/sectiondetector.h"
#include "mixer/playerinfo.h"
#include "moc_qmldjmantra.cpp"
#include "sources/soundsourceproxy.h"
#include "track/keyutils.h"
#include "track/track.h"
#include "waveform/waveform.h"
#include "util/duration.h"
#include "widget/djmantra/camelot.h"
#include "widget/djmantra/wdeckcounter.h"

namespace mixxx {
namespace qml {

namespace {

// The classic interface only when chosen: the Qt Quick one is the default
const ConfigKey kClassicKey(QStringLiteral("[DJMantra]"), QStringLiteral("classic_ui"));

djmantra::camelot::Code codeOf(double keyValue) {
    return djmantra::camelot::fromKey(KeyUtils::keyFromNumericValue(keyValue));
}

} // namespace

QmlDJMantra::QmlDJMantra(UserSettingsPointer pConfig, QObject* pParent)
        : QObject(pParent),
          m_pConfig(std::move(pConfig)) {
    hideBars();
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
    if (!pTrack || !pTrack->getGenre().trimmed().isEmpty()) {
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
    std::thread([pThis, pTrack, artist, title] {
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
                [pTrack, genre] {
                    if (pTrack->getGenre().trimmed().isEmpty()) {
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
        const QStringList candidates = {
                QStringLiteral("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage"),
                QStringLiteral("/sys/class/kgsl/kgsl-3d0/gpubusy"),
                QStringLiteral("/sys/kernel/ged/hal/gpu_utilization"),
                QStringLiteral("/proc/mtk_mali/utilization"),
                QStringLiteral("/sys/kernel/gpu/gpu_busy"),
                QStringLiteral("/sys/class/misc/mali0/device/utilization"),
                QStringLiteral("/sys/devices/platform/mali.0/utilization"),
        };
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
    result[QStringLiteral("gpu")] = m_gpuSource.isEmpty() ? -1 : gpuLoadFrom(m_gpuSource);
    return result;
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

void QmlDJMantra::setQuickInterface(bool on) {
    if (m_pConfig) {
        m_pConfig->setValue(kClassicKey, !on);
        m_pConfig->save();
    }
    qInfo() << "Quick: interface at the next start:" << (on ? "Qt Quick" : "classic");
}

bool QmlDJMantra::quickInterface() const {
    return !m_pConfig || !m_pConfig->getValue(kClassicKey, false);
}

void QmlDJMantra::log(const QString& text) const {
    qInfo().noquote() << "Quick:" << text;
}

} // namespace qml
} // namespace mixxx
