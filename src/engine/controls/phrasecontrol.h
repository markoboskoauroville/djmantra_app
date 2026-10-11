#pragma once

#include <memory>

#include "audio/frame.h"
#include "engine/controls/enginecontrol.h"
#include "preferences/usersettings.h"
#include "track/beats.h"
#include "track/track_decl.h"

class ControlObject;
class ControlProxy;
class ControlPushButton;

/// DJ Mantra: where the deck is in its phrase, one count per beat, for the
/// beat lights that align two songs "1 with 1" (djay-style slicer lights).
///
/// Controls of the deck group:
///   beat_in_phrase   0 .. phrase_length-1, -1 without a beat grid (read-only)
///   phrase_length    4, 8 (default), 16 or 32 beats
///   phrase_anchor    "1" of the phrase as an engine sample position; -1 = auto:
///                    the main cue if set, otherwise the first beat of the grid
///   phrase_set_one   button: the beat closest to the play position is "1"
///   phrase_jump      set to n (1-based) to jump to beat n of the current
///                    phrase, keeping the position within the beat
///   beat_number      beats since "1" (0-based, negative before it); valid
///                    while beat_in_phrase >= 0 (read-only). With
///                    beats_per_bar it gives Ableton's bar.beat counter.
///   beats_per_bar    the time signature's upper number, 1..16 (default 4)
///   beat_unit        its lower number: 2, 4 (default), 8 or 16
///
/// Counted on the beat grid itself, so beat maps and tempo changes work.
class PhraseControl : public EngineControl {
    Q_OBJECT
  public:
    PhraseControl(const QString& group, UserSettingsPointer pConfig);
    ~PhraseControl() override;

    void trackLoaded(TrackPointer pNewTrack) override;
    void trackBeatsUpdated(mixxx::BeatsPointer pBeats) override;

    /// Beats from "1" (the beat closest to `anchor`) to the beat that
    /// `position` is in: 0 on "1", negative before it. Returns false
    /// without beats.
    static bool beatNumber(const mixxx::BeatsPointer& pBeats,
            mixxx::audio::FramePos anchor,
            mixxx::audio::FramePos position,
            int* pNumber);

    /// Called with the play position after every buffer (like ClockControl).
    void updatePosition(mixxx::audio::FramePos position);

    /// The beat in the phrase at `position` (0-based), -1 without beats.
    /// Static so that it can be tested without an engine.
    static int beatInPhrase(const mixxx::BeatsPointer& pBeats,
            mixxx::audio::FramePos anchor,
            mixxx::audio::FramePos position,
            int phraseLength);

  private slots:
    void slotSetOne(double value);
    void slotJump(double value);
    void slotParametersChanged(double);

  private:
    mixxx::audio::FramePos anchor() const;

    std::unique_ptr<ControlObject> m_pBeatInPhrase;
    std::unique_ptr<ControlObject> m_pPhraseLength;
    std::unique_ptr<ControlObject> m_pPhraseAnchor;
    std::unique_ptr<ControlPushButton> m_pSetOne;
    std::unique_ptr<ControlObject> m_pJump;
    std::unique_ptr<ControlObject> m_pBeatNumber;
    std::unique_ptr<ControlObject> m_pBeatsPerBar;
    std::unique_ptr<ControlObject> m_pBeatUnit;
    std::unique_ptr<ControlProxy> m_pCuePoint;

    mixxx::BeatsPointer m_pBeats;
    mixxx::audio::FramePos m_position;
    // The beat interval of the last result, to count on cheaply
    mixxx::audio::FramePos m_prevBeat;
    mixxx::audio::FramePos m_nextBeat;
    int m_beat = -1;
    int m_beatNumber = 0;
};
