// DJ Mantra: the app's BPM / beat grid / key analysis outside the app, to
// compare with rekordbox (Marko, 12.10.2026: "rekordbox is the reference ...
// see where is the catch"). The same steps as Mixxx 2.5's defaults:
//   beats: AnalyzerQueenMaryBeats (DetectionFunction COMPLEXSD + TempoTrackV2)
//          -> BeatUtils::retrieveConstRegions -> makeConstBpm (fixed tempo,
//          rounded BPM) -> adjustPhase  (src/analyzer/plugins, src/track)
//   key:   AnalyzerQueenMaryKey (GetKeyMode, 440 Hz) -> KeyUtils::calculateGlobalKey
// Input: interleaved stereo float32 on stdin (ffmpeg -f f32le -ac 2), the
// sample rate as argv[1]. Output one line: bpm first_beat_s key_id key_name
#include <dsp/keydetection/GetKeyMode.h>
#include <dsp/onsets/DetectionFunction.h>
#include <dsp/tempotracking/TempoTrackV2.h>
#include <maths/MathUtilities.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <vector>

namespace {

// buffering_utils.cpp: DownmixAndOverlapHelper
struct Downmix {
    std::vector<double> buffer;
    size_t window = 0, step = 0, pos = 0;
    std::function<void(double*)> callback;
    void init(size_t w, size_t s, std::function<void(double*)> cb) {
        window = w; step = s; pos = 0; buffer.assign(w, 0); callback = std::move(cb);
    }
    void process(const float* in, size_t frames) {
        size_t read = 0;
        while (read < frames) {
            size_t n = std::min(frames - read, window - pos);
            for (size_t i = 0; i < n; ++i) {
                buffer[pos + i] = in ? (in[(read + i) * 2] + in[(read + i) * 2 + 1]) * 0.5 : 0;
            }
            pos += n; read += n;
            if (pos == window) {
                callback(buffer.data());
                for (size_t i = 0; i < window - step; ++i) buffer[i] = buffer[i + step];
                pos -= step;
            }
        }
    }
    void finalize() {
        size_t fill = window - pos;
        process(nullptr, std::max(fill, window / 2 - 1));
    }
};

constexpr double kMaxSecsPhaseError = 0.025;
constexpr double kMaxSecsPhaseErrorSum = 0.1;
constexpr int kMaxOutliersCount = 1;
constexpr int kMinRegionBeatCount = 16;

struct Region { double firstBeat; double beatLength; };

std::vector<Region> retrieveConstRegions(const std::vector<double>& b, double sr) {
    if (b.size() < 2) return {};
    const double maxPhaseError = kMaxSecsPhaseError * sr;
    const double maxPhaseErrorSum = kMaxSecsPhaseErrorSum * sr;
    int left = 0, right = (int)b.size() - 1;
    std::vector<Region> regions;
    while (left < (int)b.size() - 1) {
        double mean = (b[right] - b[left]) / (right - left);
        int outliers = 0; double ironed = b[left]; double errSum = 0; int i = left + 1;
        for (; i <= right; ++i) {
            ironed += mean;
            double err = ironed - b[i];
            errSum += err;
            if (std::fabs(err) > maxPhaseError) {
                outliers++;
                if (outliers > kMaxOutliersCount || i == left + 1) break;
            }
            if (std::fabs(errSum) > maxPhaseErrorSum) break;
        }
        if (i > right) {
            double border = 0;
            if (right > left + 2) {
                border = std::fabs((b[left + 1] - b[left]) + (b[right] - b[right - 1]) - 2 * mean);
            }
            if (border < maxPhaseError / 2) {
                regions.push_back({b[left], mean});
                left = right; right = (int)b.size() - 1;
                continue;
            }
        }
        right--;
    }
    regions.push_back({b.back(), 0});
    return regions;
}

std::optional<double> trySnap(double mn, double c, double mx, double f) {
    double s = std::round(c * f) / f;
    if (s > mn && s < mx) return s;
    return std::nullopt;
}

double roundBpmWithinRange(double mn, double c, double mx) {
    if (auto s = trySnap(mn, c, mx, 1.0)) return *s;
    if (c < 85.0) if (auto s = trySnap(mn, c, mx, 2.0)) return *s;
    if (c > 127.0) if (auto s = trySnap(mn, c, mx, 2.0 / 3.0)) return *s;
    if (auto s = trySnap(mn, c, mx, 3.0)) return *s;
    if (auto s = trySnap(mn, c, mx, 12.0)) return *s;
    return c;
}

double makeConstBpm(const std::vector<Region>& r, double sr, double* firstBeat) {
    int mid = 0; double longestLen = 0, longestBL = 0;
    for (int i = 0; i + 1 < (int)r.size(); ++i) {
        double len = r[i + 1].firstBeat - r[i].firstBeat;
        if (len > longestLen) { longestLen = len; longestBL = r[i].beatLength; mid = i; }
    }
    if (longestLen == 0) return 0;
    int n = (int)(longestLen / longestBL + 0.5);
    double blMin = longestBL - kMaxSecsPhaseError * sr / n;
    double blMax = longestBL + kMaxSecsPhaseError * sr / n;
    int start = mid;
    for (int i = 0; i < mid; ++i) {
        double len = r[i + 1].firstBeat - r[i].firstBeat;
        int nb = (int)(len / r[i].beatLength + 0.5);
        if (nb < kMinRegionBeatCount) continue;
        double tMin = r[i].beatLength - kMaxSecsPhaseError * sr / nb;
        double tMax = r[i].beatLength + kMaxSecsPhaseError * sr / nb;
        if (longestBL > tMin && longestBL < tMax) {
            double newLen = r[mid + 1].firstBeat - r[i].firstBeat;
            double bMin = std::max(blMin, tMin), bMax = std::min(blMax, tMax);
            int maxN = (int)std::round(newLen / bMin), minN = (int)std::round(newLen / bMax);
            if (minN != maxN) continue;
            double newBL = newLen / minN;
            if (newBL > blMin && newBL < blMax) {
                longestLen = newLen; longestBL = newBL; n = minN;
                blMin = longestBL - kMaxSecsPhaseError * sr / n;
                blMax = longestBL + kMaxSecsPhaseError * sr / n;
                start = i;
                break;
            }
        }
    }
    for (int i = (int)r.size() - 2; i > mid; --i) {
        double len = r[i + 1].firstBeat - r[i].firstBeat;
        int nb = (int)(len / r[i].beatLength + 0.5);
        if (nb < kMinRegionBeatCount) continue;
        double tMin = r[i].beatLength - kMaxSecsPhaseError * sr / nb;
        double tMax = r[i].beatLength + kMaxSecsPhaseError * sr / nb;
        if (longestBL > tMin && longestBL < tMax) {
            double newLen = r[i + 1].firstBeat - r[start].firstBeat;
            double bMin = std::max(blMin, tMin), bMax = std::min(blMax, tMax);
            int maxN = (int)std::round(newLen / bMin), minN = (int)std::round(newLen / bMax);
            if (minN != maxN) continue;
            double newBL = newLen / minN;
            if (newBL > blMin && newBL < blMax) {
                longestLen = newLen; longestBL = newBL; n = minN;
                break;
            }
        }
    }
    blMin = longestBL - kMaxSecsPhaseError * sr / n;
    blMax = longestBL + kMaxSecsPhaseError * sr / n;
    double bpm = roundBpmWithinRange(60.0 * sr / blMax, 60.0 * sr / longestBL, 60.0 * sr / blMin);
    if (firstBeat) {
        *firstBeat = std::fmod(r[start].firstBeat, 60.0 * sr / bpm);
    }
    return bpm;
}

double adjustPhase(double firstBeat, double bpm, double sr, const std::vector<double>& beats) {
    const double bl = 60 * sr / bpm;
    const double startOffset = std::fmod(firstBeat, bl);
    double adj = 0, count = 0;
    for (double b : beats) {
        double off = std::fmod(b - startOffset, bl);
        if (off > bl / 2) off -= bl;
        if (std::fabs(off) < kMaxSecsPhaseError * sr) { adj += off; count++; }
    }
    return firstBeat + (count > 0 ? adj / count : 0);
}

const char* kKeyNames[] = {"?", "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B",
        "Cm", "C#m", "Dm", "Ebm", "Em", "Fm", "F#m", "Gm", "G#m", "Am", "Bbm", "Bm"};

} // namespace

