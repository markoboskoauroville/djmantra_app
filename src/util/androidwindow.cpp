#include "util/androidwindow.h"

#include <QApplication>
#include <QEvent>
#include <QTimer>
#include <QWidget>
#include <QtDebug>

#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#endif

#include "moc_androidwindow.cpp"

namespace djmantra {

AndroidWindow::AndroidWindow(QWidget* pWindow)
        : QObject(pWindow),
          m_pWindow(pWindow) {
    pWindow->installEventFilter(this);
    hideSystemBars();
#if defined(Q_OS_ANDROID)
    auto* pTimer = new QTimer(this);
    connect(pTimer, &QTimer::timeout, this, &AndroidWindow::followSystemBars);
    pTimer->start(300);
    // Android left stale pixels where only a part of the window was repainted
    // (round 10: pads after a mode change, the One Deck time and header):
    // the whole window is repainted after every touch and a few times a second
    auto* pRepaint = new QTimer(this);
    connect(pRepaint, &QTimer::timeout, m_pWindow, qOverload<>(&QWidget::update));
    pRepaint->start(400);
    qApp->installEventFilter(this);
#endif
}

void AndroidWindow::followSystemBars() {
#if defined(Q_OS_ANDROID)
    const QJniObject array = QJniObject::callStaticObjectMethod(
            "com/djmantra/app/WindowBridge", "insets", "()[I");
    if (!array.isValid()) {
        return;
    }
    QJniEnvironment env;
    const auto jArray = static_cast<jintArray>(array.object());
    if (env->GetArrayLength(jArray) != 4) {
        return;
    }
    jint px[4];
    env->GetIntArrayRegion(jArray, 0, 4, px);
    // Android gives pixels, Qt lays out in device independent pixels
    const qreal ratio = m_pWindow->devicePixelRatioF();
    const QMargins margins(qRound(px[0] / ratio),
            qRound(px[1] / ratio),
            qRound(px[2] / ratio),
            qRound(px[3] / ratio));
    if (margins != m_margins) {
        m_margins = margins;
        qInfo() << "Window: the system bars take" << margins;
        m_pWindow->setContentsMargins(margins);
    }
#endif
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
    if (pEvent->type() == QEvent::MouseButtonRelease && pObject->isWidgetType()) {
        QTimer::singleShot(60, m_pWindow, qOverload<>(&QWidget::update));
    }
    // Only a rotation hides the bars again: a reactivated window does not
    // (round 10: the bars a swipe brought were hidden at once again)
    if (pObject == m_pWindow && pEvent->type() == QEvent::Resize) {
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
