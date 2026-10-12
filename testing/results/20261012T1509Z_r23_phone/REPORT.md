# Round 23 on the phone (Nothing Phone (2a), A142, Android 16)

Build: run 38265760924, commit 7478fb6. Started by the tester's build watcher, 12.10.2026. Landscape again (the phone is sideways, auto-rotate on).

## Part 0: smoke - PASS
- Ready in 4 s (`interface ready 919x413`), no qml error, `Sound output open: "Phone audio output ..." (clock) 48000 Hz, 2 channels`, `AudioRouter: 9 output(s) added` (`p0_start.png`).

## Part 1: transport - PASS
- 3 Sense (tap) and Addis (NEXT). PLAY deck 1: `play 1`, `auto cue: headphones to [Channel2]`; PLAY deck 2: `start [Channel2] from the cued beat 5 with the other deck's beat 5`, `beat launch ... on beat 5`.
- 30 s both playing: xruns 0, late 0, slowest 2.3-3.0 ms.

## Part 2: sliders and knobs - PASS
- Crossfader left and back; HIGH deck 1 dragged down: counter-clockwise, ring dims; double tap: back.
- Tester note: the start log had no `Quick: map` lines (my `logcat -c` ran before the layout logged them); a restart gave the map.

## Part 3: CUE after play-from-cue - FAIL
- 32 BEATS, deck 2 stopped, LED 17: `cue [Channel2] at beat 17`, `cue set [Channel2] at phrase beat 17` (46 ms later); matrix 37.1 (`p3_cued17.png`). PLAY (deck 1 playing): `start ... from the cued beat 17 ...`, `beat launch ... on beat 17`; played 2.3 s, pause (`play 0`), CUE (`cue_default pressed`): **the deck stood at 38.2 = beat 22, LED 22, not 17** (`p3_back17.png`).
- Again with LED 3: `cue set ... at phrase beat 3`, matrix 33.3; start waited 10 s for deck 1's beat 3, played 10 s, pause, CUE: **38.4, not 33.3** (`p3_back3.png`).
- The CUE press after a pause does not move the deck at all: the position after CUE is where the pause left it. This looks like Mixxx's default cue mode: CUE on a paused deck that is not on the cue point SETS a new cue there (in CDJ / Pioneer mode it returns to the cue point). Suggest `[ChannelN],cue_mode` = CDJ (or a return-to-cue of our own: `cue_gotoandstop`), and log the cue position on CUE.

## Part 4: CPU - FAIL (half fixed)
Both decks playing (deck 1 restarted: `beat launch [Channel1] on beat 27`), 8 s on each tab, `top -p` over 3 s; the app's meter beside it:

| tab | top (% of one core) | app meter | round 22 |
|---|---|---|---|
| Songs | 51.6 % | C 6% | 81.6 % |
| Phrasing | 76.3 % | C 10% | 79.0 % |
| Mixer | 74.6 % | C 9% | 77.6 % |
| Keys | 49.0 % | C 6% | 80.3 % |

Top threads (`top -H`):
- Songs: qtMainLoopThread 22.6 %, Controller 8.0 %, AudioTrack 7.0 %, QtThread 5.6 %, CachingReaderWorker 2.0 + 1.3 %, mali-event-handler 1.3 %.
- Phrasing: qtMainLoopThread 27.0 %, **QtThread 20.6 %** (the render thread: 4x the Songs tab), Controller 8.0 %, AudioTrack 7.6 %, mali-event-handler 3.6 %, CachingReaderWorker 1.6 + 1.3 %.
So the Phrasing tab's cost is drawing: the scrolling waveform (repainted every frame) and the LEDs; the Mixer tab likely the two level meters and knob rings repainting every frame. Songs and Keys are inside the 32-55 % target now.
- `Controller` at 8 % on every tab: a controller-script thread busy with no controller attached? Worth a look.
- Every tab still works after switching back and forth (`tab_1/2/3.png`: LEDs lit, meters moving, wheel with keys). Audio: xruns 0, late 0, slowest 4.3 ms.

## Part 5: audio router - PASS (what the phone alone allows); plug-in parts NOT TESTED
- Settings now logs `Quick: audio output speaker:A142|Phone speaker|2`: the list shows "Phone default (follows plugging in)" and "Phone speaker" for MASTER, and Off / Phone default / Phone speaker for HEADPHONES (`p5_settings.png`). Only the speaker is reported (nothing else is plugged in). Round 22 fail fixed.
- The ← stays at the top while the page scrolls (`p5_scrolled.png`). Fixed.
- MASTER = Phone speaker, HEADPHONES = Phone default → APPLY: `Sound output open: "Phone audio output ..." (second output)` and `Sound output open: "Phone speaker" (clock)`, both 48000 Hz 2 ch, device 3; `audio route master -> speaker:A142 channels 1-2`, `headphones -> phone default channels 1-2`; sound, xruns 0, late 0, slowest 3.9 ms.
- Put back to MASTER = Phone default, HEADPHONES = Off (`audio route master -> phone default`, `headphones -> none`), so the pre-listen does not also play on the speaker.
- NOT TESTED (needs Marko to plug something in): the list growing on plug-in, MASTER on a USB / Bluetooth output, channels 3-4 on the Mix Ultra, unplug fallback.
