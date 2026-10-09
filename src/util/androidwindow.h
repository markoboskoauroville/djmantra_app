#pragma once

#include <QObject>

class QWidget;

namespace djmantra {

/// DJ Mantra on Android: keeps the main window over the whole screen.
///
/// Hides the status and navigation bars (immersive, back with a swipe from
/// the edge) at the start and after every rotation, and repaints the window
/// shortly after a rotation (the Nothing Phone 2 stayed black after one).
class AndroidWindow : public QObject {
    Q_OBJECT
  public:
    explicit AndroidWindow(QWidget* pWindow);

    static void hideSystemBars();

  protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;

  private:
    QWidget* m_pWindow;
};

} // namespace djmantra
