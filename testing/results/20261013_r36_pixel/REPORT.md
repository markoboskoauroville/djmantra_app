# Round 36 on the Pixel 7 - V16 (a82f54e)

Tester "mantra dj app tester"; Pixel 7 over adb Wi-Fi; the CQ-12T on the Pixel's USB-C (phone = host). 13.10.2026 16:44 local.

## Part 6 (first): CQ-12T on the Pixel's USB - the reopen loop is FIXED, but still NO signal on the CQ
- Log:
  ```
  16:44:17.781 Sound output open: ... 48000 Hz, 2 channels, 1024 frames per callback, burst 96 , perf LowLatency
  16:44:18.923 Sound output disconnected ( "ErrorTimeout" ), reopening on the current route
  16:44:18.923 Sound output: the low-latency (MMAP) stream timed out, reopening without MMAP (PerformanceMode None)
  16:44:19.004 Sound output open: ... 48000 Hz, 2 channels, 1024 frames per callback, burst 2048 , perf None
  ```
  then **no more ErrorTimeout** (0 in the next 15 s and after). PASS for the loop.
- audio_flinger: `AudioOut_D`, **type 0 (MIXER)**, 48000 Hz, float, **Channel count 2**, Output devices 0x4000
  (USB_DEVICE) (`p6_audio_flinger_usb_mixer_thread.txt`). The HAL still runs the CQ at 16 ch 32-bit.
- Deck 1 playing Nanana: app `Output level: peak 0.31, xruns 0` (one `slowest callback 54.3 ms, late 1` right after the
  reopen). **CQ MixPad (Mac, over the LAN): Ip1/2's input and fader meters stay dark** (`p6_cq_mixpad_ip12_meters_dark.png`),
  so the 2 channels Android mixes into the 16-channel USB stream do not show on USB 1-2 (or MixPad's meters are not live:
  they never moved in any test today, so a known signal would be the check).
- HEADPHONES = USB 3-4 not tried yet (the master does not arrive). 60 s crackle recording: not possible, the CQ's
  output is not on the Mac.
