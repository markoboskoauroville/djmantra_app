# CQ-12T on the Mac (13.10.2026, tester "mantra dj app tester")

## As found at 11:0x local: 16 in / 16 out (Multitrack mode, to be confirmed by Marko)
- `system_profiler SPAudioDataType`: **CQ12T - Audio**, Allen&Heath Ltd, **Input Channels 16, Output Channels 16**,
  **Current SampleRate 48000**, Transport USB.
- `system_profiler SPUSBDataType`: CQ12T, Vendor ID **0x22f0**, Product ID **0x0026**, version 0.01, up to 480 Mb/s
  (USB 2.0 high speed), bus power 0 mA required (self powered).
- CoreAudio `kAudioDevicePropertyAvailableNominalSampleRates`: **48000 only** (no 44.1, no 96 kHz).
  PortAudio accepts 44.1 / 88.2 / 96 kHz only because CoreAudio converts; the device itself runs at 48 kHz.
  So on Android the app's 44.1 kHz stream would be resampled to 48 kHz by AudioFlinger, or the app should open at 48 kHz.

## Stereo mode
Not yet measured: Marko has to switch the mixer (Configuration -> USB -> streaming mode); the voice was down.

## CQ MixPad 1.2.2 on the MacBook Pro's built-in display (Color LCD, `screencapture -D 2`)
- Screenshot works: `cqmixpad_processing.png` (PROCESSING page: Ip1/2 0 dB fader -3 dB, Ip3/4 and Ip5 off; main Ip1/2 -3 dB; USB and Bluetooth icons lit).
- The USB streaming mode and the input sources are on the CONFIG page: reaching it needs a click. Clicking moves Marko's
  pointer, so by his rule (MANTRA_MANIFEST marko-tests.md) it waits for his "done".
