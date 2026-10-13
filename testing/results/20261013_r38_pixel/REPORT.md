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

## Part 1: MIXER page - PASS (one glyph)
`p1_mixer_page.png`: "EQ" at the top, per deck GAIN / HIGH / MID / LOW / FILTER, the FX button, headphones, the
faders and VU; **nothing overlaps, no cut labels** (Pixel 7 portrait, 411 dp). The FX bar of V16 is gone.
- The **"▾" after EQ / NEURAL MIX is a missing-glyph box** (`p1_eq_arrow_glyph_missing.png`): the font has no ▾
  (same problem as the ✕ in round 32): draw it.

## Part 2: EQ dropdown - PASS (one small)
- Tap "EQ": a menu below it, Equalizer (lit) / Neural Mix EQ. Neural Mix EQ: `EQ mode: Neural Mix EQ`, the button
  reads NEURAL MIX, **both** decks' labels HARMONY / VOCALS / DRUMS (`p2_neural_p3_chooser.png`, left).
  Equalizer: `EQ mode: Equalizer`, labels back.
- A tap outside closes the menu when it lands on the matrix / top; **a tap near the crossfader (lower screen) did not
  close it** (the menu stayed open).
## Part 3: the FX button - PASS
- Tap: `FX: deck 1 FLANGER on` (lit yellow), tap: `... off` (dark). A still hold of ~1 s opens the drop-up chooser
  above the button (FLANGER / UNDERWATER / COOLDOWN / ECHO OUT, the current one lit) (`p2_neural_p3_chooser.png`, right).
  Picking UNDERWATER: `FX: deck 1 UNDERWATER`, the button reads UNDERWATER and is on; same for COOLDOWN, ECHO OUT.
  (A hold with 1 px of movement did not open it: fine for a finger? to check by Marko.)
- After an app restart the button still reads ECHO OUT. PASS. Deck 2's button unchanged meanwhile (FLANGER).

## Other app on the CQ (is it Android or our stream?) - Android plays the CQ at the RIGHT speed
- DJ Mantra stopped; 4B.mp3 opened in RS Audio Player (RS File Manager) with the CQ on the phone's USB: its clock went
  **00:26 -> 00:49 in 22 s of wall time** (1.0x). Android's own MIXER thread on USB_DEVICE (2 ch, 48 kHz) plays at
  real time for another app, so the 0.3x is in DJ Mantra's stream (its AAudio settings / callback), not the platform.
- (Whether it was audible on the CQ: not checked, MixPad meters unreliable; the speed is the point.)
