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

SmallReverb::SmallReverb(double sampleRate) {
    setSampleRate(sampleRate);
}

void SmallReverb::setSampleRate(double sampleRate) {
    static constexpr std::array<double, kLines> kMs{29.7, 37.1, 41.1, 43.7};
    for (int i = 0; i < kLines; ++i) {
        const auto length = static_cast<std::size_t>(kMs[i] * sampleRate / 1000.0);
        if (m_lines[i].size() != length) {
            m_lines[i].assign(length, 0.0f);
            m_pos[i] = 0;
        }
    }
}

void SmallReverb::clear() {
    for (int i = 0; i < kLines; ++i) {
        std::fill(m_lines[i].begin(), m_lines[i].end(), 0.0f);
        m_damp[i] = 0;
    }
}

void SmallReverb::process(
        float inL, float inR, float decay, float level, float* pL, float* pR) {
    std::array<float, kLines> out;
    for (int i = 0; i < kLines; ++i) {
        // Damping: a one-pole lowpass on each line darkens the tail.
        m_damp[i] += 0.55f * (m_lines[i][m_pos[i]] - m_damp[i]);
        out[i] = m_damp[i];
    }
    // Hadamard mix (orthogonal, scaled by 1/2) keeps the energy.
    const float a = out[0] + out[1], b = out[0] - out[1];
    const float c = out[2] + out[3], d = out[2] - out[3];
    const std::array<float, kLines> mixed{
            0.5f * (a + c), 0.5f * (b + d), 0.5f * (a - c), 0.5f * (b - d)};
    const std::array<float, kLines> in{inL, inL, inR, inR};
    for (int i = 0; i < kLines; ++i) {
        m_lines[i][m_pos[i]] = 0.5f * in[i] + decay * mixed[i];
        if (++m_pos[i] >= static_cast<int>(m_lines[i].size())) {
            m_pos[i] = 0;
        }
    }
    *pL += level * (out[0] + out[2]);
    *pR += level * (out[1] + out[3]);
}

Underwater::Underwater(double sampleRate)
        : m_sampleRate(sampleRate) {
    reset();
}

void Underwater::setSampleRate(double sampleRate) {
    m_sampleRate = sampleRate;
}

void Underwater::reset() {
    m_phase = 0;
    m_cutoff = 1200;
    m_a.reset();
    m_b.reset();
}

void Underwater::process(
        const float* pIn, float* pOut, int frames, double amount, double mix) {
    amount = clamp01(amount);
    mix = clamp01(mix);
    const double target = 1200.0 * std::pow(180.0 / 1200.0, amount);
    const double q = 1.1 + 2.4 * amount; // resonant, more so when deeper
    // Resonance raises the corner; give back the body it takes.
    const auto makeUp = static_cast<float>(1.0 + 0.6 * amount);
    for (int start = 0; start < frames; start += kControlFrames) {
        const int n = std::min(kControlFrames, frames - start);
        m_cutoff = glide(m_cutoff, target, 0.08, n, m_sampleRate);
        m_phase += 2 * kPi * 0.35 * n / m_sampleRate;
        if (m_phase > 2 * kPi) {
            m_phase -= 2 * kPi;
        }
        const double cutoff = m_cutoff * (1.0 + 0.22 * std::sin(m_phase));
        m_a.set(m_sampleRate, cutoff, q);
        m_b.set(m_sampleRate, cutoff, 0.707);
        for (int f = start; f < start + n; ++f) {
            for (int ch = 0; ch < 2; ++ch) {
                const float dry = pIn[2 * f + ch];
                const float wet = makeUp * m_b.process(m_a.process(dry, ch), ch);
                pOut[2 * f + ch] = static_cast<float>(dry * (1 - mix) + wet * mix);
            }
        }
    }
}

Cooldown::Cooldown(double sampleRate)
        : m_sampleRate(sampleRate),
          m_reverb(sampleRate) {
    reset();
}

void Cooldown::setSampleRate(double sampleRate) {
    if (sampleRate != m_sampleRate) {
        m_sampleRate = sampleRate;
        m_reverb.setSampleRate(sampleRate);
    }
}

