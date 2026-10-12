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

namespace {

using Bands = std::array<double, 3>;

// Mean of points [from, to) (indices clamped; at least one point)
Bands spanMean(const std::vector<Bands>& points, double from, double to) {
    const int size = static_cast<int>(points.size());
    const int a = std::clamp(static_cast<int>(std::round(from)), 0, size - 1);
    const int b = std::clamp(static_cast<int>(std::round(to)), a + 1, size);
    Bands sum{0, 0, 0};
    for (int i = a; i < b; ++i) {
        for (int c = 0; c < 3; ++c) {
            sum[c] += points[i][c];
        }
    }
    for (int c = 0; c < 3; ++c) {
        sum[c] /= (b - a);
    }
    return sum;
}

double medianOf(std::vector<double> v) {
    if (v.empty()) {
        return 0;
    }
    std::sort(v.begin(), v.end());
    const size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

// numpy.percentile (linear)
double percentileOf(std::vector<double> v, double p) {
    if (v.empty()) {
        return 0;
    }
    std::sort(v.begin(), v.end());
    const double pos = (v.size() - 1) * p / 100.0;
    const size_t lo = static_cast<size_t>(std::floor(pos));
    const size_t hi = std::min(v.size() - 1, lo + 1);
    return v[lo] + (v[hi] - v[lo]) * (pos - lo);
}

// Foote's checkerboard kernel (Gaussian tapered) along the diagonal, >= 0,
// normalised to a peak of 1
std::vector<double> checkerNovelty(const std::vector<std::vector<double>>& ssm, int half) {
    const int n = static_cast<int>(ssm.size());
    std::vector<double> g(2 * half);
    for (int i = 0; i < 2 * half; ++i) {
        const double x = -1.5 + 3.0 * i / (2 * half - 1);
        g[i] = std::exp(-0.5 * x * x);
    }
    std::vector<double> nov(n, 0);
    double peak = 0;
    for (int i = 0; i < n; ++i) {
        double sum = 0;
        for (int r = 0; r < 2 * half; ++r) {
            const int row = std::clamp(i - half + r, 0, n - 1);
            for (int k = 0; k < 2 * half; ++k) {
                const int col = std::clamp(i - half + k, 0, n - 1);
                const double sign = (r < half) == (k < half) ? 1.0 : -1.0;
                sum += g[r] * g[k] * sign * ssm[row][col];
            }
        }
        nov[i] = std::max(0.0, sum);
        peak = std::max(peak, nov[i]);
    }
    for (auto& v : nov) {
        v /= peak + 1e-9;
    }
    return nov;
}

} // namespace

QList<int> sectionBeatsV2(const std::vector<std::array<double, 3>>& points,
        const std::vector<double>& beats,
        int beatsPerBar) {
    QList<int> result;
    if (beatsPerBar < 1) {
        beatsPerBar = 4;
    }
    if (points.empty() || beats.size() < static_cast<size_t>(16 * beatsPerBar)) {
        return result;
    }
    const auto logBands = [](const Bands& b) {
        return Bands{std::log1p(b[0]), std::log1p(b[1]), std::log1p(b[2])};
    };
    // 1. The downbeat: the beat phase with the largest bar-to-bar band change
    int downbeat = 0;
    double bestChange = -1;
    for (int p = 0; p < beatsPerBar; ++p) {
        double change = 0;
        Bands previous{};
        bool first = true;
        for (size_t i = p; i + beatsPerBar < beats.size(); i += beatsPerBar) {
            const Bands now = logBands(spanMean(points, beats[i], beats[i + beatsPerBar]));
            if (!first) {
                for (int c = 0; c < 3; ++c) {
                    change += std::fabs(now[c] - previous[c]);
                }
            }
            previous = now;
            first = false;
        }
        if (change > bestChange) {
            bestChange = change;
            downbeat = p;
        }
    }
    std::vector<int> barBeat; // the beat index each bar starts on
    for (size_t i = downbeat; i + beatsPerBar < beats.size(); i += beatsPerBar) {
        barBeat.push_back(static_cast<int>(i));
    }
    const int nb = static_cast<int>(barBeat.size());
    if (nb < 16) {
        return result;
    }
    // 2. Per bar: the bands (0..230 per band's peak), their quarters, the kick
    std::vector<Bands> b3(nb);
    std::vector<std::array<double, 12>> quarters(nb);
    std::vector<double> pulse(nb);
    Bands peak{1e-9, 1e-9, 1e-9};
    for (int k = 0; k < nb; ++k) {
        const double from = beats[barBeat[k]];
        const double to = beats[barBeat[k] + beatsPerBar];
        b3[k] = spanMean(points, from, to);
        double pulseSum = 0;
        for (int q = 0; q < 4; ++q) {
            const double qa = from + (to - from) * q / 4;
            const double qb = from + (to - from) * (q + 1) / 4;
            const Bands m = logBands(spanMean(points, qa, qb));
            for (int c = 0; c < 3; ++c) {
                quarters[k][q * 3 + c] = m[c];
            }
            // the low band's peak at the head of this quarter over its median
            const int a = std::clamp(static_cast<int>(qa), 0, static_cast<int>(points.size()) - 1);
            const int b = std::clamp(static_cast<int>(qb), a + 1, static_cast<int>(points.size()));
            std::vector<double> seg;
            for (int i = a; i < b; ++i) {
                seg.push_back(points[i][0]);
            }
            const int headSize = std::max(1, static_cast<int>(seg.size()) / 4);
            const double head = *std::max_element(seg.begin(), seg.begin() + headSize);
            const double top = *std::max_element(seg.begin(), seg.end());
            pulseSum += (head - medianOf(seg)) / (top + 1e-9);
        }
        pulse[k] = pulseSum / 4;
        for (int c = 0; c < 3; ++c) {
            peak[c] = std::max(peak[c], b3[k][c]);
        }
    }
    for (auto& b : b3) {
        for (int c = 0; c < 3; ++c) {
            b[c] = 230 * b[c] / peak[c];
        }
    }
    std::vector<double> kickRaw(nb);
    for (int k = 0; k < nb; ++k) {
        kickRaw[k] = pulse[k] * b3[k][0];
    }
    const double p90 = percentileOf(kickRaw, 90) + 1e-9;
    std::vector<bool> on(nb);
    bool state = kickRaw[0] / p90 > 0.5;
    for (int k = 0; k < nb; ++k) {
        const double v = std::clamp(kickRaw[k] / p90, 0.0, 1.5);
        if (state && v < 0.45) { // tuned (0.35 / 0.6 before)
            state = false;
        } else if (!state && v > 0.7) {
            state = true;
        }
        on[k] = state;
    }
    for (int i = 0; i < nb;) { // a 1-bar state is a fill
        int j = i;
        while (j < nb && on[j] == on[i]) {
            ++j;
        }
        if (j - i < 2 && i > 0) {
            for (int k = i; k < j; ++k) {
                on[k] = on[i - 1];
            }
        }
        i = j;
    }
    std::vector<double> kickChange(nb, 0), bandChange(nb, 0), change(nb, 0);
    for (int k = 1; k < nb; ++k) {
        kickChange[k] = on[k] != on[k - 1] ? 1 : 0;
        for (int c = 0; c < 3; ++c) {
            change[k] += std::fabs(std::log1p(b3[k][c]) - std::log1p(b3[k - 1][c]));
        }
    }
    for (int k = 4; k < nb - 4; ++k) {
        for (int c = 0; c < 3; ++c) {
            double before = 0;
            double after = 0;
            for (int i = 0; i < 4; ++i) {
                before += b3[k - 4 + i][c] / 4;
                after += b3[k + i][c] / 4;
            }
            bandChange[k] = std::max(bandChange[k],
                    std::fabs(std::log((after + kFloor) / (before + kFloor))));
        }
    }
    // 3. The phrase "1": the 8-bar offset where the kick and the bands change most
    int phase = 0;
    double bestVote = -1;
    for (int o = 0; o < 8; ++o) {
        double sum = 0;
        int count = 0;
        for (int k = o; k < nb; k += 8) {
            sum += change[k];
            ++count;
        }
        // tuned on Marko's 150 songs (testing/sections): the kick in the
        // vote cost 7 % exact sections; the bands only
        const double vote = count ? sum / count : 0;
        if (vote > bestVote) {
            bestVote = vote;
            phase = o;
        }
    }
    // 4. Novelty: the bars' self-similarity (bands + rhythm), z-scored, cosine
    constexpr int kDims = 15;
    std::vector<std::array<double, kDims>> f(nb);
    for (int k = 0; k < nb; ++k) {
        for (int c = 0; c < 3; ++c) {
            f[k][c] = 3 * std::log1p(b3[k][c]);
        }
        for (int i = 0; i < 12; ++i) {
            f[k][3 + i] = quarters[k][i];
        }
    }
    for (int d = 0; d < kDims; ++d) {
        double mean = 0;
        for (int k = 0; k < nb; ++k) {
            mean += f[k][d] / nb;
        }
        double var = 0;
        for (int k = 0; k < nb; ++k) {
            var += (f[k][d] - mean) * (f[k][d] - mean) / nb;
        }
        const double sd = std::sqrt(var) + 1e-6;
        for (int k = 0; k < nb; ++k) {
            f[k][d] = (f[k][d] - mean) / sd;
        }
    }
    for (auto& row : f) {
        double norm = 0;
        for (double v : row) {
            norm += v * v;
        }
        norm = std::sqrt(norm) + 1e-9;
        for (double& v : row) {
            v /= norm;
        }
    }
    std::vector<std::vector<double>> ssm(nb, std::vector<double>(nb));
    for (int i = 0; i < nb; ++i) {
        for (int j = 0; j < nb; ++j) {
            double dot = 0;
            for (int d = 0; d < kDims; ++d) {
                dot += f[i][d] * f[j][d];
            }
            ssm[i][j] = dot;
        }
    }
    const auto nov4 = checkerNovelty(ssm, 4);
    const auto nov8 = checkerNovelty(ssm, 8);
    // 5. The score per bar, weighted to the phrase grid
    std::vector<double> nov(nb), bc(nb), score(nb);
    for (int k = 0; k < nb; ++k) {
        nov[k] = 0.5 * nov4[k] + 0.5 * nov8[k];
        bc[k] = std::max(0.0, bandChange[k] - 0.3);
        const int rel = ((k - phase) % 8 + 8) % 8;
        const double grid = rel == 0 ? 1.0 : rel == 4 ? 0.75 : 0.35;
        score[k] = (nov[k] + 0.6 * kickChange[k] + 0.8 * bc[k]) * grid;
    }
    const double median = medianOf(score);
    std::vector<double> deviation(nb);
    for (int k = 0; k < nb; ++k) {
        deviation[k] = std::fabs(score[k] - median);
    }
    const double threshold = median + 1.5 * medianOf(deviation) + 0.15;
    std::vector<int> order(nb);
    for (int k = 0; k < nb; ++k) {
        order[k] = k;
    }
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return score[a] > score[b]; });
    std::vector<int> peaks;
    for (int k : order) {
        if (score[k] < threshold || k < 4 || k > nb - 4) {
            continue;
        }
        if (std::all_of(peaks.begin(), peaks.end(), [k](int p) { return std::abs(k - p) >= 8; })) {
            peaks.push_back(k);
        }
    }
    for (int k = 4; k < nb; ++k) {
        const int rel = ((k - phase) % 8 + 8) % 8;
        if (kickChange[k] > 0 && (rel == 0 || rel == 4) &&
                std::all_of(peaks.begin(), peaks.end(), [k](int p) { return std::abs(k - p) >= 4; })) {
            peaks.push_back(k);
        }
    }
    std::sort(peaks.begin(), peaks.end());
    for (int k : peaks) {
        const bool kick = kickChange[std::max(0, k - 1)] > 0 || kickChange[k] > 0 ||
                (k + 1 < nb && kickChange[k + 1] > 0);
        if (kick || nov[k] >= 0.5 || bc[k] >= 0.35) {
            result.append(barBeat[k]);
        }
    }
    return result;
}

