#include "engine/controls/phrasecontrol.h"
#include "engine/controls/sectiondetector.h"

#include <gtest/gtest.h>

#include "control/controlobject.h"
#include "test/signalpathtest.h"
#include "track/beats.h"
#include "widget/djmantra/wdeckcounter.h"

using mixxx::audio::FramePos;

namespace {

constexpr mixxx::audio::SampleRate kRate(44100);
// 120 BPM: a beat every 22050 frames, the grid starts at frame 1000
constexpr double kBeat = 22050;
constexpr double kFirst = 1000;

mixxx::BeatsPointer grid() {
    return mixxx::Beats::fromConstTempo(kRate, FramePos(kFirst), mixxx::Bpm(120));
}

FramePos inBeat(int n) {
    // Somewhere inside beat n (counted from the first beat)
    return FramePos(kFirst + n * kBeat + kBeat / 3);
}

} // namespace

TEST(PhraseControlTest, countsBeatsFromTheAnchor) {
    const auto pBeats = grid();
    const FramePos anchor(kFirst);
    for (int n = 0; n < 20; ++n) {
        EXPECT_EQ(n % 8, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(n), 8)) << n;
    }
    EXPECT_EQ(5, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(13), 8));
    EXPECT_EQ(1, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(13), 4));
    EXPECT_EQ(13, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(13), 16));
    // Exactly on a beat
    EXPECT_EQ(3, PhraseControl::beatInPhrase(pBeats, anchor, FramePos(kFirst + 3 * kBeat), 8));
}

TEST(PhraseControlTest, anchorInsideTheSongAndBeforeIt) {
    const auto pBeats = grid();
    // "1" is beat 10 of the grid (e.g. the main cue on the first drop)
    const FramePos anchor(kFirst + 10 * kBeat);
    EXPECT_EQ(0, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(10), 8));
    EXPECT_EQ(7, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(17), 8));
    EXPECT_EQ(0, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(18), 8));
    // Before "1" the count runs on backwards: beat 9 is the last of the phrase before
    EXPECT_EQ(7, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(9), 8));
    // Beat 2 is 8 beats before "1": also a "1"
    EXPECT_EQ(0, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(2), 8));
    EXPECT_EQ(1, PhraseControl::beatInPhrase(pBeats, anchor, inBeat(3), 8));
}

TEST(PhraseControlTest, beatNumberFromTheAnchor) {
    const auto pBeats = grid();
    const FramePos anchor(kFirst + 10 * kBeat);
    int n = 99;
    EXPECT_TRUE(PhraseControl::beatNumber(pBeats, anchor, inBeat(10), &n));
    EXPECT_EQ(0, n);
    EXPECT_TRUE(PhraseControl::beatNumber(pBeats, anchor, inBeat(27), &n));
    EXPECT_EQ(17, n);
    EXPECT_TRUE(PhraseControl::beatNumber(pBeats, anchor, inBeat(7), &n));
    EXPECT_EQ(-3, n);
    EXPECT_FALSE(PhraseControl::beatNumber(mixxx::BeatsPointer(), anchor, inBeat(7), &n));
}

