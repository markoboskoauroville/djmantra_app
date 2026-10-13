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
        drawDjay(pPainter, pWaveform, std::min(waveformCompletion, dataSize));
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
// middle, a darker outline from the local peak and a brighter body from the level,
// each column in djay's colour; the song's loud parts fill the height.
void QmlWaveformOverview::drawDjay(QPainter* pPainter,
        ConstWaveformPointer pWaveform,
        int completion) const {
    const int points = completion / 2;
    if (points < 2 || width() <= 0 || height() <= 0) {
        return;
    }
    std::vector<double> level(points);
    std::vector<int> sorted(points);
    for (int i = 0; i < points; ++i) {
        level[i] = (pWaveform->getAll(2 * i) + pWaveform->getAll(2 * i + 1)) / 2.0;
        sorted[i] = static_cast<int>(level[i]);
    }
    const auto at = [&](int percent) {
        auto nth = sorted.begin() + static_cast<std::ptrdiff_t>((points - 1) * percent / 100);
        std::nth_element(sorted.begin(), nth, sorted.end());
        return std::max(1, *nth);
    };
    const double colourScale = 1.0 / at(99); // the colour's inputs, as fitted
    const double heightScale = 0.95 / at(97); // the loud parts fill the height
    const double middle = height() / 2;
    const double step = width() / points;
    pPainter->save();
    pPainter->setClipRect(boundingRect());
    pPainter->setRenderHint(QPainter::Antialiasing, false);
    QPen pen;
    pen.setWidthF(std::max(1.0, step + 0.5));
    for (int i = 0; i < points; ++i) {
        double peak = level[i];
        for (int j = std::max(0, i - 2); j <= std::min(points - 1, i + 2); ++j) {
            peak = std::max(peak, level[j]);
        }
        const QColor colour = djmantra::djayColor(
                (pWaveform->getLow(2 * i) + pWaveform->getLow(2 * i + 1)) / 2.0 * colourScale,
                (pWaveform->getMid(2 * i) + pWaveform->getMid(2 * i + 1)) / 2.0 * colourScale,
                (pWaveform->getHigh(2 * i) + pWaveform->getHigh(2 * i + 1)) / 2.0 * colourScale,
                level[i] * colourScale);
        const double x = (i + 0.5) * step;
        const double outer = std::min(1.0, peak * heightScale) * middle;
        const double inner = std::min(1.0, level[i] * heightScale) * middle * 0.7;
        pen.setColor(colour.darker(170));
        pPainter->setPen(pen);
        pPainter->drawLine(QPointF(x, middle - outer), QPointF(x, middle + outer));
        pen.setColor(colour.lighter(115));
        pPainter->setPen(pen);
        pPainter->drawLine(QPointF(x, middle - inner), QPointF(x, middle + inner));
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
