#include "qml/qmldjmantra.h"

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QScreen>
#include <QStandardPaths>
#include <QTimer>
#include <QtDebug>
#include <algorithm>
#include <vector>
#include <cmath>

#if defined(Q_OS_ANDROID)
#include <QJniEnvironment>
#include <QJniObject>
#endif

#include "control/controlproxy.h"
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

const ConfigKey kQuickKey(QStringLiteral("[DJMantra]"), QStringLiteral("quick_ui"));

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
    if (changed) {
        qInfo() << "Quick: insets" << m_insets[0] << m_insets[1] << m_insets[2] << m_insets[3];
        emit insetsChanged();
    }
#endif
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
        m_pConfig->setValue(kQuickKey, on);
        m_pConfig->save();
    }
    qInfo() << "Quick: interface at the next start:" << (on ? "Qt Quick" : "classic");
}

bool QmlDJMantra::quickInterface() const {
    return m_pConfig && m_pConfig->getValue(kQuickKey, false);
}

void QmlDJMantra::log(const QString& text) const {
    qInfo().noquote() << "Quick:" << text;
}

} // namespace qml
} // namespace mixxx
