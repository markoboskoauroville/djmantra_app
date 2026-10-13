#pragma once

#include <QImage>

#include "track/track_decl.h"

/// DJ Mantra: the real sound wave for the lanes' deep zoom (Marko 13.10.2026: "yes",
/// djay draws each kick's wiggle when zoomed in). Mixxx's waveform keeps ~441 peaks a
/// second without sign; this decodes the song once more (in the background, when a
/// deck loads it) into ~2000 signed min / max points a second, mono, scaled to the
/// song's peak: a texture of 4096-wide rows, R = min, G = max (128 = 0).
namespace djmantra {

struct FineWave {
    static constexpr int kStride = 4096;
    static constexpr int kPointsPerSecond = 2000;

    QImage image;   // RGBX, kStride wide; empty if the song could not be read
    int length = 0; // points
    /// Decodes pTrack's file (slow: call it off the GUI thread)
    static FineWave build(const TrackPointer& pTrack);
};

} // namespace djmantra
