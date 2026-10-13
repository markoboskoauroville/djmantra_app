#include "effects/backends/builtin/djayfx.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {

constexpr double kRate = 48000;
constexpr int kBuffer = 512;

std::vector<float> sine(double hz, int frames) {
    std::vector<float> out(2 * frames);
    for (int f = 0; f < frames; ++f) {
        out[2 * f] = out[2 * f + 1] = static_cast<float>(
                0.5 * std::sin(2 * M_PI * hz * f / kRate));
    }
    return out;
}

double rms(const std::vector<float>& buffer, int fromFrame, int toFrame) {
    double sum = 0;
    for (int f = fromFrame; f < toFrame; ++f) {
        sum += buffer[2 * f] * buffer[2 * f];
    }
    return std::sqrt(sum / (toFrame - fromFrame));
}

template<typename Fx, typename... Args>
std::vector<float> run(Fx& fx, const std::vector<float>& in, Args... args) {
    std::vector<float> out(in.size());
    const int frames = static_cast<int>(in.size() / 2);
    for (int f = 0; f < frames; f += kBuffer) {
        fx.process(in.data() + 2 * f, out.data() + 2 * f, std::min(kBuffer, frames - f), args...);
    }
    return out;
}

TEST(DjayFxTest, UnderwaterKeepsTheLowsAndDrownsTheHighs) {
    const int frames = static_cast<int>(kRate);
    djmantra::Underwater low(kRate), high(kRate);
    const auto lows = run(low, sine(80, frames), 0.5, 1.0);
    const auto highs = run(high, sine(5000, frames), 0.5, 1.0);
    EXPECT_GT(rms(lows, frames / 2, frames), 0.25);
    EXPECT_LT(rms(highs, frames / 2, frames), 0.01);
}

TEST(DjayFxTest, UnderwaterDryIsUnchanged) {
    djmantra::Underwater fx(kRate);
    const auto in = sine(5000, 4096);
    const auto out = run(fx, in, 1.0, 0.0);
    EXPECT_EQ(in, out);
}

TEST(DjayFxTest, CooldownGlidesTheHighsAway) {
    const int frames = static_cast<int>(kRate * 2);
    djmantra::Cooldown fx(kRate);
    const auto out = run(fx, sine(8000, frames), 1.0, 1.0);
    // At once it still sounds; after the glide the highs are gone.
    EXPECT_GT(rms(out, 0, 2400), 0.1);
    EXPECT_LT(rms(out, frames - 24000, frames), 0.02);
}

TEST(DjayFxTest, EchoOutFadesTheSongAndRingsOn) {
    const int frames = static_cast<int>(kRate * 4);
    djmantra::EchoOut fx(kRate);
    // 2 beats at 120 BPM = 1 s.
    const auto out = run(fx, sine(440, frames), 2.0, 0.5, 1.0);
    const int second = static_cast<int>(kRate);
    EXPECT_GT(rms(out, 0, second / 10), 0.3);             // the song plays on at first
    EXPECT_GT(rms(out, second + 100, 2 * second), 0.1);  // the first echo, no dry
    // Each repeat is quieter than the one before.
    EXPECT_LT(rms(out, 3 * second + 100, 4 * second), rms(out, second + 100, 2 * second));
}

} // namespace
