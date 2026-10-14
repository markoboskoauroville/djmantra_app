#include "analyzer/analyzerbeats.h"

#include <QFileInfo>
#include <QHash>
#include <QString>
#include <QVector>
#include <QtDebug>

#include "analyzer/analyzertrack.h"
#include "analyzer/constants.h"
#include "analyzer/plugins/analyzerqueenmarybeats.h"
#include "analyzer/plugins/analyzersoundtouchbeats.h"
#include "library/rekordbox/rekordboxconstants.h"
#include "track/beatfactory.h"
#include "track/track.h"

// static
QList<mixxx::AnalyzerPluginInfo> AnalyzerBeats::availablePlugins() {
    QList<mixxx::AnalyzerPluginInfo> plugins;
    // First one below is the default
    plugins.append(mixxx::AnalyzerQueenMaryBeats::pluginInfo());
    plugins.append(mixxx::AnalyzerSoundTouchBeats::pluginInfo());
    return plugins;
}

// static
mixxx::AnalyzerPluginInfo AnalyzerBeats::defaultPlugin() {
    const auto plugins = availablePlugins();
    DEBUG_ASSERT(!plugins.isEmpty());
    return plugins.at(0);
}

AnalyzerBeats::AnalyzerBeats(UserSettingsPointer pConfig, bool enforceBpmDetection)
        : m_pConfig(pConfig),
          m_bpmSettings(pConfig),
          m_enforceBpmDetection(enforceBpmDetection),
          m_bPreferencesReanalyzeOldBpm(false),
          m_bPreferencesReanalyzeImported(false),
          m_bPreferencesFixedTempo(true),
          m_bPreferencesFastAnalysis(false),
          m_maxFramesToProcess(0),
          m_currentFrame(0) {
}

bool AnalyzerBeats::initialize(const AnalyzerTrack& track,
        mixxx::audio::SampleRate sampleRate,
        SINT frameLength) {
    if (frameLength <= 0) {
        return false;
    }

    bool bPreferencesBeatDetectionEnabled =
            m_enforceBpmDetection || m_bpmSettings.getBpmDetectionEnabled();
    if (!bPreferencesBeatDetectionEnabled) {
        qDebug() << "Beat calculation is deactivated";
        return false;
    }

    bool bpmLock = track.getTrack()->isBpmLocked();
    if (bpmLock) {
        qDebug() << "Track is BpmLocked: Beat calculation will not start";
        return false;
    }

    m_bPreferencesFixedTempo = track.getOptions().useFixedTempo.value_or(
            m_bpmSettings.getFixedTempoAssumption());
    m_bPreferencesReanalyzeOldBpm = m_bpmSettings.getReanalyzeWhenSettingsChange();
    m_bPreferencesReanalyzeImported = m_bpmSettings.getReanalyzeImported();
    m_bPreferencesFastAnalysis = m_bpmSettings.getFastAnalysis();

    const auto plugins = availablePlugins();
    if (!plugins.isEmpty()) {
        m_pluginId = defaultPlugin().id();
        QString pluginId = m_bpmSettings.getBeatPluginId();
        for (const auto& info : plugins) {
            if (info.id() == pluginId) {
                m_pluginId = pluginId; // configured Plug-In available
                break;
            }
        }
    }

    qDebug() << "AnalyzerBeats preference settings:"
             << "\nPlugin:" << m_pluginId
             << "\nFixed tempo assumption:" << m_bPreferencesFixedTempo
             << "\nRe-analyze when settings change:" << m_bPreferencesReanalyzeOldBpm
             << "\nRe-analyze imported from other software:" << m_bPreferencesReanalyzeImported
             << "\nFast analysis:" << m_bPreferencesFastAnalysis;

    m_sampleRate = sampleRate;
    // In fast analysis mode, skip processing after
    // kFastAnalysisSecondsToAnalyze seconds are analyzed.
    if (m_bPreferencesFastAnalysis) {
        m_maxFramesToProcess =
                mixxx::kFastAnalysisSecondsToAnalyze * m_sampleRate;
    } else {
        m_maxFramesToProcess = frameLength;
    }
    m_currentFrame = 0;

    // if we can load a stored track don't reanalyze it
    bool bShouldAnalyze = shouldAnalyze(track.getTrack());

    DEBUG_ASSERT(!m_pPlugin);
    if (bShouldAnalyze) {
        if (m_pluginId == mixxx::AnalyzerQueenMaryBeats::pluginInfo().id()) {
            m_pPlugin = std::make_unique<mixxx::AnalyzerQueenMaryBeats>();
        } else if (m_pluginId == mixxx::AnalyzerSoundTouchBeats::pluginInfo().id()) {
            m_pPlugin = std::make_unique<mixxx::AnalyzerSoundTouchBeats>();
        } else {
            // This must not happen, because we have already verified above
            // that the PlugInId is valid
            DEBUG_ASSERT(false);
        }

        if (m_pPlugin) {
            if (m_pPlugin->initialize(m_sampleRate)) {
                qDebug() << "Beat calculation started with plugin" << m_pluginId;
            } else {
                qDebug() << "Beat calculation will not start.";
                m_pPlugin.reset();
                bShouldAnalyze = false;
            }
        } else {
            bShouldAnalyze = false;
        }
    }
    return bShouldAnalyze;
}

