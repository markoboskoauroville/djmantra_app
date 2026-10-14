# V33 (43031f4) basics on the Pixel 7, 13.10.2026 23:52-00:02, the CQ-12T in Stereo mode

- Install, start: OK. Nanana deck 1, 4B deck 2. PLAY 1 `play 1`, PLAY 2 `beat launch [Channel2] on beat 8`.
- **Both decks on the CQ (Stereo): `Output rate` 48,185-48,199 frames/s of 48000**, xruns 0, late 0. Real time.
- FX button deck 1: tap `FX: deck 1 COOLDOWN on`, tap `... off`. PASS
- Volume fader deck 1 dragged to the bottom (the screen shows it down): the logged `Output level` peak stayed 0.309 ->
  0.305 with deck 1 alone playing. Either the logged peak is measured before the faders, or the fader did not cut
  the sound: **to check by ear**. An EQ knob drag gives no log line (not checked further).
- **Deepest zoom 0.04** (V32): the lane shows the signed wave as **one thin line**, like djay (`deepest_zoom_signed_line_3_frames.png`).
  At this zoom the wave moves more than half the lane per screen frame, so smoothness cannot be judged there.
- **Smoothness (V33)**, zoom 0.46, both decks playing, `adb shell screenrecord` (60 fps, 540 px wide): the lane moved
  **7-12 px on every frame** (8, 11, 9, 11, 12, 8, 8, 10, 10, 9, 8, 10, 11, 10, 7, 9, 10, 11, 9, 8): no frozen frames,
  no 50 px jumps. PASS (a ±2 px jitter remains).
- The CQ is left in **Stereo** (Marko's test approval; real time). Phone settings: STOP TIME OFF, CQ MIXER usb.
