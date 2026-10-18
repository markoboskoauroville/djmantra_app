#include "analyzer/rbxanalysis.h"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "analyzer/sectionnames.h"

namespace {

constexpr int kRate = 44100;
constexpr double kBpm = 120.0;
constexpr double kFirstKick = 0.25; // seconds

// A house loop at 120 BPM: a kick on every beat (a click, then a 55 Hz thump), a hi-hat on every
// off-beat and a pad; bars 33 to 48 drop the kick and the hat (a breakdown), the pad stays
std::vector<float> houseLoop(int bars) {
    const double beat = 60.0 / kBpm;
    std::vector<float> out(static_cast<size_t>((kFirstKick + bars * 4 * beat + 1.0) * kRate), 0.0f);
    unsigned seed = 1;
    const auto noise = [&seed] {
        seed = seed * 1103515245u + 12345u;
        return ((seed >> 16) & 0x7fff) / 16384.0 - 1.0;
    };
    for (int b = 0; b < bars * 4; ++b) {
        const bool breakdown = b / 4 >= 32 && b / 4 < 48;
        const size_t kick = static_cast<size_t>((kFirstKick + b * beat) * kRate);
        if (!breakdown) {
            for (int i = 0; i < kRate / 4 && kick + i < out.size(); ++i) {
                const double t = static_cast<double>(i) / kRate;
                const double click = i < kRate / 500 ? 0.5 * noise() : 0.0;
                out[kick + i] += static_cast<float>(0.8 * std::exp(-t * 12) * std::sin(2 * M_PI * 55 * t) + click);
            }
            const size_t hat = kick + static_cast<size_t>(beat / 2 * kRate);
            for (int i = 0; i < kRate / 40 && hat + i < out.size(); ++i) {
                out[hat + i] += static_cast<float>(0.15 * std::exp(-i / 200.0) * noise());
            }
        }
    }
    // a bass note per bar and a pad chord per 4 bars, changing on the downbeat (the structure the
    // downbeat stage reads); the bass rests in the breakdown
    const double notes[4] = {55.0, 55.0 * 4 / 3, 55.0 * 3 / 2, 55.0 * 6 / 5};
    for (size_t i = static_cast<size_t>(kFirstKick * kRate); i < out.size(); ++i) {
        const double t = static_cast<double>(i) / kRate - kFirstKick;
        const int bar = static_cast<int>(t / (4 * beat));
        if (bar >= bars) {
            break;
        }
        const bool breakdown = bar >= 32 && bar < 48;
        if (!breakdown) {
            out[i] += static_cast<float>(0.3 * std::sin(2 * M_PI * notes[bar % 4] * t));
        }
        out[i] += static_cast<float>(0.05 * std::sin(2 * M_PI * 2 * notes[(bar / 4) % 4] * 2 * t));
    }
    return out;
}

} // namespace

TEST(RbxAnalysisTest, gridOnTheKickAndTheBreakdownNamed) {
    const auto audio = houseLoop(64);
    const auto r = djmantra::rbx::analyse(audio.data(), audio.size(), kRate);
    EXPECT_NEAR(kBpm, r.bpm, 0.05);
    ASSERT_GT(r.beats.size(), 200u);
    // every beat on a kick (or where one would be in the breakdown), within 3 ms
    const double beat = 60.0 / kBpm;
    for (double t : r.beats) {
        const double k = std::round((t - kFirstKick) / beat);
        EXPECT_NEAR(kFirstKick + k * beat, t, 0.003) << t;
    }
    bool breakdown = false;
    for (const auto& section : r.sections) {
        if (section.name == "BREAKDOWN" || section.name == "BUILD-UP") {
            breakdown = true;
            // bar 33 starts at 0.25 + 32 * 2 s
            EXPECT_NEAR(kFirstKick + 32 * 4 * beat, section.secs, 4 * beat + 0.01);
        }
    }
    EXPECT_TRUE(breakdown);
}

TEST(RbxAnalysisTest, namesAsOnMarkosPage) {
    using djmantra::sections::Bar;
    std::vector<Bar> bars;
    // 16 quiet bars with the beat, 32 full, 16 without the kick, 32 full, 16 quiet with the beat
    for (int b = 0; b < 112; ++b) {
        const bool quiet = b < 16 || b >= 96;
        const bool beatless = b >= 48 && b < 64;
        bars.push_back({quiet ? -14.0 : 0.0, beatless ? -20.0 : 0.0, 0.0});
    }
    const auto named = djmantra::sections::name(bars, {16, 32, 48, 64, 80, 96});
    // the DROP is the part where the beat comes back, MAIN the part after it (as on the page)
    ASSERT_EQ(6u, named.size());
    EXPECT_EQ("INTRO", named[0].second);
    EXPECT_EQ("MAIN", named[1].second);
    EXPECT_EQ(16u, named[1].first);
    EXPECT_EQ("BREAKDOWN", named[2].second);
    EXPECT_EQ(48u, named[2].first);
    EXPECT_EQ("DROP", named[3].second);
    EXPECT_EQ(64u, named[3].first);
    EXPECT_EQ("MAIN", named[4].second);
    EXPECT_EQ("OUTRO", named[5].second);
    EXPECT_EQ(96u, named[5].first);
}
