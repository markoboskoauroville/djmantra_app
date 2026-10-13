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
