#include <QtDebug>
#include <QStringList>

#include "track/beatfactory.h"

#include <cmath>
#include "track/beatutils.h"

namespace {

const QString kRoundingVersion = QStringLiteral("V4");

} // namespace

// static
QString BeatFactory::getPreferredVersion(bool fixedTempo) {
    if (fixedTempo) {
        return BEAT_GRID_2_VERSION;
    }
    return BEAT_MAP_VERSION;
}

QString BeatFactory::getPreferredSubVersion(
        const QHash<QString, QString>& extraVersionInfo) {
    const char* kSubVersionKeyValueSeparator = "=";
    const char* kSubVersionFragmentSeparator = "|";
    QStringList fragments;

    QHashIterator<QString, QString> it(extraVersionInfo);
    while (it.hasNext()) {
        it.next();
        if (it.key().contains(kSubVersionKeyValueSeparator) ||
                it.key().contains(kSubVersionFragmentSeparator) ||
                it.value().contains(kSubVersionKeyValueSeparator) ||
                it.value().contains(kSubVersionFragmentSeparator)) {
            qDebug() << "ERROR: Your analyzer key/value contains invalid "
                        "characters:"
                     << it.key() << ":" << it.value() << "Skipping.";
            continue;
        }
        fragments << QString("%1%2%3").arg(
                it.key(), kSubVersionKeyValueSeparator, it.value());
    }

    fragments << QString("rounding%1%2")
                         .arg(kSubVersionKeyValueSeparator, kRoundingVersion);

    std::sort(fragments.begin(), fragments.end());
    return (fragments.size() > 0) ? fragments.join(kSubVersionFragmentSeparator)
                                  : "";
}

mixxx::BeatsPointer BeatFactory::makePreferredBeats(
        const QVector<mixxx::audio::FramePos>& beats,
        const QHash<QString, QString>& extraVersionInfo,
        bool fixedTempo,
        mixxx::audio::SampleRate sampleRate,
        bool allowBeatMap) {
    DEBUG_ASSERT(sampleRate.isValid());
#ifdef DEBUG_PRINT_BEATS
    for (mixxx::audio::FramePos beat : beats) {
        qDebug().noquote() << QString::number(beat.value(), 'g', 8);
    }
#endif

    QVector<BeatUtils::ConstRegion> constantRegions =
            BeatUtils::retrieveConstRegions(beats, sampleRate);
#ifdef DEBUG_PRINT_BEATS
    for (auto& region : constantRegions) {
        qDebug().noquote() << QString::number(region.firstBeat.value(), 'g', 8)
                           << QString::number(region.beatLength, 'g', 8);
    }
#endif
    if (constantRegions.isEmpty()) {
        return nullptr;
    }

    const QString version = getPreferredVersion(fixedTempo);
    const QString subVersion = getPreferredSubVersion(extraVersionInfo);

    if (version == BEAT_GRID_2_VERSION) {
        mixxx::audio::FramePos firstBeat = mixxx::audio::kStartFramePos;
        const mixxx::Bpm constBPM = BeatUtils::makeConstBpm(
                constantRegions, sampleRate, &firstBeat);
        if (firstBeat.isValid() && allowBeatMap &&
                !BeatUtils::followsOneTempo(constantRegions, constBPM, firstBeat, sampleRate)) {
            // DJ Mantra: the tempo drifts (a live drummer): a beat map
            return mixxx::Beats::fromBeatPositions(
                    sampleRate, BeatUtils::getBeats(constantRegions), subVersion);
        }
        mixxx::Bpm gridBpm = constBPM;
#ifdef __ANDROID_PORT__
        // DJ Mantra 14.10.2026 (djay compare): a drifting song on a steady grid gets its
        // AVERAGE tempo, as djay (Aum Ganesha speeds up ~110 -> 118: djay 115, the longest
        // steady stretch gave 116.7)
        if (firstBeat.isValid() && !allowBeatMap && beats.size() > 32 &&
                !BeatUtils::followsOneTempo(constantRegions, constBPM, firstBeat, sampleRate)) {
            const double span = beats.last().value() - beats.first().value();
            if (span > 0) {
                const double average = 60.0 * sampleRate.toDouble() * (beats.size() - 1) / span;
                // the same octave as the steady stretch (no half / double tempo)
                if (average > constBPM.value() * 0.75 && average < constBPM.value() * 1.33) {
                    gridBpm = mixxx::Bpm(std::round(average * 10.0) / 10.0);
                    qInfo() << "Beats: the tempo drifts; the average" << gridBpm.value()
                            << "instead of the steady stretch's" << constBPM.value();
                }
            }
        }
#endif
        if (firstBeat.isValid()) {
            firstBeat = BeatUtils::adjustPhase(firstBeat, gridBpm, sampleRate, beats);
            auto pGrid = mixxx::Beats::fromConstTempo(
                    sampleRate, firstBeat.toNearestFrameBoundary(), gridBpm, subVersion);
            return pGrid;
        } else {
            qWarning() << "Failed to create beat grid: Invalid first beat";
        }
    } else if (version == BEAT_MAP_VERSION) {
        QVector<mixxx::audio::FramePos> ironedBeats = BeatUtils::getBeats(constantRegions);
        auto pBeatMap = mixxx::Beats::fromBeatPositions(sampleRate, ironedBeats, subVersion);
        return pBeatMap;
    } else {
        qDebug() << "ERROR: Could not determine what type of beatgrid to create.";
    }
    return nullptr;
}
