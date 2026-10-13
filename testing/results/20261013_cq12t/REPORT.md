# CQ-12T on the Pixel's USB (CQ12T.md part B), V14, 13.10.2026 15:37-15:45

## What Android sees - the CQ is there, but with no profiles
- `dumpsys usb`: phone = host (dfp), device vendor 8944 (0x22f0) product 38 (0x26), "Allen&Heath Ltd", class 239/2/1.
- `dumpsys media.audio_policy`: **"USB Device Out" {AUDIO_DEVICE_OUT_USB_DEVICE, card=1;device=0} "USB-Audio - CQ12T"
  - Profiles (0)**; the same for "USB Device In" (Profiles (0), preferred 16-bit mono 48000). .
- The USB HAL (`audio_hw_usb`, `phone_audio_hw_usb.log`): **`usb_device_config: bit 32, format 0x3, rate 48000 chan 16`**:
  the HAL drives the CQ at 16 channels, 32-bit, 48 kHz (Multitrack), both directions (tx and rx proxies open).

## The app's output on the CQ - FAIL: a reopen loop every ~1.1 s
- The app opens `Phone audio output (speaker, USB or Bluetooth)` -> AAudio **MMAP** stream on device 515 (the CQ),
  48000 Hz, **2 channels**, burst 96, `perf = LL`; audio_flinger: MMAP_PLAYBACK thread, 2 ch float, "Frames written: 0".
- About 1.1 s later: `SoundDeviceOboe - Sound output disconnected ( "ErrorTimeout" ), reopening on the current route`,
  then open again: **0 times** in the phone log (`phone_app_oboe_loop.log`), the stream number was s#124 by 15:40.
- CQ MixPad (Mac) with deck 1 playing: **no signal on Ip1/2 (USB 1-2)** (`phone_cq_mixpad_meters_no_usb_signal.png`).
- So no sound reaches the CQ. Likely the MMAP (exclusive, low latency) path with a 2-channel stream on a 16-channel
  USB device: suggest opening without MMAP (AAudio sharing / no LowLatency, or AAUDIO_POLICY_NEVER) and/or opening
  the device's own 16 channels with an index mask, master in 1-2, headphones in 3-4.
- Settings -> MASTER / HEADPHONES (USB 1-2 / 3-4) not tested: no working output to choose yet.

## CQ MixPad on the phone - blocked
Play Store: "Can't install CQ Mixpad" (package com.allenheath.minimicro.mixpad): Finsky `AuthFailureError: User needs to
(re)enter credentials`, status 1400: the phone's Google account must be signed in again (Marko's password).
