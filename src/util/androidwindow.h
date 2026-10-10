#pragma once

#include <QMargins>
#include <QObject>

class QWidget;

namespace djmantra {

/// DJ Mantra on Android: keeps the main window over the whole screen.
///
/// Hides the status and navigation bars at the start and after every
/// rotation. When they come back (a swipe from the edge), the window's
/// content makes room for them instead of lying under them, and takes the
/// whole screen again when they hide (owner, 10.10.2026). Repaints the window
/// shortly after a rotation (the Nothing Phone 2 stayed black after one).
class AndroidWindow : public QObject {
    Q_OBJECT
  public:
    explicit AndroidWindow(QWidget* pWindow);

    static void hideSystemBars();

  protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;

  private:
    /// The room the visible bars take, as the window's content margins
    void followSystemBars();

    QWidget* m_pWindow;
    QMargins m_margins;
};

} // namespace djmantra
