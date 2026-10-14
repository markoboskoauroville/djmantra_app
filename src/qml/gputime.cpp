#include "qml/gputime.h"

#include <QElapsedTimer>
#include <QQuickWindow>
#include <QtDebug>
#include <atomic>
#include <memory>

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
// round 32: the Pixel's GLES driver gave no GPU timestamps: then the render
// thread's time per frame (recording + submitting the frame's GPU work)
std::atomic<long long> s_renderUs{0};
std::atomic<bool> s_renderSeen{false};
std::atomic<bool> s_described{false};

} // namespace

void watch(QQuickWindow* pWindow) {
    if (!pWindow) {
        return;
    }
    qInfo() << "Quick: GPU meter watches the window";
    auto pTimer = std::make_shared<QElapsedTimer>();
    QObject::connect(
            pWindow,
            &QQuickWindow::beforeRendering,
            pWindow,
            [pTimer] { pTimer->start(); },
            Qt::DirectConnection);
    QObject::connect(
            pWindow,
            &QQuickWindow::afterRendering,
            pWindow,
            [pTimer] {
                if (pTimer->isValid()) {
                    s_renderUs.fetch_add(pTimer->nsecsElapsed() / 1000);
                    s_renderSeen.store(true);
                }
            },
            Qt::DirectConnection);
#ifdef DJMANTRA_GPU_TIMESTAMPS
    QQuickGraphicsConfiguration config = pWindow->graphicsConfiguration();
    config.setTimestamps(true);
    pWindow->setGraphicsConfiguration(config);
    // the render thread, after each frame: the GPU time of a finished frame
    QObject::connect(
            pWindow,
            &QQuickWindow::afterFrameEnd,
            pWindow,
            [pWindow] {
                if (!s_described.exchange(true)) {
                    if (QRhi* pRhi = pWindow->rhi()) {
                        qInfo() << "Quick: renderer" << pRhi->backendName()
                                << "GPU timestamps"
                                << (pRhi->isFeatureSupported(QRhi::Timestamps) ? "supported"
                                                                                : "NOT supported");
                    }
                }
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
    if (s_reported.load()) {
        return s_busyUs.load();
    }
    return s_renderSeen.load() ? s_renderUs.load() : -1;
}

bool fromGpuTimestamps() {
    return s_reported.load();
}

} // namespace djmantra::gputime
