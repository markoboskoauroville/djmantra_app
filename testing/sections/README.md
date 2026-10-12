# Section detection: the app today vs a reference (tester, 12.10.2026)

Marko: "the second number ... should reset on every section. That's why section detection needs to be perfect ...
compare it ... with something from Serato or from Pioneer Rekordbox software. Must be in that level of quality."

Tool: `sections.py` (numpy, scipy, matplotlib; run offline on the phone in PRoot, ~3 s per song). No audio in git.
Per song: `<song>.png` (picture) and `<song>.json` (numbers). 8 songs from the phone's Music folder.

## How to read a picture
Top: timbre per bar (40 mel bands). Then the app's 3 waveform bands (low / mid / high), the kick (grey = kick
strength per bar, black line = kick on / off), the reference's novelty, and the reference's labels.
**Red** = the app's detector today (a faithful port of `sectiondetector.cpp`, on the same beat grid).
**Cyan** = the reference: solid = major section (kick in / out, a strong change), dotted = minor (an 8-bar phrase
inside a part). Grey lines = 8-bar phrases. Right: the bar self-similarity matrix (blocks = parts).

## Findings
1. **The app's sections are late.** On the phone (logcat `Sections:`) they land 0.3-1.1 s after the real change
   (Aktysia +0.3-0.4 s, Addis +0.6 s, 3 Sense +1.1 s; at 120-122 BPM that is 1-2 beats). The bar / phrase grid is
   counted from the song's first detected beat (or the cue), not from the music's real downbeat and phrase start.
   In the port, counting phrases from the first beat puts them **1 bar late** on 5 of 8 songs (Aktysia 64 vs 63,
   80 vs 79, 184 vs 183, 208 vs 207: the kick changes at 63, 79, 183, 207).
2. **Half of the changes are missed.** Against the reference's major sections (60 in 8 songs): the app is exactly
   on 12, 1-4 bars off on 33, misses 11, and has 6 that are not there. On the phone it found 3 of 7 in Addis and
   5 of 9 in Aktysia. Energy in 3 bands over 4 bars sees the bass coming or going, but not a change of sound with
   the same loudness (a new lead, the breakdown's parts), and a change between its 4-bar lines is pushed to the
   next one.
3. **Every real change sits on an 8-bar phrase line**, counted from the right "1". In the reference 70 of 80
   section starts are on the 8-bar grid; the rest are 4-bar lines inside an intro / outro or a 1-bar pickup.

## The reference (what Rekordbox's phrase analysis does, in DSP)
Serato DJ has no automatic section / phrase detection (cue points and beatgrids only). Rekordbox (6+) has
"Phrase analysis": it labels Intro / Up / Down / Chorus / Outro (plus Verse / Bridge for other music) on an
8 / 16 / 32-bar grid, and that is the bar to reach. The reference does the same steps:
1. **Beat grid**, constant tempo: onset strength (log-mel flux), every BPM 100-140 in 0.05 steps with its best
   phase, then +-0.1 BPM in 0.005 steps (all 8 songs: 120 / 122 / 124, as the library says).
2. **Downbeat**: the beat phase whose bar-to-bar change of low / mid / high is largest.
3. **Kick track** per bar: the 35-110 Hz band's pulse on the beats; on / off with hysteresis (on > 0.6, off < 0.35);
   **a state that lasts 1 bar is a fill and is ignored** (the bar without kick before a phrase start).
4. **Phrase "1"**: the 8-bar offset where the kick changes and the band changes vote most (not the first beat).
5. **Novelty**: per bar a vector (40 log-mel bands, 12 chroma, the 3 bands), z-scored, cosine self-similarity
   between all bars, Foote checkerboard kernels of 4 and 8 bars (Gaussian tapered), averaged.
6. **Score per bar** = novelty + 0.6 x kick change + 0.8 x band change (the app's own |log ratio| over 4 bars,
   minus 0.3), x 1.0 on an 8-bar line, 0.75 on a 4-bar line, 0.35 elsewhere.
7. **Peaks** above median + 1.5 x MAD + 0.15, at least 8 bars apart; every kick change on a 4 / 8-bar line is a
   section even if weak. **Major** = a kick change, novelty >= 0.5 or band change >= 0.35; the rest minor.
8. **Labels**: kick on and loud = Chorus, kick on and quieter = Up, no kick before the first Chorus = Intro,
   after the last = Outro, between = Down.

What the app has to port (C++): 3 and 4 work from the waveform it already has (low band ~ kick); 5 needs a
spectrum per bar (a small mel analysis while the track is analysed, or the waveform's 3 bands as a weaker
stand-in); the counter reset should use the **major** sections.

## Limits (honest)
- There is no human truth here: "reference" means a stronger algorithm judged by the pictures, not a proven
  answer. The way to a real comparison with Rekordbox: Marko analyses the same songs in Rekordbox (Phrase
  analysis on), exports them to a USB stick, and copies `PIONEER/USBANLZ/` to the phone; the `.EXT` files hold the
  phrase list (PSSI tags), which the tester can read and compare bar by bar.
- 8 songs, all 4/4 electronic at 120-124 BPM. Tempo changes, live drums or non-4/4 are not covered.
