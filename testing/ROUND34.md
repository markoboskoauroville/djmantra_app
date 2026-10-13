# Round 34 (V12; tester "mantra dj app tester", Pixel 7; rules: testing/PROTOCOL.md)

The lanes' real bug (round 33): Mixxx's QML player built the waveform texture only on `waveformUpdated`, which a
track analysed before never sends after loading: the texture stayed empty for every song (classic or djay shader).
V12 builds it when the track loads, opaque (the "all" byte in the PNG's alpha dimmed or dropped the bands), at most
every 2 s during an analysis.

## 1. The scrolling lanes (Phrasing page) and the scrub window show the wave: thin, mirrored, coloured arrows,
orange numbered bar lines. Screenshots next to djay (Nanana, 4B, Brick House, Lovely Day). A song analysed for the
first time on the phone too (a new file): the wave appears when the analysis ends.
## 2. CPU on the Phrasing page, both decks playing (`top`), vs 48-72 % in round 32.
## 3. Then the CQ parts (round 32 parts 4-5) when the Pixel's cable is on the CQ.

## 4. (V13) Thinner as djay: lanes 0.35 / scrub 0.22 of the height, the overview's body 0.6 of before. Screenshots
vs djay. CPU: `top -H -n 1 -b -p $(pidof com.djmantra.app)` on the Phrasing page, both playing: which threads
use the most (render / main / audio / analysis)?

## 5. (V14) CPU: the matrix display paints only its lit dots (the unlit grid and dim cells once, as a layer):
the same per-thread `top -H` on the Phrasing page, both playing; and on the Songs and Mixer pages. The matrix must
look exactly as before (screenshot).
