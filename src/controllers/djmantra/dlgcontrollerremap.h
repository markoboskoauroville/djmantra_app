#pragma once

#include <QDialog>
#include <QPixmap>
#include <QWidget>

class ControlPickerMenu;
class QLabel;
class QPushButton;

namespace djmantra {

struct ControllerLayoutControl;

/// The controller drawn from its layout (photo and control outlines). A
/// control with its own function is drawn amber with the function's name.
class ControllerMapView : public QWidget {
    Q_OBJECT
  public:
    ControllerMapView(ControlPickerMenu* pPicker, QWidget* pParent);

    void setShift(bool shift);

  signals:
    void controlTapped(int index);

  protected:
    void paintEvent(QPaintEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;

  private:
    QRectF controlRect(const ControllerLayoutControl& control) const;
    QRectF imageRect() const;

    ControlPickerMenu* m_pPicker;
    QPixmap m_photo;
    bool m_shift;
};

/// DJ Mantra: the virtual controller. Tap any button, pad, fader, knob,
/// encoder or jog wheel of the Hercules DJControl Mix Ultra and choose any
/// function of the app for it (or back to the controller mapping). SHIFT
/// switches to the functions with SHIFT held.
class DlgControllerRemap : public QDialog {
    Q_OBJECT
  public:
    explicit DlgControllerRemap(QWidget* pParent);

  private slots:
    void slotControlTapped(int index);

  private:
    void chooseFunction(int controlIndex, int inputIndex);
    void updateStatus();

    ControlPickerMenu* m_pPicker;
    ControllerMapView* m_pView;
    QPushButton* m_pShift;
    QLabel* m_pStatus;
};

} // namespace djmantra
