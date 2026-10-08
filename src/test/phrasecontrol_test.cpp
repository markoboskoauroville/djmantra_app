#include "engine/controls/phrasecontrol.h"

#include <gtest/gtest.h>

#include "control/controlobject.h"
#include "test/signalpathtest.h"
#include "track/beats.h"

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
    // Jump to beat 8 of the phrase: 5 beats on, same position within the beat
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_jump"), 8);
    ProcessBuffer();
    ProcessBuffer();
    EXPECT_EQ(7, beat());
    const double seconds =
            ControlObject::get(ConfigKey(m_sGroup1, "playposition")) * m_pTrack->getDuration();
    EXPECT_NEAR(12.2 * 0.5, seconds, 0.02);
    // And back to beat 1
    ControlObject::set(ConfigKey(m_sGroup1, "phrase_jump"), 1);
    ProcessBuffer();
    ProcessBuffer();
    EXPECT_EQ(0, beat());
}
