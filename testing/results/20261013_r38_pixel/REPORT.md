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

## CORRECTION (19:15): Android's own path to the CQ is slow too - not only our stream
- YouTube (com.google.android.youtube) playing to the CQ (DJ Mantra stopped): Android's USB MIXER thread `AudioOut_15`
  (Output devices 0x4000 USB_DEVICE, 48000 Hz, 2 ch, float, HAL frame count 960, Standby no) wrote
  **25,485 and 26,025 frames/s** over two 20 s windows (`dumpsys media.audio_flinger` "Frames written" deltas):
  **~0.54x of 48000**. So the platform (USB HAL with the CQ at 16 ch 32-bit) does not run the CQ at real time either.
- RS Audio Player's 1.0x clock earlier was most likely its own wall clock (or its own USB driver), not proof of the
  platform: my conclusion "it is our stream" above was wrong. DJ Mantra's 0.30x may be the platform's 0.54x plus our
  callback pattern.
- (YouTube showed "Allow YouTube to send you notifications?": left unanswered for Marko.)

## V26 (8733cea), 19:30-19:36
### Part 13: format variants on the CQ - all ~0.3x
```
Could not open 16 channels ( "ErrorOutOfRange" ), trying 2
... reopening without MMAP (PerformanceMode None); Sound buffer: 16384 of 16384; burst 8192
19:31:03 Output rate: 13648 of 48000 -> Output too slow: trying variant 1   (format I16, 48000 Hz)
19:31:19 Output rate: 14982            -> variant 2 (format I32, 48000 Hz)
19:31:36 Output rate: 14727            -> variant 3 (format I16, 48000 Hz)
19:31:52 Output rate: 14890
```
- Android's USB threads, "Frames written" deltas over 20 s windows while **our** deck played:
  **AudioOut_D** (MIXER, USB_DEVICE, HAL frame count 480, HAL buffer 3840 bytes) +285,120 / 20 s = **14,256 frames/s**
  (= our Output rate); **AudioOut_15** (MIXER, USB_DEVICE, HAL frame count 960, buffer 7680 bytes) unchanged.
- With **YouTube** (round 38 correction) the audio went through **AudioOut_15** at ~26,000 frames/s.
- So Android has two USB output threads for the CQ: ours lands on AudioOut_D (480-frame periods) at 0.30x,
  YouTube's on AudioOut_15 (960-frame periods) at 0.54x. Both slow; the one with the longer period is faster:
  the CQ's USB seems to accept a fixed number of HAL writes per second, not frames.
  (AudioOut_D is likely the "fast"/low-latency USB mix port, AudioOut_15 the normal one; our stream may be
  routed to the fast one because of its usage / flags.)
### Part 14: Echo Out diagnostics - the effect itself logs nothing
```
19:35:35.107 FX: deck 1 ECHO OUT on  (slot org.djmantra.effects.echoout, button_parameter1)
19:35:39.274 FX: deck 1 ECHO OUT off (slot org.djmantra.effects.echoout, button_parameter1)
19:35:42.444 FX: deck 1 ECHO OUT on  (...)
19:35:46.557 FX: deck 1 ECHO OUT off (...)
```
No `Echo Out:` line at all from the effect (deck 1 playing): the slot's button_parameter1 is set, but the effect
does not seem to process / log (not loaded in the chain? disabled slot? wrong parameter index?).

## V27 (b2f49a6), 19:54-19:59
### Part 16: the deep-buffer variants reach the YouTube ceiling (~0.58x), but the app does not keep them
| variant | Output rate | Android USB thread (Frames-written delta) |
|---|---|---|
| first open (perf None, Float) | 13,759 | AudioOut_D ~14,000/s |
| 1: Float, **PowerSaving** | **27,918** | **AudioOut_15** ~25,000/s |
| 2: I16, **PowerSaving** | **27,251** | AudioOut_15 |
| 3: I16, perf None | 14,780 | AudioOut_D |
| 4: I32, perf None | 14,613 | AudioOut_D |
| 5: I16, perf None | 14,862, then 14,389 / 13,800 / 14,290 (kept) | AudioOut_D ~14,000/s |
- PowerSaving lands on the deep-buffer thread AudioOut_15 (HAL period 960) at 0.58x, the same as YouTube; every
  perf-None variant lands on AudioOut_D (period 480) at 0.30x. **The app keeps trying after the best one and ends on
  variant 5 (0.30x)**: it should keep the fastest variant (PowerSaving Float) when none reaches 80 %.
