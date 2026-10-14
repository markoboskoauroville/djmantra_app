# Round 33 on the Pixel 7 - V11 (92e4965)

Tester "mantra dj app tester"; Pixel 7 over adb Wi-Fi, portrait 411x914. 13.10.2026 13:42 local.

## Part 1: waveform - FAIL (lanes and scrub still empty; overview colours now right)
- **Phrasing lanes**, both decks playing 10 s (`phrasing_lanes_empty.png`): no wave, only bar / beat lines (as V9, V10).
- **Scrub window** (`scrub_window_empty.png`): no wave.
- Log, identical to V10:
  ```
  Quick: waveform shader [Channel2]: compiled
  Quick: waveform shader [Channel2] made, status 0, level 1
  Quick: waveform shader [Channel1] made, status 1, level 1
  ```
  Still no `compiled` for Channel1, no ERROR, and the classic fallback does not show either.
- **Overview** (`cmp_nanana.png`, `cmp_4b.png`, djay on top): now pink-red / violet with green-teal breaks, as djay.
  PASS for colour; at phone width it is still blocks rather than djay's thin spikes.

## Part 2: GPU meter - PASS
- Log: `Quick: GPU meter watches the window`, `Quick: renderer OpenGL GPU timestamps NOT supported`
  (and the old `GPU load from "nowhere (the phone does not tell)"`).
- "G" shows **0 %** idle and **5 %** with both decks playing on the Phrasing page ("C" 2 % -> 8 %)
  (`gpu_meter_idle_playing.png`).

## Round 32 bugs
- ✕ now drawn as two strokes (`close_drawn.png`). PASS
- Neural Mix log: to NEURAL `Neural Mix [Channel1] on`, back to EQ `... off`. PASS
