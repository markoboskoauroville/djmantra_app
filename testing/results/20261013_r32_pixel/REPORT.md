# Round 32 on the Pixel 7 - V10 (99d5f7b)

Tester "mantra dj app tester" on Marko's Mac; Pixel 7 now over **adb on Wi-Fi** (192.168.1.103:5555), portrait
(interface 411x914). 13.10.2026 12:55 local. Smoke: `Qt Quick (GPU)`, sound output open 48000 Hz 2 ch.

## Part 1: waveform vs djay - FAIL (lanes and scrub window still empty; overview better)
Log (all lines about it):
```
Quick: waveform shader [Channel2]: compiled
Quick: waveform shader [Channel2] made, status 0, level 1
Quick: waveform shader [Channel1] made, status 1, level 1
```
(no `compiled` line for Channel1; no ERROR line; no classic-waveform fallback either.)
- **Phrasing lanes**, both decks playing > 10 s (`phrasing_lanes_empty.png`): still only #141414, orange bar lines with
  numbers, grey beat lines, centre line. **No wave, no arrows, for either deck.**
- **Scrub window** (hold on deck 1's overview, `scrub_window_empty.png`): bar line 126 and beat lines, no wave.
- **Overview** (`cmp_nanana.png`, `cmp_4b.png`, djay on top): now saturated (fixed); the breaks and shape match.
  At phone width the columns are wide blocks rather than djay's thin spikes, and the body leans orange-brown where
  djay is pink-red / violet.

## Part 1b: PLAY looks like NEXT - PASS
`buttons_paused.png`: PLAY is a white triangle of NEXT's size and fill; `buttons_playing.png`: pause bars (yellow).

## Seen
- 4B shows 6A in the list and on the deck when loaded; while SYNCed to Nanana (126 -> 130 BPM) the header shows
  **1A** (the key moved by the tempo, +0.5 semitone rounds to +1): so round 31's "1A" was the tempo-shifted key, not a bug.

## Part 2: GPU meter - FAIL
- "G" shows `--` idle and with both decks playing on the Phrasing page (`gpu_meter_idle.png`, `gpu_meter_playing.png`).
- The only GPU log line: `Quick: GPU load from "nowhere (the phone does not tell)"`. **No** `GPU time per frame from
  the renderer's timestamps` line.
- As shell: `cat /sys/devices/platform/28000000.mali/utilization` -> Permission denied (as in round 31).
- For scale: `top` shows the app at 10.3 % CPU (of one core) with both decks playing; the meter's "C" says 2 %
  (so "C" is of all 8 cores?).

## Part 3: round 31's 1b / 1c / 2 on V10
### Neural Mix - PASS with one log bug
- Mixer page, deck 1: EQ -> **NEURAL**, the knob names become HARMONY / VOCALS / DRUMS (`neural/knobs_*.png`, left
  to right: as set, HARMONY down, VOCALS down, DRUMS down, after double-tap reset: each back).
- **The log is inverted**: switching to NEURAL logs `Neural Mix [Channel1] off`, switching back to EQ logs `... on`
  (checked twice). The sound follows the button (the knobs act as stems only in NEURAL).
- Recorded from the phone (scrcpy audio, 8 s each, deck 1 Nanana; the mp3s are in the private repo djmantra-test-songs, `recordings/r32_neural_v10/`). Each knob dragged down ~2/3
  (a swipe did not reach 0): overall level / band energy (dB) against `full`:
  | | rms | <100 Hz | 100-300 | 300-3k | 3k-12k |
  |---|---|---|---|---|---|
  | full | -18.9 | 82.1 | 79.4 | 80.5 | 68.7 |
  | HARMONY down | -22.4 | **66.1** (-16) | 75.2 | 77.2 | 64.9 |
  | VOCALS down | -24.7 | 74.8 | 71.7 | **73.9** (-6.6) | 64.2 |
  | DRUMS down | -21.5 | 81.3 | 76.5 | 74.6 | **62.8** (-6) |
  Each knob removes its part (HARMONY takes the bass line with it, VOCALS the mids, DRUMS the hats / highs); the
  ears' check (voice gone, band stays) is for Marko: the mp3s are here.
- EQ <-> NEURAL while playing: no gap in the output level lines, no jump against deck 2 (beat counter unchanged).
  A click by ear not checked (the phone's sound goes to BlackHole).
- CPU (`top`, deck 1 alone playing, Mixer page): NEURAL 11-15 %, EQ 10-17 %: no visible difference.
### Every overlay closes - PASS (one look bug)
- Landscape: cover -> song list (Drive150) -> ✕: `song list closed (✕)`, `browse closed`, decks back, nothing loaded.
  Again with Android back: `browse closed`. Portrait: menu ✕ closes, Settings ✕ closes, menu + back closes.
- **The ✕ is drawn as a missing-glyph box** (a rectangle with an X inside, `close_glyph_missing.png`) on the menu,
  Settings and the song list: the font has no "✕".
### Basics - PASS; CPU on Phrasing unchanged
- CUE both, PLAY 1, PLAY 2 -> `beat launch [Channel2] on beat 8`; 30 s both playing on the Phrasing page:
  `xruns 0`, `late 0`, slowest callback 3.7-6.4 ms.
- CPU, Phrasing page, both playing (deck 1 in NEURAL): 48-72 % of a core (5 samples: 72, 48, 66, 66, 72), as round 25 (65 %).
