#include "analyzer/rbxanalysis.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>

#include "analyzer/sectionnames.h"
#include "dsp/transforms/FFT.h"

// Ported from rbxport (https://github.com/chrisle/rbxport, crates/rbl-analysis/src: onset.rs,
// attack.rs, tempo.rs, downbeat.rs), Copyright Chris Le, GPL-2.0-or-later. The comments that
// explain a choice are rbxport's reasons, shortened; see its docs/algorithms for the measurements.
namespace djmantra::rbx {

namespace {

constexpr double kPi = 3.14159265358979323846;

// ------------------------------------------------------------------ onset.rs

constexpr int kFrame = 1024;
constexpr int kHop = 256;

struct Envelope {
    std::vector<double> values;
    double rate = 0;        // envelope samples per second
    double originSecs = 0;  // the time of sample 0: the centre of the first frame
};

std::vector<double> hann(int n) {
    std::vector<double> w(n);
    for (int i = 0; i < n; ++i) {
        w[i] = 0.5 - 0.5 * std::cos(2.0 * kPi * i / n);
    }
    return w;
}

// Spectral flux: per frame, the sum of every bin's rise since the frame before. Rises only, so a
// held note or a decay adds nothing and a kick, which raises many bins at once, adds a lot.
Envelope onsetEnvelope(const float* samples, size_t count, int sampleRate) {
    Envelope env;
    env.rate = static_cast<double>(sampleRate) / kHop;
    env.originSecs = kFrame / 2.0 / sampleRate;
    if (count < static_cast<size_t>(kFrame) || sampleRate <= 0) {
        return env;
    }
    FFTReal fft(kFrame);
    const auto window = hann(kFrame);
    std::vector<double> in(kFrame), mag(kFrame), previous(kFrame / 2 + 1, 0.0);
    const size_t frames = (count - kFrame) / kHop + 1;
    env.values.reserve(frames);
    for (size_t f = 0; f < frames; ++f) {
        const float* chunk = samples + f * kHop;
        for (int i = 0; i < kFrame; ++i) {
            in[i] = chunk[i] * window[i];
        }
        fft.forwardMagnitude(in.data(), mag.data());
        double flux = 0;
        for (int b = 0; b <= kFrame / 2; ++b) {
            const double rise = mag[b] - previous[b];
            previous[b] = mag[b];
            if (rise > 0) {
                flux += rise;
            }
        }
        env.values.push_back(flux);
    }
    // A local mean subtracted and clipped at zero (a quiet intro and a loud drop count alike),
    // then scaled to a peak of one
    constexpr int kWindow = 16;
    const auto original = env.values;
    const int n = static_cast<int>(original.size());
    if (n > kWindow) {
        std::vector<double> prefix(n + 1, 0.0);
        for (int i = 0; i < n; ++i) {
            prefix[i + 1] = prefix[i] + original[i];
        }
        for (int i = 0; i < n; ++i) {
            const int lo = std::max(0, i - kWindow);
            const int hi = std::min(n, i + kWindow);
            const double mean = (prefix[hi] - prefix[lo]) / (hi - lo);
            env.values[i] = std::max(0.0, original[i] - mean);
        }
    }
    const double peak = *std::max_element(env.values.begin(), env.values.end());
    if (peak > 0) {
        for (double& v : env.values) {
            v /= peak;
        }
    }
    return env;
}

// ------------------------------------------------------------------ attack.rs

struct Biquad {
    double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    static Biquad make(double b0, double b1, double b2, double a0, double a1, double a2) {
        Biquad q;
        q.b0 = b0 / a0;
        q.b1 = b1 / a0;
        q.b2 = b2 / a0;
        q.a1 = a1 / a0;
        q.a2 = a2 / a0;
        return q;
    }
    // RBJ cookbook, Q = 1/sqrt(2)
    static Biquad highPass(double hz, int rate) {
        const double w = 2 * kPi * hz / rate;
        const double alpha = std::sin(w) / (2 * std::sqrt(2.0));
        const double c = std::cos(w);
        const double half = (1 + c) / 2;
        return make(half, -(1 + c), half, 1 + alpha, -2 * c, 1 - alpha);
    }
    static Biquad lowPass(double hz, int rate) {
        const double w = 2 * kPi * hz / rate;
        const double alpha = std::sin(w) / (2 * std::sqrt(2.0));
        const double c = std::cos(w);
        const double half = (1 - c) / 2;
        return make(half, 1 - c, half, 1 + alpha, -2 * c, 1 - alpha);
    }
    double run(double x) {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }
};

struct Attack {
    double secs;
    double height;
};

// The track band-passed to the kick's click (900-9000 Hz), kept as its RMS in 1 ms steps
class AttackMap {
  public:
    static constexpr double kReachSecs = 0.015;

