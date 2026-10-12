#include "engine/controls/sectiondetector.h"

#include <QHash>
#include <QMutex>
#include <algorithm>
#include <array>
#include <cmath>

#include "engine/controls/phrasecontrol.h"

namespace djmantra {

namespace {
constexpr int kPhraseBars = 4;
// Keeps quiet bands from looking like huge changes
constexpr double kFloor = 4.0;
} // namespace

QList<int> sectionBars(const QList<std::array<double, 3>>& energies,
        int firstPhraseBar,
        double threshold) {
    const int bars = static_cast<int>(energies.size());
    QList<SectionScore> candidates;
    const auto mean = [&](int from, int to, int band) {
        double sum = 0;
        for (int b = from; b < to; ++b) {
            sum += energies[b][band];
        }
        return sum / std::max(1, to - from);
    };
    int first = firstPhraseBar % kPhraseBars;
    if (first < 0) {
        first += kPhraseBars;
    }
    for (int bar = first; bar < bars; bar += kPhraseBars) {
        if (bar < kPhraseBars || bar + kPhraseBars > bars) {
            continue;
        }
        double score = 0;
        for (int band = 0; band < 3; ++band) {
            const double before = mean(bar - kPhraseBars, bar, band);
            const double after = mean(bar, bar + kPhraseBars, band);
            score = std::max(score, std::fabs(std::log((after + kFloor) / (before + kFloor))));
        }
        if (score > threshold) {
            candidates.append({bar, score});
        }
    }
    // Of neighbours one phrase apart, the stronger change
    QList<int> result;
    for (int i = 0; i < candidates.size(); ++i) {
        const auto& c = candidates[i];
        const bool weakerThanPrevious = i > 0 &&
                candidates[i - 1].bar == c.bar - kPhraseBars &&
                candidates[i - 1].score > c.score;
        const bool weakerThanNext = i + 1 < candidates.size() &&
                candidates[i + 1].bar == c.bar + kPhraseBars &&
                candidates[i + 1].score >= c.score;
        if (!weakerThanPrevious && !weakerThanNext) {
            result.append(c.bar);
        }
    }
    return result;
}

QList<mixxx::audio::FramePos> detectSections(const Waveform& waveform,
        const mixxx::BeatsPointer& pBeats,
        mixxx::audio::FramePos one,
        int beatsPerBar,
        mixxx::audio::FramePos trackEnd) {
    QList<mixxx::audio::FramePos> result;
    const int size = waveform.getDataSize();
    const double ratio = waveform.getAudioVisualRatio();
    if (!pBeats || !one.isValid() || !trackEnd.isValid() || size <= 0 || ratio <= 0 ||
            waveform.getCompletion() < size) {
        return result;
    }
    if (beatsPerBar < 1) {
        beatsPerBar = 4;
    }
    // The bar starts: every beat counted from "1" that begins a bar
    QList<mixxx::audio::FramePos> barStarts;
    int firstBarNumber = 0;
    auto beat = pBeats->firstBeat();
    int guard = 0;
    while (beat.isValid() && beat < trackEnd && guard++ < 100000) {
        int number;
        if (PhraseControl::beatNumber(pBeats, one, beat, &number) &&
                ((number % beatsPerBar) + beatsPerBar) % beatsPerBar == 0) {
            if (barStarts.isEmpty()) {
                firstBarNumber = static_cast<int>(
                        std::floor(static_cast<double>(number) / beatsPerBar));
            }
            barStarts.append(beat);
        }
        const auto next = pBeats->findNextBeat(beat + 1);
        if (!next.isValid() || next <= beat) {
            break;
        }
        beat = next;
    }
    if (barStarts.size() < 3 * kPhraseBars) {
        return result;
    }
    barStarts.append(trackEnd);
    // Mean low / mid / high per bar (both channels: even and odd points)
    QList<std::array<double, 3>> energies;
    for (int b = 0; b + 1 < barStarts.size(); ++b) {
        const int from = std::clamp(
                2 * static_cast<int>(barStarts[b].value() / ratio), 0, size - 1);
        const int to = std::clamp(
                2 * static_cast<int>(barStarts[b + 1].value() / ratio), from + 1, size);
        std::array<double, 3> sum{0, 0, 0};
        for (int i = from; i < to; ++i) {
            sum[0] += waveform.getLow(i);
            sum[1] += waveform.getMid(i);
            sum[2] += waveform.getHigh(i);
        }
        const double count = std::max(1, to - from);
        energies.append({sum[0] / count, sum[1] / count, sum[2] / count});
    }
    // Bar numbers: phrase boundaries are multiples of 4 counted from "1"
    // energies[i] is bar firstBarNumber + i; a phrase starts where that is a multiple of 4
    const QList<int> bars = sectionBars(energies, -firstBarNumber);
    for (int bar : bars) {
        result.append(barStarts[bar]);
    }
    return result;
}

namespace {

QMutex s_sectionsMutex;
QHash<QString, QList<double>> s_sections;

} // namespace

void publishSections(const QString& group, const QList<double>& fractions) {
    const QMutexLocker locker(&s_sectionsMutex);
    s_sections.insert(group, fractions);
}

QList<double> publishedSections(const QString& group) {
    const QMutexLocker locker(&s_sectionsMutex);
    return s_sections.value(group);
}

} // namespace djmantra
