# Round 32 (V10; rules: testing/PROTOCOL.md; tester "mantra dj app tester", Pixel 7)

Build: V10 (android-latest shows "V10"). Fixes for round 31:
- Scrolling lanes / scrub window drew no waveform: the shader used GLSL built-in names (`index`, `distance`) as
  variables (the phone's GPU compiler rejects them; the desktop's accepts). Renamed.
- Overview colours washed out: each column now takes the colour of its loudest moment (no averaging) and its peak
  from a ~1/18 s window (djay's overview rate): saturated and spiky as djay.
- GPU meter (top right, "G"): Pixel's Mali load file is tried first (`/sys/devices/platform/*.mali/utilization`);
  if Android does not let the app read it, the app measures its own frames' GPU time (Qt renderer timestamps).
  Log: `Quick: GPU load from ...` and `Quick: GPU time per frame from the renderer's timestamps`.

## 1. Waveform vs djay Pro (as round 31 part 1): overview colours and spikes; the Phrasing lanes and the scrub
window now show the thin mirrored wave with coloured arrows. Screenshots side by side with djay (Nanana, 4B,
Brick House, Lovely Day). If a lane is still empty: `adb logcat -d | grep -iE "shader|qsb|rhi|ShaderEffect"`.
## 2. GPU meter: the "G" number moves (idle vs both decks playing on the Phrasing page). Which log line says the
source? Compare with `adb shell cat /sys/devices/platform/*.mali/utilization` (as shell) while playing.
## 3. Round 31's parts 1b (Neural Mix), 1c (every overlay closes with ✕ / back), 2 (basics, CPU).
## 4. testing/CQ12T.md part B (the CQ-12T on the phone, USB tracks 1-4).
