#pragma once

#include <QDialog>
#include <QPixmap>
#include <QWidget>

class ControlPickerMenu;
class QLabel;
class QPushButton;

namespace djmantra {

struct ControllerLayoutControl;

/// The controller drawn from its layout like the Controller Mapper (Mantra)
/// app: orange controls, the selected one green, chosen functions in white.
class ControllerMapView : public QWidget {
    Q_OBJECT
  public:
    ControllerMapView(ControlPickerMenu* pPicker, QWidget* pParent);

    void setShift(bool shift);
    void setSelected(int index);
    int selected() const {
        return m_selected;
    }

  signals:
    void controlTapped(int index);

  protected:
    void paintEvent(QPaintEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;

  private:
    QRectF controlRect(const ControllerLayoutControl& control) const;
    QRectF area() const;
    double unit() const;
    QPointF origin() const;

    ControlPickerMenu* m_pPicker;
    bool m_shift;
    int m_selected;
};

/// DJ Mantra: the virtual controller. Tap any button, pad, fader, knob,
/// encoder or jog wheel of the Hercules DJControl Mix Ultra and choose any
/// function of the app for it (or back to the controller mapping). SHIFT
/// switches to the functions with SHIFT held.
class DlgControllerRemap : public QDialog {
    Q_OBJECT
  public:
    explicit DlgControllerRemap(QWidget* pParent);
    ~DlgControllerRemap() override;

    /// Selects the control that sent this MIDI message (a press on the
    /// real controller)
    void selectFromMidi(int status, int data1);

  private slots:
    void slotControlTapped(int index);

  private:
    void chooseFunction(int controlIndex, int inputIndex);
    void select(int index);
    void updateStatus();

    ControlPickerMenu* m_pPicker;
    ControllerMapView* m_pView;
    QPushButton* m_pShift;
    QLabel* m_pStatus;
    QLabel* m_pSelected;
    QPushButton* m_pChange;
};

} // namespace djmantra
