#pragma once

class QQuickWindow;

/// DJ Mantra: the GPU time the app's own frames take (Marko 13.10.2026: "fix the GPU
/// monitor ... Pixel phone can show you GPU status"). Phones rarely let an app read
/// the driver's load file (SELinux), so the window asks Qt's renderer for GPU
/// timestamps (QRhi: GL_EXT_disjoint_timer_query / Vulkan queries) and adds up how
/// long the GPU worked on each frame. Works on any phone whose GPU has timer queries.
namespace djmantra::gputime {

/// Before the window's first frame (QQmlApplicationEngine::objectCreated)
void watch(QQuickWindow* pWindow);
/// The GPU's busy time for the app's frames since the start, in microseconds;
/// -1 while no frame has reported a time (no timer queries on this GPU)
long long busyMicroseconds();
/// true: busyMicroseconds() is the GPU's own time; false: the render thread's
bool fromGpuTimestamps();

} // namespace djmantra::gputime
