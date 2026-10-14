# djay Pro's four favourite FX, recorded on Marko's Mac (13.10.2026, tester "mantra dj app tester")

**The audio files are in the private repo markoboskoauroville/djmantra-test-songs, folder `fx/djay_20261013/` (same names, .flac).**

Song: **(It Goes Like) Nanana - Edit**, **130.0 BPM** (djay), key 6A. Every take starts at **0:58** of the song
(Deck 1 -> Go to Beginning, Play, 58 s), djay's output -> **BlackHole 2ch** -> ffmpeg (avfoundation ":1"), 48 kHz stereo,
FLAC. djay's FX bar in **MANUAL** mode, deck 1: slot 1 Flanger, slot 2 Underwater / Cooldown, slot 3 Echo Out
(`djay_fx_bar_deck1.png`); **D/W knobs at full wet**. Switched with djay's menus (Deck 1 -> FX -> FX n Enabled /
Select Next), sliders dragged with cliclick (Marko away, he allowed it). Each take: level before / after checked, so
djay played the whole take. The timings of each step: `takes_log.json`; the script: `record_djay_fx.py`.
Per-second band levels of every take: `analysis_1s_bands.txt` (rms | <150 Hz | 150 Hz-2 kHz | >2 kHz | centroid).

| file | steps (s from the file's start) |
|---|---|
| `flanger_dry8_on8_off4` | dry 0, ON 8.2, OFF 16.4 (slider middle) |
| `flanger_slider_left_mid_right` | slider left 1.1 (dry), ON 4.3, middle 9.5, right 14.7, OFF 18.9 |
| `underwater_dry8_on8_off4` | dry 0, ON 8.2, OFF 16.4 (slider middle) |
| `underwater_slider_left_mid_right` | as above |
| `cooldown_dry8_on8_off4` | dry 0, ON 8.2, OFF 16.4 (slider middle) |
| `cooldown_slider_left_mid_right` | as above |
| `echoout_1beat_dry4_on12_off4` | dry 0, ON 4.2 (1 beat), OFF 16.4 |
| `echoout_2beats_dry4_on12_off4` | dry 0, ON 4.2 (2 beats), OFF 16.4 |

## What the numbers show (listen to confirm)
- **Cooldown** (slider middle): at ON the level drops ~6.5 dB at once, the lows (<150 Hz) fall ~18 dB, the highs
  ~6 dB, the centroid rises 350 -> 650-750 Hz (a thin, filtered sound); OFF: back within ~1 s.
  **Slider**: left = hardly any change; middle = lows -17 dB (as above); **right = fades the deck to near silence in
  ~2 s** (-20 -> -50 dB, all bands). So the slider is the depth, and at the right end Cooldown is a fade-out.
- **Echo Out**: at ON the song stops feeding in and only echoes remain, each repeat quieter: the level falls
  **~10 dB per second at 1 beat**, **~6.5 dB per second at 2 beats**, to silence (-90 dB) after ~6 s (1 beat) /
  ~10 s (2 beats); the colour stays (band balance unchanged while it dies). **OFF: the song is back at once** at
  full level. Echo spacing from the envelope: ~0.42 s for 1 beat and ~0.42 / ~0.85 s for 2 beats (the beat at
  130 BPM is 0.46 s; my envelope estimate is rough: check by ear / waveform).
  **djay's Echo Out goes only to 2 beats** (the > arrow is greyed at 2; < gives 1): no 4-beat setting exists.
- **Flanger** (slider middle): no big level or band change (a sweep, not a filter); 1-s bands move by 1-3 dB.
- **Underwater**: in these clean takes, ON with the slider at left / middle showed **no measurable change**, and the
  slider at **right** took the highs (>2 kHz) down ~20 dB (muffled) for ~3 s. In an earlier, discarded session
  (slider left, slot 2 just switched to Underwater) ON gave lows -30 dB and level -10 dB. So either Underwater needs
  the slider far right to act, or the ON in the clean take did not engage: **listen to `underwater_*` before tuning**.
- Not recorded: Flanger routed to vocals only (needs djay's Choose FX -> Routing dropdown).
