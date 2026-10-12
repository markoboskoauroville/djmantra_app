#pragma once

#include <QList>
#include <array>

#include "audio/frame.h"
#include "track/beats.h"
#include "waveform/waveform.h"

/// DJ Mantra: where a song's arrangement changes (Marko, 12.10.2026: "count
/// down to the next change": the bass comes in or goes, the vocal goes, the
/// pattern changes; no need to name the parts).
///
/// Per bar of the beat grid it takes the mean low, mid and high of the
/// track's waveform. At every phrase boundary (4 bars, counted from "1") it
/// compares the 4 bars before with the 4 bars after: a band that grows or
/// shrinks by more than about 1.65x marks a section start. Of two candidates
/// 4 bars apart only the stronger one is kept.
namespace djmantra {

struct SectionScore {
    int bar;
    double score;
};

/// Bar energies in, section start bars out (testable without a track).
/// `energies` holds {low, mid, high} per bar.
QList<int> sectionBars(const QList<std::array<double, 3>>& energies,
        int firstPhraseBar,
        double threshold = 0.5);

/// The section starts of a whole track (frames), empty while the waveform
/// is not complete or without beats
QList<mixxx::audio::FramePos> detectSections(const Waveform& waveform,
        const mixxx::BeatsPointer& pBeats,
        mixxx::audio::FramePos one,
        int beatsPerBar,
        mixxx::audio::FramePos trackEnd);

} // namespace djmantra