void Cooldown::reset() {
    m_lowpass = 16000;
    m_highpass = 20;
    m_wet = 0;
    m_lp.reset();
    m_hp.reset();
    m_reverb.clear();
}

void Cooldown::process(
        const float* pIn, float* pOut, int frames, double amount, double mix) {
    amount = clamp01(amount);
    mix = clamp01(mix);
    const double lowTarget = 3000.0 * std::pow(400.0 / 3000.0, amount);
    const double highTarget = 60.0 + 60.0 * amount;
    const auto decay = static_cast<float>(0.62 + 0.2 * amount);
    for (int start = 0; start < frames; start += kControlFrames) {
        const int n = std::min(kControlFrames, frames - start);
        m_lowpass = glide(m_lowpass, lowTarget, 0.2, n, m_sampleRate);
        m_highpass = glide(m_highpass, highTarget, 0.2, n, m_sampleRate);
        m_wet = glide(m_wet, 1.0, 0.25, n, m_sampleRate);
        m_lp.set(m_sampleRate, m_lowpass, 0.8);
        m_hp.set(m_sampleRate, m_highpass, 0.707);
        const auto reverbLevel = static_cast<float>(m_wet * (0.25 + 0.3 * amount));
        const auto level = static_cast<float>(1.0 - 0.25 * m_wet * (0.5 + amount));
        for (int f = start; f < start + n; ++f) {
            float high = 0;
            float wet[2];
            for (int ch = 0; ch < 2; ++ch) {
                m_hp.process(pIn[2 * f + ch], ch, &high);
                wet[ch] = m_lp.process(high, ch);
            }
            float outL = wet[0] * level;
            float outR = wet[1] * level;
            m_reverb.process(wet[0], wet[1], decay, reverbLevel, &outL, &outR);
            pOut[2 * f] = static_cast<float>(pIn[2 * f] * (1 - mix) + outL * mix);
            pOut[2 * f + 1] = static_cast<float>(pIn[2 * f + 1] * (1 - mix) + outR * mix);
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
    m_loop.set(sampleRate, 3500, 0.707);
    m_loopHigh.set(sampleRate, 200, 0.707);
    reset();
}

void EchoOut::reset() {
    std::fill(m_buffer.begin(), m_buffer.end(), 0.0f);
    m_write = 0;
    m_elapsed = 0;
    m_delayFrames = 0;
    m_loop.reset();
    m_loopHigh.reset();
}

void EchoOut::process(const float* pIn,
        float* pOut,
        int frames,
        double beats,
        double beatSeconds,
        double mix) {
    mix = clamp01(mix);
    const int capacity = static_cast<int>(m_buffer.size() / 2);
    if (m_delayFrames == 0) {
        // The echo time is fixed when the FX is pressed, like djay's.
        const double seconds = std::max(0.05, beats * beatSeconds);
        m_delayFrames = std::clamp(
                static_cast<int>(seconds * m_sampleRate), 1, capacity - 1);
    }
    const double window = m_delayFrames;
    constexpr float kFeedback = 0.62f;
    for (int f = 0; f < frames; ++f, ++m_elapsed) {
        const double t = m_elapsed / window;
        const auto dryGain = static_cast<float>(1.0 - mix * std::min(1.0, t));
        // Send the first window into the delay; close the send over ~5 ms.
        const double sendFade = (m_elapsed - window) / (0.005 * m_sampleRate);
        const auto send = static_cast<float>(t < 1.0 ? 1.0 : std::max(0.0, 1.0 - sendFade));
        int read = m_write - m_delayFrames;
        if (read < 0) {
            read += capacity;
        }
        for (int ch = 0; ch < 2; ++ch) {
            const float echo = m_buffer[2 * read + ch];
            float high = 0;
            const float dark = m_loop.process(echo, ch);
            m_loopHigh.process(dark, ch, &high);
            m_buffer[2 * m_write + ch] = pIn[2 * f + ch] * send + high * kFeedback;
            pOut[2 * f + ch] = pIn[2 * f + ch] * dryGain + echo * static_cast<float>(mix);
        }
        if (++m_write >= capacity) {
            m_write = 0;
        }
    }
}

} // namespace djmantra
