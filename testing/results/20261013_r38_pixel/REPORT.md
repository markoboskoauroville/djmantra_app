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

## Part 4: the app's FX against djay's recordings (V21) - Flanger close, Underwater / Cooldown too strong, Echo Out does nothing
Recorded from the Pixel with scrcpy (audio capture), Nanana from 0:58 (overview tap: `jump [Channel1] to 57.86 s`),
FX via the FX button (choose by hold, tap on at 8 s, off at 16 s; Echo Out on at 4 s, off at 16 s), D/W n/a.
Output rate during the takes ~48,000 frames/s (MASTER = Phone default; with "Phone speaker" pinned, Android's
playback capture got only silence, so the first attempt was void). Audio: djmantra-test-songs `fx/app_v21_20261013/`.
Same band analysis as djay's (`p4_fx_djay_vs_app_bands.txt`), change ON minus dry in dB:

| | rms | <150 | 150-400 | 400-2k | 2k-6k | 6k-16k |
|---|---|---|---|---|---|---|
| Flanger djay | -0.8 | -1.6 | -3.2 | +2.3 | +2.4 | -1.5 |
| Flanger app | +0.2 | -0.1 | 0.0 | +3.2 | +1.7 | -0.2 |
| Underwater djay | +0.1 | +0.1 | -0.7 | +2.2 | +2.1 | **-8.5** |
| Underwater app | +0.2 | 0.0 | +0.3 | +2.8 | +1.4 | **-28.1** |
| Cooldown djay | **-5.8** | **-15.9** | -5.1 | +1.5 | -3.1 | -27.0 |
| Cooldown app | **-13.1** | **-32.1** | -15.1 | -4.1 | -8.1 | -24.4 |
| Echo Out djay (2-6 s after ON) | **-21.9** | -22.8 | -19.8 | -20.0 | -20.6 | -19.3 |
| Echo Out app | **+0.1** | -0.7 | +2.0 | +1.1 | +0.8 | +1.7 |

- Flanger: close to djay (both mild band changes). OK
- Underwater: the lowpass cuts far too much above 6 kHz (-28 vs djay -8.5); the rest matches.
- Cooldown: about twice djay's depth (-13 vs -6 dB overall, lows -32 vs -16, low-mids -15 vs -5).
- **Echo Out: no change at all**: the level stays at -16 dB for the 12 s it was on (`FX: deck 1 ECHO OUT on` logged);
  djay drops ~22 dB in the same window (the song cut, echoes dying).

## V25 (5e01717), 19:05-19:15
### Parts 9 + 11: the CQ as output - FAIL, still 0.3x
```
USB output without channel counts: trying 16 channels
Could not open 16 channels ( "ErrorOutOfRange" ), trying 2
Sound buffer: 3072 of 4128 frames
Sound output open: "Phone audio output ..." 48000 Hz, 2 channels, 1024 frames per callback, burst 96, perf LowLatency
Sound output disconnected ( "ErrorTimeout" ) ... reopening without MMAP (PerformanceMode None)
Sound buffer: 16384 of 16384 frames
Sound output open: ... 2 channels, 1024 frames per callback, burst 8192 , perf None
Output rate: 14236 frames/s of 48000 , 2 channels      (and 14138)
```
- 30 s of play: -3:51 -> -3:42 = 9 s of song. The bigger buffer (16384, burst 8192) did not change the speed.
- 16 channels refused with ErrorOutOfRange (AAudio rejects a channel count the device does not report).
- RS Audio Player is still 1.0x on the same route. Ideas: our callback has `1024 frames per callback` fixed: try
  leaving framesPerCallback unspecified (Oboe kUnspecified) on the no-MMAP stream, or a blocking write() stream;
  or `setSampleRate` unspecified and let AAudio resample.
### Part 10: PASS. The ▾ is drawn (`p10_eq_arrow_drawn.png`); a hold of 0.6 s with 12 px movement opens the chooser.
### Part 12: the FX after retuning (MASTER = Phone default, scrcpy capture, ~48,000 frames/s during the takes)
`p12_fx_djay_vs_v25_bands.txt`; audio: djmantra-test-songs `fx/app_v25_20261013/`. ON minus dry, dB:

| | rms | <150 | 150-400 | 400-2k | 2k-6k | 6k-16k |
|---|---|---|---|---|---|---|
| Flanger djay | -0.8 | -1.6 | -3.2 | +2.3 | +2.4 | -1.5 |
| Flanger V25 | +0.2 | -0.1 | +0.1 | +3.2 | +1.2 | -0.3 |
| Underwater djay | +0.1 | +0.1 | -0.7 | +2.2 | +2.1 | -8.5 |
| Underwater V25 | +0.2 | -0.1 | 0.0 | +3.3 | +3.3 | **-6.8** |
| Cooldown djay | -5.8 | -15.9 | -5.1 | +1.5 | -3.1 | -27.0 |
| Cooldown V25 | **-4.6** | **-19.6** | -4.1 | +3.3 | +0.2 | -22.6 |
| Echo Out djay | -21.9 | -22.8 | -19.8 | -20.0 | -20.6 | -19.3 |
| Echo Out V25 | **0.0** | -0.8 | +2.1 | +0.9 | +0.6 | +1.8 |

- Underwater: now within 2 dB of djay. PASS
- Cooldown: within ~4 dB per band of djay. PASS (close)
- Flanger: close (both mild). PASS
- **Echo Out: still no effect**: `FX: deck 1 ECHO OUT on` at 4.2 s, the level stays -16 dB for 12 s, then `off`.
