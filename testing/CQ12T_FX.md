# CQ-12T delay effects from the app, in time with the deck's BPM (Marko 13.10.2026)

Marko: "CQ has delay effects ... trigger these through the app ... read the BPM from the app and control the delay
effect on CQ". Only Allen & Heath's published CQ MIDI protocol (no sniffing of CQ MixPad's own traffic).

What is known (A&H forums; the cloud cannot reach the PDF):
- CQ MIDI Protocol V1.2 (allen-heath.com: "CQ MIDI Protocol"): NRPN for levels, pans, mutes (FX mutes, FX sends);
  MIDI over TCP/IP on **port 51325** at the mixer's IP, **one connection at a time**; maybe also USB MIDI.
- **No MIDI message sets an FX delay time**, and the CQ has no global tempo. But **a soft key can be Tap Tempo**
  for an FX with tap (EchoVerb, Tap Delay, Stereo Delay, Beat Delay), and a soft key is triggered by a MIDI
  **Note On** (soft key 1-3: notes 48-50, per the forum). So the app can tap the delay's tempo on its beats.

## On the Mac (the CQ on the LAN), before any app code
1. Download the official CQ MIDI Protocol PDF (allen-heath.com) into testing/results/20261013_cq12t/; copy out:
   the TCP details, the NRPN for FX send levels / FX return / FX mutes (FX1-4), the soft key notes, Scene
   recall (program change), and whether CQ firmware does MIDI over USB. Note the CQ's firmware (CQ MixPad: info).
2. In CQ MixPad: put a Tap Delay (or Beat Delay) in an FX slot, assign that FX's tap tempo to a free soft key.
   Note what was there before.
3. From the Mac (python, a TCP socket to <CQ IP>:51325): send that soft key's Note On / Note Off 4 times at
   120 BPM (500 ms apart) -> does the delay time in MixPad show 500 ms (or 1/4 at 120)? Then at 126 BPM (476 ms).
   Also: the FX mute on / off by NRPN, and an FX send level from a channel (Ip1/2) by NRPN. Close the socket after
   (one connection only). Push the script (tools/cq/cq_midi.py) and the results; message the coder.
## Then the app (the coder builds it): Settings -> CQ mixer (IP, soft key), a DELAY button per deck (FX send on
   / off from that deck's USB input), and the taps on the master deck's beats whenever its BPM changes.
