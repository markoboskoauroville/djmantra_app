# Round 32 (V10; rules: testing/PROTOCOL.md; tester "mantra dj app tester", Pixel 7)

Build: V10 (android-latest shows "V10"). Fixes for round 31:
- Scrolling lanes / scrub window drew no waveform: the shader used GLSL built-in names (`index`, `distance`) as
  variables (the phone's GPU compiler rejects them; the desktop's accepts). Renamed.
- Overview colours washed out: each column now takes the colour of its loudest moment (no averaging) and its peak
  from a ~1/18 s window (djay's overview rate): saturated and spiky as djay.
- The lanes' shader logs its status (`waveform shader ...`); on an error the classic waveform takes over.
- GPU meter (top right, "G"): Pixel's Mali load file is not readable (round 31: Permission denied even for adb), so the app measures its own frames' GPU time (Qt renderer timestamps).
  Log: `Quick: GPU load from ...` and `Quick: GPU time per frame from the renderer's timestamps`.

## 1. Waveform vs djay Pro (as round 31 part 1): overview colours and spikes; the Phrasing lanes and the scrub
window now show the thin mirrored wave with coloured arrows. Screenshots side by side with djay (Nanana, 4B,
Brick House, Lovely Day). Log: `waveform shader [ChannelN]: compiled` (or `ERROR <Qt's message>`: then the classic waveform shows instead; send me the line).
## 1b. PLAY and NEXT look alike (Marko): PLAY's triangle drawn as NEXT's (same size and fill), pause bars as before;
screenshot both decks' buttons, playing and paused.
## 2. GPU meter: the "G" number moves (idle vs both decks playing on the Phrasing page). Which log line says the
source? Compare with `adb shell cat /sys/devices/platform/*.mali/utilization` (as shell) while playing.
## 3. Round 31's parts 1b (Neural Mix), 1c (every overlay closes with ✕ / back), 2 (basics, CPU).
## 4. testing/CQ12T.md part B (the CQ-12T on the phone, USB tracks 1-4).
## 5. The CQ from the app (new in V10; testing/CQ12T_FX.md): Settings -> CQ MIXER: the CQ's IP (169.254.5.29 on the
Mac's LAN; the phone must reach it: the CQ's own Wi-Fi or the same network), CONNECT (`CQ: connected to ...`).
FX 1, SOFT KEY n (the one set to the Tap Delay's Tap Tempo in CQ MixPad). Mixer page: CQ DELAY on: in MixPad Ip1 ->
FX1 send 0 dB, FX1 unmuted, `CQ: tap tempo <bpm>`; the delay time = 60000 / BPM ms. Change the deck's tempo: after
2 s the taps come again. CQ DELAY off: the send at -inf. Put the CQ back as it was.
