# Round 36 (V16; tester "mantra dj app tester", Pixel 7; rules: testing/PROTOCOL.md)

New in V16: djay Pro's favourite FX (Marko 13.10.2026: Flanger, Underwater, Cooldown, Echo Out), a bar per deck at
the bottom of the MIXER page: 4 buttons (tap = on / off, several at once), below them a slider (the depth of the
last FX tapped; Echo Out: < 2 BEATS > instead) and a D/W knob. Log lines start `FX:`. DSP:
src/effects/backends/builtin/djayfx.h; first tuned by ear, then against your djay recordings (testing/djay/fx/).

## 0. Basics first: PLAY / pause both decks, volume faders, EQ knobs, the controller's LEDs (unchanged, a quick look).
## 1. Open MIXER: both FX bars show; log `FX: deck 1 FLANGER -> unit 1 slot 1` ... (8 lines, first start only).
## 2. Play deck 1 (Nanana, 1:00). Each FX on for 8 s, then off; record the phone's sound (observe.py listen /
the Mac's recording) and say what you hear: FLANGER (a jet sweep), UNDERWATER (muffled, drifting, resonant),
COOLDOWN (highs and deep lows glide away within ~1 s, a reverb washes in, a bit quieter). The slider at left /
middle / right for each; D/W at 0 = dry.
## 3. ECHO OUT at 1, 2, 4 BEATS: press it: the song fades out over that many beats while it echoes, then only the
echo rings on and dies down (each repeat darker). Press again: the song is back at once. Deck 2's bar acts only on
deck 2 (deck 1 plays dry meanwhile).
## 4. Same FX in djay vs the app (your recordings in testing/djay/fx/): what differs (speed, depth, colour, level)?
## 5. CPU with all 4 FX on (the usage meter), and any crackle.
