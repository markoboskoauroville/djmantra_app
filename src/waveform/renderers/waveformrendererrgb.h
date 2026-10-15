#pragma once

#include "util/class.h"
#include "waveformrenderersignalbase.h"

class WaveformRendererRGB : public WaveformRendererSignalBase {
  public:
    explicit WaveformRendererRGB(
        WaveformWidgetRenderer* waveformWidget);
    virtual ~WaveformRendererRGB();

    virtual void onSetup(const QDomNode& node);
    virtual void draw(QPainter* painter, QPaintEvent* event);

  private:
    // DJ Mantra (skin <SignalFullHeight>true</SignalFullHeight>): the loudest
    // band reaches the edge, as in djay (Mixxx scales by all three bands)
    bool m_fullHeight = false;
    DISALLOW_COPY_AND_ASSIGN(WaveformRendererRGB);
};
