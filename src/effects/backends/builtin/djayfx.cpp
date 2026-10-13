#include "effects/backends/builtin/djayfx.h"

#include <algorithm>
#include <cmath>

namespace djmantra {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr int kControlFrames = 32; // filter corners update every 32 frames

double clamp01(double value) {
    return std::clamp(value, 0.0, 1.0);
}

// One-pole smoothing towards target over `seconds` (time constant).
double glide(double current, double target, double seconds, int frames, double sampleRate) {
    const double k = 1.0 - std::exp(-frames / (seconds * sampleRate));
    return current + (target - current) * k;
}

} // namespace

void Svf::set(double sampleRate, double cutoff, double q) {
    cutoff = std::clamp(cutoff, 10.0, sampleRate * 0.45);
    const double g = std::tan(kPi * cutoff / sampleRate);
    m_k = 1.0 / q;
    m_a1 = 1.0 / (1.0 + g * (g + m_k));
    m_a2 = g * m_a1;
    m_a3 = g * m_a2;
}

float Svf::process(float x, int channel, float* pHigh) {
    double& ic1 = m_ic1[channel];
    double& ic2 = m_ic2[channel];
    const double v3 = x - ic2;
    const double v1 = m_a1 * ic1 + m_a2 * v3;
    const double v2 = ic2 + m_a2 * ic1 + m_a3 * v3;
    ic1 = 2 * v1 - ic1;
    ic2 = 2 * v2 - ic2;
    if (pHigh) {
        *pHigh = static_cast<float>(x - m_k * v1 - v2);
    }
    return static_cast<float>(v2);
}

Underwater::Underwater(double sampleRate)
        : m_sampleRate(sampleRate) {
    reset();
}

void Underwater::setSampleRate(double sampleRate) {
    m_sampleRate = sampleRate;
}

void Underwater::reset() {
    m_cutoff = 16000;
    m_a.reset();
}

// static
double Underwater::cutoffFor(double amount) {
    // round 38 (the phone vs djay): at djay's middle 6-16 kHz are only ~8.5 dB down,
    // so a gentle 12 dB slope near 5 kHz; the right end ~700 Hz
    return 16000.0 * std::pow(700.0 / 16000.0, std::pow(clamp01(amount), 1.4));
}

void Underwater::process(
        const float* pIn, float* pOut, int frames, double amount, double mix) {
    amount = clamp01(amount);
    mix = clamp01(mix);
    const double target = cutoffFor(amount);
    // djay: a small bump at the corner (+3 dB at 500 Hz - 1 kHz in the recordings)
    const double q = 0.9 + 0.6 * amount;
    for (int start = 0; start < frames; start += kControlFrames) {
        const int n = std::min(kControlFrames, frames - start);
        m_cutoff = glide(m_cutoff, target, 0.05, n, m_sampleRate);
        m_a.set(m_sampleRate, m_cutoff, q);
        for (int f = start; f < start + n; ++f) {
            for (int ch = 0; ch < 2; ++ch) {
                const float dry = pIn[2 * f + ch];
                const float wet = m_a.process(dry, ch);
                pOut[2 * f + ch] = static_cast<float>(dry * (1 - mix) + wet * mix);
            }
        }
    }
}

Cooldown::Cooldown(double sampleRate)
        : m_sampleRate(sampleRate) {
    reset();
}

void Cooldown::setSampleRate(double sampleRate) {
    m_sampleRate = sampleRate;
}

void Cooldown::reset() {
    m_lowpass = 20000;
    m_highpass = 20;
    m_level = 1;
    m_fadeDb = 0;
    m_lp.reset();
    m_lp2.reset();
    m_hp.reset();
}

