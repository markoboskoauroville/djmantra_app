#pragma once

#include <QPainter>
#include <QPointer>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickPaintedItem>

#include "qml/qmlplayerproxy.h"
#include "track/track.h"

namespace mixxx {
namespace qml {


class QmlWaveformOverview : public QQuickPaintedItem {
    Q_OBJECT
    Q_FLAGS(Channels)
    Q_PROPERTY(mixxx::qml::QmlPlayerProxy* player READ getPlayer WRITE setPlayer
                    NOTIFY playerChanged REQUIRED)
    Q_PROPERTY(Channels channels READ getChannels WRITE setChannels NOTIFY channelsChanged)
    Q_PROPERTY(Renderer renderer MEMBER m_renderer NOTIFY rendererChanged)
    Q_PROPERTY(QColor colorHigh MEMBER m_colorHigh NOTIFY colorHighChanged)
    Q_PROPERTY(QColor colorMid MEMBER m_colorMid NOTIFY colorMidChanged)
    Q_PROPERTY(QColor colorLow MEMBER m_colorLow NOTIFY colorLowChanged)
    /// DJ Mantra (round 18: "the contrast is not good"): the song's loudest
    /// point fills the height, so quiet and loud parts stand apart
    Q_PROPERTY(bool normalize MEMBER m_normalize NOTIFY normalizeChanged)
    /// DJ Mantra V78: the djay renderer as 3-BAND (sections: bass, mids and highs as nested
    /// layers) or, false, ICON (one colour per bar, V77)
    Q_PROPERTY(bool threeBand MEMBER m_threeBand NOTIFY threeBandChanged)
    QML_NAMED_ELEMENT(WaveformOverview)

  public:
    enum class ChannelFlag : int {
        LeftChannel = 1,
        RightChannel = 2,
        BothChannels = LeftChannel | RightChannel,
    };
    Q_DECLARE_FLAGS(Channels, ChannelFlag)

    enum class Renderer {
        RGB = 1,
        Filtered = 2,
        Djay = 3, // DJ Mantra: djay Pro's look (src/waveform/djaycolor.h)
    };
    Q_ENUM(Renderer)

    QmlWaveformOverview(QQuickItem* parent = nullptr);
    ~QmlWaveformOverview() override = default;

    void paint(QPainter* painter) override;

    void setPlayer(QmlPlayerProxy* player);
    QmlPlayerProxy* getPlayer() const;

    void setChannels(Channels channels);
    Channels getChannels() const;
  private slots:
    void slotTrackLoaded(TrackPointer pLoadedTrack);
    void slotTrackLoading(TrackPointer pNewTrack, TrackPointer pOldTrack);
    void slotTrackUnloaded();
    void slotWaveformUpdated();

  signals:
    void playerChanged();
    void channelsChanged(mixxx::qml::QmlWaveformOverview::Channels channels);
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    void rendererChanged(Renderer renderer);
#else
    void rendererChanged(mixxx::qml::QmlWaveformOverview::Renderer renderer);
#endif
    void colorHighChanged(const QColor& color);
    void colorMidChanged(const QColor& color);
    void colorLowChanged(const QColor& color);
    void normalizeChanged(bool normalize);
    void threeBandChanged(bool threeBand);

  private:
    void setCurrentTrack(TrackPointer pTrack);
    void drawDjay(QPainter* pPainter, ConstWaveformPointer pWaveform, int completion) const;
    void drawFiltered(QPainter* pPainter,
            Channels channels,
            ConstWaveformPointer pWaveform,
            int completion) const;
    void drawRgb(QPainter* pPainter,
            Channels channels,
            ConstWaveformPointer pWaveform,
            int completion) const;
    QColor getRgbPenColor(ConstWaveformPointer pWaveform, int completion) const;

    QPointer<QmlPlayerProxy> m_pPlayer;
    TrackPointer m_pCurrentTrack;
    Channels m_channels;
    Renderer m_renderer;
    QColor m_colorHigh;
    QColor m_colorMid;
    QColor m_colorLow;
    bool m_normalize = false;
    bool m_threeBand = true;
};

} // namespace qml
} // namespace mixxx
