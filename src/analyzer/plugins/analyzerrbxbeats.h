#pragma once

#include <QObject>
#include <vector>

#include "analyzer/plugins/analyzerplugin.h"
#include "analyzer/rbxanalysis.h"

namespace mixxx {

/// DJ Mantra V91: rbxport's beat grid (src/analyzer/rbxanalysis.cpp, ported from
/// https://github.com/chrisle/rbxport, GPL-2.0-or-later): one tempo for the song, every beat on the
/// kick's attack, the downbeat and the phrases from where the music changes; the sections named as
/// on Marko's page. The whole song is kept (mono, up to kMaxSeconds) and analysed in finalize().
class AnalyzerRbxBeats : public AnalyzerBeatsPlugin {
  public:
    static AnalyzerPluginInfo pluginInfo() {
        return AnalyzerPluginInfo(QStringLiteral("djmantra-rbxport:1"),
                QObject::tr("rbxport (Chris Le), ported by DJ Mantra"),
                QObject::tr("rbxport beat grid"),
                true);
    }
    /// Longer songs (a DJ mix) are analysed over their first 20 minutes; the grid carries on
    static constexpr int kMaxSeconds = 20 * 60;

    AnalyzerPluginInfo info() const override {
        return pluginInfo();
    }
    bool initialize(mixxx::audio::SampleRate sampleRate) override;
    bool processSamples(const CSAMPLE* pIn, SINT iLen) override;
    bool finalize() override;
    bool supportsBeatTracking() const override {
        return false;
    }
    mixxx::Bpm getBpm() const override {
        return m_result.bpm > 0 ? mixxx::Bpm(m_result.bpm) : mixxx::Bpm();
    }
    /// The first downbeat ("1"), the grid's anchor; invalid without a grid
    mixxx::audio::FramePos firstDownbeat() const;
    const djmantra::rbx::Result& result() const {
        return m_result;
    }

  private:
    mixxx::audio::SampleRate m_sampleRate;
    std::vector<float> m_mono;
    djmantra::rbx::Result m_result;
};

} // namespace mixxx