TEST(PhraseControlTest, barBeatAsInAbleton) {
    EXPECT_EQ(QStringLiteral("1.1"), WDeckCounter::barBeat(0, 4));
    EXPECT_EQ(QStringLiteral("1.4"), WDeckCounter::barBeat(3, 4));
    EXPECT_EQ(QStringLiteral("2.1"), WDeckCounter::barBeat(4, 4));
    EXPECT_EQ(QStringLiteral("17.3"), WDeckCounter::barBeat(66, 4));
    EXPECT_EQ(QStringLiteral("3.2"), WDeckCounter::barBeat(7, 3));
    EXPECT_EQ(QStringLiteral("2.6"), WDeckCounter::barBeat(11, 6));
    // Before "1": bar 0, then -1
    EXPECT_EQ(QStringLiteral("0.4"), WDeckCounter::barBeat(-1, 4));
    EXPECT_EQ(QStringLiteral("0.1"), WDeckCounter::barBeat(-4, 4));
    EXPECT_EQ(QStringLiteral("-1.4"), WDeckCounter::barBeat(-5, 4));
    // Left to the next section
    EXPECT_EQ(QStringLiteral("-6.2"), WDeckCounter::barsBeatsLeft(26, 4));
    EXPECT_EQ(QStringLiteral("-0.0"), WDeckCounter::barsBeatsLeft(0, 4));
    EXPECT_EQ(QStringLiteral("-2.1"), WDeckCounter::barsBeatsLeft(7, 3));
    // Whole seconds, cut off like the other time displays
    EXPECT_EQ(QStringLiteral("1:23/5:40/-4:16"), WDeckCounter::timeText(83.4, 256.9));
}

TEST(PhraseControlTest, nextSectionIsTheFirstBoundaryAhead) {
    const auto pBeats = grid();
    const QList<FramePos> boundaries = {FramePos(kFirst + 32 * kBeat),
            FramePos(kFirst + 16 * kBeat),
            mixxx::audio::kInvalidFramePos,
            FramePos(kFirst + 4 * kBeat)};
    EXPECT_EQ(FramePos(kFirst + 16 * kBeat),
            PhraseControl::nextSection(pBeats, inBeat(5), boundaries));
    // On a section start (or just after it): the following one
    EXPECT_EQ(FramePos(kFirst + 32 * kBeat),
            PhraseControl::nextSection(pBeats, FramePos(kFirst + 16 * kBeat), boundaries));
    EXPECT_FALSE(PhraseControl::nextSection(pBeats, inBeat(40), boundaries).isValid());
}

TEST(PhraseControlTest, prevSectionIsTheOneThePositionIsIn) {
    const auto pBeats = grid();
    const QList<FramePos> boundaries = {FramePos(kFirst + 32 * kBeat),
            FramePos(kFirst + 16 * kBeat),
            mixxx::audio::kInvalidFramePos};
    // before the first section: none (the counter runs from the song's "1")
    EXPECT_FALSE(PhraseControl::prevSection(pBeats, inBeat(5), boundaries).isValid());
    // on a section start and inside it: that one
    EXPECT_EQ(FramePos(kFirst + 16 * kBeat),
            PhraseControl::prevSection(pBeats, FramePos(kFirst + 16 * kBeat), boundaries));
    EXPECT_EQ(FramePos(kFirst + 16 * kBeat),
            PhraseControl::prevSection(pBeats, inBeat(20), boundaries));
    EXPECT_EQ(FramePos(kFirst + 32 * kBeat),
            PhraseControl::prevSection(pBeats, inBeat(40), boundaries));
}

TEST(PhraseControlTest, detectorV2FindsKickBreakdownAndBass) {
    // 96 bars of 4 beats, 20 waveform points a beat, 2 beats of lead-in: no
    // kick for 16 bars, the kick, a breakdown (no kick, a pad) at 48, the kick
    // and a bass from 64; the hats run all the way
    std::vector<std::array<double, 3>> points(40, {5, 5, 5});
    std::vector<double> beats;
    for (int bar = 0; bar < 96; ++bar) {
        for (int beat = 0; beat < 4; ++beat) {
            beats.push_back(static_cast<double>(points.size()));
            const bool kick = (bar >= 16 && bar < 48) || bar >= 64;
            for (int j = 0; j < 20; ++j) {
                double low = kick ? (j < 4 ? 200 : 20) : 10;
                if (bar >= 64) {
                    low += 40;
                }
                const double mid = bar >= 48 && bar < 64 ? 120 : 30;
                const double high = j >= 8 && j < 12 ? 60 : 20;
                points.push_back({low, mid, high});
            }
        }
    }
    beats.push_back(static_cast<double>(points.size()));
    // the beats where bars 16, 48 and 64 start
    EXPECT_EQ(QList<int>({64, 192, 256}), djmantra::sectionBeatsV2(points, beats, 4));
    // a steady song: no section
    std::vector<std::array<double, 3>> steady;
    std::vector<double> steadyBeats;
    for (int beat = 0; beat < 96 * 4; ++beat) {
        steadyBeats.push_back(static_cast<double>(steady.size()));
        for (int j = 0; j < 20; ++j) {
            steady.push_back({j < 4 ? 200.0 : 20.0, 30, 20});
        }
    }
    EXPECT_TRUE(djmantra::sectionBeatsV2(steady, steadyBeats, 4).isEmpty());
}

