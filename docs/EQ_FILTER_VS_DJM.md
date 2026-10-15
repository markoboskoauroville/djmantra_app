# DJ Mantra's EQ and filter against Pioneer's DJM mixers

Marko, 10.10.2026: swipe every EQ and the filter, check the sound, compare with
Pioneer's DJM mixers (their EQ curves, filter, manual, menu, layout).

## How it was measured

- **Engine:** `src/test/djmantra_eqcurve_test.cpp` (`mixxx-test --gtest_filter=DJMantraEqCurveTest.*`).
  - It sends sine tones (1/3 octave, 20 Hz to 20 kHz, 48 kHz) through the real effect processors DJ Mantra uses: the channel EQ "Biquad Full Kill" (Mixxx's default) and the quick-effect "Filter".
  - It does this knob position by knob position, mapping each position exactly as the on-screen knob does.
  - With `DJMANTRA_CURVES_OUT` set, it writes the tables to `docs/eq-curves/eq_curves.csv` and `docs/eq-curves/filter_curves.csv`.
  - It checks that the centre is flat (±0.5 dB) and that each band fully down is at least 20 dB lower.
- **Phone:** `tools/android/eq_sweep_test.py`, run on the Mac with BlackHole.
  - It plays pink noise on deck 1, turns HIGH, MID, LOW and FILTER with swipes, step by step, and records 3 s after each step.
  - It compares each octave band with the engine's curve, with a tolerance of ±3 dB.
  - **Not run yet:** no local session was connected.
- **Pioneer:** the official Pioneer pages and PDFs could not be opened from the cloud session (blocked by its network policy).
  - The figures below come from manual excerpts quoted on dealer, rental and forum pages. The links are in the research notes.
  - Treat Pioneer numbers marked *unverified* as unverified.

## Channel EQ

| | DJ Mantra now (Mixxx Biquad Full Kill) | Pioneer DJM-900NXS2 / DJM-A9 |
|---|---|---|
| Bands | 3: LOW, MID, HIGH | 3: LOW, MID, HI (the V10 has 4) |
| Crossovers | 250 Hz and 2.5 kHz (Mixxx default) | not published for the NXS2/A9; the first 900NXS used 70 Hz / 1 kHz / 13 kHz; the DJM-600 used 100 Hz / 1 kHz / 10 kHz |
| Maximum boost | **+12 dB** (LOW +12.1 dB at 31 Hz, MID +11.7 dB at 1 kHz, HIGH +12.0 dB at 10 kHz) | **+6 dB** |
| Fully down | full kill, isolator-like: LOW −54 dB at 50 Hz, MID −49 dB at 1 kHz, HIGH −56 dB at 10 kHz | EQ mode **−26 dB**; ISOLATOR mode **−∞** (a global EQ CURVE switch) |
| Knob at 25 % (9 o'clock) | about −9 dB in the band | not published (one DJM-400 review: about −18 dB at "9 o'clock") |
| Knob at 75 % (3 o'clock) | about +6 dB (LOW +6.0 at 50 Hz) | not published |
| Width of the low cut | fully down, LOW also takes 250 Hz down by 17 dB and 500 Hz by 4 dB | users describe the NXS2's low as "tightly focused, about 200 Hz and below" |

Measured (dB, at the band's centre):

| Knob | 0 % | 10 % | 25 % | 40 % | 50 % | 60 % | 75 % | 90 % | 100 % |
|---|---|---|---|---|---|---|---|---|---|
| LOW at 50 Hz | −53.7 | −20.2 | −9.0 | −2.6 | 0 | +2.4 | +6.0 | +9.7 | +12.1 at 31 Hz |
| MID at 1 kHz | −49.2 | −21.5 | −10.0 | −2.9 | 0 | +2.4 | +5.9 | +9.4 | +11.7 |
| HIGH at 10 kHz | −55.9 | −22.6 | −10.4 | −3.0 | 0 | +2.3 | +5.7 | +9.0 | +12.0 |

Does it behave as predicted? Yes:
- The centre is flat.
- Each band leaves the other bands alone; the overlap only shows near the crossovers.
- Fully down removes the band.

Where it differs from a DJM:
1. **Twice the boost.** At +12 dB, a boosted LOW clips easily on a phone or a club system. A DJM stops at +6 dB.
2. **There is no EQ mode.** DJ Mantra always behaves like ISOLATOR. A DJM in EQ mode never goes below −26 dB, so a "cut" still lets some of the band through. That is what most DJs are used to when they mix basslines.
3. **Different crossovers.** At 250 Hz and 2.5 kHz the bands are wider than a DJM's LOW and HI. Cutting the LOW also thins the low mids.

## The phone's EQ (V-line: DJ Mantra EQ / Neural Mix, `stemeq.h`)

On Android the deck EQ is DJ Mantra's own, not Mixxx's biquad EQ. It uses FFT bands with crossovers at 246 Hz and 2.5 kHz. Its curves are in `docs/eq-curves/phone_eq_curves.csv` (test `DJMantraEqCurveTest.PhoneEq`, Neural Mix off). Measured in dB:

| Knob | 0 % | 10 % | 20 % | 50 % | 75 % | 100 % |
|---|---|---|---|---|---|---|
| LOW at 50 Hz | −81 | −19.4 | −12.1 | 0 | +6.0 | +12.1 |
| MID at 1 kHz | −105 | −19.4 | −12.1 | 0 | +6.0 | +12.0 |
| HIGH at 10 kHz | −164 | −19.4 | −12.1 | 0 | +6.0 | +12.0 |

- **Flat bands with steep edges.** Each band is flat inside and cut steeply at its edges; the next octave over is untouched. That is a sharper isolator than any DJM.
- **Fully down is a full kill, as in a DJM's ISOLATOR mode.** A DJM in EQ mode stops at −26 dB.
- **Boost is +12 dB, against the DJM's +6 dB.** The same proposals below apply.

## Filter (the FILTER knob, quick effect)

| | DJ Mantra now (Mixxx Filter) | Pioneer Sound Color FX FILTER (900NXS2, A9) / V10 filter |
|---|---|---|
| Knob | one knob: left = low-pass, right = high-pass, centre = off | the same (A9, 900NXS2); the V10 has one knob per channel with a global LPF/HPF switch and twice the resolution |
| Slope | 12 dB per octave (2nd-order biquad) | not published |
| Resonance | Q 0.707 (no peak) by default; the effect's Q knob goes to 4 (+9 dB peak) | the PARAMETER knob = resonance; the DJM filter is known for an audible resonant peak |
| Centre | no dead zone: at 52 % the high-pass is already at 58 Hz (−4.6 dB at 50 Hz) | centre = off; the A9 has a Center Lock (a click-stop at 12 o'clock) |
| Cut-off sweep | logarithmic from 22 kHz (centre) down to 13 Hz (fully left), and 13 Hz up to 22 kHz (fully right) | range not published |
| Ends | fully left or right = silence | not published |

Measured (dB, Q 0.707):

| Knob | Cut-off | 50 Hz | 250 Hz | 1 kHz | 5 kHz | 10 kHz |
|---|---|---|---|---|---|---|
| 10 % | LPF 350 Hz | 0 | −1.0 | −18.3 | −47 | −61 |
| 30 % | LPF 3.3 kHz | 0 | 0 | 0 | −8.3 | −21.9 |
| 48 % | LPF 18 kHz | 0 | 0 | 0 | 0 | 0 (−0.8 at 16 kHz) |
| 50 % | off | 0 | 0 | 0 | 0 | 0 |
| 55 % | HPF 143 Hz | −18.5 | −0.4 | 0 | 0 | 0 |
| 70 % | HPF 1.2 kHz | −55 | −27.2 | −4.8 | 0 | 0 |
| 90 % | HPF 8.7 kHz | −92 | −63.6 | −39.5 | −11.1 | −1.6 |

The full tables, including Q 2 and Q 4, are in `docs/eq-curves/filter_curves.csv`. The filter does what it should: flat at the centre, a smooth sweep, no jumps. Compared with a DJM, it has no resonance, no centre dead zone and a gentler slope.

## What to take from the DJM (proposals)

These are not built yet. Each is a small engine or skin change.

1. **EQ/ISO switch in Settings → Sound** (a DJM has one global EQ CURVE switch):
   - **EQ:** −26 dB to +6 dB.
   - **ISOLATOR:** −∞ to +6 dB.
   - The default would be EQ, as on most club DJMs.
2. **Maximum boost +6 dB** in both modes (now +12 dB).
3. **Pioneer-like crossovers as the default:** 70 Hz / 1 kHz / 13 kHz centres from the first DJM-900NXS, the closest documented figures. The current setting is 250 Hz / 2.5 kHz. Marko's ears should decide.
4. **Filter centre:**
   - A centre lock: a dead zone of about ±3 % and a snap to the centre on release near it.
   - A small LED-style dot at the centre on the knob.
5. **Filter resonance:**
   - A default Q of about 1.3, a mild peak like the DJM's.
   - A PARAMETER (resonance) control in Settings, or on long-press of the FILTER knob.
6. **Gain staging like a DJM:**
   - A TRIM knob with the channel meter's 0 dB as the target.
   - A CLIP mark on each channel meter.
   - A peak limiter on the master, on by default, as on the 900NXS2.
7. **Headphones:** a cue/master MIX knob and a MONO SPLIT switch (DJ Mantra has headSplit already, under "Split Output").
8. **Faders:** three curves each for the channel fader and the crossfader (DJM: steep top / gradual / steep bottom; sharp / middle / gradual).
9. **Channel-strip order to keep** (the same on the 900NXS2, A9 and V10, and close to the Mix Ultra): TRIM, then HI, MID, LOW, then COLOR/FILTER, then CUE, then the fader.

## DJM menus (what the manuals show; the full tables could not be read)

- **DJM-900NXS2 UTILITY** (hold QUANTIZE):
  - MIDI channel.
  - Master out peak limiter (on).
  - Master attenuator: 0 / −3 / −6 dB.
  - Mic low cut.
  - Lock.
  - Auto standby (on, 10 h).
- **DJM-A9 UTILITY** (hold QUANTIZE/UTILITY):
  - MIXER SETTINGS and MY SETTINGS.
  - Master out attenuator and peak limiter (analog and digital).
  - Center Lock on/off.
  - Talkover mode and level.
  - Mic phantom lock.
- **DJM-V10 UTILITY:**
  - Balance.
  - Master out: peak limiter, attenuator, mono/stereo.
  - Digital master out.
  - Reference level.
  - Sampling rate.
  - The fader curves have their own screen.
  - MIDI channel, under MY SETTINGS.

For DJ Mantra's Settings, that suggests a **Mixer** section:
- EQ/ISO.
- Crossovers.
- Filter resonance.
- Filter centre lock.
- Peak limiter.
- Fader curves.
- Crossfader curve.
