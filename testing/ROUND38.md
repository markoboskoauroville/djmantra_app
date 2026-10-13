# Round 38 (V21 = V18 + the FX tuned to djay's recordings + the CQ over USB MIDI + the CQ's 16 channels; tester "mantra dj app tester", Pixel 7 + Marko's Nothing Phone if he is back; rules: testing/PROTOCOL.md)

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
## 7. V21: the CQ as the sound output plays at the right speed? (round 37: 0.27x). V21 opens a USB interface with
all its channels (log `USB output with 16 channels: opening all of them`, `Sound output open: ... 16 channels`), and
logs `Output rate: N frames/s of 48000` every ~5 s: N should be ~48000. Time 30 s of play against the wall clock;
CQ MixPad Ip1/2 meters with deck 1 playing. If it falls back (`Could not open 16 channels`), report the error.
## 8. Lanes (V21): the colour tails less magenta; a hit and the gap after it more distinct (expansion x^1.7): pairs
again at Nanana bars 1-2.
## 9. V22: the CQ again: Android lists no channel counts for it, so V22 tries 16 channels anyway (log `USB output
without channel counts: trying 16 channels`, then `Sound output open: ... 16 channels` or `Could not open 16`).
`Output rate` ~48000? the speed right (30 s against the clock)? CQ MixPad Ip1/2 meters?
## 10. V23: the EQ button's ▾ is drawn (no box); the FX button's hold is a 0.45 s timer (a finger may move a little).
## 11. V24 (the CQ speed, round 38: RS Audio Player plays at 1.0x, so it is our stream): without MMAP the stream's
capacity is now 16 callbacks and its buffer at least 2 device bursts (V23: capacity 4096 < one USB period of ~3367
frames x2, so one 1024-frame callback per period = 0.30x). Expect `Sound buffer: N of M frames` with N >= 2 x burst,
`Output rate` ~48000, the right speed (30 s vs the clock), CQ Ip1/2 meters moving.
## 12. V25: the FX after your phone recordings (part 4): Underwater = 12 dB lowpass ~4.9 kHz at the middle (was 24 dB
3.3 kHz: 6-16 kHz -28 vs djay -8.5); Cooldown fitted on djay's own recording (HP 300 Hz, LP 4 kHz 24 dB, mids kept:
simulated on djay's dry audio -5.6/-19.3/-4.4/+0.7/-2.5/-23.0 vs djay -5.8/-15.9/-5.1/+1.5/-3.1/-27); Echo Out: the FX
tap now also switches its slot on (V21: ON did nothing). Record all 4 again (same way) + the band table.
## 13. V26, the CQ speed: the output measures itself after ~5 s; too slow (< 80 %) -> it reopens as 16-bit, then
32-bit, then 16-bit at the device's own rate (log `Output too slow ... trying variant N`, `Sound output variant N:
format ..., rate`). Report every `Output rate` line and which variant (if any) reaches ~48000; the speed by ear.
## 14. V26, Echo Out: the log now says what the engine sees: `Echo Out: engaged true/false enable state N beats B
mix M` (or "(no engaged parameter)") and the tap line names the slot's effect and key. Tap ECHO OUT on/off twice and
send those lines.
## 15. V27, Echo Out: the cause found: the app set controls with ControlObject::set(), which an effect slot never
hears (Mixxx skips the sender itself), so Echo Out's slot never ran (no `Echo Out:` line). Now set through a proxy.
Expect `Echo Out: ... engaged false enable state 1 ...` when ECHO OUT is chosen, `engaged true` on the tap, the song
cut at once into its last 2 beats repeating quieter, `engaged false` = the song back. Record it as before.
## 16. V27, the CQ: the variants now start with Android's deep buffer (PowerSaving: 1 float, 2 16-bit), then 16-bit,
32-bit, device rate. Send every `Output rate` / `variant` line and the USB MIXER thread (AudioOut_*) each variant gets.
## 17. V28: the CQ keeps the fastest variant when none is real time (expect `No variant runs in real time; keeping
variant 1 (~27900 frames/s)` and it stays there, ~0.58x). Echo Out: 2 beats now -6.9 dB a round (~7.5 dB/s as djay),
1 beat -4.6 dB, the repeats thinned below 150 Hz: record and compare with djay as before.
