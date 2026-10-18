#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

/// DJ Mantra V91 (Marko 18.10.2026: "Get its beat grid and also get its detection of the song
/// section ... section names are there to be copied"): the beat grid and the phrase starts of
/// rbxport (https://github.com/chrisle/rbxport, crates/rbl-analysis, Chris Le, GPL-2.0-or-later),
/// ported to C++, and the sections named as Marko's page names them (djmantra.pages.dev, "Kako je
/// pjesma građena": INTRO, BREAKDOWN, BUILD-UP, DROP, MAIN, OUTRO).
///
/// What is ported (rbxport's "Rekordbox" preset, one constant tempo for the whole track):
/// - onset.rs: the spectral-flux onset envelope (1024 / 256, Hann, rises only, local mean removed)
/// - tempo.rs: the candidates (autocorrelation x sqrt(20 s Fourier) x a prior at 132 BPM), the fit
///   (comb period and phase, then every beat moved to its kick attack and a weighted line fitted
///   three times from both halves of the beat, the outer fifth left out), the whole-number BPM
/// - attack.rs: the kick's attack, the start of an RMS spike in 900-9000 Hz in 1 ms steps, +-15 ms
/// - downbeat.rs: 12 log bands per half beat, novelty at 1 / 2 / 4 / 8 bars, the 8 positions of the
///   bar scored by the novelty peaks (moves the grid half a beat when it was on the off-beat), the
///   phrase starts (4-bar novelty peaks on the downbeat, at least 30 % of the strongest)
/// Not ported: the tempo-change segments (rbxport's own preset) and the key.
/// rbxport names no sections (phrase.rs is a stub); the names here are DJ Mantra's own rules.
namespace djmantra::rbx {

struct Section {
    double secs = 0;  ///< where it starts, seconds from the start of the file
    std::string name; ///< INTRO, BREAKDOWN, BUILD-UP, DROP, MAIN or OUTRO
    int bars = 0;
};

struct Result {
    double bpm = 0;            ///< 0: no tempo found
    double confidence = 0;     ///< 0..1
    std::vector<double> beats; ///< every beat, seconds, from the first at or after 0
    int firstDownbeat = 0;     ///< the index in `beats` of the first beat that is "1"
    bool halfBeatMoved = false;
    std::vector<double> phraseStarts; ///< rbxport's phrase starts, seconds
    /// every 4-bar novelty peak on the downbeat: (seconds, its strength over the strongest), for tuning
    std::vector<std::pair<double, double>> phraseCandidates;
    std::vector<Section> sections;
    /// per bar from the first downbeat: its start (s), low band, kick pulse, energy, highs (ln power)
    std::vector<std::array<double, 5>> bars;
};

/// The whole analysis of a mono track (any sample rate, samples -1..1).
Result analyse(const float* samples, size_t count, int sampleRate);

} // namespace djmantra::rbx
