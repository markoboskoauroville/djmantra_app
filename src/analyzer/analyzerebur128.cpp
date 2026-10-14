#include "analyzer/analyzerebur128.h"

#include <QtDebug>

#include <algorithm>
#include <cmath>

#include "analyzer/analyzertrack.h"
#include "analyzer/constants.h"
#include "track/track.h"
#include "util/math.h"
#include "util/timer.h"

namespace {
constexpr double kReplayGain2ReferenceLUFS = -18;
} // anonymous namespace

AnalyzerEbur128::AnalyzerEbur128(UserSettingsPointer pConfig)
        : m_rgSettings(pConfig),
          m_pState(nullptr) {
}

AnalyzerEbur128::~AnalyzerEbur128() {
    cleanup(); // ...to prevent memory leaks
}

bool AnalyzerEbur128::initialize(
        const AnalyzerTrack& track,
        mixxx::audio::SampleRate sampleRate,
        SINT frameLength) {
#ifdef __ANDROID_PORT__
    // DJ Mantra AUTO GAIN (Marko 14.10.2026): each song's peak is needed; a song measured
    // before (a ratio but no peak) is measured again
    const bool needsPeak = !track.getTrack()->getReplayGain().hasPeak();
    if (!needsPeak) {
        // a song normalized to the old -1 dBFS (V44..V59): brought to 0 dBFS from its stored
        // ratio, without measuring it again
        mixxx::ReplayGain replayGain(track.getTrack()->getReplayGain());
        const double old = replayGain.getRatio() * replayGain.getPeak();
        if (std::fabs(old - db2ratio(-1.0)) < 0.005) {
            replayGain.setRatio(std::clamp(replayGain.getRatio() / db2ratio(-1.0), 0.25, 8.0));
            track.getTrack()->setReplayGain(replayGain);
            qInfo() << "Auto gain: 0 dBFS now for" << track.getTrack()->getLocation();
        }
    }
#else
    const bool needsPeak = false;
#endif
    if ((m_rgSettings.isAnalyzerDisabled(2, track.getTrack()) && !needsPeak) || frameLength <= 0) {
        qDebug() << "Skipping AnalyzerEbur128";
        return false;
    }
    DEBUG_ASSERT(m_pState == nullptr);
    m_pState = ebur128_init(
            mixxx::kAnalysisChannels,
            sampleRate,
#ifdef __ANDROID_PORT__
            EBUR128_MODE_I | EBUR128_MODE_SAMPLE_PEAK);
#else
            EBUR128_MODE_I);
#endif
    return m_pState != nullptr;
}

void AnalyzerEbur128::cleanup() {
    if (m_pState) {
        ebur128_destroy(&m_pState);
        // ebur128_destroy clears the pointer but let's not rely on that.
        m_pState = nullptr;
    }
}

bool AnalyzerEbur128::processSamples(const CSAMPLE* pIn, SINT count) {
    VERIFY_OR_DEBUG_ASSERT(m_pState) {
        return false;
    }
    ScopedTimer t(u"AnalyzerEbur128::processSamples()");
    size_t frames = count / mixxx::kAnalysisChannels;
    int e = ebur128_add_frames_float(m_pState, pIn, frames);
    VERIFY_OR_DEBUG_ASSERT(e == EBUR128_SUCCESS) {
        qWarning() << "AnalyzerEbur128::processSamples() failed with" << e;
        return false;
    }
    return true;
}

void AnalyzerEbur128::storeResults(TrackPointer pTrack) {
    VERIFY_OR_DEBUG_ASSERT(m_pState) {
        return;
    }
    double averageLufs;
    int e = ebur128_loudness_global(m_pState, &averageLufs);
    VERIFY_OR_DEBUG_ASSERT(e == EBUR128_SUCCESS) {
        qWarning() << "AnalyzerEbur128::storeResults() failed with" << e;
        return;
    }
    if (averageLufs == -HUGE_VAL ||
            averageLufs == HUGE_VAL ||
            // This catches 0 and abnormal values inf and -inf (that may have
            // slipped through in libebur128 for some reason.
            !util_isnormal(averageLufs)) {
        qWarning() << "AnalyzerEbur128::storeResults() averageLufs invalid:"
                   << averageLufs;
        return;
    }

    const double fReplayGain2 = kReplayGain2ReferenceLUFS - averageLufs;
    mixxx::ReplayGain replayGain(pTrack->getReplayGain());
    replayGain.setRatio(db2ratio(fReplayGain2));
#ifdef __ANDROID_PORT__
    // DJ Mantra AUTO GAIN, the normalized GAIN (Marko 14.10.2026: "When it's on zero, the song
    // peak reaches the zero on dB ... song never can peak with the gain"): the song's sample
    // peak brought to 0 dBFS (-12 .. +18 dB at most), applied by Mixxx's ReplayGain stage while
    // AUTO GAIN is on; the GAIN knob only cuts from there (MixerPage)
    double peak = 0;
    for (unsigned int channel = 0; channel < static_cast<unsigned int>(mixxx::kAnalysisChannels); ++channel) {
        double channelPeak = 0;
        if (ebur128_sample_peak(m_pState, channel, &channelPeak) == EBUR128_SUCCESS) {
            peak = std::max(peak, channelPeak);
        }
    }
    if (peak > 0.001) {
        replayGain.setPeak(static_cast<CSAMPLE>(std::min(peak, 1.0)));
        replayGain.setRatio(std::clamp(1.0 / peak, 0.25, 8.0));
        qInfo() << "Auto gain:" << pTrack->getLocation() << "peak" << ratio2db(peak)
                << "dB, gain" << ratio2db(replayGain.getRatio()) << "dB";
    }
#endif
    pTrack->setReplayGain(replayGain);
    qDebug() << "ReplayGain 2.0 (libebur128) result is" << fReplayGain2
             << "dB for" << pTrack->getFileInfo();
}
