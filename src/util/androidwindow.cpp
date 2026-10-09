#include "util/androidwindow.h"

#include <QEvent>
#include <QTimer>
#include <QWidget>
#include <QtDebug>

#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QJniObject>
#endif

#include "moc_androidwindow.cpp"

namespace djmantra {

AndroidWindow::AndroidWindow(QWidget* pWindow)
        : QObject(pWindow),
          m_pWindow(pWindow) {
    pWindow->installEventFilter(this);
    hideSystemBars();
}

// static
void AndroidWindow::hideSystemBars() {
#if defined(Q_OS_ANDROID)
    QJniObject::callStaticMethod<void>("com/djmantra/app/WindowBridge",
            "immersive",
            "(Landroid/content/Context;)V",
            QNativeInterface::QAndroidApplication::context().object());
#endif
}

bool AndroidWindow::eventFilter(QObject* pObject, QEvent* pEvent) {
    if (pObject == m_pWindow &&
            (pEvent->type() == QEvent::Resize ||
                    pEvent->type() == QEvent::WindowActivate)) {
        hideSystemBars();
        if (pEvent->type() == QEvent::Resize) {
            // Repaint after the rotation has settled
            QWidget* pWindow = m_pWindow;
            for (int delay : {300, 1200}) {
                QTimer::singleShot(delay, pWindow, [pWindow] {
                    // the system can show the bars again after a rotation
                    // (round 5: status and navigation bars back in portrait)
                    hideSystemBars();
                    pWindow->update();
                    const auto children = pWindow->findChildren<QWidget*>();
                    for (QWidget* pChild : children) {
                        if (pChild->isVisible()) {
                            pChild->update();
                        }
                    }
                });
            }
        }
    }
    return QObject::eventFilter(pObject, pEvent);
}

} // namespace djmantra
