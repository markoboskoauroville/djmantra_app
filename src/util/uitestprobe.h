#pragma once

#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QTimer>

class ControlProxy;
class QWidget;

namespace djmantra {

/// DJ Mantra UI test probe (--ui-test).
///
/// Logs, for tools/android/ui_test.py:
/// - the widget map: every visible skin widget with its object name, type,
///   control and place on the screen in device pixels ("UI widget ..."),
///   at the start and whenever the layout changes (rotation, page switch);
/// - every change of a control a widget shows ("UI control [Group],key = v").
class UiTestProbe : public QObject {
    Q_OBJECT
  public:
    explicit UiTestProbe(QWidget* pWindow);

  protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;

  private slots:
    void dumpWidgetMap();

  private:
    void scheduleDump();
    void watchControls();
    void watchControl(const QString& group, const QString& item);

    QPointer<QWidget> m_pWindow;
    QTimer m_dumpTimer;
    QSet<QString> m_watched;
    QList<ControlProxy*> m_proxies;
    int m_mapNumber;
};

} // namespace djmantra
