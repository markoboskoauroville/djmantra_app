#include "analyzer/sectionnames.h"

#include <algorithm>

namespace djmantra::sections {

namespace {

double percentile(std::vector<double> v, double p) {
    if (v.empty()) {
        return 0;
    }
    std::sort(v.begin(), v.end());
    return v[std::min(v.size() - 1, static_cast<size_t>(p / 100.0 * (v.size() - 1) + 0.5))];
}

constexpr double kFullWithinDb = 11.0;
constexpr double kBeatlessDb = 9.0;
constexpr double kBuildUpRiseDb = 4.3;
constexpr size_t kFillBars = 4;

} // namespace

std::vector<std::pair<size_t, std::string>> name(const std::vector<Bar>& bars, std::vector<size_t> startBars) {
    std::vector<std::pair<size_t, std::string>> out;
    const size_t n = bars.size();
    if (n == 0) {
        return out;
    }
    startBars.erase(std::remove_if(startBars.begin(), startBars.end(), [n](size_t b) { return b >= n; }),
            startBars.end());
    startBars.push_back(0);
    std::sort(startBars.begin(), startBars.end());
    startBars.erase(std::unique(startBars.begin(), startBars.end()), startBars.end());

    std::vector<double> energy, kick;
    for (const auto& bar : bars) {
        energy.push_back(bar.energy);
        kick.push_back(bar.kick);
    }
    const double usualKick = percentile(kick, 80);
    std::vector<bool> beatless(n);
    size_t withBeat = 0;
    for (size_t b = 0; b < n; ++b) {
        beatless[b] = bars[b].kick < usualKick - kBeatlessDb;
        withBeat += beatless[b] ? 0 : 1;
    }
    if (withBeat < 8) {
        out.emplace_back(0, "AMBIENT");
        return out;
    }
    const double mainLevel = percentile(energy, 75);
    struct Part {
        size_t from, to;
        double beatless, energy;
    };
    std::vector<Part> parts;
    for (size_t i = 0; i < startBars.size(); ++i) {
        const size_t from = startBars[i];
        const size_t to = i + 1 < startBars.size() ? startBars[i + 1] : n;
        Part part{from, to, 0, 0};
        for (size_t b = from; b < to; ++b) {
            part.beatless += beatless[b] ? 1 : 0;
            part.energy += bars[b].energy;
        }
        part.beatless /= static_cast<double>(to - from);
        part.energy /= static_cast<double>(to - from);
        parts.push_back(part);
    }
    std::vector<bool> full(parts.size());
    int first = -1, last = -1;
    for (size_t i = 0; i < parts.size(); ++i) {
        full[i] = parts[i].energy >= mainLevel - kFullWithinDb && parts[i].beatless < 0.5;
        if (full[i]) {
            last = static_cast<int>(i);
            if (first < 0) {
                first = static_cast<int>(i);
            }
        }
    }
    std::string previous;
    for (size_t i = 0; i < parts.size(); ++i) {
        const Part& part = parts[i];
        const int index = static_cast<int>(i);
        const size_t length = part.to - part.from;
        std::string label;
        if (first < 0 || index < first) {
            label = "INTRO";
        } else if (index > last) {
            label = "OUTRO";
        } else if (part.beatless >= 0.5 && length >= kFillBars) {
            const size_t tail = std::min<size_t>(8, length);
            const bool rising = tail >= 2 && bars[part.to - 1].high - bars[part.to - tail].high >= kBuildUpRiseDb;
            if (rising && length <= 8) {
                label = "BUILD-UP";
            } else if (rising) {
                out.emplace_back(part.from, "BREAKDOWN");
                out.emplace_back(part.to - 8, "BUILD-UP");
                previous = "BUILD-UP";
                continue;
            } else {
                label = "BREAKDOWN";
            }
        } else {
            label = previous == "BREAKDOWN" || previous == "BUILD-UP" ? "DROP" : "MAIN";
        }
        out.emplace_back(part.from, label);
        previous = label;
    }
    // Neighbours with the same name are one section (16 bars of MAIN, not four of 4)
    std::vector<std::pair<size_t, std::string>> merged;
    for (const auto& part : out) {
        if (merged.empty() || merged.back().second != part.second) {
            merged.push_back(part);
        }
    }
    return merged;
}

} // namespace djmantra::sections
