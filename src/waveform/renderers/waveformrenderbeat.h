#pragma once

#include <QColor>

#include "skin/legacy/skincontext.h"
#include "util/class.h"
#include "waveform/renderers/waveformrendererabstract.h"

class WaveformRenderBeat : public WaveformRendererAbstract {
  public:
    explicit WaveformRenderBeat(WaveformWidgetRenderer* waveformWidgetRenderer);
    virtual ~WaveformRenderBeat();

    virtual void setup(const QDomNode& node, const SkinContext& context);
    virtual void draw(QPainter* painter, QPaintEvent* event);

  private:
    QColor m_beatColor;
    QVector<QLineF> m_beats;
    // DJ Mantra (skin <BarNumbers>true</BarNumbers>): the bar number at the
    // top of every bar's first beat, as djay shows it
    bool m_barNumbers = false;
    QColor m_barNumberColor;

    DISALLOW_COPY_AND_ASSIGN(WaveformRenderBeat);
};