    AttackMap(const float* samples, size_t count, int sampleRate)
            : m_sampleRate(sampleRate) {
        m_step = std::max(1, static_cast<int>(0.001 * sampleRate));
        const double nyquist = sampleRate / 2.0;
        Biquad hp = Biquad::highPass(std::clamp(900.0, 1.0, nyquist * 0.99), sampleRate);
        Biquad lp = Biquad::lowPass(std::clamp(9000.0, 1.0, nyquist * 0.99), sampleRate);
        m_rms.reserve(count / m_step + 1);
        double sum = 0;
        int in = 0;
        for (size_t i = 0; i < count; ++i) {
            const double y = lp.run(hp.run(samples[i]));
            sum += y * y;
            if (++in == m_step) {
                m_rms.push_back(std::sqrt(sum / in));
                sum = 0;
                in = 0;
            }
        }
        if (in > 0) {
            m_rms.push_back(std::sqrt(sum / in));
        }
    }

    // The attack nearest `secs` within `reach` either side: among the rises at least half as
    // steep as the window's steepest, the nearest (the steepest outright let a grid drift onto
    // the clap after the kick). None where nothing rises: a breakdown, a beatless intro.
    std::optional<Attack> within(double secs, double reach) const {
        const int n = static_cast<int>(m_rms.size());
        if (n < 3 || m_sampleRate <= 0) {
            return std::nullopt;
        }
        const double stepSecs = static_cast<double>(m_step) / m_sampleRate;
        const int centre = std::max(0, static_cast<int>(std::lround(secs / stepSecs)));
        const int reachSteps = std::max(1, static_cast<int>(std::ceil(reach / stepSecs)));
        const int lo = std::max(1, centre - reachSteps);
        const int hi = std::min(centre + reachSteps, n - 1);
        if (hi <= lo) {
            return std::nullopt;
        }
        const auto rise = [&](int i) { return m_rms[i] - m_rms[i - 1]; };
        double steepest = 0;
        int steepestAt = lo;
        for (int i = lo; i <= hi; ++i) {
            if (rise(i) > steepest) {
                steepest = rise(i);
                steepestAt = i;
            }
        }
        if (steepest <= 0) {
            return std::nullopt;
        }
        // A rise no bigger than the window's ordinary wobble is not a spike
        std::vector<double> rises;
        for (int i = lo; i <= hi; ++i) {
            rises.push_back(std::fabs(rise(i)));
        }
        std::nth_element(rises.begin(), rises.begin() + rises.size() / 2, rises.end());
        if (steepest < rises[rises.size() / 2] * 4.0) {
            return std::nullopt;
        }
        int chosen = steepestAt;
        double chosenRise = steepest;
        for (int i = lo; i <= hi; ++i) {
            if (rise(i) >= steepest * 0.5 && std::abs(i - centre) < std::abs(chosen - centre)) {
                chosen = i;
                chosenRise = rise(i);
            }
        }
        // Back to where the spike's run of rises starts
        int start = chosen;
        while (start > lo && rise(start - 1) > chosenRise * 0.1) {
            --start;
        }
        const double floor = m_rms[start - 1];
        double top = floor;
        for (int i = start; i <= hi; ++i) {
            top = std::max(top, m_rms[i]);
        }
        // The middle of the bin before the rise (its left edge was a half-bin early bias)
        const double sample = (start - 1) * static_cast<double>(m_step) + m_step / 2.0;
        return Attack{sample / m_sampleRate, top - floor};
    }

