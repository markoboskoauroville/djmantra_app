# Round 31 on the Pixel 7 - V9 (9a2e4df, release "DJ Mantra V9 (test build 9a2e4df)")

Tester "mantra dj app tester" on Marko's Mac, Pixel 7 over USB. 13.10.2026 11:17 local. Smoke: `Qt Quick (GPU)`,
`interface ready 914x411`, `Sound output open ... 48000 Hz, 2 channels`.

## Part 1: the waveform against djay Pro - FAIL (overview good, lanes and scrub window empty)
Songs: Nanana (deck 1), 4B (deck 2, NEXT). djay reference drawn from the decoded .djayMetadata (testing/djay/render.py).

**Overview (deck header): shape PASS, colours paler than djay.**
- `cmp_nanana.png`, `cmp_4b.png`: djay on top, the app below (same width). The shape matches: mirrored, outline + body,
  the quiet breaks in the same places (Nanana ~51-60 % and ~79-85 % green / teal, as djay; 4B the teal intro bars
  at ~10-17 % and the break at ~45-62 %). The app's section marks sit on djay's visible energy changes.
- **Colours are washed out**: djay's body is saturated red-pink / blue-violet; the app's is greyish beige-pink with
  faint colour, except the part LEFT of the play head (before the cue) which is saturated red. djay's deck header
  is the same saturation everywhere. So either the unplayed part is dimmed too much, or the colours are mixed toward grey.
- djay's outline is spiky (thin peaks per hit); the app's outline is smooth / block-like.
- 4B, around 50-62 %: djay blue-violet, then red; the app orange-beige there.

**Scrolling lanes (Phrasing page, portrait, both decks playing 10 s +): NO WAVEFORM.**
- `phrasing_lanes_empty.png`, `phrasing_page.png`: only the #141414 background, the orange bar lines with numbers
  (10, 11, 12 ... 17, 18, 19) and grey beat lines, and the centre line. No mirrored wave, no arrows, for either deck.
- No log line about the lanes; no qml error.

**Scrub window (hold on deck 1's overview): opens, NO WAVEFORM** (`scrub_window_empty.png`): bar 24 line, grey
beat lines, nothing else.

## Fixed since round 30 (seen here)
- `Sections:` logged once per load (was 3 times). PASS
- 4B key: song list 6A, landscape deck header 6A. **But the portrait deck header still shows "House 1A"** (`phrasing_page.png`).
