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
    const auto lows = run(low, sine(80, frames), 1.0, 1.0);
    const auto highs = run(high, sine(5000, frames), 1.0, 1.0);
    EXPECT_GT(rms(lows, frames / 2, frames), 0.3);
    EXPECT_LT(rms(highs, frames / 2, frames), 0.01);
    EXPECT_NEAR(djmantra::Underwater::cutoffFor(0.5), 4870, 100); // djay's middle
}

TEST(DjayFxTest, UnderwaterDryIsUnchanged) {
    djmantra::Underwater fx(kRate);
    const auto in = sine(5000, 4096);
    const auto out = run(fx, in, 1.0, 0.0);
    EXPECT_EQ(in, out);
}

TEST(DjayFxTest, CooldownIsABandPassAndQuieter) {
    const int frames = static_cast<int>(kRate);
    const double full = 0.5 / std::sqrt(2.0);
    djmantra::Cooldown a(kRate), b(kRate), c(kRate);
    const auto mid = run(a, sine(1000, frames), 0.5, 1.0);
    const auto low = run(b, sine(60, frames), 0.5, 1.0);
    const auto high = run(c, sine(9000, frames), 0.5, 1.0);
    // 1 kHz: kept (djay: +1.5 dB there); 60 Hz and 9 kHz: well down
    EXPECT_NEAR(20 * std::log10(rms(mid, frames / 2, frames) / full), 0.5, 1.5);
    EXPECT_LT(rms(low, frames / 2, frames), full * 0.12);
    EXPECT_LT(rms(high, frames / 2, frames), full * 0.15);
}

TEST(DjayFxTest, CooldownAtTheRightEndFadesOut) {
    const int frames = static_cast<int>(kRate * 4);
    djmantra::Cooldown fx(kRate);
    const auto out = run(fx, sine(1000, frames), 1.0, 1.0);
    EXPECT_GT(rms(out, 0, 4800), 0.1);
    EXPECT_LT(rms(out, frames - 4800, frames), 0.5 * std::pow(10.0, -45 / 20.0));
}

TEST(DjayFxTest, EchoOutLoopsTheLastBeats) {
    const int second = static_cast<int>(kRate);
    djmantra::EchoOut fx(kRate);
    // 2 beats at 120 BPM = 1 s. One second of song, then engaged for three seconds.
    const auto song = sine(440, second);
    std::vector<float> out(song.size());
    fx.process(song.data(), out.data(), second, false, 2.0, 0.5, 1.0);
    EXPECT_EQ(song, out); // not engaged: the song as it is
    const std::vector<float> silence(2 * 3 * second, 0.0f);
    // the input goes on but is cut: feed noise-free silence to see only the echo
    std::vector<float> echo(silence.size());
    for (int f = 0; f < 3 * second; f += kBuffer) {
        fx.process(silence.data() + 2 * f, echo.data() + 2 * f, std::min(kBuffer, 3 * second - f),
                true, 2.0, 0.5, 1.0);
    }
    const double first = rms(echo, 1000, second);
    const double second2 = rms(echo, second + 1000, 2 * second);
    EXPECT_GT(first, 0.3);                       // the last 2 beats again, full
    EXPECT_NEAR(second2 / first, 0.6, 0.05);     // each round 0.6 as loud
}

} // namespace
