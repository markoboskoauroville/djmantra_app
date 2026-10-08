# Android port roadmap

DJ Mantra ports the Mixxx 2.5.6 engine to Android. This file tracks the plan and its status.

Upstream Mixxx has no Android support. It does have an experimental iOS target, a Qt 6
build, and an experimental QML UI (`res/qml`, `src/qml`), and the port builds on all three.

## Target

| | |
|---|---|
| Android | 10+ (API 29). That is what the Mix Ultra requires, and the NDK MIDI API needs it too. |
| ABI | `arm64-v8a` (phones); `x86_64` for the CI emulator |
| Qt | Qt 6 for Android |
| Audio | Oboe/AAudio |
| MIDI | Android MIDI (`android.media.midi`, NDK `AMidi`), USB and Bluetooth LE |
| UI | QML, touch-first layout |

## What has to change

### 1. Audio: `SoundDeviceOboe` (new)
PortAudio, which Mixxx uses on desktop and iOS, has no Android backend. So we add
`src/soundio/sounddeviceoboe.{h,cpp}`, a `SoundDevice` subclass built on Oboe, next to
`SoundDevicePortAudio`.

- Enumerate output devices (speaker, wired, USB, Bluetooth A2DP) through `AudioManager.getDevices()`,
  called over JNI.
- Open one Oboe stream per device with `setDeviceId()`, so **master and cue can go to two
  devices at once**, for example a Bluetooth speaker and USB headphones.
- `SoundManager` already handles several devices, with one clock device and the others kept
  in sync, and it has per-output delay (Main / Headphone / Booth delay). That delay is
  how Bluetooth latency gets compensated. We also add a measured-latency hint from
  `AudioTrack`/AAudio timestamps.
- Use multichannel streams for 4-output USB interfaces (master 1/2, cue 3/4).

### 2. MIDI: `AndroidMidiController` (new)
PortMidi has no Android backend.

- Java side, `packaging/android/src/.../MidiBridge.java`:
  - `MidiManager` device discovery.
  - BLE scan for the MIDI service `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`.
  - `openBluetoothDevice()`.
  - Runtime permissions: `BLUETOOTH_SCAN`, `BLUETOOTH_CONNECT`.
- C++ side, `src/controllers/midi/androidmidicontroller.{h,cpp}` and an enumerator.
  It uses `AMidiDevice_fromJava`, `AMidiOutputPort` and `AMidiInputPort`, and feeds the existing
  `MidiController` so **all existing mappings, scripting and MIDI learn work unchanged**.

### 3. Build and packaging
- CMake: an `ANDROID` branch next to the existing `IOS` one. `qt_add_executable` (which
  produces a shared library for Android), `QT_ANDROID_PACKAGE_SOURCE_DIR=packaging/android`,
  and `res/` bundled as assets.
- Off on Android: PortAudio, PortMidi, HID, BULK/libusb, broadcast (shout), LV2 (lilv),
  keychain, X11, DBus, UPower, HSS1394.
- Dependencies for `arm64-android`:
  - Qt via `aqtinstall`.
  - Through vcpkg: Oboe, rubberband, soundtouch, taglib, chromaprint, fftw3, libebur128,
    libkeyfinder, flac, ogg, vorbis, opus, opusfile, sndfile, mp3lame, libmad, libid3tag,
    protobuf, ms-gsl.
  - Note: AAC/M4A decoding comes later, through FFmpeg or NDK `AMediaCodec`.
- CI (GitHub Actions): build the APK, then run on an API 34 `pixel_7` emulator.

### 4. Touch UI (`res/qml/djmantra/`)
This uses the familiar mobile DJ layout, but it is our own design and does not copy
Algoriddim's djay artwork or branding.

- Landscape: two decks with platters and waveforms across the top, the mixer (EQ, filter,
  gain, crossfader) in the centre, and pads below.
- Library browser with search, using Android's storage access for music files.
- Settings: audio routing per output, with delay; controllers, with mapping picker and MIDI learn.

### 5. Hercules DJControl Mix Ultra
- Capture the controller's MIDI messages over BLE on a real phone. Use those to write a
  `Hercules DJControl Mix Ultra.midi.xml` and its script.
- Expose Mixxx's existing MIDI learn in the QML UI, so any control can be remapped.

## Milestones

- [x] **M0** Import Mixxx 2.5.6 unmodified (one commit, so our changes diff against it)
- [x] **M1** Desktop Linux baseline: build plus the full test suite (851/851 pass, before and after the M3 changes)
- [x] **M2** Android dependencies cross-compiled in CI (`arm64-android`, `x64-android`)
- [ ] **M3** Mixxx core compiles and links for Android, with sound and MIDI stubbed
- [ ] **M4** `SoundDeviceOboe`: audio plays on the emulator and phone
- [ ] **M5** `AndroidMidiController`: USB and BLE MIDI input and output
- [ ] **M6** Touch QML UI usable on a Pixel 7 sized screen
- [ ] **M7** Mix Ultra mapping verified on the real controller; MIDI learn in the UI
- [ ] **M8** Dual output (Bluetooth + USB) with delay compensation, tested on the user's phone

## What can be tested where

| Check | Cloud container | GitHub Actions | Your phone |
|---|---|---|---|
| Desktop build + Mixxx test suite | yes | yes | |
| Android APK build | needs `dl.google.com` allowed | yes | |
| App launch, UI, audio on emulator | no KVM | yes (Pixel 7 AVD) | |
| BLE MIDI with Mix Ultra | | | yes |
| Bluetooth + USB dual output | | | yes |