TEST(PhraseControlTest, sectionsWhereTheArrangementChanges) {
    // 16 bars without bass, 16 with, 16 without the highs, 16 as before
    QList<std::array<double, 3>> bars;
    for (int i = 0; i < 16; ++i) {
        bars.append({10, 80, 60});
    }
    for (int i = 0; i < 16; ++i) {
        bars.append({120, 80, 60});
    }
    for (int i = 0; i < 16; ++i) {
        bars.append({120, 80, 5});
    }
    for (int i = 0; i < 16; ++i) {
        bars.append({120, 80, 5});
    }
    EXPECT_EQ(QList<int>({16, 32}), djmantra::sectionBars(bars, 0));
    // Small changes are not sections
    QList<std::array<double, 3>> steady;
    for (int i = 0; i < 32; ++i) {
        steady.append({100.0 + (i % 3), 80, 60});
    }
    EXPECT_TRUE(djmantra::sectionBars(steady, 0).isEmpty());
    // Phrases counted from "1": with bar 0 being bar -2 of the song, a change at
    // index 18 (bar 16) is found, one at index 16 (bar 14) is not a phrase start
    QList<std::array<double, 3>> shifted;
    for (int i = 0; i < 18; ++i) {
        shifted.append({10, 80, 60});
    }
    for (int i = 0; i < 18; ++i) {
        shifted.append({120, 80, 60});
    }
    EXPECT_EQ(QList<int>({18}), djmantra::sectionBars(shifted, 2));
}

TEST(PhraseControlTest, anchorSlightlyOffTheGridSnaps) {
    const auto pBeats = grid();
    // A cue set 40 ms late still counts as its beat
    const FramePos lateCue(kFirst + 4 * kBeat + 0.04 * kRate);
    EXPECT_EQ(0, PhraseControl::beatInPhrase(pBeats, lateCue, inBeat(4), 8));
    EXPECT_EQ(1, PhraseControl::beatInPhrase(pBeats, lateCue, inBeat(5), 8));
    const FramePos earlyCue(kFirst + 4 * kBeat - 0.04 * kRate);
    EXPECT_EQ(0, PhraseControl::beatInPhrase(pBeats, earlyCue, inBeat(4), 8));
}

TEST(PhraseControlTest, noGridNoBeat) {
    EXPECT_EQ(-1, PhraseControl::beatInPhrase(nullptr, FramePos(0), FramePos(100), 8));
    EXPECT_EQ(-1, PhraseControl::beatInPhrase(grid(), mixxx::audio::kInvalidFramePos, inBeat(3), 8));
}

