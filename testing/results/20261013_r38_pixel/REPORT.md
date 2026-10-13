# Round 38 on the Pixel 7 - V21 (b035bb7); the CQ-12T on the Pixel's USB; 13.10.2026 18:14 local

## Part 7 (first): the CQ as the output - FAIL, still ~0.3x; the 16-channel path never runs
- Default route ("Phone audio output", follows the USB): MMAP times out once, reopens with perf None, **2 channels**;
  `Output rate: 14240 frames/s of 48000 , 2 channels`, `14169 ...` (0.30x). The deck: -3:51 -> -3:42 = 9 s of song in
  30 s of play. No `USB output with 16 channels` line.
- Settings -> MASTER: the list offers `USB: USB-Audio - CQ12T` **without channel pairs** (no "1-2 / 3-4": Android
  gives the app no channel count for the CQ, its device profiles are empty). Chosen + APPLY: `audio route master ->
  usb:USB-Audio - CQ12T channels 1-2`, `Sound output open: "USB: USB-Audio - CQ12T" ... 2 channels ... perf LowLatency`,
  MMAP ErrorTimeout, reopen perf None, **2 channels** again; `Output rate: 13011`, `14259 frames/s of 48000`.
  Again **no** `USB output with 16 channels` / `Could not open 16 channels` line: the app never tries 16.
- audio_flinger: `AudioOut_15` MIXER thread on USB_DEVICE, 48000 Hz, Channel count 2, HAL frame count 960, while the
  USB HAL (`audio_hw_usb`) configures the CQ as `bit 32, format 0x3, rate 48000 chan 16`. Android's mixer believes
  2 ch; the device runs 16 ch: the stream drains at ~0.3x.
- Suggestion: don't rely on AudioDeviceInfo.getChannelCounts() (empty for the CQ); for a USB device try
  setChannelCount(16) (or an index mask 0xFFFF) and fall back on error; or read the count from UsbManager's
  interface / alsa card. Also the MASTER / HEADPHONES lists then need the pairs without a reported count.
- MASTER left on the CQ for now.
