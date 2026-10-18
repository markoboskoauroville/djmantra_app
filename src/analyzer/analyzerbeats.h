#pragma once

#include <QHash>
#include <QList>
#include <memory>

#include "analyzer/analyzer.h"
#include "analyzer/plugins/analyzerplugin.h"
#include "track/beats.h"
#include "preferences/beatdetectionsettings.h"
#include "preferences/usersettings.h"

namespace mixxx {
class AnalyzerQueenMaryBeats;
} // namespace mixxx

class AnalyzerBeats : public Analyzer {
  public:
    explicit AnalyzerBeats(
            UserSettingsPointer pConfig,
            bool enforceBpmDetection = false);
    ~AnalyzerBeats() override = default;

    static QList<mixxx::AnalyzerPluginInfo> availablePlugins();
    static mixxx::AnalyzerPluginInfo defaultPlugin();

    bool initialize(const AnalyzerTrack& track,
            mixxx::audio::SampleRate sampleRate,
            SINT frameLength) override;
    bool processSamples(const CSAMPLE* pIn, SINT count) override;
    void storeResults(TrackPointer tio) override;
    void cleanup() override;

  private:
    bool shouldAnalyze(TrackPointer pTrack) const;
    static QHash<QString, QString> getExtraVersionInfo(
            const QString& pluginId, bool bPreferencesFastAnalysis);

    UserSettingsPointer m_pConfig;
    BeatDetectionSettings m_bpmSettings;
    std::unique_ptr<mixxx::AnalyzerBeatsPlugin> m_pPlugin;
    /// DJ Mantra: the grid moved by half a beat when the off-beat lines up
    /// with the kick better than the grid does
    int beatGridMode() const;
    int beatEngine() const;
    bool beatOnKick() const;
    mixxx::BeatsPointer onTheBeat(const mixxx::BeatsPointer& pBeats,
            const mixxx::AnalyzerQueenMaryBeats& plugin,
            const QString& fileName) const;
    const bool m_enforceBpmDetection;
    QString m_pluginId;
    bool m_bPreferencesReanalyzeOldBpm;
    bool m_bPreferencesReanalyzeImported;
    bool m_bPreferencesFixedTempo;
    bool m_bPreferencesFastAnalysis;

    mixxx::audio::SampleRate m_sampleRate;
    SINT m_maxFramesToProcess;
    SINT m_currentFrame;
};
