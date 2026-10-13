#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <vector>

#include "effects/backends/builtin/stemeq.h"

namespace {

constexpr int kRate = 44100;
constexpr double kPi = 3.14159265358979323846;

// 3 s: a 440 Hz tone in the middle (a "voice") + independent noise per channel
std::vector<float> testSignal() {
    const int frames = 3 * kRate;
    std::vector<float> x(2 * frames);
    std::mt19937 random(1);
    std::normal_distribution<float> noise(0, 0.1f);
    for (int i = 0; i < frames; ++i) {
        const float tone = 0.3f * static_cast<float>(std::sin(2 * kPi * 440 * i / kRate));
        x[2 * i] = tone + noise(random);
        x[2 * i + 1] = tone + noise(random);
    }
    return x;
}

std::vector<float> run(const std::vector<float>& x, const djmantra::StemEq::Gains& gains) {
    djmantra::StemEq eq(kRate);
    std::vector<float> y(x.size());
    const int frames = static_cast<int>(x.size() / 2);
    for (int i = 0; i < frames; i += 512) {
        eq.process(&x[2 * i], &y[2 * i], std::min(512, frames - i), gains);
    }
    return y;
}

// the 440 Hz tone's level in the left channel, from frame `from`, `delay` frames late
double toneLevel(const std::vector<float>& v, int from, int delay) {
    double re = 0;
    double im = 0;
    const int n = kRate;
    for (int i = from; i < from + n; ++i) {
        const double phase = 2 * kPi * 440 * (i - delay) / kRate;
        re += v[2 * i] * std::cos(phase);
        im += v[2 * i] * std::sin(phase);
    }
    return 2 * std::hypot(re, im) / n;
}

} // namespace

TEST(StemEqTest, transparentAtUnityInBothModes) {
    const auto x = testSignal();
    for (bool stems : {false, true}) {
        djmantra::StemEq::Gains gains;
        gains.low = gains.mid = gains.high = 0.999; // not the unity shortcut
        gains.stems = stems;
        const auto y = run(x, gains);
        const int delay = djmantra::StemEq::latencyFrames();
        double error = 0;
        double signal = 0;
        for (size_t i = 2 * kRate; i < x.size() / 2; ++i) {
            const double d = y[2 * i] / 0.999 - x[2 * (i - delay)];
            error += d * d;
            signal += x[2 * i] * x[2 * i];
        }
        EXPECT_LT(error / signal, 1e-8) << "stems " << stems;
    }
}

TEST(StemEqTest, vocalsMidAndDrumsLow) {
    const auto x = testSignal();
    const int delay = djmantra::StemEq::latencyFrames();
    const double before = toneLevel(x, kRate, 0);
    djmantra::StemEq::Gains gains;
    gains.stems = true;
    gains.mid = 0; // vocals off: the steady tone in the middle goes
    EXPECT_LT(toneLevel(run(x, gains), kRate, delay), 0.05 * before);
    gains.mid = 1;
    gains.low = 0; // drums off: a steady tone is not a drum, it stays
    EXPECT_GT(toneLevel(run(x, gains), kRate, delay), 0.8 * before);
    gains.stems = false;
    gains.low = 1;
    gains.mid = 0; // the normal mid band: 440 Hz is in it
    EXPECT_LT(toneLevel(run(x, gains), kRate, delay), 0.05 * before);
}
