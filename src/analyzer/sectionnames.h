#pragma once

#include <string>
#include <utility>
#include <vector>

/// DJ Mantra V91: song sections named as on Marko's page (djmantra.pages.dev, "Kako je pjesma
/// građena", the instrumental structure): INTRO, BREAKDOWN, BUILD-UP, DROP, MAIN, OUTRO, and
/// AMBIENT for a song that never has a beat. Used by the rbxport analysis (src/analyzer/
/// rbxanalysis.cpp) and by the app's own section finder (sectiondetector.cpp), so both name alike.
namespace djmantra::sections {

struct Bar {
    double energy = 0; ///< the bar's level, dB (any reference)
    double kick = 0;   ///< the low band at the beats (the kick), dB
    double high = 0;   ///< the highs, dB
};

/// Names the parts that start at `startBars` (bar indices, ascending; a first start at 0 is added).
/// Returns (start bar, name); a breakdown whose highs rise into the beat's return ends in a BUILD-UP
/// of up to 8 bars, so there can be more names than starts.
///
/// The rules (measured 18.10.2026 on 82 of Marko's songs against rekordbox's phrase classes):
/// - the song plays "in full" where a part's level is within 11 dB of its main level (the 75th
///   percentile of its bars); INTRO before the first such part, OUTRO after the last: as rekordbox
///   (80 % of the time against 77 % for "all full")
/// - a bar is beatless when its kick is 9 dB under the song's usual kick (Marko's test, request 51:
///   "the easiest test is when the beat stops and then comes back"); a part of 4 bars or more that
///   is beatless for half its bars is a BREAKDOWN (a shorter one is a fill)
/// - the last bars of a breakdown are a BUILD-UP when the highs rise 4 dB or more into the return
/// - the first part with the beat after a breakdown is the DROP; every other one MAIN
/// - neighbours with the same name are merged into one section
std::vector<std::pair<size_t, std::string>> name(const std::vector<Bar>& bars, std::vector<size_t> startBars);

} // namespace djmantra::sections
