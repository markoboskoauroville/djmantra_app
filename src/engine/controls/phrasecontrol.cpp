#include "engine/controls/phrasecontrol.h"

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include "moc_phrasecontrol.cpp"
#include "track/track.h"

namespace {

constexpr int kDefaultPhraseLength = 8;

int validLength(double value) {
    const int length = static_cast<int>(value);
    return (length == 4 || length == 8 || length == 16 || length == 32) ? length
                                                                        : kDefaultPhraseLength;
}

} // namespace

PhraseControl::PhraseControl(const QString& group, UserSettingsPointer pConfig)
        : EngineControl(group, pConfig),
          m_pBeatInPhrase(std::make_unique<ControlObject>(ConfigKey(group, "beat_in_phrase"))),
          m_pPhraseLength(std::make_unique<ControlObject>(ConfigKey(group, "phrase_length"))),
          m_pPhraseAnchor(std::make_unique<ControlObject>(ConfigKey(group, "phrase_anchor"))),
          m_pSetOne(std::make_unique<ControlPushButton>(ConfigKey(group, "phrase_set_one"))),
          m_pJump(std::make_unique<ControlObject>(ConfigKey(group, "phrase_jump"))),
          m_pBeatNumber(std::make_unique<ControlObject>(ConfigKey(group, "beat_number"))),
          m_pBeatsPerBar(std::make_unique<ControlObject>(ConfigKey(group, "beats_per_bar"))),
          m_pBeatUnit(std::make_unique<ControlObject>(ConfigKey(group, "beat_unit"))),
          m_pCuePoint(std::make_unique<ControlProxy>(group, "cue_point", this)) {
    m_pBeatInPhrase->setReadOnly();
    m_pBeatInPhrase->forceSet(-1);
    m_pPhraseLength->set(kDefaultPhraseLength);
    m_pBeatNumber->setReadOnly();
    // 4/4 until the time signature is detected per track
    m_pBeatsPerBar->set(4);
    m_pBeatUnit->set(4);
    m_pPhraseAnchor->set(-1);
    connect(m_pSetOne.get(), &ControlObject::valueChanged, this, &PhraseControl::slotSetOne);
    connect(m_pJump.get(), &ControlObject::valueChanged, this, &PhraseControl::slotJump);
    connect(m_pPhraseLength.get(),
            &ControlObject::valueChanged,
            this,
            &PhraseControl::slotParametersChanged);
    connect(m_pPhraseAnchor.get(),
            &ControlObject::valueChanged,
            this,
            &PhraseControl::slotParametersChanged);
    m_pCuePoint->connectValueChanged(this, &PhraseControl::slotParametersChanged);
}

PhraseControl::~PhraseControl() = default;

void PhraseControl::trackLoaded(TrackPointer pNewTrack) {
    // A new track starts with an automatic anchor
    m_pPhraseAnchor->set(-1);
    m_position = mixxx::audio::kInvalidFramePos;
    trackBeatsUpdated(pNewTrack ? pNewTrack->getBeats() : mixxx::BeatsPointer());
}

void PhraseControl::trackBeatsUpdated(mixxx::BeatsPointer pBeats) {
    m_pBeats = pBeats;
    m_prevBeat = mixxx::audio::kInvalidFramePos;
    m_nextBeat = mixxx::audio::kInvalidFramePos;
    m_beat = -1;
    m_pBeatInPhrase->forceSet(-1);
}

void PhraseControl::slotParametersChanged(double) {
    // Count again from the anchor at the next update
    m_prevBeat = mixxx::audio::kInvalidFramePos;
    m_nextBeat = mixxx::audio::kInvalidFramePos;
    if (m_position.isValid()) {
        updatePosition(m_position);
    }
}

mixxx::audio::FramePos PhraseControl::anchor() const {
    const auto userAnchor =
            mixxx::audio::FramePos::fromEngineSamplePosMaybeInvalid(m_pPhraseAnchor->get());
    if (userAnchor.isValid() && userAnchor >= mixxx::audio::kStartFramePos) {
        return userAnchor;
    }
    const auto cue = mixxx::audio::FramePos::fromEngineSamplePosMaybeInvalid(m_pCuePoint->get());
    if (cue.isValid() && cue >= mixxx::audio::kStartFramePos) {
        return cue;
    }
    const mixxx::BeatsPointer pBeats = m_pBeats;
    return pBeats ? pBeats->firstBeat() : mixxx::audio::kInvalidFramePos;
}

