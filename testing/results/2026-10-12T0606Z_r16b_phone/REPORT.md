# Round 16b on the phone — 12.10.2026, build 0e6c640 (run 38209637906), new interface

sha256 `c0544ceeb87bbc8069720ae7cdb518cfb82447cfe86c30de2a740d6955a07207`. Stopped by Marko after part 2 (redesign below).

| # | what | expected | result | pass/fail |
|---|---|---|---|---|
| 0 | Smoke check | interface ready | `Interface: Qt Quick (GPU)`, `Quick: interface ready 413x919`, both decks playing, sections found | PASS |
| 1a | Style, hole, menu icon | horoscope look, three-line icon, hole clear | Navy/yellow horoscope style, bottom tabs Mix / Mixer / Keys, three-line menu icon, hole clear (`m1-mix-hole.png`) | PASS |
| 1b | JUMP icon | drawn | **The icon under "JUMP" is tofu** (empty box + bar) | FAIL |
| 1c | Matrix | bar.beat, countdown, running lights | Works: deck 1 `8.1 … -9.0` (= 9 bars to its change at 32.2 s ✓), deck 2 `27.4 … -25.1` (✓ change at 67.3 s). **The dim deck number runs into the bar number:** "1" + "36.2" reads as "136.2" | PASS / readability FAIL |
| 2a | Beat launch | deck 2 stopped, pad 1 → starts on deck 1's phrase beat 1 | `Quick: beat launch [Channel2] on beat 1`. The matrix shows both decks on step 7 together, then both beat-1 cells yellow at the same moment (`m2-launch-*`) | PASS |
| 2b | JUMP (deck 1) | to the next section | 1:11 → 2:44 (the change at 160.2 s). Log `jump to the next section at 7.06298e+06` (raw samples, not seconds). **The new countdown says -9.0, but the next change (192.2 s) is ~14 bars away** | PASS / countdown check |
| 2c | KEY MATCH deck 2 (against 8A) | a compatible key | 12B → **7B**, and the badge itself says "8A > 7B diagonal" (risky). Compatible with 8A: 8A, 8B, 7A, 9A | FAIL |
| 2d | KEY MATCH off | back to its own key | 5B (its own key) | PASS |
| 2e | Key + | one semitone up | 5B → 12B (+1 semitone = +7 on the wheel) | PASS |
| 2f | Bars | stay hidden | Once, the status bar was faintly visible over the menu icon (`m3-keyplus`) | watch |

Not run: AUTO MIX, EQ cut-only, Mixer, Keys, library, loudness (Marko stopped the round for the redesign).
