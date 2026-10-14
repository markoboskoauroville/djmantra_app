# Round 37 (V17; tester "mantra dj app tester", Pixel 7; rules: testing/PROTOCOL.md)

From ROUND35 part 1 (djay vs V16 at the same bars): djay draws each deck's wave centred in its own lane, mirrored
up and down, translucent, kicks crimson and hats pink / lilac; V16 grew deck 1 up from the edge, opaque, orange.
V17: Settings -> LANE: MIRRORED (djay, default) / HALF (big, round 18). Mirrored = centred in each deck's lane,
0.8 of its height; colours: less green in red kicks (crimson), some red in the highs (lilac); body alpha 0.85, edges
fading. Also: beat / bar lines only near the screen (render-thread CPU).

## 0. Basics: PLAY / pause both decks, faders, knobs, LEDs (a quick look).
## 1. Phrasing with 2 songs loaded: each deck's wave centred and mirrored in its lane; bar / beat lines and bar
numbers still there while playing, after a seek, after a zoom change.
## 2. The same pairs as ROUND35 part 1 (Nanana bars 1-2 and 9-11) djay vs V17, testing/results/<date>_r37_pixel/pairs/:
what still differs (colour per hit, height, translucency, shape).
## 3. Settings -> LANE HALF, back to Phrasing: the round-18 half waves; then MIRRORED again.
## 4. CPU (usage meter, top -H: the render thread "QSGRenderThread") on Phrasing playing both decks: V16 was 18-21 %.
## 5. ROUND36 parts not done yet (FX bar, Echo Out beats, CPU with FX), CQ Ip1/2 meters if you find a way to see them.