class PhraseControlEngineTest : public BaseSignalPathTest {
  protected:
    void SetUp() override {
        const QString location = getTestDir().filePath(QStringLiteral("sine-30.wav"));
        m_pTrack = Track::newTemporary(location);
        loadTrack(m_pMixerDeck1, m_pTrack);
        const auto sampleRate = m_pTrack->getSampleRate();
        // 120 BPM grid from the start of the file
        ASSERT_TRUE(m_pTrack->trySetBeats(mixxx::Beats::fromConstTempo(
                sampleRate, mixxx::audio::kStartFramePos, mixxx::Bpm(120))));
        // "1" at the start (by default it is the main cue's beat)
        ControlObject::set(ConfigKey(m_sGroup1, "phrase_anchor"), 0);
        ProcessBuffer();
    }
    double beat() const {
        return ControlObject::get(ConfigKey(m_sGroup1, "beat_in_phrase"));
    }
    void seekToBeat(double beats) {
        // play position in seconds -> fraction of the track
        ControlObject::set(ConfigKey(m_sGroup1, "playposition"),
                beats * 0.5 / m_pTrack->getDuration());
        // The seek happens in the next buffer
        ProcessBuffer();
        ProcessBuffer();
    }
    TrackPointer m_pTrack;
};

TEST_F(PhraseControlEngineTest, followsThePlayPosition) {
    seekToBeat(0.3);
    EXPECT_EQ(0, beat());
    seekToBeat(3.4);
    EXPECT_EQ(3, beat());
    seekToBeat(9.5);
    EXPECT_EQ(1, beat());
    // Ableton's counter: beat 9 from "1" is bar 3, beat 2 in 4/4
    EXPECT_EQ(9, ControlObject::get(ConfigKey(m_sGroup1, "beat_number")));
    EXPECT_EQ(4, ControlObject::get(ConfigKey(m_sGroup1, "beats_per_bar")));
    EXPECT_EQ(4, ControlObject::get(ConfigKey(m_sGroup1, "beat_unit")));
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_length"), 4);
    ProcessBuffer();
    EXPECT_EQ(1, beat());
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_length"), 16);
    ProcessBuffer();
    EXPECT_EQ(9, beat());
}

TEST_F(PhraseControlEngineTest, setOneAndJump) {
    seekToBeat(5.2);
    EXPECT_EQ(5, beat());
    // "This beat is 1"
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_set_one"), 1);
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_set_one"), 0);
    ProcessBuffer();
    EXPECT_EQ(0, beat());
    seekToBeat(7.2);
    EXPECT_EQ(2, beat());
    // Jump to beat 8 of the phrase: 5 beats on; the deck is stopped, so it
    // lands exactly on the beat (playing, it keeps the place within the beat)
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_jump"), 8);
    ProcessBuffer();
    ProcessBuffer();
    EXPECT_EQ(7, beat());
    const double seconds =
            ControlObject::get(ConfigKey(m_sGroup1, "playposition")) * m_pTrack->getDuration();
    EXPECT_NEAR(12.0 * 0.5, seconds, 0.02);
    // And back to beat 1
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_jump"), 1);
    ProcessBuffer();
    ProcessBuffer();
    EXPECT_EQ(0, beat());
}

TEST_F(PhraseControlEngineTest, jumpWhilePlayingWaitsForTheBeat) {
    // Round 19 (Marko: "as Ableton with snap 1 beat"): a jump on a playing
    // deck waits for the next beat and happens on it: the beat that would
    // have come is replaced by the chosen one, the music stays in time
    seekToBeat(2.6);
    ASSERT_EQ(2, beat());
    ControlObject::set(ConfigKey(m_sGroup1, "play"), 1);
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_jump"), 6);
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_jump"), 0);
    ProcessBuffer();
    // Still beat 3 (index 2) until its end: nothing jumps at the press
    EXPECT_EQ(2, beat());
    int buffers = 0;
    while (beat() == 2 && buffers < 200) {
        ProcessBuffer();
        ++buffers;
    }
    ProcessBuffer();
    ProcessBuffer();
    // On the next beat the deck is on beat 6 (index 5), just after its start
    EXPECT_EQ(5, beat());
    const double seconds =
            ControlObject::get(ConfigKey(m_sGroup1, "playposition")) * m_pTrack->getDuration();
    EXPECT_NEAR(5.0 * 0.5, seconds, 0.1);
    ControlObject::set(ConfigKey(m_sGroup1, "play"), 0);
}
