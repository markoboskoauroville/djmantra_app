#include "waveform/widgets/waveformwidgetabstract.h"

#include <QWidget>

#include "waveform/renderers/waveformwidgetrenderer.h"

WaveformWidgetAbstract::WaveformWidgetAbstract(const QString& group)
        : WaveformWidgetRenderer(group),
          m_initSuccess(false) {
    m_widget = nullptr;
}

WaveformWidgetAbstract::~WaveformWidgetAbstract() {
}

void WaveformWidgetAbstract::hold() {
    if (m_widget) {
        m_widget->hide();
    }
}

void WaveformWidgetAbstract::release() {
    if (m_widget) {
        m_widget->show();
    }
}

void WaveformWidgetAbstract::preRender(VSyncThread* vsyncThread) {
    WaveformWidgetRenderer::onPreRender(vsyncThread);
}

mixxx::Duration WaveformWidgetAbstract::render() {
    if (m_widget) {
#ifdef __ANDROID_PORT__
        // DJ Mantra: an immediate repaint() on Android paints into a null
        // device (hundreds of "QPainter: Painter not active" warnings per
        // frame, which slowed every touch, round 6): schedule it instead
        // The renderer can keep an older size (a page of a stack shown
        // again, the system bars coming and going): the waveform was drawn
        // into the top part only, the rest stale ("3 waveforms", 10.10.2026)
        const float dpr = static_cast<float>(m_widget->devicePixelRatioF());
        if (getWidth() != m_widget->width() || getHeight() != m_widget->height() ||
                getDevicePixelRatio() != dpr) {
            qInfo() << "Waveform: renderer size" << getWidth() << getHeight()
                    << getDevicePixelRatio() << "-> widget" << m_widget->width()
                    << m_widget->height() << dpr;
            resizeRenderer(m_widget->width(), m_widget->height(), dpr);
        }
        m_widget->update();
#else
        m_widget->repaint(); // Repaints the widget directly by calling paintEvent()
#endif
    }
    // Time for Painter setup, unknown in this case
    return mixxx::Duration();
}

void WaveformWidgetAbstract::resize(int width, int height) {
    qreal devicePixelRatio = 1.0;
    if (m_widget) {
        m_widget->resize(width, height);
        devicePixelRatio = m_widget->devicePixelRatioF();
    }
    resizeRenderer(width, height, static_cast<float>(devicePixelRatio));
}
