#include "qml/gputime.h"

#include <QQuickWindow>
#include <QtDebug>
#include <atomic>

// the renderer's GPU timestamps came with Qt 6.6 (Android: Qt 6.8; the desktop
// test build's Qt 6.4 reports no GPU time)
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
#include <QQuickGraphicsConfiguration>
#include <rhi/qrhi.h>
#define DJMANTRA_GPU_TIMESTAMPS
#endif

namespace djmantra::gputime {

namespace {

std::atomic<long long> s_busyUs{0};
std::atomic<bool> s_reported{false};

} // namespace

void watch(QQuickWindow* pWindow) {
#ifndef DJMANTRA_GPU_TIMESTAMPS
    Q_UNUSED(pWindow);
#else
    if (!pWindow) {
        return;
    }
    QQuickGraphicsConfiguration config = pWindow->graphicsConfiguration();
    config.setTimestamps(true);
    pWindow->setGraphicsConfiguration(config);
    // the render thread, after each frame: the GPU time of a finished frame
    QObject::connect(
            pWindow,
            &QQuickWindow::afterFrameEnd,
            pWindow,
            [pWindow] {
                QRhiSwapChain* pSwapChain = pWindow->swapChain();
                if (!pSwapChain) {
                    return;
                }
                QRhiCommandBuffer* pCommands = pSwapChain->currentFrameCommandBuffer();
                if (!pCommands) {
                    return;
                }
                const double seconds = pCommands->lastCompletedGpuTime();
                if (seconds > 0 && seconds < 1) {
                    s_busyUs.fetch_add(static_cast<long long>(seconds * 1e6));
                    if (!s_reported.exchange(true)) {
                        qInfo() << "Quick: GPU time per frame from the renderer's timestamps";
                    }
                }
            },
            Qt::DirectConnection);
#endif
}

long long busyMicroseconds() {
    return s_reported.load() ? s_busyUs.load() : -1;
}

} // namespace djmantra::gputime