int main(int argc, char** argv) {
    const double sr = argc > 1 ? std::atof(argv[1]) : 44100;
    // beats
    const float kStepSecs = 0.01161f;
    const int step = static_cast<int>(sr * kStepSecs);
    const int window = MathUtilities::nextPowerOfTwo(static_cast<int>(sr / 50));
    DFConfig cfg;
    cfg.DFType = DF_COMPLEXSD; cfg.stepSize = step; cfg.frameLength = window; cfg.dbRise = 3;
    cfg.adaptiveWhitening = false; cfg.whiteningRelaxCoeff = -1; cfg.whiteningFloor = -1;
    DetectionFunction df(cfg);
    std::vector<double> results;
    Downmix beatHelper;
    beatHelper.init(window, step, [&](double* w) { results.push_back(df.processTimeDomain(w)); });
    // key
    GetKeyMode::Config kc(sr, 440);
    // experiments (the app uses the defaults): KEY_OVERLAP, KEY_HPCP, KEY_MEDIAN
    if (getenv("KEY_OVERLAP")) kc.frameOverlapFactor = atoi(getenv("KEY_OVERLAP"));
    if (getenv("KEY_HPCP")) kc.hpcpAverage = atof(getenv("KEY_HPCP"));
    if (getenv("KEY_MEDIAN")) kc.medianAverage = atof(getenv("KEY_MEDIAN"));
    GetKeyMode keyMode(kc);
    std::vector<std::pair<int, double>> keys;
    long long frame = 0;
    int prevKey = 0;
    Downmix keyHelper;
    keyHelper.init(keyMode.getBlockSize(), keyMode.getHopSize(), [&](double* w) {
        int k = keyMode.process(w);
        if (k >= 1 && k <= 24 && k != prevKey) { keys.push_back({k, (double)frame}); prevKey = k; }
    });
    std::vector<float> buf(2 * 4096);
    size_t got;
    while ((got = fread(buf.data(), sizeof(float), buf.size(), stdin)) > 0) {
        size_t frames = got / 2;
        frame += frames;  // as AnalyzerQueenMaryKey: counted before processing
        beatHelper.process(buf.data(), frames);
        keyHelper.process(buf.data(), frames);
    }
    beatHelper.finalize();
    keyHelper.finalize();
    // finalize beats
    int nonZero = (int)results.size();
    while (nonZero > 0 && results[nonZero - 1] <= 0.0) --nonZero;
    std::vector<double> d, period, tempi, beats;
    for (int i = 2; i < nonZero; ++i) { d.push_back(results[i]); period.push_back(0.0); }
    TempoTrackV2 tt(sr, step);
    tt.calculateBeatPeriod(d, period, tempi);
    tt.calculateBeats(d, period, beats);
    std::vector<double> beatFrames;
    for (double b : beats) beatFrames.push_back(b * step + step / 2);
    double bpm = 0, first = 0;
    auto regions = retrieveConstRegions(beatFrames, sr);
    if (!regions.empty()) {
        bpm = makeConstBpm(regions, sr, &first);
        if (bpm > 0) first = adjustPhase(first, bpm, sr, beatFrames);
    }
    // global key: the longest total duration
    std::map<int, double> hist;
    for (size_t i = 0; i < keys.size(); ++i) {
        double next = i + 1 < keys.size() ? keys[i + 1].second : (double)frame;
        hist[keys[i].first] += next - keys[i].second;
    }
    int key = 0; double best = 0;
    if (keys.size() == 1) key = keys[0].first;
    else for (auto& [k, v] : hist) if (v > best) { best = v; key = k; }
    printf("%.4f %.4f %d %s\n", bpm, first / sr, key, kKeyNames[key]);
    // BEATS=1: the ironed beat map too (Mixxx BeatUtils::getBeats, "assume constant tempo" off),
    // one line "B <seconds>" per beat, and the regions "G <first s> <beat length s>"
    if (getenv("BEATS")) {
        for (size_t i = 0; i + 1 < regions.size(); ++i) {
            printf("G %.4f %.5f\n", regions[i].firstBeat / sr, regions[i].beatLength / sr);
            for (double b = regions[i].firstBeat; b < regions[i + 1].firstBeat - 100; b += regions[i].beatLength) {
                printf("B %.4f\n", b / sr);
            }
        }
    }
    return 0;
}
