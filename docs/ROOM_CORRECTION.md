# Room correction (V68)

Marko, 14.10.2026: an EQ on the output to correct the room, as IK Multimedia's ARC ("I press the button, the sweep is
happening ... and then the curve is generated"), FabFilter-style display, measured in several places and averaged,
rooms saved, seven places, BYPASS, a correction guide, the phone's or an external microphone.

## Signal path

- **The EQ:** `DJ Mantra Room EQ` (`src/effects/backends/builtin/roomeqeffect.cpp`), 31 ISO third-octave peaking bands
  (20 Hz - 20 kHz, Q 4.318, +-12 dB), in Mixxx's output effect chain `[OutputEffectRack_[Master]_Effect1]`
  (parameter1..31, enabled). It is applied to `[MasterOutput]`: the speakers hear it, the recording / broadcast does not.
- **The test signal:** a WAV (`RoomEqBridge.writeSignal`) played on `[Sampler9]` (Android has 9 samplers; the Mix Ultra's
  pads use 1-8). Through the mixer it reaches every output (USB PA, Bluetooth). Level: the sampler's volume (TEST LEVEL).
  The correction is switched off while a place is measured.
- **The microphone:** `AudioRecord` with `UNPROCESSED` (no AGC / noise suppression), `setPreferredDevice` for a USB /
  headset / Bluetooth microphone (`RoomEqBridge.inputs`).

## Measurement (RoomEqBridge.java)

- **Sweep** (default): exponential sine sweep 20 Hz - 20 kHz (Farina), deconvolved with its inverse filter (time reversed,
  -6 dB/oct) into the impulse response, gated from 2 ms before its peak for 0.68 s (half-Hann taper), FFT 32768.
- **Pink noise:** Paul Kellet's filter; Welch average (FFT 8192, Blackman-Harris, 50 % overlap) as an RTA.
- The same analysis of the sent signal is subtracted (the generator's own deviations cancel). Output: 31 third-octave
  bands and 61 sixth-octave columns, relative to the 200 Hz - 2 kHz average, plus the recording's peak (overload check).
- Tested in the cloud on a simulated room (+8 dB at 63 Hz, first-order roll-off at 8 kHz, 83 ms latency, noise): sweep
  63 Hz +7.6, 8 kHz -2.7; pink noise within 1 dB of it.

## Correction (RoomEqPage.qml `apply`)

1. Power average of the places in the average (each place can be left out).
2. Relative to 200 Hz - 2 kHz.
3. Target: FLAT, HOUSE (+4 dB shelf below 80 Hz, -0.8 dB/oct above 1 kHz), CLUB (+7 dB below 70 Hz, -1 dB/oct above 2 kHz).
4. No correction below the speakers' low limit (the first band no more than 12 dB down; +3 dB at most there) or above
   16 kHz (phone microphones are not reliable there).
5. (target - measured) x STRENGTH (50 / 75 / 100 %), limited to -12 / +6 dB, smoothed 1-2-1 over the neighbours.
6. Shifted so the highest band is 0 dB: cut only (never drives the speakers harder, never clips).

## Rooms

`Music/djmantra_app/Room_Correction/rooms.json`: `[{name, target, strength, points: [7 x null | {thirds[31],
columns[61], kind, level, time, use}]}]`; the room in use: setting `room_current`. The applied curve: setting `room_eq`
(31 values), `room_eq_on`.

## Open

- The IK Multimedia manuals (ARC, iLoud MTM) to compare: ikmultimedia.com was blocked in the cloud (asked to allow it).
- A calibration file for measurement microphones (UMIK-1 style .txt) would make 20-100 Hz and > 10 kHz exact.