namespace {

// The learned model (testing/sections/learn/learn.py, 12.10.2026)
constexpr int kV3Features = 20;
constexpr double kV3W[kV3Features] = {-0.209382, 0.453735, 0.252804, -0.19666, 0.210246, 0.123065,
        -0.125752, 0.169679, -0.0496278, 0.0465346, -0.119272, -0.0208984, -0.0993343, 0.228287,
        0.211971, -0.0247236, -0.0761221, -0.04336, -0.336709, 0.20421};
constexpr double kV3B = -0.17862991077174878;
constexpr double kV3Mu[kV3Features] = {0.280492, 0.208267, 0.162295, 0.173146, 0.191839, 0.275827,
        0.267643, 0.168424, 0.250137, 0.240638, 0.170458, 0.219792, 0.229267, 0.349417, 0.252799,
        0.128049, 0.0658737, 0.0143942, 0.0143942, 4.58622};
constexpr double kV3Sd[kV3Features] = {0.258823, 0.205962, 0.18109, 0.195467, 0.280045, 0.298463,
        0.354939, 0.281898, 0.292713, 0.332933, 0.27281, 0.264326, 0.316374, 0.358401, 0.434617,
        0.334144, 0.248061, 0.119109, 0.119109, 0.716047};
constexpr double kV3Threshold = 0.5;
constexpr int kV3Gap = 3;

} // namespace

