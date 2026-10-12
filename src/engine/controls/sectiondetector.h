#pragma once

#include <QList>
#include <QString>
#include <array>
#include <vector>

#include "audio/frame.h"
#include "track/beats.h"
#include "waveform/waveform.h"

/// DJ Mantra: where a song's arrangement changes (Marko, 12.10.2026: the
/// matrix's bar.beat restarts at every section; "Rekordbox phrase quality").
/// detectSections() runs detector v2 (sectionBeatsV2, below) on the track's
/// waveform; sectionBars() is the first detector, kept for its tests.
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

/// Detector v2 (12.10.2026, scored against a reference on Marko's songs in
/// testing/sections/: sections.py detector_v2 is the same steps in Python).
/// In: the waveform's {low, mid, high} per point (mono) and the beats in
/// point units. Out: the beats (indices into `beats`) where a major section
/// starts. Steps: the downbeat by vote (the beat phase whose bar-to-bar band
/// change is largest); per bar the bands, their four quarters (the rhythm)
/// and the kick (the low band's peak on the beats; on / off with hysteresis,
/// a 1-bar state is a fill); the phrase "1" by vote (the 8-bar offset where
/// the bands change most); a bar self-similarity novelty
/// (checkerboard, 4 and 8 bars); score = novelty + kick change + band change,
/// weighted to the 8-bar grid; peaks 8 bars apart; a kick change on a 4 / 8
/// bar line is always a section.
QList<int> sectionBeatsV2(const std::vector<std::array<double, 3>>& points,
        const std::vector<double>& beats,
        int beatsPerBar = 4);

/// The section starts of a whole track (frames), empty while the waveform
/// is not complete or without beats
QList<mixxx::audio::FramePos> detectSections(const Waveform& waveform,
        const mixxx::BeatsPointer& pBeats,
        mixxx::audio::FramePos one,
        int beatsPerBar,
        mixxx::audio::FramePos trackEnd);

/// The detected section starts of each deck's track, as fractions of the
/// track (0..1), for the interface's overview (round 18: "sections need to
/// be clearly seen"). Written by PhraseControl, read by the Qt Quick UI.
void publishSections(const QString& group, const QList<double>& fractions);
QList<double> publishedSections(const QString& group);

} // namespace djmantra
