#pragma once

#include <QVector>
#include <optional>

#include "audio/frame.h"
#include "track/bpm.h"

class BeatUtils {
  public:
    struct ConstRegion {
        mixxx::audio::FramePos firstBeat;
        mixxx::audio::FrameDiff_t beatLength;
    };

    static mixxx::Bpm calculateBpm(const QVector<mixxx::audio::FramePos>& beats,
            mixxx::audio::SampleRate sampleRate);

    static QVector<ConstRegion> retrieveConstRegions(
            const QVector<mixxx::audio::FramePos>& coarseBeats,
            mixxx::audio::SampleRate sampleRate);

    static mixxx::Bpm calculateAverageBpm(int numberOfBeats,
            mixxx::audio::SampleRate sampleRate,
            mixxx::audio::FramePos lowerFrame,
            mixxx::audio::FramePos upperFrame);

    static mixxx::Bpm makeConstBpm(
            const QVector<ConstRegion>& constantRegions,
            mixxx::audio::SampleRate sampleRate,
            mixxx::audio::FramePos* pFirstBeat);

    static mixxx::audio::FramePos adjustPhase(
            mixxx::audio::FramePos firstBeat,
            mixxx::Bpm bpm,
            mixxx::audio::SampleRate sampleRate,
            const QVector<mixxx::audio::FramePos>& beats);

    static QVector<mixxx::audio::FramePos> getBeats(const QVector<ConstRegion>& constantRegions);

    /// DJ Mantra: false when the music does not keep one tempo (a live
    /// drummer): under 55 % of the ironed beats lie within 25 ms of the
    /// constant grid and no steady region covers 15 % of the song. Then a
    /// beat map follows the music better (testing/sections/REKORDBOX_COMPARE.md:
    /// on Marko's 144 songs, beats within 40 ms of rekordbox's 89.9 -> 93.9 %).
    static bool followsOneTempo(const QVector<ConstRegion>& constantRegions,
            mixxx::Bpm bpm,
            mixxx::audio::FramePos firstBeat,
            mixxx::audio::SampleRate sampleRate);

    static std::optional<mixxx::Bpm> trySnap(mixxx::Bpm minBpm,
            mixxx::Bpm centerBpm,
            mixxx::Bpm maxBpm,
            double fraction);

    static mixxx::Bpm roundBpmWithinRange(
            mixxx::Bpm minBpm, mixxx::Bpm centerBpm, mixxx::Bpm maxBpm);
};

Q_DECLARE_TYPEINFO(BeatUtils::ConstRegion, Q_PRIMITIVE_TYPE);
