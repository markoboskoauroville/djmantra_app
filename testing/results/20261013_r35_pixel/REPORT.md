# Round 35 part 1: djay Pro vs the app at the same bars (13.10.2026 17:00-17:15, app V16 a82f54e)

djay Pro on Marko's Mac (built-in display), Nanana on deck 1, sound into BlackHole. djay's lane zoom set with
View -> Waveforms -> Waveform Zoom + / - (no mouse). djay's waveform options (View -> Waveforms): **Bottom ✓,
Horizontal ✓** (Top / Center / Vertical off). Position by Deck 1 -> Go to Beginning + Play / Pause.

## What djay draws (pairs/*, djay on top, the app below, both cropped to the SAME bars and width)
1. **Mirrored around the centre line**, symmetric up and down. The app draws deck 1 only ABOVE the centre (deck 2 below).
2. **Thin**: at this zoom djay's wave is ~15 % of its lane's height (peak to peak); the app's fills ~80 % of its half.
   (On this Mac's two-deck layout djay's lane is short; Marko's "tall" screenshot may be another layout / zoom.)
3. **It is the actual sound wave, not bars**: each hit is an arrow-shaped burst whose inside is the oscillating waveform
   (the kick's sine wiggles are visible, `djay_nanana_bar1-2_zoomed_more.png`), sharp attack, tail fading to a thin
   line between hits. The app draws vertical needle columns (a peak per pixel column) with no oscillation inside.
4. **Colours per event**: the kick burst crimson red, the off-beat hat burst pink / lilac, the space between blue /
   violet / a little green; translucent, no outline. The app: orange-red, salmon, violet, grey-blue, more orange
   than djay's crimson, opaque.
5. **Timing / structure match**: kick on every beat and an off-beat hat in between in both; bar lines on the same
   positions (djay yellow bar lines + grey beat lines; the app orange + grey). PASS
6. djay shows bar numbers small at the top-left of each bar line, as the app.

## Also seen
- The app's deck 1 played only ~18 s of a 58 s run: my first PLAY tap left no `play 1` line in the log, the
  second logged `play 0` (to check: a tap lost while the screen woke, or a log line lost).