// static
bool PhraseControl::beatNumber(const mixxx::BeatsPointer& pBeats,
        mixxx::audio::FramePos anchor,
        mixxx::audio::FramePos position,
        int* pNumber) {
    if (!pBeats || !anchor.isValid() || !position.isValid()) {
        return false;
    }
    // "1" is the beat closest to the anchor, so that a cue a few ms off the
    // grid still counts as its beat
    const auto one = pBeats->findClosestBeat(anchor);
    if (!one.isValid()) {
        return false;
    }
    mixxx::audio::FramePos prev;
    mixxx::audio::FramePos next;
    if (!pBeats->findPrevNextBeats(position, &prev, &next, true) || !prev.isValid()) {
        return false;
    }
    if (prev == one) {
        // Beats::numBeatsInRange() returns -1 for an empty range at frame 0
        *pNumber = 0;
    } else if (prev > one) {
        // Beats in [one, prev] minus one: prev is beat n (0-based)
        *pNumber = pBeats->numBeatsInRange(one, prev);
    } else {
        *pNumber = -pBeats->numBeatsInRange(prev, one);
    }
    return true;
}

// static
int PhraseControl::beatInPhrase(const mixxx::BeatsPointer& pBeats,
        mixxx::audio::FramePos anchor,
        mixxx::audio::FramePos position,
        int phraseLength) {
    int n;
    if (phraseLength <= 0 || !beatNumber(pBeats, anchor, position, &n)) {
        return -1;
    }
    return ((n % phraseLength) + phraseLength) % phraseLength;
}

void PhraseControl::updatePosition(mixxx::audio::FramePos position) {
    m_position = position;
    const mixxx::BeatsPointer pBeats = m_pBeats;
    if (!pBeats || !position.isValid()) {
        if (m_beat != -1) {
            m_beat = -1;
            m_pBeatInPhrase->forceSet(-1);
        }
        return;
    }
    const int length = validLength(m_pPhraseLength->get());
    int beat;
    if (m_beat >= 0 && m_prevBeat.isValid() && m_nextBeat.isValid() &&
            position >= m_prevBeat && position < m_nextBeat) {
        return; // same beat
    } else if (m_beat >= 0 && m_nextBeat.isValid() && position >= m_nextBeat &&
            position < pBeats->findNextBeat(m_nextBeat + 1)) {
        beat = (m_beat + 1) % length; // the next beat while playing on
        ++m_beatNumber;
        m_prevBeat = m_nextBeat;
        m_nextBeat = pBeats->findNextBeat(m_nextBeat + 1);
    } else {
        int number;
        if (beatNumber(pBeats, anchor(), position, &number)) {
            m_beatNumber = number;
            beat = ((number % length) + length) % length;
        } else {
            beat = -1;
        }
        pBeats->findPrevNextBeats(position, &m_prevBeat, &m_nextBeat, true);
    }
    // The number first: a beat_in_phrase listener reads it
    if (m_pBeatNumber->get() != m_beatNumber) {
        m_pBeatNumber->forceSet(m_beatNumber);
    }
    if (beat != m_beat) {
        m_beat = beat;
        m_pBeatInPhrase->forceSet(beat);
    }
}

void PhraseControl::slotSetOne(double value) {
    if (value <= 0) {
        return;
    }
    const mixxx::BeatsPointer pBeats = m_pBeats;
    if (!pBeats || !m_position.isValid()) {
        return;
    }
    const auto beat = pBeats->findClosestBeat(m_position);
    if (beat.isValid()) {
        // Own writes do not notify our own slot: count again here
        m_pPhraseAnchor->set(beat.toEngineSamplePos());
        slotParametersChanged(0);
    }
}

void PhraseControl::slotJump(double value) {
    const int target = static_cast<int>(value) - 1;
    const mixxx::BeatsPointer pBeats = m_pBeats;
    if (target < 0 || !pBeats || !m_position.isValid() || m_beat < 0) {
        return;
    }
    const int length = validLength(m_pPhraseLength->get());
    const int beats = (target % length) - m_beat;
    if (beats == 0) {
        return;
    }
    // Same offset within the beat, `beats` beats away
    const auto prev = pBeats->findPrevBeat(m_position);
    const auto destinationBeat = pBeats->findNthBeat(prev, beats > 0 ? beats + 1 : beats - 1);
    if (!prev.isValid() || !destinationBeat.isValid()) {
        return;
    }
    const auto destination = destinationBeat + (m_position - prev);
    seekExact(destination);
}
