#include <dsp/onsets/DetectionFunction.h>
#include <dsp/tempotracking/TempoTrackV2.h>

// Class header comes after library includes here since our preprocessor
// definitions interfere with qm-dsp's headers.
#include "analyzer/plugins/analyzerqueenmarybeats.h"

#include "analyzer/constants.h"

#include <cmath>

namespace mixxx {
namespace {

// This determines the resolution of the resulting BeatMap.
// ~12 ms (86 Hz) is a fair compromise between accuracy and analysis speed,
// also matching the preferred window/step sizes from BeatTrack VAMP.
// For a 44.1 kHz track, we go in 512 sample steps
// TODO: kStepSecs and the waveform sample rate of 441
// (defined in AnalyzerWaveform::initialize) do not align well and thus
// generate interference. Currently we are at this odd factor: 441 * 0.01161 = 5.12.
// This should be adjusted to be an integer.
constexpr float kStepSecs = 0.01161f;
// results in 43 Hz @ 44.1 kHz / 47 Hz @ 48 kHz / 47 Hz @ 96 kHz
constexpr int kMaximumBinSizeHz = 50; // Hz
constexpr double kPi = 3.14159265358979323846;

DFConfig makeDetectionFunctionConfig(int stepSizeFrames, int windowSize) {
    // These are the defaults for the VAMP beat tracker plugin we used in Mixxx
    // 2.0.
    DFConfig config;
    config.DFType = DF_COMPLEXSD;
    config.stepSize = stepSizeFrames;
    config.frameLength = windowSize;
    config.dbRise = 3;
    config.adaptiveWhitening = false;
    config.whiteningRelaxCoeff = -1;
    config.whiteningFloor = -1;
    return config;
}

} // namespace

AnalyzerQueenMaryBeats::AnalyzerQueenMaryBeats()
        : m_windowSize(0),
          m_stepSizeFrames(0) {
}

AnalyzerQueenMaryBeats::~AnalyzerQueenMaryBeats() {
}

bool AnalyzerQueenMaryBeats::initialize(mixxx::audio::SampleRate sampleRate) {
    m_detectionResults.clear();
    m_sampleRate = sampleRate;
    m_stepSizeFrames = static_cast<int>(m_sampleRate * kStepSecs);
    m_windowSize = MathUtilities::nextPowerOfTwo(m_sampleRate / kMaximumBinSizeHz);
    m_pDetectionFunction = std::make_unique<DetectionFunction>(
            makeDetectionFunctionConfig(m_stepSizeFrames, m_windowSize));
    // DJ Mantra: the kick band (bins 3 and 4 of this window: ~130-175 Hz at
    // 44.1 kHz), a plain DFT of two bins with a Hann window
    m_kickBand.clear();
    m_kickOnsets.clear();
    m_kickWindow.resize(m_windowSize);
    m_kickCos.resize(2 * m_windowSize);
    m_kickSin.resize(2 * m_windowSize);
    for (int n = 0; n < m_windowSize; ++n) {
        m_kickWindow[n] = 0.5 - 0.5 * std::cos(2 * kPi * n / (m_windowSize - 1));
        for (int b = 0; b < 2; ++b) {
            const double phase = 2 * kPi * (3 + b) * n / m_windowSize;
            m_kickCos[b * m_windowSize + n] = std::cos(phase);
            m_kickSin[b * m_windowSize + n] = std::sin(phase);
        }
    }
    qDebug() << "input sample rate is " << m_sampleRate << ", step size is " << m_stepSizeFrames;

    m_helper.initialize(
            m_windowSize, m_stepSizeFrames, [this](double* pWindow, size_t) {
                double magnitude = 0;
                for (int b = 0; b < 2; ++b) {
                    double re = 0;
                    double im = 0;
                    for (int n = 0; n < m_windowSize; ++n) {
                        const double x = pWindow[n] * m_kickWindow[n];
                        re += x * m_kickCos[b * m_windowSize + n];
                        im -= x * m_kickSin[b * m_windowSize + n];
                    }
                    magnitude += std::sqrt(re * re + im * im);
                }
                m_kickBand.push_back(std::log1p(magnitude));
                // TODO(rryan) reserve?
                m_detectionResults.push_back(
                        m_pDetectionFunction->processTimeDomain(pWindow));
                return true;
            });
    return true;
}

bool AnalyzerQueenMaryBeats::processSamples(const CSAMPLE* pIn, SINT iLen) {
    DEBUG_ASSERT(iLen % kAnalysisChannels == 0);
    if (!m_pDetectionFunction) {
        return false;
    }

    return m_helper.processStereoSamples(pIn, iLen);
}

bool AnalyzerQueenMaryBeats::finalize() {
    m_helper.finalize();

    int nonZeroCount = static_cast<int>(m_detectionResults.size());
    while (nonZeroCount > 0 && m_detectionResults.at(nonZeroCount - 1) <= 0.0) {
        --nonZeroCount;
    }

    std::vector<double> df;
    std::vector<double> beatPeriod;
    std::vector<double> tempi;
    const auto required_size = std::max(0, nonZeroCount - 2);
    df.reserve(required_size);
    beatPeriod.reserve(required_size);

    // skip first 2 results as it might have detect noise as onset
    // that's how vamp does and seems works best this way
    for (int i = 2; i < nonZeroCount; ++i) {
        df.push_back(m_detectionResults.at(i));
        beatPeriod.push_back(0.0);
    }

    TempoTrackV2 tt(m_sampleRate, m_stepSizeFrames);
    tt.calculateBeatPeriod(df, beatPeriod, tempi);

    std::vector<double> beats;
    tt.calculateBeats(df, beatPeriod, beats);

    m_resultBeats.reserve(static_cast<int>(beats.size()));
    for (size_t i = 0; i < beats.size(); ++i) {
        // we add the halve m_stepSizeFrames here, because the beat
        // is detected between the two samples.
        const auto result = mixxx::audio::FramePos(
                (beats.at(i) * m_stepSizeFrames) + m_stepSizeFrames / 2);
        m_resultBeats.push_back(result);
    }

    m_kickOnsets.resize(m_kickBand.size());
    for (size_t i = 0; i < m_kickBand.size(); ++i) {
        m_kickOnsets[i] = i == 0 ? 0 : std::max(0.0, m_kickBand[i] - m_kickBand[i - 1]);
    }
    m_kickBand.clear();
    m_pDetectionFunction.reset();
    return true;
}

} // namespace mixxx
