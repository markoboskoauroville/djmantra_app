#pragma once

#include <QOpenGLWidget>

class WGLWidget;

/// DJ Mantra: the GPU surface of a WGLWidget on Android.
///
/// Mixxx draws its OpenGL widgets into a QOpenGLWindow inside a window
/// container. On Android such a child window gets no EGL surface and the app
/// crashed on its first paint, so the phone drew everything on the CPU. A
/// QOpenGLWidget renders into a framebuffer object on the GPU instead, and Qt
/// composes the whole window with OpenGL ES.
///
/// The waveform factory renders on its own clock (makeCurrent, paint,
/// swapBuffers). Here that maps to: makeCurrent() binds the framebuffer object,
/// the widget paints into it, and swapBuffers() asks Qt to compose it
/// (update()); the frame is kept (PartialUpdate), so the paint that follows
/// does not draw it twice. Mouse and touch go to the WGLWidget underneath.
class OpenGLChildWidget : public QOpenGLWidget {
    Q_OBJECT
  public:
    explicit OpenGLChildWidget(WGLWidget* pWidget);

    void widgetDestroyed();
    /// After a frame was drawn from outside paintGL(): show it
    void present();

  protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

  private:
    WGLWidget* m_pWidget;
    bool m_framePending = false;
};
