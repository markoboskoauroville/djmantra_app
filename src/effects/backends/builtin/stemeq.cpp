#include <dsp/transforms/FFT.h>

// after qm-dsp's headers (as analyzerqueenmarybeats.cpp)
#include "effects/backends/builtin/stemeq.h"

#include <algorithm>
#include <cmath>

namespace djmantra {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kEps = 1e-12;
// a frame is 256 / 44100 s: ~0.2 s to cross-fade, ~30 ms for the knobs
constexpr double kBlendStep = 0.035;
constexpr double kGainSmoothing = 0.5;

// 0 below `hz` / sqrt(2), 1 above `hz` * sqrt(2): a raised cosine over one octave
double softStep(double f, double hz) {
    if (f <= 0) {
        return 0;
    }
    const double x = std::log2(f / hz) + 0.5; // 0 .. 1 across the octave
    if (x <= 0) {
        return 0;
    }
    if (x >= 1) {
        return 1;
    }
    return 0.5 - 0.5 * std::cos(kPi * x);
}

} // namespace

StemEq::StemEq(double sampleRate)
        : m_sampleRate(sampleRate),
          m_pFft(std::make_unique<FFTReal>(kSize)),
          m_window(kSize),
          m_frame(kSize),
          m_time(kSize),
          m_mag(kBins),
          m_history(kHistory, std::vector<double>(kBins, 0.0)),
          m_bandLow(kBins),
          m_bandHigh(kBins),
          m_vocalBand(kBins),
          m_mask(kBins),
          m_scratch(kHistory) {
    m_sorted.reserve(2 * kSpread + 2);
    for (int i = 0; i < kSize; ++i) {
        m_window[i] = 0.5 - 0.5 * std::cos(2 * kPi * i / kSize); // periodic Hann
    }
    for (int c = 0; c < 2; ++c) {
        m_in[c].assign(kSize, 0.0);
        m_acc[c].assign(kSize, 0.0);
        m_ready[c].assign(kHop, 0.0);
        m_re[c].assign(kSize, 0.0);
        m_im[c].assign(kSize, 0.0);
    }
    computeBandMasks();
}

StemEq::~StemEq() = default;

void StemEq::setSampleRate(double sampleRate) {
    if (sampleRate > 0 && sampleRate != m_sampleRate) {
        m_sampleRate = sampleRate;
        computeBandMasks();
    }
}

void StemEq::setCrossovers(double lowHz, double highHz) {
    if (lowHz > 0 && highHz > lowHz && (lowHz != m_lowHz || highHz != m_highHz)) {
        m_lowHz = lowHz;
        m_highHz = highHz;
        computeBandMasks();
    }
}

void StemEq::computeBandMasks() {
    for (int k = 0; k < kBins; ++k) {
        const double f = k * m_sampleRate / kSize;
        m_bandHigh[k] = softStep(f, m_highHz);
        m_bandLow[k] = 1 - softStep(f, m_lowHz);
        // vocals: 150 Hz .. 7 kHz
        m_vocalBand[k] = softStep(f, 150) * (1 - softStep(f, 7000));
    }
}

void StemEq::reset() {
    for (int c = 0; c < 2; ++c) {
        std::fill(m_in[c].begin(), m_in[c].end(), 0.0);
        std::fill(m_acc[c].begin(), m_acc[c].end(), 0.0);
        std::fill(m_ready[c].begin(), m_ready[c].end(), 0.0);
    }
    for (auto& row : m_history) {
        std::fill(row.begin(), row.end(), 0.0);
    }
    m_fill = 0;
}

void StemEq::process(const float* pIn, float* pOut, int frames, const Gains& gains) {
    for (int i = 0; i < frames; ++i) {
        const double left = pIn[2 * i];
        const double right = pIn[2 * i + 1];
        m_in[0][kSize - kHop + m_fill] = left;
        m_in[1][kSize - kHop + m_fill] = right;
        pOut[2 * i] = static_cast<float>(m_ready[0][m_fill]);
        pOut[2 * i + 1] = static_cast<float>(m_ready[1][m_fill]);
        if (++m_fill == kHop) {
            processFrame(gains);
            m_fill = 0;
        }
    }
}

