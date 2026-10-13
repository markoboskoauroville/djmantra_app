# Round 38 (V20 = V18 + the FX tuned to djay's recordings + the CQ over USB MIDI; tester "mantra dj app tester", Pixel 7 + Marko's Nothing Phone if he is back; rules: testing/PROTOCOL.md)

Marko 13.10.2026 17:22 (testing/results/20261013_marko_ui/REQUEST.md): one EQ dropdown as djay Android, one FX button
per deck (tap on/off, hold = drop-up chooser), nothing overlapping. V18 MIXER page: "EQ ▾" at the top (Equalizer /
Neural Mix EQ, both decks together; the per-deck EQ/NEURAL buttons are gone); per deck under FILTER: the FX button
(its name = the chosen effect, lit when on), then headphones. The V16 FX bar is gone. Cloud preview (412 dp wide):
testing/results/20261013_marko_ui/v18_mixer_preview.png. Neural Mix Crossfaders (djay's 3rd entry) is not built yet.

## 0. Basics: PLAY / pause, faders, knobs, LEDs (quick).
## 1. MIXER page: screenshot; nothing overlaps, no cut labels (GAIN..FILTER, FX, headphones, faders, VU).
## 2. EQ ▾ -> Neural Mix EQ: both decks' labels turn DRUMS / VOCALS / HARMONY, log `EQ mode: Neural Mix EQ`, the
knobs act on drums / vocals / harmonies; -> Equalizer: back. A tap outside closes the menu.
## 3. FX button deck 1: tap = on (lit, log `FX: deck 1 FLANGER on`), tap = off. Hold 0.5 s: the chooser opens above it;
pick UNDERWATER: the button says UNDERWATER, it is on; then COOLDOWN, ECHO OUT (the song fades out over 2 beats into
its echo; tap again = the song back). Deck 2 the same, only on deck 2. The choice stays after an app restart.
## 4. V19: the FX tuned to your djay recordings (testing/djay/fx/notes.md): UNDERWATER = a lowpass at 3.3 kHz
(djay's middle), level kept; COOLDOWN = band-pass (lows < 400 Hz and highs > 3 kHz cut) and -6.5 dB at once, back at
once; ECHO OUT = runs while chosen; tap = the song stops at once and its LAST 2 beats loop, each round -4.4 dB
(0.6), silent after ~10 s; tap again = the song back at once. Record each from the phone the same way as djay's
(same song, 0:58, ON 8 s) and run your band analysis on both: the numbers side by side in the report.
## 5. ROUND37's lane parts if not done (mirrored lanes, pairs).
## 6. V20: the CQ over its own USB MIDI (the CQ on the phone's USB): Settings -> CQ MIXER -> USB. Expect the status
`connected over USB MIDI (Allen&Heath Ltd CQ12T ...)`, log `DJMantraCQ: Open: ...`. Then MIXER -> CQ DELAY (shown when
connected): the CQ's FX mute (FX1-4 per Settings) toggles in MixPad / on the CQ screen, and the tap tempo soft key
blinks at the on-air deck's BPM. Report the log lines `CQ: mute ...` / `CQ: tap tempo ...` and what the CQ shows.