void Cooldown::process(
        const float* pIn, float* pOut, int frames, double amount, double mix) {
    amount = clamp01(amount);
    mix = clamp01(mix);
    // the band-pass and the level reach djay's middle at 0.5 and stay there
    const double depth = std::min(1.0, amount / 0.5);
    // round 38 (the phone was ~2x deeper than djay), fitted on djay's own recording
    // (within ~3 dB per band): highpass 300 Hz (12 dB), lowpass 4 kHz (24 dB), the
    // 400 Hz - 2 kHz band slightly up (+1 dB) as djay's
    const double lowTarget = 20000.0 * std::pow(4000.0 / 20000.0, depth);
    const double highTarget = 20.0 * std::pow(300.0 / 20.0, depth);
    const double fadeRate = 15.0 * std::max(0.0, (amount - 0.6) / 0.4); // dB a second
    for (int start = 0; start < frames; start += kControlFrames) {
        const int n = std::min(kControlFrames, frames - start);
        m_fadeDb = std::min(90.0, m_fadeDb + fadeRate * n / m_sampleRate);
        const double levelTarget = std::pow(10.0, (1.0 * depth - m_fadeDb) / 20.0);
        m_lowpass = glide(m_lowpass, lowTarget, 0.05, n, m_sampleRate);
        m_highpass = glide(m_highpass, highTarget, 0.05, n, m_sampleRate);
        m_level = glide(m_level, levelTarget, 0.05, n, m_sampleRate);
        m_lp.set(m_sampleRate, m_lowpass, 0.707);
        m_lp2.set(m_sampleRate, m_lowpass, 0.707);
        m_hp.set(m_sampleRate, m_highpass, 0.707);
        const auto level = static_cast<float>(m_level);
        for (int f = start; f < start + n; ++f) {
            for (int ch = 0; ch < 2; ++ch) {
                float high = 0;
                m_hp.process(pIn[2 * f + ch], ch, &high);
                const float wet = m_lp2.process(m_lp.process(high, ch), ch) * level;
                pOut[2 * f + ch] = static_cast<float>(pIn[2 * f + ch] * (1 - mix) + wet * mix);
            }
        }
    }
}

EchoOut::EchoOut(double sampleRate)
        : m_sampleRate(0) {
    setSampleRate(sampleRate);
}

void EchoOut::setSampleRate(double sampleRate) {
    if (sampleRate == m_sampleRate) {
        return;
    }
    m_sampleRate = sampleRate;
    m_buffer.assign(static_cast<std::size_t>(kMaxDelaySeconds * sampleRate) * 2, 0.0f);
    reset();
}

void EchoOut::reset() {
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    m_write = 0;
    m_delayFrames = 0;
    m_wet = 0;
}

void EchoOut::process(const float* pIn,
        float* pOut,
        int frames,
        bool engaged,
        double beats,
        double beatSeconds,
        double mix) {
    mix = clamp01(mix);
    const int capacity = static_cast<int>(m_buffer.size() / 2);
    if (!engaged || m_delayFrames == 0) {
        // the echo time follows the tempo until it is engaged
        const double seconds = std::max(0.05, beats * beatSeconds);
        m_delayFrames = std::clamp(static_cast<int>(seconds * m_sampleRate), 1, capacity - 1);
    }
    // round 38 (the phone vs djay): djay's 2-beat echo dies ~6.5 dB a second (-6 dB a
    // round at 130 BPM), its 1-beat one ~10 dB a second (-4.6 dB a round), the lows kept
    // level with the rest (V28's lowcut in the loop overshot)
    const float feedback = beats > 1.5 ? 0.5f : 0.59f;
    const float step = static_cast<float>(1.0 / (0.005 * m_sampleRate)); // 5 ms ramps
    const auto wetTarget = static_cast<float>(engaged ? mix : 0.0);
    for (int f = 0; f < frames; ++f) {
        if (m_wet < wetTarget) {
            m_wet = std::min(wetTarget, m_wet + step);
        } else if (m_wet > wetTarget) {
            m_wet = std::max(wetTarget, m_wet - step);
        }
        int read = m_write - m_delayFrames;
        if (read < 0) {
            read += capacity;
        }
        for (int ch = 0; ch < 2; ++ch) {
            const float in = pIn[2 * f + ch];
            const float echo = m_buffer[2 * read + ch];
            // engaged: the loop of the last beats, each round 0.6 as loud;
            // not engaged: the input fills the line
            m_buffer[2 * m_write + ch] = in * (1 - m_wet) + echo * feedback * m_wet;
            pOut[2 * f + ch] = in * (1 - m_wet) + echo * m_wet;
        }
        if (++m_write >= capacity) {
            m_write = 0;
        }
    }
}

} // namespace djmantra
