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
  then open again: again and again (`phone_app_oboe_loop.log`: the stream number had reached s#124 by 15:40; the phone log rolled over before a count).
- CQ MixPad (Mac) with deck 1 playing: **no signal on Ip1/2 (USB 1-2)** (`phone_cq_mixpad_meters_no_usb_signal.png`).
- So no sound reaches the CQ. Likely the MMAP (exclusive, low latency) path with a 2-channel stream on a 16-channel
  USB device: suggest opening without MMAP (AAudio sharing / no LowLatency, or AAUDIO_POLICY_NEVER) and/or opening
  the device's own 16 channels with an index mask, master in 1-2, headphones in 3-4.
- Settings -> MASTER / HEADPHONES (USB 1-2 / 3-4) not tested: no working output to choose yet.

## CQ MixPad on the phone - installed
First "Can't install CQ Mixpad" (Finsky `AuthFailureError: User needs to (re)enter credentials`, status 1400): the
phone's second Google account had been signed out. Marko removed that account himself (16:06); with his Gmail account
the Play Store installed **CQ Mixpad 1.2.2 (com.allenheath.minimicro.mixpad)** at 16:17. It can reach the CQ only once
the CQ is on the phone's network ("mantra" router) or the phone joins the CQ's own Wi-Fi.

## The Stereo test (13.10.2026 23:40-23:46, Marko: "yes, test stereo") - REAL TIME in Stereo
- Before (`stereo_test_before_multitrack.png`): Stream Mode Multitrack, Sample Rate 48 kHz, Source Point Post Preamp,
  11/12 ST IN, 13/14 Out5/Out6, 15/16 Main LR. Switched in CQ MixPad (Mac): CONFIG -> USB/SD -> Stream Mode **Stereo**
  (`stereo_test_after_stereo.png`). The phone kept the USB device (no replug).
- Android now: "USB Device Out" card=1 **Profiles (1): AUDIO_FORMAT_PCM_32_BIT, 48000** (in Multitrack: Profiles (0)).
- App V31, MASTER = Phone default (the CQ): `Sound output open: ... 48000 Hz, 2 channels, burst 96, perf LowLatency`,
  **no ErrorTimeout, no variant search**; `Output rate: 48809`, then **48,196-48,217 frames/s of 48000** for 30 s.
  Deck 1: -3:51 -> -3:21 = **30 s of song in 30 s of play**. xruns 0, late 0, slowest callback 5-11 ms.
- CQ MixPad's meters still show nothing (as all day), so the CQ input it lands on was not seen on the meters.
- **The CQ is left in Stereo** (it plays at real speed). In Stereo only 2 USB channels go to the CQ: the master; the
  headphones' USB 3-4 do not exist. To go back: CONFIG -> USB/SD -> Stream Mode Multitrack.
