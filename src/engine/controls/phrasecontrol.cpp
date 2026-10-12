#include "engine/controls/phrasecontrol.h"

#include <QStringList>
#include <QTimer>

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include <algorithm>
#include "moc_phrasecontrol.cpp"
#include "engine/controls/sectiondetector.h"
#include "track/track.h"

namespace {

constexpr int kDefaultPhraseLength = 8;
// Hot cues that count as section starts (the pads show 8)
constexpr int kSectionHotcues = 16;

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
          m_pSectionBeatsLeft(
                  std::make_unique<ControlObject>(ConfigKey(group, "section_beats_left"))),
          m_pSectionFound(std::make_unique<ControlObject>(ConfigKey(group, "section_found"))),
          m_pSectionJump(std::make_unique<ControlPushButton>(ConfigKey(group, "section_jump"))),
          m_pSectionCount(std::make_unique<ControlObject>(ConfigKey(group, "section_count"))) {
    m_pSectionCount->setReadOnly();
    m_pSectionCount->forceSet(-1);
    m_pBeatInPhrase->setReadOnly();
    m_pBeatInPhrase->forceSet(-1);
    m_pPhraseLength->set(kDefaultPhraseLength);
    m_pBeatNumber->setReadOnly();
    // 4/4 until the time signature is detected per track
    m_pBeatsPerBar->set(4);
    m_pBeatUnit->set(4);
    m_pSectionBeatsLeft->setReadOnly();
    m_pSectionBeatsLeft->forceSet(-1);
    m_pSectionFound->setReadOnly();
    m_pPhraseAnchor->set(-1);
    connect(m_pSetOne.get(), &ControlObject::valueChanged, this, &PhraseControl::slotSetOne);
    connect(m_pJump.get(), &ControlObject::valueChanged, this, &PhraseControl::slotJump);
    connect(m_pSectionJump.get(),
            &ControlObject::valueChanged,
            this,
            &PhraseControl::slotSectionJump);
    connect(m_pPhraseLength.get(),
            &ControlObject::valueChanged,
            this,
            &PhraseControl::slotParametersChanged);
    connect(m_pPhraseAnchor.get(),
            &ControlObject::valueChanged,
            this,
            &PhraseControl::slotParametersChanged);
}

PhraseControl::~PhraseControl() = default;

void PhraseControl::connectCues() {
    if (m_pCuePoint) {
        return;
    }
    const QString group = getGroup();
    m_pCuePoint = std::make_unique<ControlProxy>(
            group, "cue_point", this, ControlFlag::AllowMissingOrInvalid);
    m_pCuePoint->connectValueChanged(this, &PhraseControl::slotParametersChanged);
    m_pTrackSamples = std::make_unique<ControlProxy>(
            group, "track_samples", this, ControlFlag::AllowMissingOrInvalid);
    m_pPlay = std::make_unique<ControlProxy>(
            group, "play", this, ControlFlag::AllowMissingOrInvalid);
    // Not intro_start / outro_end: the analysis puts them at the first and
    // the last sound, so JUMP went to the end of the song (round 18)
    QStringList names = {QStringLiteral("intro_end_position"),
            QStringLiteral("outro_start_position")};
    for (int i = 1; i <= kSectionHotcues; ++i) {
        names << QStringLiteral("hotcue_%1_position").arg(i);
    }
    for (const auto& name : names) {
        m_sectionCues.push_back(std::make_unique<ControlProxy>(
                group, name, this, ControlFlag::AllowMissingOrInvalid));
    }
}

void PhraseControl::trackLoaded(TrackPointer pNewTrack) {
    std::atomic_store(&m_pSections, std::shared_ptr<const QList<mixxx::audio::FramePos>>());
    djmantra::publishSections(getGroup(), {});
    m_pSectionCount->forceSet(-1);
    // The detection reads the waveform on the main thread
    QMetaObject::invokeMethod(
            this,
            [this, pNewTrack] {
                disconnect(m_waveformConnection);
                m_pSectionTrack = pNewTrack;
                m_detectTries = 0;
                if (pNewTrack) {
                    m_waveformConnection = connect(pNewTrack.get(),
                            &Track::waveformUpdated,
                            this,
                            &PhraseControl::slotDetectSections);
                }
                slotDetectSections();
            },
            Qt::QueuedConnection);
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

void PhraseControl::slotDetectSections() {
    const TrackPointer pTrack = m_pSectionTrack;
    if (!pTrack) {
        return;
    }
    const ConstWaveformPointer pWaveform = pTrack->getWaveform();
    const mixxx::BeatsPointer pBeats = pTrack->getBeats();
    // Round 18: 2 of 4 songs never got sections: the waveform was done
    // before the beats (or the track length) and nothing asked again. Until
    // the detection ran, it asks again every 2 s (for 3 minutes).
    const auto retry = [this, pTrack] {
        if (m_detectTries++ < 90) {
            QTimer::singleShot(2000, this, [this, pTrack] {
                if (m_pSectionTrack == pTrack) {
                    slotDetectSections();
                }
            });
        }
    };
    if (!pWaveform || !pBeats || !m_pTrackSamples || m_pTrackSamples->get() <= 0) {
        retry();
        return;
    }
    const auto end = mixxx::audio::FramePos::fromEngineSamplePos(m_pTrackSamples->get());
    const auto sections = djmantra::detectSections(*pWaveform,
            pBeats,
            anchor(),
            static_cast<int>(m_pBeatsPerBar->get()),
            end);
    if (sections.isEmpty() && pWaveform->getCompletion() < pWaveform->getDataSize()) {
        retry(); // not analysed yet
        return;
    }
    m_detectTries = 1000; // done for this track
    std::atomic_store(&m_pSections,
            std::make_shared<const QList<mixxx::audio::FramePos>>(sections));
    QList<double> fractions;
    for (const auto& position : sections) {
        fractions.append(end.value() > 0 ? position.value() / end.value() : 0.0);
    }
    djmantra::publishSections(getGroup(), fractions);
    m_pSectionCount->forceSet(sections.size());
    QStringList at;
    for (const auto& position : sections) {
        at << QString::number(position.value() / pTrack->getSampleRate(), 'f', 1);
    }
    qInfo().noquote() << "Sections:" << getGroup() << sections.size() << "changes, at"
                      << at.join(QStringLiteral(" s, ")) << "s";
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
    const auto cue = m_pCuePoint
            ? mixxx::audio::FramePos::fromEngineSamplePosMaybeInvalid(m_pCuePoint->get())
            : mixxx::audio::kInvalidFramePos;
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

// static
mixxx::audio::FramePos PhraseControl::nextSection(const mixxx::BeatsPointer& pBeats,
        mixxx::audio::FramePos position,
        const QList<mixxx::audio::FramePos>& boundaries) {
    if (!position.isValid()) {
        return mixxx::audio::kInvalidFramePos;
    }
    // A section that starts on the current beat (or just behind it) is not the next one
    double halfBeat = 0;
    if (pBeats) {
        mixxx::audio::FramePos prev;
        mixxx::audio::FramePos next;
        if (pBeats->findPrevNextBeats(position, &prev, &next, true) && prev.isValid() &&
                next.isValid()) {
            halfBeat = (next - prev) / 2;
        }
    }
    mixxx::audio::FramePos best = mixxx::audio::kInvalidFramePos;
    for (const auto& boundary : boundaries) {
        if (boundary.isValid() && boundary > position + halfBeat &&
                (!best.isValid() || boundary < best)) {
            best = boundary;
        }
    }
    return best;
}

void PhraseControl::updateSection(mixxx::audio::FramePos position) {
    const mixxx::BeatsPointer pBeats = m_pBeats;
    if (!pBeats || m_beat < 0) {
        m_pSectionBeatsLeft->forceSet(-1);
        m_pSectionFound->forceSet(0);
        m_nextSection = -1.0;
        return;
    }
    QList<mixxx::audio::FramePos> boundaries;
    if (const auto pSections = std::atomic_load(&m_pSections)) {
        boundaries = *pSections;
    }
    for (const auto& pCue : m_sectionCues) {
        const double samplePos = pCue->get();
        if (samplePos >= 0) {
            boundaries.append(mixxx::audio::FramePos::fromEngineSamplePos(samplePos));
        }
    }
    auto target = nextSection(pBeats, position, boundaries);
    const bool found = target.isValid();
    if (!found && m_pTrackSamples && m_pTrackSamples->get() > 0) {
        // The last beat of the track
        target = pBeats->findPrevBeat(
                mixxx::audio::FramePos::fromEngineSamplePos(m_pTrackSamples->get()));
    }
    int left = -1;
    int targetNumber;
    if (target.isValid() &&
            beatNumber(pBeats, anchor(), pBeats->findClosestBeat(target), &targetNumber)) {
        left = std::max(0, targetNumber - m_beatNumber);
    }
    if (m_pSectionBeatsLeft->get() != left) {
        m_pSectionBeatsLeft->forceSet(left);
    }
    if (m_pSectionFound->get() != (found ? 1 : 0)) {
        m_pSectionFound->forceSet(found ? 1 : 0);
    }
    m_nextSection = found ? target.toEngineSamplePos() : -1.0;
}

void PhraseControl::updatePosition(mixxx::audio::FramePos position) {
    m_position = position;
    const mixxx::BeatsPointer pBeats = m_pBeats;
    if (!pBeats || !position.isValid()) {
        if (m_beat != -1) {
            m_beat = -1;
            m_pBeatInPhrase->forceSet(-1);
            updateSection(position);
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
        // A queued jump happens now, on the beat (Marko, round 19: "as Ableton
        // Live with snap 1 beat ... seamless, the listeners cannot notice"):
        // the beat that just started is replaced by the chosen one, at the
        // same place within it, so the music goes on in time
        const int target = m_pendingJump.exchange(-1);
        if (target >= 0) {
            const int beats = (target % length) - beat;
            if (beats != 0) {
                const auto destinationBeat =
                        pBeats->findNthBeat(m_prevBeat, beats > 0 ? beats + 1 : beats - 1);
                if (destinationBeat.isValid()) {
                    seekExact(destinationBeat + (position - m_prevBeat));
                    m_beatsJumped.fetch_add(1);
                }
            }
        }
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
    // Once per beat (the same beat returned early above)
    updateSection(position);
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
    // Stopped: exactly on the beat (the deck then starts from it in time,
    // Marko 12.10.2026: "select beat 1, it starts to play from beat 1 in
    // sync"); playing: the same place within the beat, `beats` beats away
    const bool playing = m_pPlay && m_pPlay->get() > 0;
    const int length = validLength(m_pPhraseLength->get());
    const int beats = (target % length) - m_beat;
    const auto prev = pBeats->findPrevBeat(m_position);
    if (!prev.isValid()) {
        return;
    }
    if (playing) {
        // Quantized to the beat: the jump waits for the next beat and
        // happens on it (updatePosition), so it is seamless
        m_pendingJump.store(target % length);
        return;
    }
    const auto destinationBeat = beats == 0
            ? prev
            : pBeats->findNthBeat(prev, beats > 0 ? beats + 1 : beats - 1);
    if (!destinationBeat.isValid()) {
        return;
    }
    seekExact(destinationBeat);
}

void PhraseControl::slotSectionJump(double value) {
    if (value <= 0) {
        return;
    }
    const mixxx::BeatsPointer pBeats = m_pBeats;
    const double next = m_nextSection;
    if (!pBeats || !m_position.isValid() || next < 0) {
        qInfo() << getGroup() << "jump: no section change ahead";
        return;
    }
    const auto section = pBeats->findClosestBeat(mixxx::audio::FramePos::fromEngineSamplePos(next));
    const auto prev = pBeats->findPrevBeat(m_position);
    if (!section.isValid()) {
        return;
    }
    const bool playing = m_pPlay && m_pPlay->get() > 0;
    // Playing: the same place within the beat, so the mix stays in time
    const auto destination = playing && prev.isValid() ? section + (m_position - prev) : section;
    // In seconds, with the cues that also count as section changes (round
    // 16b: after a jump the countdown named a change the detection had not
    // found; a cue is the likely one)
    const double sampleRate = frameInfo().sampleRate.isValid()
            ? static_cast<double>(frameInfo().sampleRate.value())
            : 44100.0;
    QStringList cues;
    for (const auto& pCue : m_sectionCues) {
        const double samplePos = pCue->get();
        if (samplePos >= 0) {
            cues << QStringLiteral("%1 %2 s")
                            .arg(pCue->getKey().item)
                            .arg(mixxx::audio::FramePos::fromEngineSamplePos(samplePos)
                                            .value() /
                                            sampleRate,
                                    0,
                                    'f',
                                    1);
        }
    }
    qInfo() << getGroup() << "jump to the next section at"
            << QString::number(section.value() / sampleRate, 'f', 1) << "s; cues:"
            << (cues.isEmpty() ? QStringLiteral("none") : cues.join(QStringLiteral(", ")));
    seekExact(destination);
}