  private:
    std::vector<double> m_rms;
    int m_step = 1;
    int m_sampleRate = 0;
};

// ------------------------------------------------------------------ tempo.rs

constexpr double kMinBpm = 70.0;
constexpr double kMaxBpm = 180.0;
constexpr double kPriorCentre = 132.0;
constexpr double kPriorWidth = 0.5;

struct Fit {
    double phase; // envelope samples
    double period;
};

double interpolate(const std::vector<double>& table, double x) {
    if (table.empty()) {
        return 0;
    }
    x = std::clamp(x, 0.0, static_cast<double>(table.size() - 1));
    const size_t i = static_cast<size_t>(x);
    const double frac = x - i;
    const double a = table[i];
    const double b = i + 1 < table.size() ? table[i + 1] : a;
    return a + (b - a) * frac;
}

double sampleAt(const std::vector<double>& values, double x) {
    if (x < 0) {
        return 0;
    }
    const size_t i = static_cast<size_t>(x);
    const double frac = x - i;
    const double a = i < values.size() ? values[i] : 0;
    const double b = i + 1 < values.size() ? values[i + 1] : 0;
    return a + (b - a) * frac;
}

// The Fourier magnitude of the envelope at one tempo, over 20 s Hann windows (a whole track
// resolves 0.2 BPM and cancels a candidate a tenth of a BPM off; 20 s resolves 3 BPM)
double fourierMagnitude(const std::vector<double>& centred, double rate, double bpm) {
    const double omega = 2 * kPi * bpm / 60.0 / rate;
    const size_t n = centred.size();
    const size_t window = std::clamp<size_t>(static_cast<size_t>(20.0 * rate), 64, std::max<size_t>(n, 64));
    const size_t hop = std::max<size_t>(1, window / 2);
    const double stepRe = std::cos(omega), stepIm = std::sin(omega);
    double total = 0, windows = 0;
    for (size_t start = 0; start + window <= n; start += hop) {
        double re = 1, im = 0, sumRe = 0, sumIm = 0;
        for (size_t i = 0; i < window; ++i) {
            const double h = 0.5 - 0.5 * std::cos(2 * kPi * i / window);
            const double w = centred[start + i] * h;
            sumRe += w * re;
            sumIm += w * im;
            const double nextRe = re * stepRe - im * stepIm;
            im = re * stepIm + im * stepRe;
            re = nextRe;
        }
        total += std::sqrt(sumRe * sumRe + sumIm * sumIm) / window;
        windows += 1;
    }
    return windows > 0 ? total / windows : 0;
}

double tempoPrior(double bpm) {
    if (bpm <= 0) {
        return 0;
    }
    const double x = std::log(bpm / kPriorCentre) / kPriorWidth;
    return std::exp(-0.5 * x * x);
}

struct Candidate {
    double bpm, acf, fourier, score;
};

bool related(double a, double b) {
    if (a <= 0 || b <= 0) {
        return false;
    }
    const double ratio = a > b ? a / b : b / a;
    for (double r : {1.0, 2.0, 3.0, 4.0, 1.5, 4.0 / 3.0}) {
        if (std::fabs(ratio - r) < 0.02 * r) {
            return true;
        }
    }
    return false;
}

// Every tempo worth considering, best first: autocorrelation peaks in range, with their multiples
// and simple fractions, scored autocorrelation x sqrt(fourier) x prior. The Fourier term rules out
// three halves of the beat (kick, hat, kick, hat correlates but has no spectral line).
std::vector<Candidate> candidates(const std::vector<double>& values, double rate) {
    std::vector<Candidate> out;
    const size_t n = values.size();
    double mean = 0;
    for (double v : values) {
        mean += v;
    }
    mean /= n;
    std::vector<double> centred(n);
    double variance = 0;
    for (size_t i = 0; i < n; ++i) {
        centred[i] = values[i] - mean;
        variance += centred[i] * centred[i];
    }
    variance /= n;
    if (variance <= 0) {
        return out;
    }
    const auto lagOf = [rate](double bpm) { return rate * 60.0 / bpm; };
    const size_t minLag = std::max<size_t>(2, static_cast<size_t>(std::floor(lagOf(kMaxBpm * 2))));
    const size_t maxLag = std::min<size_t>(static_cast<size_t>(std::ceil(lagOf(kMinBpm / 2))), n / 2);
    if (maxLag <= minLag + 2) {
        return out;
    }
    std::vector<double> acf;
    acf.reserve(maxLag - minLag + 1);
    for (size_t lag = minLag; lag <= maxLag; ++lag) {
        const size_t count = n - lag;
        double sum = 0;
        for (size_t i = 0; i < count; ++i) {
            sum += centred[i] * centred[i + lag];
        }
        acf.push_back(count > 0 ? sum / (count * variance) : 0);
    }
    const auto acfAt = [&](double lag) { return interpolate(acf, lag - static_cast<double>(minLag)); };
    const auto inRange = [](double bpm) { return bpm >= kMinBpm && bpm <= kMaxBpm; };
    std::vector<double> bpms;
    const size_t lo = static_cast<size_t>(std::floor(lagOf(kMaxBpm)));
    const size_t hi = static_cast<size_t>(std::ceil(lagOf(kMinBpm)));
    for (size_t lag = std::max(lo, minLag + 1); lag < std::min(hi, maxLag - 1); ++lag) {
        const double here = acfAt(lag);
        if (here <= acfAt(lag - 1.0) || here < acfAt(lag + 1.0) || here <= 0) {
            continue;
        }
        const double a = acfAt(lag - 1.0), c = acfAt(lag + 1.0);
        const double denom = a - 2 * here + c;
        const double offset = std::fabs(denom) > 1e-12 ? 0.5 * (a - c) / denom : 0;
        const double bpm = rate * 60.0 / (lag + std::clamp(offset, -0.5, 0.5));
        for (double ratio : {1.0, 2.0, 0.5, 1.5, 2.0 / 3.0, 3.0, 1.0 / 3.0, 4.0 / 3.0, 0.75}) {
            const double relative = bpm * ratio;
            if (inRange(relative) &&
                    std::none_of(bpms.begin(), bpms.end(), [&](double b) {
                        return std::fabs(b - relative) < relative * 0.01;
                    })) {
                bpms.push_back(relative);
            }
        }
    }
    if (bpms.empty()) {
        return out;
    }
    std::vector<double> fourier;
    double fourierPeak = 0, acfPeak = 0;
    for (double bpm : bpms) {
        fourier.push_back(fourierMagnitude(centred, rate, bpm));
        fourierPeak = std::max(fourierPeak, fourier.back());
        acfPeak = std::max(acfPeak, acfAt(lagOf(bpm)));
    }
    for (size_t i = 0; i < bpms.size(); ++i) {
        const double a = std::max(0.0, acfAt(lagOf(bpms[i])) / std::max(acfPeak, 1e-12));
        const double f = fourierPeak > 0 ? fourier[i] / fourierPeak : 0;
        out.push_back({bpms[i], a, f, a * std::sqrt(f) * tempoPrior(bpms[i])});
    }
    std::sort(out.begin(), out.end(), [](const Candidate& x, const Candidate& y) {
        return x.score > y.score;
    });
    return out;
}

class Reader {
  public:
    Reader(const Envelope& env, const AttackMap& attacks)
            : values(env.values), m_env(env), m_attacks(attacks) {
    }
    // The beat nearest envelope sample `x`: the kick's attack within the map's reach, as an
    // envelope sample and its height
    std::optional<std::pair<double, double>> snap(double x, double reach) const {
        const double secs = m_env.originSecs + x / m_env.rate;
        const double reachSecs = std::min(reach / m_env.rate, AttackMap::kReachSecs);
        if (const auto a = m_attacks.within(secs, reachSecs)) {
            return std::make_pair((a->secs - m_env.originSecs) * m_env.rate, a->height);
        }
        return std::nullopt;
    }
    double rate() const {
        return m_env.rate;
    }
    const std::vector<double>& values;

