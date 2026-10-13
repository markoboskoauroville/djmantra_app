# Round 33 (V11; tester "mantra dj app tester", Pixel 7; rules: testing/PROTOCOL.md)

Fixes for round 32:
- Lanes / scrub window: the shader took the wave's height from the texture's alpha ("all"), which reaches the
  phone as nothing; now from the 3 bands (as the classic shader). The Phrasing lanes are twice as tall (each deck
  shows half of its mirrored wave).
- Colours: refitted with Mixxx's exact bands (fidlib Bessel -3 dB, "all" ^0.632): pink-red / violet as djay, not
  orange-brown. Preview: testing/djay/v11_overview_nanana.png (djay top, V11 below).
- GPU meter: the Pixel gave no GPU timestamps; now the render thread's time per frame when there are none. Log:
  `Quick: GPU meter watches the window`, `Quick: renderer <api> GPU timestamps supported / NOT supported`.

- Round 32 bugs: the ✕ is drawn (two strokes; the phone's font had no ✕), the Neural Mix log says on / off the right way.

## 1. Waveform vs djay (Nanana, 4B, Brick House, Lovely Day): lanes, scrub window, overview colours. Screenshots.
## 2. GPU meter: the "G" value idle / both decks playing; the two log lines above.
## 3. Round 32 parts not done yet (3: Neural Mix, overlays, basics + CPU; 4: CQ12T part B; 5: CQ DELAY).
