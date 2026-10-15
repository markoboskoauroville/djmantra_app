#include "qml/qmlwaveformoverview.h"

#include <algorithm>
#include <vector>

#include "mixer/basetrackplayer.h"
#include "waveform/djaycolor.h"
#include "moc_qmlwaveformoverview.cpp"

namespace {
constexpr double kDesiredChannelHeight = 255;
} // namespace

namespace mixxx {
namespace qml {

QmlWaveformOverview::QmlWaveformOverview(QQuickItem* parent)
        : QQuickPaintedItem(parent),
          m_pPlayer(nullptr),
          m_channels(ChannelFlag::BothChannels),
          m_renderer(Renderer::RGB),
          m_colorHigh(0xFF0000),
          m_colorMid(0x00FF00),
          m_colorLow(0x0000FF) {
    connect(this, &QmlWaveformOverview::threeBandChanged, this, [this] {
        update();
    });
    connect(this, &QmlWaveformOverview::djayMixChanged, this, [this] {
        update();
    });
}

QmlPlayerProxy* QmlWaveformOverview::getPlayer() const {
    return m_pPlayer;
}

void QmlWaveformOverview::setPlayer(QmlPlayerProxy* pPlayer) {
    if (m_pPlayer == pPlayer) {
        return;
    }

    if (m_pPlayer != nullptr) {
        m_pPlayer->internalTrackPlayer()->disconnect(this);
    }

    m_pPlayer = pPlayer;

    if (m_pPlayer != nullptr) {
        setCurrentTrack(m_pPlayer->internalTrackPlayer()->getLoadedTrack());
        connect(m_pPlayer->internalTrackPlayer(),
                &BaseTrackPlayer::newTrackLoaded,
                this,
                &QmlWaveformOverview::slotTrackLoaded);
        connect(m_pPlayer->internalTrackPlayer(),
                &BaseTrackPlayer::loadingTrack,
                this,
                &QmlWaveformOverview::slotTrackLoading);
        connect(m_pPlayer->internalTrackPlayer(),
                &BaseTrackPlayer::playerEmpty,
                this,
                &QmlWaveformOverview::slotTrackUnloaded);
    }

    emit playerChanged();
    update();
}

QmlWaveformOverview::Channels QmlWaveformOverview::getChannels() const {
    return m_channels;
}

void QmlWaveformOverview::setChannels(QmlWaveformOverview::Channels channels) {
    if (m_channels == channels) {
        return;
    }

    m_channels = channels;
    emit channelsChanged(channels);
}

void QmlWaveformOverview::slotTrackLoaded(TrackPointer pTrack) {
    // TODO: Investigate if it's a bug that this debug assertion fails when
    // passing tracks on the command line
    // DEBUG_ASSERT(m_pCurrentTrack == pTrack);
    setCurrentTrack(pTrack);
}

void QmlWaveformOverview::slotTrackLoading(TrackPointer pNewTrack, TrackPointer pOldTrack) {
    Q_UNUSED(pOldTrack); // only used in DEBUG_ASSERT
    DEBUG_ASSERT(m_pCurrentTrack == pOldTrack);
    setCurrentTrack(pNewTrack);
}

void QmlWaveformOverview::slotTrackUnloaded() {
    setCurrentTrack(nullptr);
}

void QmlWaveformOverview::setCurrentTrack(TrackPointer pTrack) {
    // TODO: Check if this is actually possible
    if (m_pCurrentTrack == pTrack) {
        return;
    }

    if (m_pCurrentTrack != nullptr) {
        disconnect(m_pCurrentTrack.get(), nullptr, this, nullptr);
    }

    m_pCurrentTrack = pTrack;
    if (pTrack != nullptr) {
        connect(pTrack.get(),
                &Track::waveformSummaryUpdated,
                this,
                &QmlWaveformOverview::slotWaveformUpdated);
        // DJ Mantra: the djay look draws from the full waveform
        connect(pTrack.get(),
                &Track::waveformUpdated,
                this,
                &QmlWaveformOverview::slotWaveformUpdated);
    }
    slotWaveformUpdated();
}

void QmlWaveformOverview::slotWaveformUpdated() {
    update();
}

void QmlWaveformOverview::paint(QPainter* pPainter) {
    TrackPointer pTrack = m_pCurrentTrack;
    if (!pTrack) {
        return;
    }

    ConstWaveformPointer pWaveform = pTrack->getWaveformSummary();
    if (!pWaveform) {
        return;
    }

    const int dataSize = pWaveform->getDataSize();
    if (dataSize == 0) {
        return;
    }

    constexpr int actualCompletion = 0;
    // Always multiple of 2
    const int waveformCompletion = pWaveform->getCompletion();
    // Test if there is some new to draw (at least of pixel width)
    const int completionIncrement = waveformCompletion - actualCompletion;

    const qreal desiredWidth = static_cast<qreal>(dataSize) / 2;
    const double visiblePixelIncrement = completionIncrement * desiredWidth / dataSize;
    if (waveformCompletion < (dataSize - 2) &&
            (completionIncrement < 2 || visiblePixelIncrement == 0)) {
        return;
    }

    const int nextCompletion = actualCompletion + completionIncrement;

    if (m_renderer == Renderer::Djay) {
        // the full waveform once analysed (the summary averages the colours away)
        const ConstWaveformPointer pFull = pTrack->getWaveform();
        if (pFull && pFull->getDataSize() > 0 &&
                pFull->getCompletion() >= pFull->getDataSize()) {
            drawDjay(pPainter, pFull, pFull->getDataSize());
        } else {
            drawDjay(pPainter, pWaveform, std::min(waveformCompletion, dataSize));
        }
        return;
    }

    const Channels channels = m_channels;
    // DJ Mantra: the song's loud parts fill the height (round 18, Marko: "the
    // waveform needs to be much bigger"): the 97th percentile of the drawn
    // values reaches the top, the few peaks above it are clipped
    double gain = 1.0;
    if (m_normalize) {
        std::vector<int> values;
        values.reserve(std::max(0, waveformCompletion));
        for (int i = 0; i < waveformCompletion && i < dataSize; ++i) {
            if (m_renderer == Renderer::Filtered) {
                values.push_back(std::max({static_cast<int>(pWaveform->getLow(i)),
                        static_cast<int>(pWaveform->getMid(i) * 1.5),
                        static_cast<int>(pWaveform->getHigh(i) * 2)}));
            } else {
                values.push_back(pWaveform->getAll(i));
            }
        }
        if (!values.empty()) {
            const auto nth = values.begin() + static_cast<std::ptrdiff_t>(values.size() * 97 / 100);
            std::nth_element(values.begin(), nth, values.end());
            if (*nth > 0) {
                gain = std::min(6.0, kDesiredChannelHeight / *nth);
            }
        }
        pPainter->setClipRect(boundingRect());
    }
    pPainter->save();

    switch (channels) {
    case static_cast<int>(ChannelFlag::LeftChannel):
        // Draw both channels.
        // Set the y axis to half the height of the item
        pPainter->translate(0.0, height());
        // Set the x axis to half the height of the item
        pPainter->scale(width() / desiredWidth, gain * height() / kDesiredChannelHeight);
        break;
    case static_cast<int>(ChannelFlag::RightChannel):
        // Set the x axis to half the height of the item
        pPainter->scale(width() / desiredWidth, gain * height() / kDesiredChannelHeight);
        break;
    default:
        // Draw both channels.
        // Set the y axis to half the height of the item
        pPainter->translate(0.0, height() / 2);
        // Set the x axis to half the height of the item
        pPainter->scale(width() / desiredWidth, gain * height() / (2 * kDesiredChannelHeight));
    }

    Renderer renderer = m_renderer;
    for (int currentCompletion = actualCompletion;
            currentCompletion < nextCompletion;
            currentCompletion += 2) {
        switch (renderer) {
        case Renderer::Filtered:
            drawFiltered(pPainter, channels, pWaveform, currentCompletion);
            break;
        default:
            drawRgb(pPainter, channels, pWaveform, currentCompletion);
        }
    }
    pPainter->restore();
}

void QmlWaveformOverview::drawRgb(QPainter* pPainter,
        Channels channels,
        ConstWaveformPointer pWaveform,
        int completion) const {
    const double offsetX = completion / 2.0;

    if (channels.testFlag(ChannelFlag::LeftChannel)) {
        // Draw left channel
        const QColor leftColor = getRgbPenColor(pWaveform, completion);
        if (leftColor.isValid()) {
            const uint8_t leftValue = pWaveform->getAll(completion);
            pPainter->setPen(leftColor);
            pPainter->drawLine(QPointF(offsetX, -leftValue), QPointF(offsetX, 0.0));
        }
    }

    if (channels.testFlag(ChannelFlag::RightChannel)) {
        // Draw right channel
        QColor rightColor = getRgbPenColor(pWaveform, completion + 1);
        if (rightColor.isValid()) {
            const uint8_t rightValue = pWaveform->getAll(completion + 1);
            pPainter->setPen(rightColor);
            pPainter->drawLine(QPointF(offsetX, 0.0), QPointF(offsetX, rightValue));
        }
    }
}

void QmlWaveformOverview::drawFiltered(QPainter* pPainter,
        Channels channels,
        ConstWaveformPointer pWaveform,
        int completion) const {
    const double offsetX = completion / 2.0;

    if (channels.testFlag(ChannelFlag::LeftChannel)) {
        const uint8_t leftHigh = pWaveform->getHigh(completion);
        pPainter->setPen(m_colorHigh);
        pPainter->drawLine(QPointF(offsetX, 2 * -leftHigh), QPointF(offsetX, 0.0));

        const uint8_t leftMid = pWaveform->getMid(completion);
        pPainter->setPen(m_colorMid);
        pPainter->drawLine(QPointF(offsetX, 1.5 * -leftMid), QPointF(offsetX, 0.0));

        const uint8_t leftLow = pWaveform->getLow(completion);
        pPainter->setPen(m_colorLow);
        pPainter->drawLine(QPointF(offsetX, -leftLow), QPointF(offsetX, 0.0));
    }

    if (channels.testFlag(ChannelFlag::RightChannel)) {
        const uint8_t rightHigh = pWaveform->getHigh(completion + 1);
        pPainter->setPen(m_colorHigh);
        pPainter->drawLine(QPointF(offsetX, 0), QPointF(offsetX, 2 * rightHigh));

        const uint8_t rightMid = pWaveform->getMid(completion + 1) * 2;
        pPainter->setPen(m_colorMid);
        pPainter->drawLine(QPointF(offsetX, 0), QPointF(offsetX, 1.5 * rightMid));

        const uint8_t rightLow = pWaveform->getLow(completion + 1);
        pPainter->setPen(m_colorLow);
        pPainter->drawLine(QPointF(offsetX, 0), QPointF(offsetX, rightLow));
    }
}

// DJ Mantra: djay Pro's overview (testing/djay/screenshots): mirrored around the
// middle, a darker outline (the peak) and a brighter body (the level), on #141414.
// Round 31: averaged colours came out beige / grey; djay takes each column's colour
// from one moment (its colours are per 5.8 ms) and its peaks from short windows
// (~1/18 s, its overview's rate), so the columns read saturated and spiky. Here:
// per column a window of ~1/18 s of the full waveform, its colour from the loudest
// point in it, its outline from that point and its body from the window's mean.
void QmlWaveformOverview::drawDjay(QPainter* pPainter,
        ConstWaveformPointer pWaveform,
        int completion) const {
    const int points = completion / 2;
    const int columns = static_cast<int>(width());
    if (points < 2 || columns < 2 || height() <= 0) {
        return;
    }
    const auto all = [&](int i) {
        return (pWaveform->getAll(2 * i) + pWaveform->getAll(2 * i + 1)) / 2.0;
    };
    const double perColumn = static_cast<double>(points) / columns;
    const double seconds = m_pCurrentTrack ? m_pCurrentTrack->getDuration() : 0.0;
    const int window = std::max(1,
            static_cast<int>(seconds > 0 ? points / (seconds * 17.7) : perColumn));
    struct Column {
        int loudest;
        double peak;
        double mean;
        double bands; // max(low, mid, high) at the loudest point (the colour's scale)
    };
    std::vector<Column> cols(columns);
    std::vector<int> peaks(columns);
    std::vector<int> bandPeaks(columns);
    for (int x = 0; x < columns; ++x) {
        const int centre = static_cast<int>((x + 0.5) * perColumn);
        const int from = std::clamp(centre - window / 2, 0, points - 1);
        const int to = std::clamp(from + window, from + 1, points);
        Column c{from, 0.0, 0.0, 0.0};
        for (int i = from; i < to; ++i) {
            const double v = all(i);
            c.mean += v;
            if (v > c.peak) {
                c.peak = v;
                c.loudest = i;
            }
        }
        c.mean /= (to - from);
        const int i = c.loudest;
        c.bands = std::max({(pWaveform->getLow(2 * i) + pWaveform->getLow(2 * i + 1)) / 2.0,
                (pWaveform->getMid(2 * i) + pWaveform->getMid(2 * i + 1)) / 2.0,
                (pWaveform->getHigh(2 * i) + pWaveform->getHigh(2 * i + 1)) / 2.0});
        cols[x] = c;
        peaks[x] = static_cast<int>(c.peak);
        bandPeaks[x] = static_cast<int>(c.bands);
    }
    const auto at = [](std::vector<int> v, int percent) {
        auto nth = v.begin() + static_cast<std::ptrdiff_t>((v.size() - 1) * percent / 100);
        std::nth_element(v.begin(), nth, v.end());
        return std::max(1, *nth);
    };
    const double colourScale = 1.0 / at(bandPeaks, 99); // the colour's inputs, as fitted
    const double heightScale = 0.8 / at(peaks, 97);     // the loud parts reach 4/5
    const double middle = height() / 2;
    // V77 (Marko 15.10.2026: as sharp as the launcher icon's waveform): solid bars, 2 px wide
    // with a 1 px black gap, each ONE colour (djmantra::spectralColor) and as tall as the
    // loudest point under it, mirrored; edges on whole pixels, no outline, no translucency
    constexpr int kPitch = 3;
    constexpr double kBar = 2.0;
    pPainter->save();
    pPainter->setClipRect(boundingRect());
    pPainter->setRenderHint(QPainter::Antialiasing, false);
    pPainter->setPen(Qt::NoPen);
    if (m_djayMix) {
        // DJAY (V80, Marko 15.10.2026, djay One Deck screenshots): as the scrolling waveform's
        // DJAY style, a column per pixel, no gaps, its colour the mix of the bands at its
        // loudest moment (bass red + mids green + highs blue, brightest at full), mirrored and
        // as tall as the loudest point
        for (int x = 0; x < columns; ++x) {
            const Column& c = cols[x];
            const int i = c.loudest;
            const double low = (pWaveform->getLow(2 * i) + pWaveform->getLow(2 * i + 1)) / 2.0;
            const double mid = (pWaveform->getMid(2 * i) + pWaveform->getMid(2 * i + 1)) / 2.0;
            const double high =
                    (pWaveform->getHigh(2 * i) + pWaveform->getHigh(2 * i + 1)) / 2.0 * 0.6;
            const double r = low * 1.0 + mid * 0.22 + high * 0.22;
            const double g = low * 0.16 + mid * 0.90 + high * 0.45;
            const double b = low * 0.10 + mid * 0.22 + high * 1.0;
            const double strongest = std::max({r, g, b});
            const QColor colour = strongest > 0
                    ? QColor::fromRgbF(static_cast<float>(r / strongest),
                              static_cast<float>(g / strongest),
                              static_cast<float>(b / strongest))
                    : QColor(48, 48, 48);
            const double outer =
                    std::max(0.5, std::round(std::min(1.0, c.peak * heightScale) * middle));
            pPainter->fillRect(QRectF(x, std::round(middle - outer), 1.0, 2 * outer), colour);
        }
        pPainter->restore();
        return;
    }
    if (m_threeBand) {
        // 3-BAND (V78, Marko 15.10.2026: "sections must be clearly seen through the waveform
        // shape and colors"): as res/shaders/djaysignal_qml.frag, three nested layers per bar,
        // bass red at the back, mids green, highs blue-violet in front, each band's height its
        // level over the bar (half its RMS, half its peak) times its measured reach
        const QColor kBass(237, 33, 43);
        const QColor kMids(41, 219, 140);
        const QColor kHighs(115, 122, 255);
        constexpr double kReach[3] = {2.4, 2.6, 0.5};
        // as the scrolling waveform's lane at heightGain 0.62, scaled to this lane
        constexpr double kGain = 0.62;
        for (int x0 = 0; x0 + 1 < columns; x0 += kPitch) {
            const int from = std::clamp(static_cast<int>(x0 * perColumn), 0, points - 1);
            const int to = std::clamp(static_cast<int>((x0 + kPitch) * perColumn), from + 1, points);
            double peak[3] = {0, 0, 0};
            double square[3] = {0, 0, 0};
            // at most ~64 reads a bar
            const int stride = std::max(1, (to - from) / 64);
            int count = 0;
            for (int i = from; i < to; i += stride) {
                const double band[3] = {
                        (pWaveform->getLow(2 * i) + pWaveform->getLow(2 * i + 1)) / 2.0,
                        (pWaveform->getMid(2 * i) + pWaveform->getMid(2 * i + 1)) / 2.0,
                        (pWaveform->getHigh(2 * i) + pWaveform->getHigh(2 * i + 1)) / 2.0};
                for (int b = 0; b < 3; ++b) {
                    peak[b] = std::max(peak[b], band[b]);
                    square[b] += band[b] * band[b];
                }
                ++count;
            }
            const QColor colours[3] = {kBass, kMids, kHighs};
            for (int b = 0; b < 3; ++b) {
                const double level = 0.5 * (std::sqrt(square[b] / std::max(count, 1)) + peak[b]) *
                        colourScale;
                const double h = std::round(std::min(1.0, level * kReach[b] * kGain) * middle);
                if (h >= 0.5) {
                    pPainter->fillRect(QRectF(x0, std::round(middle - h), kBar, 2 * h), colours[b]);
                }
            }
        }
        pPainter->restore();
        return;
    }
    for (int x0 = 0; x0 + 1 < columns; x0 += kPitch) {
        // the loudest of the bar's columns
        int best = x0;
        for (int x = x0 + 1; x < std::min(columns, x0 + kPitch); ++x) {
            if (cols[x].peak > cols[best].peak) {
                best = x;
            }
        }
        const Column& c = cols[best];
        const int i = c.loudest;
        const QColor colour = djmantra::spectralColor(
                (pWaveform->getLow(2 * i) + pWaveform->getLow(2 * i + 1)) / 2.0 * colourScale,
                (pWaveform->getMid(2 * i) + pWaveform->getMid(2 * i + 1)) / 2.0 * colourScale,
                (pWaveform->getHigh(2 * i) + pWaveform->getHigh(2 * i + 1)) / 2.0 * colourScale);
        const double outer = std::max(0.5, std::round(std::min(1.0, c.peak * heightScale) * middle));
        pPainter->fillRect(QRectF(x0, std::round(middle - outer), kBar, 2 * outer), colour);
    }
    pPainter->restore();
}

QColor QmlWaveformOverview::getRgbPenColor(ConstWaveformPointer pWaveform, int completion) const {
    // Retrieve "raw" LMH values from waveform
    qreal low = static_cast<qreal>(pWaveform->getLow(completion));
    qreal mid = static_cast<qreal>(pWaveform->getMid(completion));
    qreal high = static_cast<qreal>(pWaveform->getHigh(completion));

    // Do matrix multiplication
    qreal red = low * m_colorLow.redF() + mid * m_colorMid.redF() + high * m_colorHigh.redF();
    qreal green = low * m_colorLow.greenF() + mid * m_colorMid.greenF() +
            high * m_colorHigh.greenF();
    qreal blue = low * m_colorLow.blueF() + mid * m_colorMid.blueF() + high * m_colorHigh.blueF();

    // Normalize and draw
    qreal max = math_max3(red, green, blue);
    if (max > 0.0) {
        QColor color;
        color.setRgbF(
                static_cast<float>(red / max),
                static_cast<float>(green / max),
                static_cast<float>(blue / max));
        return color;
    }
    return QColor();
}

} // namespace qml
} // namespace mixxx
