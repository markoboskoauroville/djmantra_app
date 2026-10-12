#pragma once

#ifndef WGLWIDGET_H
#error "Do not include this file, include wglwidget.h instead"
#endif

#include <QWidget>

////////////////////////////////
// QOpenGLWindow based WGLWidget
////////////////////////////////

class QPaintDevice;
class QOpenGLWindow;
class OpenGLWindow;
class OpenGLChildWidget;
class TrackDropTarget;

class WGLWidget : public QWidget {
  public:
    WGLWidget(QWidget* parent);
    ~WGLWidget();

    bool isContextValid() const;

    bool shouldRender() const;

    void makeCurrentIfNeeded();
    void doneCurrent();

    void swapBuffers();

    // called (indirectly) by WaveformWidgetFactory
    virtual void paintGL();
    // called by OpenGLWindow (OpenGLChildWidget on Android)
    virtual void resizeGL(int w, int h);
    virtual void initializeGL();

    void setTrackDropTarget(TrackDropTarget* pTarget);
    TrackDropTarget* trackDropTarget() const;

    /// nullptr on Android (OpenGLChildWidget, a QOpenGLWidget, instead)
    QOpenGLWindow* getOpenGLWindow() const;

  protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    QPaintDevice* paintDevice();

  private:
#ifdef __ANDROID_PORT__
    OpenGLChildWidget* m_pChild = nullptr;
#endif
    OpenGLWindow* m_pOpenGLWindow;
    QWidget* m_pContainerWidget;
    TrackDropTarget* m_pTrackDropTarget;
};
