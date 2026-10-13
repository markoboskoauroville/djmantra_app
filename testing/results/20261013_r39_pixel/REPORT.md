# Round 39 on the Pixel 7 - V30 (4247bec); 13.10.2026 22:49-23:03

## Part 1: deep zoom, the real wave - PASS (djay comparison waits for the Mac's screen)
- Load Nanana 22:49:43.6 -> `Fine wave: 464203 points, 22 frames each, for ".../(It Goes Like) Nanana - Edit.mp3"`
  at 22:49:47.6: **4.0 s**, 2,000 points per second of song. The line is logged **twice** (two threads, same time).
  No xrun / late during it.
- Zoom + steps: `waveform zoom 2.00, 1.33, 0.89, 0.59, 0.50` (0.50 is the deepest; ~2 beats = ~0.9 s across the lane)
  (`p1_zoom_levels_2.0_1.33_0.59_0.50.png`). At 0.59-0.50 the kicks turn into regular wiggles ~21 px apart = ~55 Hz,
  the kick's own frequency (`p1_deepest_zoom_detail.png`): the real wave. It is drawn mirrored (symmetric), colours as
  before. Zoomed out: the envelope again.
## Part 2: FX depth / Echo Out beats - PASS with two notes
- Hold the FX button: the chooser above it, nothing cut; under the effects **DEPTH** (a slider) for Flanger /
  Underwater / Cooldown (`p2_chooser_depth.png`), **1 BEAT / 2 BEATS** for Echo Out (`p2_chooser_echo_beats.png`).
  1 BEAT: `FX: deck 1 ECHO OUT 1 beats`, lit. Each effect keeps its own depth.
- **Underwater** (recorded, djmantra-test-songs `recordings/r39_depth_v30/`): slider **left = open** (bands as dry);
  **right = 2-6 kHz -25 dB, 6-16 kHz -43 dB, 400 Hz-2 kHz unchanged**: a ~2 kHz lowpass, not ~700 Hz as specified.
  (Middle was not captured cleanly: the slider was still at the right from a muddled first take.)
- **Cooldown** at the middle: rms ~-5 dB, lows -19 dB, 400-2k ~0, 2-6k -1, 6-16k -23 dB (djay -5.8 / -15.9 / ... / -27):
  close. Left / right not captured: in this take the effect was already on when the recording began although
  `COOLDOWN off` had been logged just before, and none of the FX taps during the take were logged; the scrcpy
  capture reopens the app's output at its start (see below), which may re-apply the effect's state.
- **Note on recording:** while scrcpy captures the phone's sound, Android moves the app's output off the CQ
  (`Sound output disconnected ( "ErrorDisconnected" )`, reopen, Output rate ~48,000) and back afterwards: all FX
  recordings ran at full speed but did not go to the CQ.

## V31 (67477d3), 23:24-23:32: djay pairs (Mac awake again)
- **Fine wave built once?** No: `Fine wave:` still logged **2 times** on load (Nanana).
- **Lanes, bars 1 to 2.6** (`pairs/nanana_bar01-02.6_djay_vs_v31.png`, djay above): colours and the kick / hat pattern
  as djay; djay ~15 % of its lane, V31 ~70 % of its half; djay's hits are separate short bursts with a thin line between,
  V31's are fuller and run together. (Bars 9-11: same look as in round 37, not repeated.)
- **Deepest zoom** (`pairs/nanana_deepest_zoom_first_beat_djay_vs_v31.png`): the first beat of bar 1 (kick, then the
  off-beat hat), both at their deepest zoom (djay View -> Waveforms -> Waveform Zoom + x8; the app's zoom 0.50, which
  goes deeper than djay: one beat is ~half its lane, djay's ~1/8):
  - **djay draws the signed waveform as one thin line**: the kick is ~9 clear sine cycles shrinking, then a thin flat
    line, the blue hat's small wiggle, the next hit; tiny height (~15 % of the lane), no fill.
  - **V31 draws filled, mirrored columns** (a peak envelope per pixel column, symmetric up and down): no single line,
    the kick's cycles not readable; its height ~80 %.
  - To look like djay at deep zoom: draw the signed samples as a polyline (min / max per pixel only when there are more
    samples than pixels), not mirrored, thin, at djay's height.