QList<int> sectionBeatsV3(const std::vector<std::array<double, 3>>& points,
        const std::vector<double>& beats,
        int beatsPerBar) {
    QList<int> result;
    if (beatsPerBar < 1) {
        beatsPerBar = 4;
    }
    if (points.empty() || beats.size() < static_cast<size_t>(16 * beatsPerBar)) {
        return result;
    }
    const auto logBands = [](const Bands& b) {
        return Bands{std::log1p(b[0]), std::log1p(b[1]), std::log1p(b[2])};
    };
    // The downbeat: as v2
    int downbeat = 0;
    double bestChange = -1;
    for (int p = 0; p < beatsPerBar; ++p) {
        double change = 0;
        Bands previous{};
        bool first = true;
        for (size_t i = p; i + beatsPerBar < beats.size(); i += beatsPerBar) {
            const Bands now = logBands(spanMean(points, beats[i], beats[i + beatsPerBar]));
            if (!first) {
                for (int c = 0; c < 3; ++c) {
                    change += std::fabs(now[c] - previous[c]);
                }
            }
            previous = now;
            first = false;
        }
        if (change > bestChange) {
            bestChange = change;
            downbeat = p;
        }
    }
    std::vector<int> barBeat;
    for (size_t i = downbeat; i + beatsPerBar < beats.size(); i += beatsPerBar) {
        barBeat.push_back(static_cast<int>(i));
    }
    const int nb = static_cast<int>(barBeat.size());
    if (nb < 16) {
        return result;
    }
    // Per bar: the bands (0..230 per band's peak) and their quarters
    std::vector<Bands> b3(nb);
    std::vector<std::array<double, 12>> quarters(nb);
    Bands peak{1e-9, 1e-9, 1e-9};
    for (int k = 0; k < nb; ++k) {
        const double from = beats[barBeat[k]];
        const double to = beats[barBeat[k] + beatsPerBar];
        b3[k] = spanMean(points, from, to);
        for (int q = 0; q < 4; ++q) {
            const Bands m = logBands(spanMean(points,
                    from + (to - from) * q / 4,
                    from + (to - from) * (q + 1) / 4));
            for (int c = 0; c < 3; ++c) {
                quarters[k][q * 3 + c] = m[c];
            }
        }
        for (int c = 0; c < 3; ++c) {
            peak[c] = std::max(peak[c], b3[k][c]);
        }
    }
    for (auto& b : b3) {
        for (int c = 0; c < 3; ++c) {
            b[c] = 230 * b[c] / peak[c];
        }
    }
    // The bars' self-similarity (bands + rhythm), z-scored, cosine
    constexpr int kDims = 15;
    std::vector<std::array<double, kDims>> f(nb);
    for (int k = 0; k < nb; ++k) {
        for (int c = 0; c < 3; ++c) {
            f[k][c] = 3 * std::log1p(b3[k][c]);
        }
        for (int i = 0; i < 12; ++i) {
            f[k][3 + i] = quarters[k][i];
        }
    }
    for (int d = 0; d < kDims; ++d) {
        double mean = 0;
        for (int k = 0; k < nb; ++k) {
            mean += f[k][d] / nb;
        }
        double var = 0;
        for (int k = 0; k < nb; ++k) {
            var += (f[k][d] - mean) * (f[k][d] - mean) / nb;
        }
        const double sd = std::sqrt(var) + 1e-6;
        for (int k = 0; k < nb; ++k) {
            f[k][d] = (f[k][d] - mean) / sd;
        }
    }
    for (auto& row : f) {
        double norm = 0;
        for (double v : row) {
            norm += v * v;
        }
        norm = std::sqrt(norm) + 1e-9;
        for (double& v : row) {
            v /= norm;
        }
    }
    std::vector<std::vector<double>> ssm(nb, std::vector<double>(nb));
    for (int i = 0; i < nb; ++i) {
        for (int j = 0; j < nb; ++j) {
            double dot = 0;
            for (int d = 0; d < kDims; ++d) {
                dot += f[i][d] * f[j][d];
            }
            ssm[i][j] = dot;
        }
    }
    const auto nov1 = checkerNovelty(ssm, 1);
    const auto nov2 = checkerNovelty(ssm, 2);
    const auto nov4 = checkerNovelty(ssm, 4);
    const auto nov8 = checkerNovelty(ssm, 8);
    // Band changes over w bars: |log((after + 4) / (before + 4))| per band
    const auto change = [&](int w, int k, int c) {
        if (k < w || k > nb - w) {
            return 0.0;
        }
        double before = 0;
        double after = 0;
        for (int i = 0; i < w; ++i) {
            before += b3[k - w + i][c] / w;
            after += b3[k + i][c] / w;
        }
        return std::fabs(std::log((after + kFloor) / (before + kFloor)));
    };
    std::vector<double> prob(nb);
    for (int k = 0; k < nb; ++k) {
        double x[kV3Features];
        x[0] = nov1[k];
        x[1] = nov2[k];
        x[2] = nov4[k];
        x[3] = nov8[k];
        double c4max = 0;
        for (int c = 0; c < 3; ++c) {
            x[4 + c] = change(1, k, c);
            x[7 + c] = change(2, k, c);
            x[10 + c] = change(4, k, c);
            c4max = std::max(c4max, x[10 + c]);
        }
        x[13] = c4max;
        x[14] = k % 4 == 0 ? 1 : 0;
        x[15] = k % 8 == 0 ? 1 : 0;
        x[16] = k % 16 == 0 ? 1 : 0;
        x[17] = k < 2 ? 1 : 0;
        x[18] = k > nb - 3 ? 1 : 0;
        x[19] = (std::log1p(b3[k][0]) + std::log1p(b3[k][1]) + std::log1p(b3[k][2])) / 3;
        double z = kV3B;
        for (int i = 0; i < kV3Features; ++i) {
            z += kV3W[i] * (x[i] - kV3Mu[i]) / kV3Sd[i];
        }
        prob[k] = 1 / (1 + std::exp(-z));
    }
    std::vector<int> order(nb);
    for (int k = 0; k < nb; ++k) {
        order[k] = k;
    }
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return prob[a] > prob[b]; });
    std::vector<int> peaks;
    for (int k : order) {
        if (prob[k] < kV3Threshold) {
            break;
        }
        if (std::all_of(peaks.begin(), peaks.end(), [k](int p) { return std::abs(k - p) >= kV3Gap; })) {
            peaks.push_back(k);
        }
    }
    std::sort(peaks.begin(), peaks.end());
    for (int k : peaks) {
        result.append(barBeat[k]);
    }
    return result;
}