- 0.58x is Android's ceiling on this CQ in Multitrack: the Stereo / own-driver decision (needs_marko.md) stands.
### Part 15: Echo Out works - PASS (decay a bit slower than djay)
```
Echo Out:  engaged true enable state 1 beats 2 mix 1     (on)
Echo Out:  engaged false enable state 1 beats 2 mix 1    (off)    x2
```
Recorded (scrcpy, Nanana 0:58, ON at 4.2 s, OFF at 16.4 s; djmantra-test-songs `fx/app_v27_20261013/`): per second
-16 ... -21, -25, -30, -35, -40, -45, -49, -54, -59, -64, -69 dB, OFF -> -18 / -16 at once.
Band change vs dry (`p15_echoout_djay_vs_v27.txt`): 2-6 s after ON djay -21.9 dB, V27 -17.6; 6-10 s djay -52.4,
V27 -36.7: **djay dies faster (~7.5 dB/s vs ~5 dB/s)**; the first 2 s: djay drops the lows more (-7 vs -2 dB at <150 Hz),
V27 the mids (-7 at 400-2k).

## V28 (abefba6), 20:21-20:26 - part 17
### The CQ keeps the fastest variant - PASS (at Android's 0.55x ceiling)
- Variants as V27 (1: Float PowerSaving 27,813; 2: I16 PowerSaving 27,237; 3-5 perf None ~14,700), then
  `No variant runs in real time; keeping variant 1 ( 27813 frames/s)`, and it stays: 27,735, 25,894, 26,130, 25,758,
  25,568, 25,560 frames/s (0.53-0.58x). 122 s of play = 54 s of song (the first 67 s include the variant search).
### Echo Out decay - now faster and thinner than djay
Per second after ON (4.2 s): -18, -23, -31, -39, -48, -55, -62, -70, -79, -87, -94, -105 (~8 dB/s); OFF -> -24 / -16.
Band change vs dry (`p17_echoout_djay_vs_v28.txt`), V28 vs djay:
- 0-2 s after ON: rms -10.4 vs -4.4; lows (<150) -16.8 vs -7.1
- 2-6 s: rms -33.9 vs -21.9; lows **-59.1 vs -22.8**
- 6-10 s: rms -63.9 vs -52.4; lows -97.3 vs -54.5
So V28 overshoots: ~8 dB/s vs djay's ~7.5 is close, but it starts ~6 dB lower in the first 2 s and the lows are
thinned far more than djay's (djay keeps the lows at the same level as the other bands while it dies).
(V27 was the other way: ~5 dB/s, lows kept.) Halfway between, with lows kept like the other bands, would match.

## V29 (454c5f3) - part 18: Echo Out matches djay - PASS
Per second after ON: -18, -25, -31, -38, -45, -51, -58, -65, -71, -78, -84 (~6.6 dB/s); OFF -> -24 / -16 at once.
Change vs dry (`p18_echoout_djay_vs_v29.txt`), V29 vs djay:
- 0-2 s: rms -4.2 vs -4.4 (lows -2.6 vs -7.1, mids -7.8 vs -2.7)
- 2-6 s: rms -23.4 vs -21.9, every band within 5 dB (lows -23.6 vs -22.8)
- 6-10 s: rms -49.5 vs -52.4, every band within 3.5 dB
All four FX now within a few dB of djay's recordings (Flanger, Underwater, Cooldown on V25; Echo Out on V29).
Small remaining difference: in the first 2 s djay dips the lows more and V29 the mids.

## V29: ROUND35 part 4, STOP TIME - PASS
Settings -> STOP TIME (OFF / 0.5 / 1 / 2 / 3 s). Deck 1 Nanana playing, PLAY pressed to stop, the phone's sound recorded
(scrcpy; djmantra-test-songs `recordings/stop_time_v29/`), level / spectral centroid every 0.25 s:
- **OFF**: no `turntable` line; the sound ends within 0.25 s.
- **1 s**: `turntable stop [Channel1] 1 s`; the level falls -16 -> -21 -> -29 -> -45 dB and silent within ~0.75 s,
  the centroid 870 -> 420 -> 385 Hz (the pitch falls).
- **3 s**: `turntable stop [Channel1] 3 s`; ~2.5 s of slow-down: -17 ... -22, -25, -25, -35, -26, -35, -96 dB, centroid
  at the end 1075 -> 822 -> 410 -> 261 Hz (the pitch falls).
- **PLAY again during the 3 s slow-down** (1.3 s in): `turntable stop [Channel1] cancelled` and the deck plays on at its
  level, no gap (recording continuous at -15..-18 dB). The log then prints `button [Channel1],play 0` right after
  "cancelled" though the deck plays (the log line's value looks wrong; the sound is right).
- Not checked: a synced deck at another tempo, and the controller's PLAY (needs the Mix Ultra on).

## V29: ROUND37 part 4, CPU on Phrasing with both decks playing (Nanana + 4B, lanes drawn)
Process 40, 48, 47 % of a core (V14: 48-52, V13: 62-76). Per thread (`top -H`, 3 samples): **QtThread (render) 15, 8, 7 %**
(V16: 18-21), AudioTrack 15, 4, 4 % (the audio callback; on the CQ route it now runs on Android's AudioTrack /
PowerSaving variant), qtMainLoopThread 11, 0, 4 %, mali / binder 4 % now and then. `cpu_v29_phrasing_both_playing.png`.
Note: the output is the CQ (0.55x), so the decks advance slower than real time in this measurement.