void StemEq::processFrame(const Gains& gains) {
    // the knobs and the mode, smoothed per frame (no zipper noise)
    m_gLow += kGainSmoothing * (gains.low - m_gLow);
    m_gMid += kGainSmoothing * (gains.mid - m_gMid);
    m_gHigh += kGainSmoothing * (gains.high - m_gHigh);
    m_blend = gains.stems ? std::min(1.0, m_blend + kBlendStep)
                          : std::max(0.0, m_blend - kBlendStep);

    for (int c = 0; c < 2; ++c) {
        for (int i = 0; i < kSize; ++i) {
            m_frame[i] = m_in[c][i] * m_window[i];
        }
        m_pFft->forward(m_frame.data(), m_re[c].data(), m_im[c].data());
    }
    // the magnitude of both channels, remembered for the harmonic median; only
    // with the stems on (or on the way): the 9 frames fill again in ~50 ms, well
    // inside the 0.2 s cross-fade
    if (gains.stems || m_blend > 0) {
        std::vector<double>& now = m_history[m_historyPos];
        for (int k = 0; k < kBins; ++k) {
            const double l = std::sqrt(m_re[0][k] * m_re[0][k] + m_im[0][k] * m_im[0][k]);
            const double r = std::sqrt(m_re[1][k] * m_re[1][k] + m_im[1][k] * m_im[1][k]);
            m_mag[k] = 0.5 * (l + r);
            now[k] = m_mag[k];
        }
        m_historyPos = (m_historyPos + 1) % kHistory;
    }

    const bool unity = std::fabs(m_gLow - 1) < 1e-4 && std::fabs(m_gMid - 1) < 1e-4 &&
            std::fabs(m_gHigh - 1) < 1e-4;
    if (unity) {
        std::fill(m_mask.begin(), m_mask.end(), 1.0);
    } else {
        // the percussive median: a window of 2 * kSpread + 1 bins, kept sorted while
        // it slides (one insert and one removal per bin instead of a selection)
        std::vector<double>& window = m_sorted;
        window.clear();
        if (m_blend > 0) {
            for (int j = 0; j <= std::min(kBins - 1, kSpread); ++j) {
                window.insert(std::upper_bound(window.begin(), window.end(), m_mag[j]), m_mag[j]);
            }
        }
        for (int k = 0; k < kBins; ++k) {
            const double normal = m_gLow * m_bandLow[k] + m_gHigh * m_bandHigh[k] +
                    m_gMid * std::max(0.0, 1 - m_bandLow[k] - m_bandHigh[k]);
            if (m_blend <= 0) {
                m_mask[k] = normal;
                continue;
            }
            if (k > 0) {
                // slide: bin k + kSpread comes in, bin k - kSpread - 1 goes out
                if (k + kSpread < kBins) {
                    const double in = m_mag[k + kSpread];
                    window.insert(std::upper_bound(window.begin(), window.end(), in), in);
                }
                if (k - kSpread - 1 >= 0) {
                    const double out = m_mag[k - kSpread - 1];
                    window.erase(std::lower_bound(window.begin(), window.end(), out));
                }
            }
            const double percussive = window[window.size() / 2];
            // steady in time: the median of this bin over the last kHistory frames
            // (an insertion sort of 9 values)
            for (int t = 0; t < kHistory; ++t) {
                const double v = m_history[t][k];
                int j = t;
                while (j > 0 && m_scratch[j - 1] > v) {
                    m_scratch[j] = m_scratch[j - 1];
                    --j;
                }
                m_scratch[j] = v;
            }
            const double harmonic = m_scratch[kHistory / 2];
            const double p2 = percussive * percussive;
            const double drums = p2 / (p2 + harmonic * harmonic + kEps);
            // in the middle of the stereo image: left and right alike
            double vocals = 0;
            if (m_vocalBand[k] > 0) {
                const double ll = m_re[0][k] * m_re[0][k] + m_im[0][k] * m_im[0][k];
                const double rr = m_re[1][k] * m_re[1][k] + m_im[1][k] * m_im[1][k];
                const double lr = m_re[0][k] * m_re[1][k] + m_im[0][k] * m_im[1][k];
                double centre = std::clamp(2 * lr / (ll + rr + kEps), 0.0, 1.0);
                centre *= centre;
                centre *= centre;
                vocals = (1 - drums) * centre * m_vocalBand[k];
            }
            const double harmonies = std::max(0.0, 1 - drums - vocals);
            const double stems = m_gLow * drums + m_gMid * vocals + m_gHigh * harmonies;
            m_mask[k] = m_blend * stems + (1 - m_blend) * normal;
        }
    }

    // Hann analysis x Hann synthesis at 75 % overlap sums to 1.5
    constexpr double kNorm = 1.0 / 1.5;
    for (int c = 0; c < 2; ++c) {
        for (int k = 0; k < kBins; ++k) {
            m_re[c][k] *= m_mask[k];
            m_im[c][k] *= m_mask[k];
        }
        m_pFft->inverse(m_re[c].data(), m_im[c].data(), m_time.data());
        for (int i = 0; i < kSize; ++i) {
            m_acc[c][i] += m_time[i] * m_window[i] * kNorm;
        }
        std::copy(m_acc[c].begin(), m_acc[c].begin() + kHop, m_ready[c].begin());
        std::move(m_acc[c].begin() + kHop, m_acc[c].end(), m_acc[c].begin());
        std::fill(m_acc[c].end() - kHop, m_acc[c].end(), 0.0);
        std::move(m_in[c].begin() + kHop, m_in[c].end(), m_in[c].begin());
    }
}

} // namespace djmantra
