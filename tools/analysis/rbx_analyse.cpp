// Offline run of src/analyzer/rbxanalysis.cpp (the rbxport port): reads 32-bit float mono samples
// (ffmpeg -f f32le -ac 1) and prints the result as JSON. Build: tools/analysis/rbx_build.sh
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "analyzer/rbxanalysis.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: rbx_analyse samples.f32 rate\n");
        return 2;
    }
    FILE* f = std::fopen(argv[1], "rb");
    if (!f) {
        return 1;
    }
    std::vector<float> samples;
    float buffer[65536];
    size_t n;
    while ((n = std::fread(buffer, sizeof(float), 65536, f)) > 0) {
        samples.insert(samples.end(), buffer, buffer + n);
    }
    std::fclose(f);
    const auto r = djmantra::rbx::analyse(samples.data(), samples.size(), std::atoi(argv[2]));
    std::printf("{\"bpm\":%.4f,\"confidence\":%.3f,\"half\":%s,\"first_downbeat\":%d,\"beats\":[",
            r.bpm, r.confidence, r.halfBeatMoved ? "true" : "false", r.firstDownbeat);
    for (size_t i = 0; i < r.beats.size(); ++i) {
        std::printf("%s%.4f", i ? "," : "", r.beats[i]);
    }
    std::printf("],\"phrases\":[");
    for (size_t i = 0; i < r.phraseStarts.size(); ++i) {
        std::printf("%s%.3f", i ? "," : "", r.phraseStarts[i]);
    }
    std::printf("],\"candidates\":[");
    for (size_t i = 0; i < r.phraseCandidates.size(); ++i) {
        std::printf("%s[%.3f,%.3f]", i ? "," : "", r.phraseCandidates[i].first, r.phraseCandidates[i].second);
    }
    std::printf("],\"bars\":[");
    for (size_t i = 0; i < r.bars.size(); ++i) {
        std::printf("%s[%.3f,%.3f,%.3f,%.3f,%.3f]", i ? "," : "", r.bars[i][0], r.bars[i][1], r.bars[i][2], r.bars[i][3], r.bars[i][4]);
    }
    std::printf("],\"sections\":[");
    for (size_t i = 0; i < r.sections.size(); ++i) {
        std::printf("%s[%.3f,\"%s\",%d]", i ? "," : "", r.sections[i].secs, r.sections[i].name.c_str(), r.sections[i].bars);
    }
    std::printf("]}\n");
    return 0;
}
