# Learning rekordbox's phrases (12.10.2026)

rekordbox 7.2.19's phrase analysis of Marko's songs (`../rekordbox.json`, 2556 phrase starts in 165 songs) is
the target. `dataset.py <rekordbox.xml>`: per bar of rekordbox's own beat grid (XML TEMPO markers; 2555 of 2556
phrase starts fall exactly on its bar lines) the features the app can compute from its waveform: bar
self-similarity novelty (checkerboard 1 / 2 / 4 / 8 bars), band changes (low / mid / high, 1 / 2 / 4 bars), the
4 / 8 / 16-bar position, the energy. `learn.py`: logistic regression on half the songs, tested on the other half.

On the test songs (exact bar / within one bar, F1):
- detector v2 (V4), given rekordbox's grid: 0.24 / 0.40 (precision 0.66 within a bar, recall 0.29)
- learned (logistic): 0.37 / 0.61 (precision 0.56, recall 0.67)
- gradient-boosted trees: 0.39 / 0.56 (no real gain; not worth the code)

Note: on this disco / funk / pop set rekordbox's phrases are mostly 4 bars (also 2, 3, 5, 7), not a strict
8 / 16 / 32 grid; v2's 8-bar grid assumption is why it misses so many.
