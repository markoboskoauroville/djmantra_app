#pragma once

#include <QList>
#include <QString>
#include <QStringList>
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

/// Detector v3 (V6): the same bars as v2 (its downbeat vote), then a small
/// logistic model learned from rekordbox 7's phrase analysis of Marko's songs
/// (testing/sections/learn; sections.py detector_v3 is the same in Python):
/// per bar the novelty (checkerboard 1 / 2 / 4 / 8 bars), the band changes
/// over 1 / 2 / 4 bars, the 4 / 8 / 16-bar position and the energy; peaks at
/// least 3 bars apart. Out: the beats (indices into `beats`) where a phrase
/// starts. Against rekordbox's phrase starts, within one bar: F1 0.50 (v2 0.31).
///
/// V8 (Marko 13.10.2026): sections are a choice in Settings, an algorithm and a
/// sensitivity (1 = only the strongest changes .. 5 = the most changes):
/// - kEnergy (default): v3's phrase starts, then only those with a big change of
///   energy (majorOnly: the weakest change goes until every one left changes a band
///   by the level's minimum; ~8 per song at level 3)
/// - kPhrases: every phrase start, as rekordbox (~20 per song at level 3)
/// - kEnergyOnly: no learned model: every 4 bars a candidate, then as kEnergy
/// - kClassic: detector v2 (the 8-bar grid; the sensitivity does not change it)
enum class SectionAlgorithm {
    kEnergy = 0,
    kPhrases = 1,
    kEnergyOnly = 2,
    kClassic = 3,
    /// V91: rbxport's phrase starts, found during the beat analysis (src/analyzer/rbxanalysis.cpp);
    /// without them (an older analysis) kEnergy. Every algorithm's sections carry the names of
    /// Marko's page (src/analyzer/sectionnames.h)
    kRbxport = 4,
};

struct SectionOptions {
    SectionAlgorithm algorithm = SectionAlgorithm::kEnergy;
    int sensitivity = 3; // 1..5
};

QString sectionAlgorithmName(SectionAlgorithm algorithm);

/// `strengths` (optional): per section found, its energy change (log; the low band x1.5)
QList<int> sectionBeatsV3(const std::vector<std::array<double, 3>>& points,
        const std::vector<double>& beats,
        int beatsPerBar = 4,
        const SectionOptions& options = SectionOptions(),
        QList<double>* strengths = nullptr);

/// The section starts of a whole track (frames), empty while the waveform
/// is not complete or without beats
QList<mixxx::audio::FramePos> detectSections(const Waveform& waveform,
        const mixxx::BeatsPointer& pBeats,
        mixxx::audio::FramePos one,
        int beatsPerBar,
        mixxx::audio::FramePos trackEnd,
        const SectionOptions& options = SectionOptions(),
        QList<double>* strengths = nullptr,
        QStringList* names = nullptr);

/// The detected section starts of each deck's track, as fractions of the
/// track (0..1), for the interface's overview (round 18: "sections need to
/// be clearly seen"). Written by PhraseControl, read by the Qt Quick UI.
void publishSections(const QString& group, const QList<double>& fractions);
QList<double> publishedSections(const QString& group);

/// V91: the sections the rbxport analysis named while it analysed the song (src/analyzer/
/// rbxanalysis.cpp: rbxport's phrase starts, named INTRO / BREAKDOWN / BUILD-UP / DROP / MAIN /
/// OUTRO as on Marko's page), kept per song in the app's data folder (sections/<md5>.json)
struct NamedSection {
    double secs = 0;
    QString name;
    int bars = 0;
};
void saveNamedSections(const QString& trackLocation, const QList<NamedSection>& sections);
QList<NamedSection> loadNamedSections(const QString& trackLocation);
bool hasNamedSections(const QString& trackLocation);
/// Each deck's section names, in the order of publishSections() (empty when unnamed)
void publishSectionNames(const QString& group, const QStringList& names);
QStringList publishedSectionNames(const QString& group);

} // namespace djmantra
