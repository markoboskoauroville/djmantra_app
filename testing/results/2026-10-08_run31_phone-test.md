# Local test, run #31 (Android CI 37786564456, commit 61a4f62), real Pixel 7

*8.10.2026, 16:58. Tested on the real Pixel 7 (Android 16, over Wi-Fi adb), arm64-v8a APK from the run's artifact. Nothing in the source was changed.*

## Verdict

**The libomp fix works. The app now gets further and crashes 24 s after launch.** M5 (MIDI, Mix Ultra, beat lights) could not be tested: everything is BLOCKED behind this crash.

| check | result |
|---|---|
| `lib/arm64-v8a/libomp.so` inside the APK (961 440 bytes) | PASS |
| `libomp.so` loads (`nativeloader: Load .../libomp.so ... ok`), no `dlopen` failure | PASS (the run #30 crash is fixed) |
| App starts (cold start 243 ms to first frame) | PASS |
| App reaches its main window | **FAIL**: critical-error dialog, then SIGSEGV |
| Name shown to the user is DJ Mantra | **FAIL**: launcher label `mixxx`, dialog title "Mixxx - Critical error", versionName 2.5.6 |
| M5 test list (MIDI USB/BLE, pads, jog, LEDs, beat lights) | BLOCKED by the crash |

## Problem 1: `qResourcePath is empty` on Android (blocks release)

On first start the app shows a dialog **"Mixxx - Critical error: qResourcePath is empty, this should not happen -- did our developers forget to define `__UNIX__`, `__WINDOWS__` or `__APPLE__`??"** (screenshot `2026-10-08_run31_pixel7/01_qresourcepath_dialog.png`).

Cause, read in the source: `src/preferences/configobject.cpp` picks the resource folder with `#if defined(__UNIX__)` / `__WINDOWS__` / `Q_OS_IOS` / `Q_OS_MACOS`. **There is no `Q_OS_ANDROID` branch**, so on Android `qResourcePath` stays empty. After OK the log says `Loading resources from "/"`, SELinux denies the read of `/` (`avc: denied { read } ... tcontext=u:object_r:rootfs:s0`), so no skin, controller mappings or other resources are found.

## Problem 2: SIGSEGV after the dialog (blocks release)

About 1 s after `Loading resources from "/"`: `Fatal signal 11 (SIGSEGV), SEGV_MAPERR, fault addr 0x0 in tid qtMainLoopThread`, *Cause: null pointer dereference*. Backtrace (full one in `tombstone.txt`):

```
#00..#03 libmixxx_arm64-v8a.so (no symbols)
#04 libQt6Core
#05 libQt6OpenGL
#06 libQt6OpenGL  QOpenGLWindow::resizeEvent(QResizeEvent*)
#07 libQt6Gui     QWindow::event
...  QWindowContainer::event -> QWidget::setVisible -> show_helper
#37 libmixxx_arm64-v8a.so (main+3136)
```

So a `QOpenGLWindow` subclass in mixxx (a waveform or spinny widget, shown when the main window becomes visible) dereferences null in its resize/initialise path. It is most likely a consequence of Problem 1 (nothing loaded from `/`), but it should not crash even without resources. A symbolised build (`-g`, or the unstripped `.so` as an artifact) would name the function.

Also seen: right after the dialog the app opened the Android folder picker (SAF, "Can't use this folder", screenshot 02). It was still on screen after the process died.

## Problem 3: the name (requested by Marko)

Marko, 8.10.2026: **"This app should be called Djmantra, not Mixxx. So rename also."** What the user sees today:

- launcher label: `mixxx` (APK `application-label`)
- dialog titles: "Mixxx - ..." (`QCoreApplication::setApplicationName(VersionStore::applicationName())`, `src/main.cpp:207`)
- `versionName` 2.5.6 (Mixxx's version), config folder `files/.mixxx/mixxx.cfg`

## Requests for the cloud

1. Add a `Q_OS_ANDROID` branch to the resource-path choice in `src/preferences/configobject.cpp`: package the `res/` folder into the APK (`assets:/`) and copy it on first start to `QStandardPaths::AppDataLocation` (or read it from `assets:/` directly), so `qResourcePath` is never empty on Android.
2. Make the `QOpenGLWindow` widget that crashes in `resizeEvent` safe when its resources or GL context are missing (null-check), and ship symbols for `libmixxx_arm64-v8a.so` as a CI artifact so the next tombstone names the function.
3. Rename everything the user sees to **DJ Mantra**: the launcher label, the window and dialog titles (`VersionStore::applicationName()`), the about box. The app's own version should replace Mixxx's 2.5.6 as `versionName`. (Internal folders such as `.mixxx` can stay if moving them is risky, but say so.)
4. In CI, a smoke test on an emulator that launches the APK and fails the job if a critical-error dialog or a tombstone appears in the first 30 s.

Then I run the M5 list on the Pixel 7 with the Mix Ultra.
