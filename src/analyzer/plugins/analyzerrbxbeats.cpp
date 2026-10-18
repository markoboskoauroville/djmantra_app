#include "analyzer/plugins/analyzerrbxbeats.h"

#include "analyzer/constants.h"

namespace mixxx {

bool AnalyzerRbxBeats::initialize(mixxx::audio::SampleRate sampleRate) {
    m_sampleRate = sampleRate;
    m_mono.clear();
    m_result = djmantra::rbx::Result();
    if (!sampleRate.isValid()) {
        return false;
    }
    m_mono.reserve(static_cast<size_t>(sampleRate.value()) * 60 * 6);
    return true;
}

bool AnalyzerRbxBeats::processSamples(const CSAMPLE* pIn, SINT iLen) {
    const size_t limit = static_cast<size_t>(m_sampleRate.value()) * kMaxSeconds;
    constexpr SINT kChannels = mixxx::kAnalysisChannels;
    for (SINT i = 0; i + 1 < iLen && m_mono.size() < limit; i += kChannels) {
        m_mono.push_back((pIn[i] + pIn[i + 1]) * 0.5f);
    }
    return true;
}

bool AnalyzerRbxBeats::finalize() {
    m_result = djmantra::rbx::analyse(m_mono.data(), m_mono.size(), static_cast<int>(m_sampleRate.value()));
    std::vector<float>().swap(m_mono);
    return m_result.bpm > 0;
}

mixxx::audio::FramePos AnalyzerRbxBeats::firstDownbeat() const {
    if (m_result.beats.empty() || m_result.firstDownbeat >= static_cast<int>(m_result.beats.size())) {
        return mixxx::audio::kInvalidFramePos;
    }
    return mixxx::audio::FramePos(m_result.beats[m_result.firstDownbeat] * m_sampleRate.value());
}

} // namespace mixxx
