# Round 37 on the Pixel 7 - V17 (bf1f6d7), CQ-12T on the Pixel's USB; 13.10.2026 17:35-17:52

## Part 1: lanes mirrored per deck - PASS
`v17_lanes_two_decks_mirrored.png`: each deck centred and mirrored in its own lane (Nanana top, 4B bottom), bar
numbers and beat lines while playing.

## Part 2: pair at the same bars (Nanana bar 1 to 2.6) - closer, still different
`pairs/nanana_bar01-02.6_djay_vs_v17.png` (djay above, V17 below, same bars and width):
- Colour: now near djay (crimson kicks, pink / lilac hats). djay also has **blue / violet low tails** between the
  hits that V17 lacks; V17 is more magenta overall.
- **Height**: djay ~15 % of its lane, V17 ~75 %.
- **Shape**: djay = one distinct burst per event (kick, then the off-beat hat), the oscillation visible inside,
  falling to a thin line between; V17 = denser blurred columns, the hits less separated, no thin line between.
- Bars / beat lines: same positions. PASS
(Bar 9-11 pair not made: see the next point, the app's deck did not reach bar 10.)

## NEW BUG: with the CQ-12T as the output, the deck plays at ~0.27x real time
- PLAY at 17:49:39 (`play 1`), PAUSE at 17:50:01 (`play 0`): 22 s of wall time, the deck's remaining time went
  -3:46 -> -3:40: **6 s of song in 22 s**. Same earlier: 18 s of song in 58 s (V16), 6 s in 17.4 s (V17).
  No xruns, no `late`, `Output level` lines every 10 s, slowest callback 20 ms.
- So the output stream (MIXER, perf None, 2 ch 48 kHz, on the CQ's USB) consumes ~1/4 of the frames it should:
  the deck's clock is the sound callback, so the whole deck runs slow. Together with the dark CQ meters this says
  the USB output is not running at 48 kHz stereo as Android's mixer thinks (the HAL configures the CQ at 16 ch
  32-bit 48 kHz while Android's USB device profile list is empty).
- The CQ's USB mode now (CQ MixPad on the Mac, read only, nothing changed): **Stream Mode Multitrack, 48 kHz**
  (`cq_mixpad_usb_multitrack_48k_now.png`); Android names the USB alt settings "CQ12T - Stereo Out" / "Stereo In"
  anyway (the descriptor's names).
