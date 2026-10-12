# Round 22 (rules: testing/PROTOCOL.md: this order, push + one-line message after each part, "round 22 finished")

Build: the green Android run of the newest commit with this file. Marko's requests after round 20 (r20 REPORT.md end):
automatic pre-listen, the audio router (MASTER / HEADPHONES on any output). Plus what round 21 left open.
Grep once per part: `adb logcat -d | grep -E "Quick:|SoundDeviceOboe|AudioRouter|Sound:"`.

## 0. Smoke (10 s): `Quick: interface ready`, not black, no `qml` error; `Sound output open: ... (clock)`.

## 1. Transport (quick): PLAY / pause, aligned start; 30 s both playing: xruns flat, late 0.

## 2. Sliders and knobs (quick): crossfader, one EQ knob.

## 3. LEDs and CUEING (new behaviour, Marko: "I press stop, I press 32, and I'm cueing")
- STOPPED deck: a press on a beat (Phrasing LED or matrix cell) CUES it: the deck goes to that beat exactly, the CUE
  point is set there (CUE then returns to it), the LED gets a white frame; NOTHING starts. Log `Quick: cue [ChannelN]
  at beat M`. The phrase numbering does not move ("1" stays "1": the matrix still counts the same beats after the cue).
- Then PLAY with the other deck playing: it waits for the other deck's beat M and starts on it (beat 32 with beat 32):
  `Quick: start [ChannelN] from the cued beat M with the other deck's beat M`, then `beat launch ... on beat M`; both
  rows' lit cells together. The other deck stopped: PLAY starts at once from the cue.
- PLAY without a cued beat: as before (the other deck's next beat, same phrase beat).
- PLAYING deck: a press jumps on the next beat (seamless), as before. 32 BEATS LEDs as round 20.

## 4. AUTO CUE (new; djay Pro's "Auto select": "automatically selects the deck that isn't playing as your pre-cue
source"; Marko: "when the deck in the headphones goes on air, the headphones switch to the other deck")
- The rule: a deck is ON AIR when it plays, its volume is up and the crossfader does not cut it (deck 1: crossfader
  not full right; deck 2: not full left). When exactly ONE deck is on air, the headphones (PFL) are on the OTHER deck,
  only that one. Both on air (a blend in progress): nothing changes. Neither: nothing changes.
- Deck 1 playing (fader up, crossfader middle), deck 2 loaded, stopped: within ~0.3 s PFL deck 2 on, deck 1 off
  (Mixer tab headphone buttons), log `Quick: auto cue: headphones to [Channel2]`.
- Start deck 2, crossfader to full right (deck 1 cut) or deck 1 stopped: the headphones go to deck 1
  (`... to [Channel1]`). Both playing with the crossfader in the middle: no change.
- Menu → SETTINGS: AUTO CUE off (log `Quick: auto cue off`): nothing switches by itself any more; on again.
  Restart the app: the setting is kept.

## 5. Audio router (new; Settings → MASTER / HEADPHONES)
- The lists show every output connected now: "Phone default", the speaker, and whatever is plugged in (wired
  headphones, the Mix Ultra or another USB interface, a Bluetooth speaker / headphones). Plug or unplug one with the
  page open: the list changes within ~1 s (log `AudioRouter: 1 output(s) added / removed`).
- MASTER = phone speaker, HEADPHONES = Off → APPLY: `Quick: audio route master -> speaker:...`, `... headphones ->
  none`; sound from the speaker.
- With a second output connected (wired headphones or Bluetooth or USB): MASTER = speaker (or USB), HEADPHONES = the
  other → APPLY: `Sound output open: <label> (second output)`; the master mix on the first, the pre-listen deck (PFL)
  on the second at the same time. Bluetooth shows the 200 ms warning.
- MULTI-CHANNEL (Marko: "listen on channels 1 and 2 and preview on 3 and 4, or vice versa"): with a 4-channel USB
  interface (the Mix Ultra: master out + headphones out), choose it for BOTH: under each a "channels 1-2 / 3-4" row
  appears; MASTER 1-2, HEADPHONES 3-4 → APPLY: `Quick: audio route master -> usb:... channels 1-2`, `headphones ->
  usb:... channels 3-4`, `Sound output open: ... (clock)` with 4 channels; the mix on the master out, the pre-listen
  in the interface's headphones. Then swapped (MASTER 3-4, HEADPHONES 1-2): the other way round. Report the
  interface's channel count from the list (log `AudioRouter`) if it shows only 1-2.
- Restart the app with the second output still connected: the route is kept. Restart with it unplugged: the app
  falls back to the phone's default (log `Sound: a routed output is not connected, back to the phone's default`)
  and plays.
- Sound on the second output for 2 minutes: any crackle? (The two outputs' clocks drift a little: a tiny drop on the
  headphones every several minutes is possible, the master is never touched.) xruns, late on the master.

## 6. Round 21 leftovers: whatever round 21 failed (see its report), CPU per tab, genre (Deezer).