bool AnalyzerBeats::shouldAnalyze(TrackPointer pTrack) const {
    bool bpmLock = pTrack->isBpmLocked();
    if (bpmLock) {
        qDebug() << "Track is BpmLocked: Beat calculation will not start";
        return false;
    }

    QString pluginID = m_bpmSettings.getBeatPluginId();
    if (pluginID.isEmpty()) {
        pluginID = defaultPlugin().id();
    }

    // If the track already has a Beats object then we need to decide whether to
    // analyze this track or not.
    const mixxx::BeatsPointer pBeats = pTrack->getBeats();
    if (!pBeats) {
        return true;
    }
    if (!pBeats->getBpmInRange(mixxx::audio::kStartFramePos,
                       mixxx::audio::FramePos{
                               pTrack->getDuration() * pBeats->getSampleRate()})
                    .isValid()) {
        // Tracks with an invalid bpm <= 0 should be re-analyzed,
        // independent of the preference settings. We expect that
        // all tracks have a bpm > 0 when analyzed. Users that want
        // to keep their zero bpm tracks could lock them to prevent
        // this re-analysis (see the check above).
        qDebug() << "Re-analyzing track with invalid BPM despite preference settings.";
        return true;
    }

    // DJ Mantra 14.10.2026: a beat map from the old "auto" default while the steady grid is
    // chosen (now the default): analysed again into one tempo, as djay
#ifdef __ANDROID_PORT__
    if (!pBeats->hasConstantTempo() && beatGridMode() == 0) {
        qInfo().noquote() << QStringLiteral("Beats: %1\ta beat map, the steady grid is chosen: again")
                                     .arg(QFileInfo(pTrack->getLocation()).fileName());
        return true;
    }
#endif

    QString subVersion = pBeats->getSubVersion();
    if (subVersion == mixxx::rekordboxconstants::beatsSubversion) {
        return m_bPreferencesReanalyzeImported;
    }

    if (subVersion.isEmpty() && pBeats->firstBeat() <= mixxx::audio::kStartFramePos &&
            m_pluginId != mixxx::AnalyzerSoundTouchBeats::pluginInfo().id()) {
        // This happens if the beat grid was created from the metadata BPM value.
        qDebug() << "First beat is 0 for grid so analyzing track to find first beat.";
        return true;
    }

    QString version = pBeats->getVersion();
    QHash<QString, QString> extraVersionInfo = getExtraVersionInfo(
            pluginID,
            m_bPreferencesFastAnalysis);
    QString newVersion = BeatFactory::getPreferredVersion(
            m_bPreferencesFixedTempo);
    QString newSubVersion = BeatFactory::getPreferredSubVersion(
            extraVersionInfo);

    if (version == newVersion && subVersion == newSubVersion) {
        // If the version and settings have not changed then if the world is
        // sane, re-analyzing will do nothing.
        return false;
    }
    // Beat grid exists but version and settings differ
    if (!m_bPreferencesReanalyzeOldBpm) {
        qDebug() << "Beat calculation skips analyzing because the track has"
                << "a BPM computed by a previous Mixxx version and user"
                << "preferences indicate we should not change it.";
        return false;
    }

    return true;
}

