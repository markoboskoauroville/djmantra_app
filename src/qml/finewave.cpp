#include "qml/finewave.h"

#include <QtDebug>
#include <algorithm>
#include <cmath>
#include <vector>

#include "sources/audiosourcestereoproxy.h"
#include "sources/localtrackcache.h"
#include "sources/soundsourceproxy.h"
#include "track/track.h"
#include "util/samplebuffer.h"

namespace djmantra {

// static
FineWave FineWave::build(const TrackPointer& pTrack) {
    FineWave result;
    if (!pTrack) {
        return result;
    }
    mixxx::AudioSource::OpenParams openParams;
    openParams.setChannelCount(mixxx::audio::ChannelCount::stereo());
    const mixxx::AudioSourcePointer pAudioSource =
            SoundSourceProxy(pTrack, djmantra::LocalTrackCache::readPath(pTrack->getLocation()))
                    .openAudioSource(openParams);
    if (!pAudioSource) {
        qWarning() << "Fine wave: could not open" << pTrack->getLocation();
        return result;
    }
    constexpr SINT kChunkFrames = 65536;
    mixxx::AudioSourceStereoProxy proxy(pAudioSource, kChunkFrames);
    const double sampleRate = pAudioSource->getSignalInfo().getSampleRate().toDouble();
    const int framesPerPoint = std::max(1, static_cast<int>(std::lround(sampleRate / kPointsPerSecond)));
    std::vector<float> mins;
    std::vector<float> maxs;
    mins.reserve(static_cast<std::size_t>(pAudioSource->frameLength() / framesPerPoint + 1));
    maxs.reserve(mins.capacity());
    mixxx::SampleBuffer buffer(kChunkFrames * 2);
    float lo = 0, hi = 0, peak = 1e-6f;
    int inPoint = 0;
    mixxx::IndexRange remaining = pAudioSource->frameIndexRange();
    while (!remaining.empty()) {
        auto range = remaining.splitAndShrinkFront(std::min(kChunkFrames, remaining.length()));
        const auto frames = proxy.readSampleFrames(mixxx::WritableSampleFrames(
                range, mixxx::SampleBuffer::WritableSlice(buffer)));
        remaining = intersect(remaining, proxy.frameIndexRange());
        const CSAMPLE* p = frames.readableData();
        if (!p) {
            continue;
        }
        const SINT n = frames.readableLength() / 2;
        for (SINT i = 0; i < n; ++i) {
            const float v = 0.5f * (p[2 * i] + p[2 * i + 1]);
            if (inPoint == 0) {
                lo = hi = v;
            } else {
                lo = std::min(lo, v);
                hi = std::max(hi, v);
            }
            if (++inPoint == framesPerPoint) {
                mins.push_back(lo);
                maxs.push_back(hi);
                peak = std::max(peak, std::max(-lo, hi));
                inPoint = 0;
            }
        }
    }
    if (mins.empty()) {
        return result;
    }
    result.length = static_cast<int>(mins.size());
    const int rows = (result.length + kStride - 1) / kStride;
    result.image = QImage(kStride, rows, QImage::Format_RGBX8888);
    result.image.fill(QColor(128, 128, 128));
    const float scale = 127.0f / peak;
    for (int i = 0; i < result.length; ++i) {
        uchar* px = result.image.scanLine(i / kStride) + 4 * (i % kStride);
        px[0] = static_cast<uchar>(std::clamp(128.0f + mins[i] * scale, 0.0f, 255.0f));
        px[1] = static_cast<uchar>(std::clamp(128.0f + maxs[i] * scale, 0.0f, 255.0f));
        px[2] = 128;
    }
    qInfo() << "Fine wave:" << result.length << "points," << framesPerPoint << "frames each, for"
            << pTrack->getLocation();
    return result;
}

} // namespace djmantra
