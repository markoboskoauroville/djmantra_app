#include "widget/openglchildwidget.h"

#include "moc_openglchildwidget.cpp"
#include "waveform/waveformwidgetfactory.h"
#include "widget/wglwidget.h"

OpenGLChildWidget::OpenGLChildWidget(WGLWidget* pWidget)
        : QOpenGLWidget(pWidget),
          m_pWidget(pWidget) {
    setFormat(WaveformWidgetFactory::getSurfaceFormat());
    // Keep the frame drawn by the waveform clock between compositions
    setUpdateBehavior(QOpenGLWidget::PartialUpdate);
    // Taps and drags belong to the widget underneath (waveform, VU meter ...)
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
}

void OpenGLChildWidget::widgetDestroyed() {
    m_pWidget = nullptr;
}

void OpenGLChildWidget::present() {
    m_framePending = true;
    update();
}

void OpenGLChildWidget::initializeGL() {
    if (m_pWidget) {
        m_pWidget->initializeGL();
    }
}

void OpenGLChildWidget::paintGL() {
    if (m_framePending) {
        // Drawn already by the waveform clock: only compose it
        m_framePending = false;
        return;
    }
    if (m_pWidget) {
        m_pWidget->paintGL();
    }
}

void OpenGLChildWidget::resizeGL(int w, int h) {
    if (m_pWidget) {
        // As OpenGLWindow: the widgets expect device pixels
        const qreal ratio = devicePixelRatioF();
        m_pWidget->resizeGL(static_cast<int>(w * ratio), static_cast<int>(h * ratio));
    }
    // The next paint must draw: the framebuffer object is new
    m_framePending = false;
}