bool AnalyzerBeats::processSamples(const CSAMPLE* pIn, SINT count) {
    VERIFY_OR_DEBUG_ASSERT(m_pPlugin) {
        return false;
    }

    m_currentFrame += count / mixxx::kAnalysisChannels;
    if (m_currentFrame > m_maxFramesToProcess) {
        return true; // silently ignore all remaining samples
    }

    return m_pPlugin->processSamples(pIn, count);
}

void AnalyzerBeats::cleanup() {
    m_pPlugin.reset();
}

void AnalyzerBeats::storeResults(TrackPointer pTrack) {
    VERIFY_OR_DEBUG_ASSERT(m_pPlugin) {
        return;
    }

    if (!m_pPlugin->finalize()) {
        qWarning() << "Beat/BPM analysis failed";
        return;
    }

    mixxx::BeatsPointer pBeats;
    if (m_pPlugin->supportsBeatTracking()) {
        QVector<mixxx::audio::FramePos> beats = m_pPlugin->getBeats();
        QHash<QString, QString> extraVersionInfo = getExtraVersionInfo(
                m_pluginId, m_bPreferencesFastAnalysis);
        pBeats = BeatFactory::makePreferredBeats(
                beats,
                extraVersionInfo,
                m_bPreferencesFixedTempo,
                m_sampleRate,
                // 14.10.2026 (djay compare): 0 is now the steady grid, as djay's (Aum
                // Ganesha: djay 115, our beat map drifted 91 / 110 / 118); 1 = beat map
                beatGridMode() == 1);
        qDebug() << "AnalyzerBeats plugin detected" << beats.size()
                 << "beats. Predominant BPM:"
                 << (pBeats ? pBeats->getBpmInRange(
                                      mixxx::audio::kStartFramePos,
                                      mixxx::audio::FramePos{
                                              pTrack->getDuration() *
                                              pBeats->getSampleRate()})
                            : mixxx::Bpm());
    } else {
        mixxx::Bpm bpm = m_pPlugin->getBpm();
        qDebug() << "AnalyzerBeats plugin detected constant BPM: " << bpm;
        pBeats = mixxx::Beats::fromConstTempo(m_sampleRate, mixxx::audio::kStartFramePos, bpm);
    }

    // DJ Mantra: the grid on the beat, not the off-beat. The QM tracker locks
    // onto the hi-hats in ~27 % of Marko's test songs (half a beat off
    // rekordbox's grid); the kick band's onsets decide between the grid and
    // the grid moved by half a beat (right on 74 of 78 songs).
    if (pBeats && pBeats->hasConstantTempo() && beatOnKick()) {
        if (const auto* pQm = dynamic_cast<const mixxx::AnalyzerQueenMaryBeats*>(m_pPlugin.get())) {
            pBeats = onTheBeat(pBeats, *pQm, QFileInfo(pTrack->getLocation()).fileName());
        }
    } else if (pBeats && !pBeats->hasConstantTempo()) {
        qInfo().noquote() << QStringLiteral("Beats: %1\tthe tempo is not constant, a beat map")
                                     .arg(QFileInfo(pTrack->getLocation()).fileName());
    }
    // V8, verbose: the beat options of Settings for this song
    qInfo().noquote() << QStringLiteral("Beats options: %1\tgrid %2\ton the kick %3")
                                 .arg(QFileInfo(pTrack->getLocation()).fileName(),
                                         beatGridMode() == 1 ? QStringLiteral("auto (beat map for live drums)")
                                                             : QStringLiteral("constant"),
                                         beatOnKick() ? QStringLiteral("on") : QStringLiteral("off"));
    pTrack->trySetBeats(pBeats);
}

