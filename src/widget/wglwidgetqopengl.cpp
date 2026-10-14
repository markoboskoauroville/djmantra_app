#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QResizeEvent>
#include <QWindow>

#include "widget/openglwindow.h"
#include "widget/tooltipqopengl.h"
#include "widget/wglwidget.h"
#ifdef __ANDROID_PORT__
#include "widget/openglchildwidget.h"
#endif

WGLWidget::WGLWidget(QWidget* pParent)
        : QWidget(pParent),
          m_pOpenGLWindow(nullptr),
          m_pContainerWidget(nullptr),
          m_pTrackDropTarget(nullptr) {
    // When the widget is resized or moved, the QOpenGLWindow visibly resizes
    // or moves before the widgets do. This can be solved by calling
    //   setAttribute(Qt::WA_PaintOnScreen);
    // here, but this comes with a clear performance penalty and drop in
    // frame rate.
}

WGLWidget::~WGLWidget() {
    ToolTipQOpenGL::singleton().stop();
#ifdef __ANDROID_PORT__
    if (m_pChild) {
        m_pChild->widgetDestroyed();
    }
#endif
    if (m_pOpenGLWindow) {
        m_pOpenGLWindow->widgetDestroyed();
    }
}

QPaintDevice* WGLWidget::paintDevice() {
    makeCurrentIfNeeded();
#ifdef __ANDROID_PORT__
    return m_pChild;
#else
    return m_pOpenGLWindow;
#endif
}

void WGLWidget::setTrackDropTarget(TrackDropTarget* pTarget) {
    m_pTrackDropTarget = pTarget;
}

TrackDropTarget* WGLWidget::trackDropTarget() const {
    return m_pTrackDropTarget;
}

void WGLWidget::showEvent(QShowEvent* event) {
#ifdef __ANDROID_PORT__
    if (!m_pChild) {
        m_pChild = new OpenGLChildWidget(this);
        m_pChild->resize(size());
        m_pChild->show();
    }
#else
    if (!m_pOpenGLWindow) {
        m_pOpenGLWindow = new OpenGLWindow(this);
        m_pContainerWidget = createWindowContainer(m_pOpenGLWindow, this);
        m_pContainerWidget->resize(size());
        m_pContainerWidget->show();
        m_pContainerWidget->setAutoFillBackground(true);
    }
#endif
    QWidget::showEvent(event);
}

void WGLWidget::resizeEvent(QResizeEvent* event) {
#ifdef __ANDROID_PORT__
    if (m_pChild) {
        m_pChild->resize(event->size());
    }
#endif
    if (m_pContainerWidget) {
        m_pContainerWidget->resize(event->size());
    }
    QWidget::resizeEvent(event);
}

bool WGLWidget::isContextValid() const {
#ifdef __ANDROID_PORT__
    return m_pChild && m_pChild->context() && m_pChild->context()->isValid();
#else
    return m_pOpenGLWindow && m_pOpenGLWindow->context() && m_pOpenGLWindow->context()->isValid();
#endif
}

void WGLWidget::makeCurrentIfNeeded() {
#ifdef __ANDROID_PORT__
    if (isContextValid()) {
        // Always: it also binds this widget's framebuffer object (the
        // widgets of one window can share a context)
        m_pChild->makeCurrent();
        // QOpenGLWidget sets the viewport only around its own paintGL()
        const qreal ratio = m_pChild->devicePixelRatioF();
        m_pChild->context()->functions()->glViewport(0,
                0,
                static_cast<int>(m_pChild->width() * ratio),
                static_cast<int>(m_pChild->height() * ratio));
    }
#else
    if (m_pOpenGLWindow && m_pOpenGLWindow->context() != QOpenGLContext::currentContext()) {
        m_pOpenGLWindow->makeCurrent();
    }
#endif
}

void WGLWidget::doneCurrent() {
#ifdef __ANDROID_PORT__
    if (m_pChild) {
        m_pChild->doneCurrent();
    }
#else
    if (m_pOpenGLWindow) {
        m_pOpenGLWindow->doneCurrent();
    }
#endif
}

void WGLWidget::paintGL() {
    // to be implemented in derived widgets if needed
}

void WGLWidget::initializeGL() {
    // to be implemented in derived widgets if needed
}

void WGLWidget::resizeGL(int w, int h) {
    Q_UNUSED(w);
    Q_UNUSED(h);
    // to be implemented in derived widgets if needed
}

void WGLWidget::swapBuffers() {
    if (shouldRender()) {
#ifdef __ANDROID_PORT__
        // Qt composes the framebuffer object into the window (on the GPU)
        m_pChild->present();
#else
        m_pOpenGLWindow->context()->swapBuffers(m_pOpenGLWindow->context()->surface());
#endif
    }
}

bool WGLWidget::shouldRender() const {
#ifdef __ANDROID_PORT__
    if (!m_pChild || !m_pChild->isVisible() || !isContextValid()) {
        return false;
    }
    const QWindow* pWindow = m_pChild->window()->windowHandle();
    return pWindow && pWindow->isExposed();
#else
    return m_pOpenGLWindow && m_pOpenGLWindow->isExposed();
#endif
}

QOpenGLWindow* WGLWidget::getOpenGLWindow() const {
    return m_pOpenGLWindow;
}