  private:
    const Envelope& m_env;
    const AttackMap& m_attacks;
};

double comb(const std::vector<double>& values, double period, double phase) {
    if (period < 2 || values.empty()) {
        return 0;
    }
    double sum = 0, count = 0;
    for (double x = phase; x < values.size(); x += period) {
        sum += sampleAt(values, x);
        count += 1;
    }
    return count > 0 ? sum / count : 0;
}

double refinePeriod(const std::vector<double>& values, double coarse, int phases) {
    const auto score = [&](double period) {
        double best = 0;
        for (int step = 0; step < phases; ++step) {
            best = std::max(best, comb(values, period, period * step / phases));
        }
        return best;
    };
    double bestScore = score(coarse), best = coarse;
    for (const auto& [span, step] : {std::pair{1.0, 0.02}, std::pair{0.03, 0.002}}) {
        const double centre = best;
        for (double candidate = centre - span; candidate <= centre + span; candidate += step) {
            const double s = score(candidate);
            if (s > bestScore) {
                bestScore = s;
                best = candidate;
            }
        }
    }
    return best;
}

double bestPhase(const std::vector<double>& values, double period, int phases) {
    double bestScore = -std::numeric_limits<double>::infinity(), best = 0;
    for (int step = 0; step < phases; ++step) {
        const double phase = period * step / phases;
        const double s = comb(values, period, phase);
        if (s > bestScore) {
            bestScore = s;
            best = phase;
        }
    }
    const double step = period / phases;
    const double a = comb(values, period, best - step), c = comb(values, period, best + step);
    const double denom = a - 2 * bestScore + c;
    const double offset = std::fabs(denom) > 1e-12 ? 0.5 * (a - c) / denom : 0;
    return best + std::clamp(offset, -1.0, 1.0) * step;
}

struct Point {
    double x, y, w;
};

// Every predicted beat moved to its kick attack, and a line fitted through them weighted by the
// spike's height, twice: the second time without the fifth furthest from the first line and
// nothing further than a tenth of a beat. Returns the fit and the support (the kept heights).
std::optional<std::pair<Fit, double>> snapAndFit(const Reader& reader, size_t from, size_t to,
        Fit current, std::optional<double> fixedPeriod) {
    const double period = current.period;
    if (period < 2) {
        return std::nullopt;
    }
    const double reach = std::max(period * 0.2, 1.0);
    const long first = static_cast<long>(std::ceil((from - current.phase) / period));
    const long last = static_cast<long>(std::floor((to - 1.0 - current.phase) / period));
    if (last < first + 4) {
        return std::nullopt;
    }
    std::vector<Point> snapped;
    for (long k = first; k <= last; ++k) {
        if (const auto hit = reader.snap(current.phase + k * period, reach)) {
            snapped.push_back({static_cast<double>(k), hit->first, hit->second});
        }
    }
    if (snapped.size() < 4) {
        return std::nullopt;
    }
    // Next to no onset: left out (an intro of pads would tilt the line)
    std::vector<double> heights;
    for (const auto& p : snapped) {
        heights.push_back(p.w);
    }
    std::nth_element(heights.begin(), heights.begin() + heights.size() / 2, heights.end());
    const double floor = heights[heights.size() / 2] * 0.2;
    std::vector<Point> strong;
    for (const auto& p : snapped) {
        if (p.w >= floor) {
            strong.push_back(p);
        }
    }
    if (strong.size() >= 4) {
        snapped = strong;
    }
    const auto line = [&](const std::vector<Point>& points) -> std::optional<std::pair<double, double>> {
        double sw = 0, sx = 0, sy = 0, sxx = 0, sxy = 0;
        for (const auto& p : points) {
            sw += p.w;
            sx += p.w * p.x;
            sy += p.w * p.y;
            sxx += p.w * p.x * p.x;
            sxy += p.w * p.x * p.y;
        }
        if (sw <= 0) {
            return std::nullopt;
        }
        if (fixedPeriod) {
            return std::make_pair(*fixedPeriod, (sy - *fixedPeriod * sx) / sw);
        }
        const double denom = sw * sxx - sx * sx;
        if (std::fabs(denom) < 1e-12) {
            return std::nullopt;
        }
        const double slope = (sw * sxy - sx * sy) / denom;
        return std::make_pair(slope, (sy - slope * sx) / sw);
    };
    auto first_line = line(snapped);
    if (!first_line) {
        return std::nullopt;
    }
    auto [slope, intercept] = *first_line;
    std::vector<double> residuals;
    for (const auto& p : snapped) {
        residuals.push_back(std::fabs(p.y - intercept - slope * p.x));
    }
    std::vector<double> sorted = residuals;
    std::sort(sorted.begin(), sorted.end());
    const size_t at = sorted.size() * 4 / 5;
    const double cutoff = std::min(at < sorted.size() ? sorted[at] : std::numeric_limits<double>::infinity(),
            period * 0.1);
    std::vector<Point> kept;
    for (const auto& p : snapped) {
        if (std::fabs(p.y - intercept - slope * p.x) <= cutoff) {
            kept.push_back(p);
        }
    }
    if (kept.size() >= 4) {
        const auto second = line(kept);
        if (!second) {
            return std::nullopt;
        }
        slope = second->first;
        intercept = second->second;
    }
    // A fit that walked away from the period fitted the wrong onsets
    if (std::fabs(slope - period) > period * 0.02) {
        return std::nullopt;
    }
    double support = 0;
    for (const auto& p : kept) {
        support += p.w;
    }
    return std::make_pair(Fit{intercept, slope}, support);
}

// How much of the music a line sits on: each beat's hit by strength, less the further it is from
// the line, nothing beyond 10 ms
double onLine(const Reader& reader, size_t from, size_t to, Fit f) {
    if (f.period < 2) {
        return 0;
    }
    const double reach = std::max(f.period * 0.2, 1.0);
    const double tolerance = 0.010 * reader.rate();
    const long first = static_cast<long>(std::ceil((from - f.phase) / f.period));
    const long last = static_cast<long>(std::floor((to - 1.0 - f.phase) / f.period));
    double sum = 0;
    for (long k = first; k <= last; ++k) {
        const double predicted = f.phase + k * f.period;
        if (const auto hit = reader.snap(predicted, reach)) {
            sum += hit->second * std::max(0.0, 1.0 - std::fabs(hit->first - predicted) / tolerance);
        }
    }
    return sum;
}

// Dance music is produced at whole tempos: a fit within 0.1 BPM of one is snapped to it when the
// whole number's line sits on at least 85 % as many hits
Fit wholeBpm(const Reader& reader, size_t from, size_t to, Fit fit) {
    if (fit.period <= 0) {
        return fit;
    }
    const double bpm = reader.rate() * 60.0 / fit.period;
    const double whole = std::round(bpm);
    if (whole <= 0 || std::fabs(bpm - whole) > 0.1) {
        return fit;
    }
    const double period = reader.rate() * 60.0 / whole;
    const Fit start{fit.phase, period};
    const auto refit = snapAndFit(reader, from, to, start, period);
    const Fit snapped = refit ? refit->first : start;
    if (onLine(reader, from, to, snapped) < onLine(reader, from, to, fit) * 0.85) {
        return fit;
    }
    return snapped;
}

std::optional<Fit> fitTrack(const Reader& reader, double bpm) {
    const auto& values = reader.values;
    const size_t n = values.size();
    if (n < 8) {
        return std::nullopt;
    }
    const double coarse = reader.rate() * 60.0 / bpm;
    const double period = refinePeriod(values, coarse, 32);
    const double phase = bestPhase(values, period, 64);
    // From the comb's phase and from half a beat later: the comb may sit on the off-beat (hats and
    // an off-beat bass carry as much flux as the kick); the one that collects more kick wins
    std::optional<std::pair<Fit, double>> best;
    for (double half : {0.0, 0.5}) {
        Fit fit{phase + half * period, period};
        double support = 0;
        for (int pass = 0; pass < 3; ++pass) {
            const auto next = snapAndFit(reader, 0, n, fit, std::nullopt);
            if (!next) {
                break;
            }
            fit = next->first;
            support = next->second;
        }
        if (!best || support > best->second) {
            best = std::make_pair(fit, support);
        }
    }
    return wholeBpm(reader, 0, n, best->first);
}

// ------------------------------------------------------------------ downbeat.rs

constexpr int kBandFrame = 2048;
constexpr int kBandHop = 512;
constexpr double kBandEdges[13] = {
        40, 65, 100, 160, 250, 400, 630, 1000, 1600, 2500, 4000, 6300, 10000};
constexpr int kBands = 12;
using Profile = std::array<double, kBands>;

std::vector<Profile> bandFrames(const float* samples, size_t count, int sampleRate) {
    std::vector<Profile> frames;
    if (count < static_cast<size_t>(kBandFrame)) {
        return frames;
    }
    FFTReal fft(kBandFrame);
    const auto window = hann(kBandFrame);
    std::vector<double> in(kBandFrame), re(kBandFrame), im(kBandFrame);
    const double hzPerBin = static_cast<double>(sampleRate) / kBandFrame;
    std::vector<int> bandOf(kBandFrame / 2 + 1, -1);
    for (int bin = 0; bin <= kBandFrame / 2; ++bin) {
        const double hz = bin * hzPerBin;
        for (int b = 0; b < kBands; ++b) {
            if (hz >= kBandEdges[b] && hz < kBandEdges[b + 1]) {
                bandOf[bin] = b;
            }
        }
    }
    const size_t total = (count - kBandFrame) / kBandHop + 1;
    frames.reserve(total);
    for (size_t f = 0; f < total; ++f) {
        const float* chunk = samples + f * kBandHop;
        for (int i = 0; i < kBandFrame; ++i) {
            in[i] = chunk[i] * window[i];
        }
        fft.forward(in.data(), re.data(), im.data());
        Profile energy{};
        for (int bin = 0; bin <= kBandFrame / 2; ++bin) {
            if (bandOf[bin] >= 0) {
                energy[bandOf[bin]] += re[bin] * re[bin] + im[bin] * im[bin];
            }
        }
        Profile profile;
        for (int b = 0; b < kBands; ++b) {
            profile[b] = std::log(energy[b] + 1e-6);
        }
        frames.push_back(profile);
    }
    return frames;
}

// One profile per slot (a beat or half a beat): the mean log band energy over its frames
std::vector<Profile> slotProfiles(const std::vector<Profile>& frames, int sampleRate,
        const std::vector<double>& starts) {
    std::vector<Profile> out;
    const double frameRate = static_cast<double>(sampleRate) / kBandHop;
    for (size_t i = 0; i < starts.size(); ++i) {
        const double start = starts[i];
        const double end = i + 1 < starts.size()
                ? starts[i + 1]
                : start + (starts.size() > 1 ? starts[1] - starts[0] : 0.5);
        const size_t first = static_cast<size_t>(std::max(0.0, std::floor(start * frameRate)));
        const size_t last = std::min(frames.size(), static_cast<size_t>(std::ceil(end * frameRate)));
        Profile profile{};
        if (first < last) {
            for (size_t f = first; f < last; ++f) {
                for (int b = 0; b < kBands; ++b) {
                    profile[b] += frames[f][b];
                }
            }
            for (double& v : profile) {
                v /= static_cast<double>(last - first);
            }
        }
        out.push_back(profile);
    }
    return out;
}

// The distance between the mean profile over `scale` slots before each slot and over `scale`
// slots from it
std::vector<double> novelty(const std::vector<Profile>& profiles, size_t scale) {
    const size_t n = profiles.size();
    std::vector<double> out(n, 0.0);
    if (n < scale * 2) {
        return out;
    }
    std::vector<Profile> prefix(n + 1, Profile{});
    for (size_t i = 0; i < n; ++i) {
        for (int b = 0; b < kBands; ++b) {
            prefix[i + 1][b] = prefix[i][b] + profiles[i][b];
        }
    }
    for (size_t i = scale; i + scale <= n; ++i) {
        double d = 0;
        for (int b = 0; b < kBands; ++b) {
            const double before = (prefix[i][b] - prefix[i - scale][b]) / scale;
            const double after = (prefix[i + scale][b] - prefix[i][b]) / scale;
            d += (after - before) * (after - before);
        }
        out[i] = std::sqrt(d);
    }
    return out;
}

// Only local maxima: a slow crescendo raises every slot alike
std::vector<std::pair<size_t, double>> peaksOf(const std::vector<double>& values) {
    std::vector<std::pair<size_t, double>> peaks;
    for (size_t i = 0; i < values.size(); ++i) {
        const double left = i > 0 ? values[i - 1] : 0;
        const double right = i + 1 < values.size() ? values[i + 1] : 0;
        if (values[i] > left && values[i] >= right && values[i] > 0) {
            peaks.emplace_back(i, values[i]);
        }
    }
    return peaks;
}

struct GridPhase {
    bool halfBeatOff = false;
    double downbeatSecs = 0;
    std::vector<double> phraseStarts;
    std::vector<std::pair<double, double>> candidates;
};

// Dance music changes at bar lines and, more strongly, at phrase lines, and every such change is
// on beat 1, a kick, never the off-beat: the 8 half-beat positions of the bar are scored by the
// novelty peaks (1, 2, 4 and 8 bars) that land on them
GridPhase gridPhase(const std::vector<Profile>& frames, int sampleRate, const std::vector<double>& beats) {
    GridPhase result;
    result.downbeatSecs = beats.empty() ? 0 : beats.front();
    if (beats.size() < 8 || frames.empty()) {
        return result;
    }
    std::vector<double> halves;
    for (size_t i = 0; i + 1 < beats.size(); ++i) {
        halves.push_back(beats[i]);
        halves.push_back((beats[i] + beats[i + 1]) / 2);
    }
    const auto profiles = slotProfiles(frames, sampleRate, halves);
    constexpr size_t kPositions = 8;
    if (profiles.size() < kPositions * 2) {
        return result;
    }
    // Identical repeated beats say nothing about the bar: a real 4-bar change is required
    const auto fourBars = peaksOf(novelty(profiles, 32));
    if (std::all_of(fourBars.begin(), fourBars.end(), [](const auto& p) { return p.second < 0.5; })) {
        return result;
    }
    std::array<double, kPositions> scores{};
    for (size_t scale : {8, 16, 32, 64}) {
        if (profiles.size() < scale * 2) {
            continue;
        }
        const auto peaks = peaksOf(novelty(profiles, scale));
        double total = 0;
        for (const auto& p : peaks) {
            total += p.second;
        }
        if (total <= 0) {
            continue;
        }
        for (const auto& p : peaks) {
            scores[p.first % kPositions] += p.second / total;
        }
    }
    const size_t best = std::max_element(scores.begin(), scores.end()) - scores.begin();
    result.halfBeatOff = best % 2 == 1;
    result.downbeatSecs = halves[best];
    // Phrase starts: 4-bar novelty peaks on the downbeat, at least 30 % of the strongest.
    // DJ Mantra: "the strongest" is the 90th percentile of the downbeat peaks, not the maximum: a
    // fade into the file's closing silence is a far bigger change than any phrase (log energy of
    // silence), and alone it left one phrase start in a whole song
    std::vector<double> onDownbeat;
    for (const auto& p : fourBars) {
        if (p.first % kPositions == best) {
            onDownbeat.push_back(p.second);
        }
    }
    std::sort(onDownbeat.begin(), onDownbeat.end());
    const double strongest = onDownbeat.empty()
            ? 0
            : onDownbeat[std::min(onDownbeat.size() - 1, static_cast<size_t>(0.9 * (onDownbeat.size() - 1) + 0.5))];
    for (const auto& p : fourBars) {
        if (p.first % kPositions == best && strongest > 0) {
            result.candidates.emplace_back(halves[p.first], p.second / strongest);
            if (p.second >= strongest * 0.3) {
                result.phraseStarts.push_back(halves[p.first]);
            }
        }
    }
    return result;
}

// ------------------------------------------------------------------ section names (DJ Mantra)

// The bars from the first downbeat, measured for the names (src/analyzer/sectionnames.h): the
// level, the kick (the low band, 40-100 Hz, at the beats) and the highs, in dB
std::vector<Section> nameSections(const std::vector<Profile>& frames, int sampleRate,
        const std::vector<double>& beats, size_t firstDownbeat, const std::vector<double>& phraseStarts,
        std::vector<std::array<double, 5>>* barsOut) {
    std::vector<Section> sections;
    if (beats.size() < firstDownbeat + 8 || frames.empty()) {
        return sections;
    }
    constexpr double kDb = 4.342944819; // ln power to dB
    std::vector<double> barStarts;
    for (size_t i = firstDownbeat; i < beats.size(); i += 4) {
        barStarts.push_back(beats[i]);
    }
    const auto bars = slotProfiles(frames, sampleRate, barStarts);
    const double frameRate = static_cast<double>(sampleRate) / kBandHop;
    const auto lowAt = [&](double secs) {
        const long f = std::lround(secs * frameRate);
        double best = -1e9;
        for (long g = f - 2; g <= f + 2; ++g) {
            if (g >= 0 && g < static_cast<long>(frames.size())) {
                best = std::max(best, std::max(frames[g][0], frames[g][1]));
            }
        }
        return best;
    };
    std::vector<sections::Bar> measured;
    for (size_t b = 0; b < bars.size(); ++b) {
        double e = 0;
        for (double v : bars[b]) {
            e += v;
        }
        double kick = 0;
        int counted = 0;
        for (size_t k = firstDownbeat + b * 4; k < firstDownbeat + b * 4 + 4 && k < beats.size(); ++k) {
            kick += lowAt(beats[k]);
            ++counted;
        }
        sections::Bar bar;
        bar.energy = e / kBands * kDb;
        bar.kick = (counted ? kick / counted : -1e9) * kDb;
        bar.high = (bars[b][9] + bars[b][10] + bars[b][11]) / 3 * kDb;
        measured.push_back(bar);
        if (barsOut) {
            barsOut->push_back({barStarts[b], (bars[b][0] + bars[b][1]) / 2 * kDb, bar.kick, bar.energy, bar.high});
        }
    }
    const double barSecs = barStarts.size() > 1 ? barStarts[1] - barStarts[0] : 2.0;
    std::vector<size_t> starts;
    for (double secs : phraseStarts) {
        const double bar = std::round((secs - barStarts.front()) / barSecs);
        if (bar > 0) {
            starts.push_back(static_cast<size_t>(bar));
        }
    }
    std::vector<size_t> lengths;
    const auto named = sections::name(measured, starts);
    for (size_t i = 0; i < named.size(); ++i) {
        const size_t from = named[i].first;
        const size_t to = i + 1 < named.size() ? named[i + 1].first : measured.size();
        // the first part starts with the song, not at its first downbeat
        const double secs = from == 0 ? 0.0 : barStarts[from];
        sections.push_back({secs, named[i].second, static_cast<int>(to - from)});
    }
    return sections;
}

} // namespace

Result analyse(const float* samples, size_t count, int sampleRate) {
    Result result;
    if (!samples || sampleRate <= 0 || count < static_cast<size_t>(sampleRate) * 4) {
        return result;
    }
    const double duration = static_cast<double>(count) / sampleRate;
    const Envelope env = onsetEnvelope(samples, count, sampleRate);
    if (env.values.size() < 64) {
        return result;
    }
    const AttackMap attacks(samples, count, sampleRate);
    const Reader reader(env, attacks);
    const auto ranked = candidates(env.values, env.rate);
    if (ranked.empty()) {
        return result;
    }
    const Candidate& best = ranked.front();
    double rival = 0;
    for (size_t i = 1; i < ranked.size(); ++i) {
        if (!related(ranked[i].bpm, best.bpm)) {
            rival = ranked[i].acf * ranked[i].fourier;
            break;
        }
    }
    const double own = best.acf * best.fourier;
    result.confidence = own > 0 ? std::clamp((own - rival) / own, 0.0, 1.0) : 0.0;
    const auto fit = fitTrack(reader, best.bpm);
    if (!fit || fit->period <= 0) {
        return result;
    }
    double periodSecs = fit->period / env.rate;
    double phaseSecs = env.originSecs + fit->phase / env.rate;
    const auto beatsFrom = [&](double phase) {
        std::vector<double> beats;
        double k = std::ceil((0.0 - phase) / periodSecs - 1e-6);
        for (double t = phase + k * periodSecs; t < duration; t = phase + (++k) * periodSecs) {
            beats.push_back(t);
        }
        return beats;
    };
    std::vector<double> beats = beatsFrom(phaseSecs);
    const auto frames = bandFrames(samples, count, sampleRate);
    const GridPhase grid = gridPhase(frames, sampleRate, beats);
    if (grid.halfBeatOff) {
        phaseSecs += periodSecs / 2;
        beats = beatsFrom(phaseSecs);
        result.halfBeatMoved = true;
    }
    // The beat nearest the downbeat is "1"
    size_t nearest = 0;
    for (size_t i = 0; i < beats.size(); ++i) {
        if (std::fabs(beats[i] - grid.downbeatSecs) < std::fabs(beats[nearest] - grid.downbeatSecs)) {
            nearest = i;
        }
    }
    result.bpm = 60.0 / periodSecs;
    result.beats = beats;
    result.firstDownbeat = static_cast<int>(nearest % 4);
    result.phraseStarts = grid.phraseStarts;
    result.phraseCandidates = grid.candidates;
    result.sections = nameSections(frames, sampleRate, beats, nearest % 4, grid.phraseStarts, &result.bars);
    return result;
}

} // namespace djmantra::rbx