// V8: Settings -> beats ([DJMantra] in the user settings). 14.10.2026: 0 = constant (the
// default, as djay; the old "auto" value 0 turns into it), 1 = auto (a beat map for live drums)
int AnalyzerBeats::beatGridMode() const {
#ifdef __ANDROID_PORT__
    constexpr int kDefault = 0; // the steady grid, as djay
#else
    constexpr int kDefault = 1; // desktop: Mixxx's own behaviour (the test suite)
#endif
    return m_pConfig ? m_pConfig->getValue(ConfigKey(QStringLiteral("[DJMantra]"),
                                                   QStringLiteral("beat_grid_mode")),
                               kDefault)
                     : kDefault;
}

bool AnalyzerBeats::beatOnKick() const {
    return m_pConfig ? m_pConfig->getValue(ConfigKey(QStringLiteral("[DJMantra]"),
                                                   QStringLiteral("beat_on_kick")),
                               1) != 0
                     : true;
}

mixxx::BeatsPointer AnalyzerBeats::onTheBeat(const mixxx::BeatsPointer& pBeats,
        const mixxx::AnalyzerQueenMaryBeats& plugin,
        const QString& fileName) const {
    const std::vector<double>& onsets = plugin.kickOnsets();
    const int step = plugin.stepSizeFrames();
    const int window = plugin.windowSize();
    const int count = static_cast<int>(onsets.size());
    const mixxx::audio::FramePos first = pBeats->firstBeat();
    if (count < 100 || step <= 0 || !first.isValid()) {
        return pBeats;
    }
    const mixxx::audio::FramePos second = pBeats->findNextBeat(first + 1);
    if (!second.isValid()) {
        return pBeats;
    }
    const double beatLength = second - first;
    const double end = static_cast<double>(count) * step;
    // the mean of the strongest onset within +-2 steps of each grid line
    const auto score = [&](double start) {
        double sum = 0;
        int n = 0;
        for (double t = start; t < end - m_sampleRate.toDouble(); t += beatLength) {
            const int i = static_cast<int>(std::lround((t - window / 2.0) / step));
            if (i <= 2 || i >= count - 3) {
                continue;
            }
            double best = 0;
            for (int j = i - 2; j <= i + 2; ++j) {
                best = std::max(best, onsets[j]);
            }
            sum += best;
            ++n;
        }
        return n > 0 ? sum / n : 0.0;
    };
    const double onBeat = score(first.value());
    const double offBeat = score(first.value() + beatLength / 2);
    if (offBeat <= onBeat) {
        return pBeats;
    }
    double moved = first.value() + beatLength / 2;
    if (moved >= beatLength) {
        moved -= beatLength;
    }
    const mixxx::Bpm bpm = pBeats->getBpmInRange(
            mixxx::audio::kStartFramePos, mixxx::audio::FramePos(end));
    qInfo().noquote() << QStringLiteral("Beats: %1\tthe grid moved half a beat onto the kick %2 -> %3")
                                 .arg(fileName)
                                 .arg(onBeat)
                                 .arg(offBeat);
    return mixxx::Beats::fromConstTempo(m_sampleRate,
            mixxx::audio::FramePos(moved).toNearestFrameBoundary(),
            bpm,
            pBeats->getSubVersion());
}

// static
QHash<QString, QString> AnalyzerBeats::getExtraVersionInfo(
        const QString& pluginId, bool bPreferencesFastAnalysis) {
    QHash<QString, QString> extraVersionInfo;
    extraVersionInfo["vamp_plugin_id"] = pluginId;
    if (bPreferencesFastAnalysis) {
        extraVersionInfo["fast_analysis"] = "1";
    }
    return extraVersionInfo;
}