QList<mixxx::audio::FramePos> detectSections(const Waveform& waveform,
        const mixxx::BeatsPointer& pBeats,
        mixxx::audio::FramePos one,
        int beatsPerBar,
        mixxx::audio::FramePos trackEnd) {
    Q_UNUSED(one); // v2 finds the downbeat and the phrase "1" itself
    QList<mixxx::audio::FramePos> result;
    const int size = waveform.getDataSize();
    const double ratio = waveform.getAudioVisualRatio();
    if (!pBeats || !trackEnd.isValid() || size <= 1 || ratio <= 0 ||
            waveform.getCompletion() < size) {
        return result;
    }
    // The waveform's points, both channels (even and odd) as one
    std::vector<std::array<double, 3>> points(size / 2);
    for (int k = 0; k < size / 2; ++k) {
        points[k] = {(waveform.getLow(2 * k) + waveform.getLow(2 * k + 1)) / 2.0,
                (waveform.getMid(2 * k) + waveform.getMid(2 * k + 1)) / 2.0,
                (waveform.getHigh(2 * k) + waveform.getHigh(2 * k + 1)) / 2.0};
    }
    // Every beat of the grid, in frames and in points
    std::vector<mixxx::audio::FramePos> beatFrames;
    std::vector<double> beats;
    auto beat = pBeats->firstBeat();
    int guard = 0;
    while (beat.isValid() && beat < trackEnd && guard++ < 100000) {
        beatFrames.push_back(beat);
        beats.push_back(beat.value() / ratio);
        const auto next = pBeats->findNextBeat(beat + 1);
        if (!next.isValid() || next <= beat) {
            break;
        }
        beat = next;
    }
    for (int index : sectionBeatsV3(points, beats, beatsPerBar)) {
        result.append(beatFrames[index]);
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
