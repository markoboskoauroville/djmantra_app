# Tester handover, 14.10.2026 00:05 (updated) (session "mantra dj app tester", Claude Code on Marko's Mac)

## Setup that is in place
- Pixel 7 over **adb on Wi-Fi**: `adb connect 192.168.1.103:5555` (tcpip 5555 set; USB-C is on the CQ-12T).
- Helpers were in the session scratchpad (gone after the session): tap map from `Quick: map` log lines (portrait:
  PLAY 1 (336,346), PLAY 2 (740,346), CUE 1 (204,346), tabs y 2352 (Songs 144, Phrasing 410, Mixer 670), FX button deck 1
  centre (113,1811) in V30+, chooser items y 1178/1294/1409/1524, DEPTH slider y 1685 x 58..447, menu (86,68),
  Settings (540,300), Settings ✕ (970,211)); songs in /sdcard/Music/Drive150.
- Sound: Mac output / input / system all **BlackHole 2ch** (Marko: never test sound on the ADAM). Phone audio is
  recorded with `scrcpy --no-video --audio-codec=raw --record=x.wav` (it reroutes the app's output off the CQ while it runs).
- djay Pro on the Mac's built-in display (Color LCD, `screencapture -D 2`); its lane zoom via View -> Waveforms ->
  Waveform Zoom +/-; transport via the Deck 1 menu (no mouse). CQ MixPad 1.2.2 on the same display.
- CQ-12T: on the Pixel's USB (audio + MIDI) and on the Mac's USB-Ethernet (169.254.5.29, MIDI TCP 51325:
  `tools/cq/cq_midi.py`). **Stream Mode STEREO since 13.10.2026 23:43** (Marko: "yes, test stereo"): the app plays at real time there (Multitrack: 0.55x). Back: CQ MixPad -> CONFIG -> USB/SD -> Stream Mode Multitrack.
- **Settings left on the phone**: STOP TIME OFF (reset), MASTER = Phone default, CQ MIXER = usb (connected), deck 1
  FX = COOLDOWN, EQ mode Equalizer.

## Done today (reports in testing/results/20261013_*)
Rounds 30-39, V8 -> V31: djay waveform (lanes, colours, mirrored, deep zoom), the new mixer (EQ dropdown, FX button,
no overlap), four FX vs djay recordings (all within a few dB on V25-V29), STOP TIME, CPU (40-48 % Phrasing), CQ over
USB MIDI (CQ DELAY verified on the CQ), CQ USB audio speed (Android ceiling 0.55x in Multitrack), djay pairs incl. the
deepest zoom on V31.

## Pending
1. ~~V33 basics~~ done 00:02 (`20261014_r39_v33_pixel/REPORT.md`): real time on the CQ in Stereo, FX tap, deep-zoom
   thin line, lanes move every frame. Open from it: the volume fader down did not lower the logged output peak (ear check).
2. **ROUND39 part 3** (Underwater right, Cooldown left / middle / right with a tap log): my three attempts on V31 were
   void (my script: the deck's play state and the FX button's top edge; STOP TIME was still 3 s from my own test).
   What is real from them: on V31 a tap on the FX button right after closing the chooser with an outside tap was
   **not logged** several times, while the next tap logged `off`; and once a hold + DEPTH drag logged
   `UNDERWATER off` (the hold's release may count as a tap). Worth a clean manual check.
3. **ROUND39 part 4** (V32: zoom to 0.04, signed thin line) pairs vs djay, and **part 5** (V33 smoothness).
4. ~~The CQ Stereo test~~ done (Marko confirmed directly): **Stereo = real time** (`20261013_cq12t/REPORT.md`). Left
   in Stereo: only the master goes over USB, no USB 3-4 headphones. Next: the app's own USB driver if Multitrack is
   wanted, or the headphones on another output.
5. **The CQ tap delay**: a Tap Delay in FX1 + a soft key as its Tap Tempo (MixPad), then ROUND32 part 5.
6. Marko's calls: the wave height (djay thin vs ours tall); YouTube's notification prompt on the Pixel.

## For the next tester session
- Start: `adb connect 192.168.1.103:5555` (if refused: Marko starts Wireless debugging once, or a USB cable for
  `adb tcpip 5555`). Check `gh release view android-latest` for the newest V, then the newest testing/ROUND*.md.
- Sound: BlackHole only (`SwitchAudioSource -t output -s "BlackHole 2ch"`, also -t system / -t input).
- Talk to Marko: MANTRA_VOICE `POST :8837/speak {"text":..., "engine":"beatrice", "device":"ADAM Audio D3V"}`; he starts
  the voice himself; clicks on his Mac only after his own "done" / "yes" in the chat (a relayed approval is not enough).
- Audio files (recordings) go to the private repo djmantra-test-songs, never to djmantra_app.
