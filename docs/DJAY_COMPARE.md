# DJ Mantra vs djay (the reference app): findings

djay for Android 5.6.9 on the Pixel 7, against DJ Mantra on the same files. Raw results are in
`testing/results/*djay_compare*`. The manual is in `docs/DJAY_MANUAL.md`.

## Analysis (14.10.2026, V41, the 7 songs of Music)

| | djay | ours | verdict |
|---|---|---|---|
| BPM | 86, 111, 115, 118, 119, 120, 120 | 84.1, 109.9, 114.6, 118.0, 118.4, 120, 120 | within 2 BPM |
| Key | 4 of 7 the same | | cura: djay 11A, ours 2B; the test tones: djay 1B / 2B, ours 11B / 12B (2 steps apart: check which is right with a tuner) |
| Grid | one tempo per song | Aum Ganesha a beat map drifting 91 / 109.9 / 117.8 | **fixed in V43**: the steady grid is the default |

Open:
- The key detector on the test tones and on cura.
- Whether the 1-2 BPM gaps (84.1 vs 86, 109.9 vs 111) are ours or djay's rounding; tap the tempo to check.

## Still to compare
- Waveform: colours (djay: red bass, yellow low-mid, green high-mid, blue highs) and zoom.
- Sound, with both apps' recorders: EQ, Neural Mix, FILTER, FX.
- Crossfader auto-play, Automix.
