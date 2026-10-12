#pragma once

#include <QList>
#include <QStringList>
#include <memory>
#include <vector>

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
///   section_beats_left  beats from the current beat to the next section
///                    (read-only), -1 without beats
///   section_found    1: section_beats_left counts to a section start (a
///                    change found in the arrangement, intro / outro
///                    markers, hot cues); 0: to the end of the track
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

    /// The start of the next section after `position`: the first of
    /// `boundaries` (frames, in any order; invalid ones are skipped) that is
    /// more than half a beat ahead. Invalid if none.
    static mixxx::audio::FramePos nextSection(const mixxx::BeatsPointer& pBeats,
            mixxx::audio::FramePos position,
            const QList<mixxx::audio::FramePos>& boundaries);

    /// The cue controls are made after this control: EngineBuffer calls
    /// this once they exist.
    void connectCues();

    /// Called with the play position after every buffer (like ClockControl).
    void updatePosition(mixxx::audio::FramePos position);

    /// The beat in the phrase at `position` (0-based), -1 without beats.
    /// Static so that it can be tested without an engine.
    static int beatInPhrase(const mixxx::BeatsPointer& pBeats,
            mixxx::audio::FramePos anchor,
            mixxx::audio::FramePos position,
            int phraseLength);

  private slots:
    /// Finds the arrangement changes once the track's waveform is complete
    /// (main thread)
    void slotDetectSections();
    void slotSetOne(double value);
    void slotJump(double value);
    void slotParametersChanged(double);

  private:
    mixxx::audio::FramePos anchor() const;
    void updateSection(mixxx::audio::FramePos position);

    std::unique_ptr<ControlObject> m_pBeatInPhrase;
    std::unique_ptr<ControlObject> m_pPhraseLength;
    std::unique_ptr<ControlObject> m_pPhraseAnchor;
    std::unique_ptr<ControlPushButton> m_pSetOne;
    std::unique_ptr<ControlObject> m_pJump;
    std::unique_ptr<ControlObject> m_pBeatNumber;
    std::unique_ptr<ControlObject> m_pBeatsPerBar;
    std::unique_ptr<ControlObject> m_pBeatUnit;
    std::unique_ptr<ControlObject> m_pSectionBeatsLeft;
    std::unique_ptr<ControlObject> m_pSectionFound;
    std::unique_ptr<ControlProxy> m_pCuePoint;
    std::unique_ptr<ControlProxy> m_pTrackSamples;
    // Section starts: intro / outro markers and hot cues
    std::vector<std::unique_ptr<ControlProxy>> m_sectionCues;
    // Section starts found in the arrangement: written by the main thread,
    // read by the engine (atomic shared_ptr)
    std::shared_ptr<const QList<mixxx::audio::FramePos>> m_pSections;
    // Main thread only
    TrackPointer m_pSectionTrack;
    QMetaObject::Connection m_waveformConnection;

    mixxx::BeatsPointer m_pBeats;
    mixxx::audio::FramePos m_position;
    // The beat interval of the last result, to count on cheaply
    mixxx::audio::FramePos m_prevBeat;
    mixxx::audio::FramePos m_nextBeat;
    int m_beat = -1;
    int m_beatNumber = 0;
};
