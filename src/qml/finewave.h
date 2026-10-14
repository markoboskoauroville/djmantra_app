#pragma once

#include <QHash>
#include <QImage>
#include <QObject>
#include <QSet>
#include <memory>

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

/// One build per song for every view (round 39: each waveform view has its own player
/// proxy, so the song was decoded twice): the last few songs' waves, by file location.
class FineWaveCache : public QObject {
    Q_OBJECT
  public:
    static FineWaveCache* instance();
    /// The wave if built; else starts the build (once) and returns null: ready() follows
    std::shared_ptr<const FineWave> request(const TrackPointer& pTrack);

  signals:
    void ready(const QString& location);

  private:
    QHash<QString, std::shared_ptr<const FineWave>> m_waves;
    QList<QString> m_order; // oldest first
    QSet<QString> m_building;
};

} // namespace djmantra
