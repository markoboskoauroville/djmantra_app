# Needs Marko (13.10.2026)

## The CQ-12T on the phone's USB plays at half speed (19:20)
Android's own USB sound path to the CQ in **Multitrack** mode (16 channels, 32-bit) runs at ~0.54x (measured with
YouTube); DJ Mantra at ~0.3x. Two ways out, your choice:
1. **CQ USB Stream Mode = Stereo** (CQ MixPad -> CONFIG -> USB/SD tab -> Stream Mode, or the CQ's own Setup -> USB/SD).
   Then the phone sends 2 channels only: the master; the headphones (USB 3-4) would not go over USB.
2. **The app gets its own USB audio driver** (as "USB Audio Player Pro" does), keeping Multitrack and 3-4 for the
   headphones: more work for the coder.
Nothing was changed on the CQ.

## YouTube on the Pixel asks "Allow YouTube to send you notifications?" - left for you.

## Also waiting for you (21:00)
- **djay vs app waveform pairs on V29** (ROUND38 part 8): the Mac's screen went dark / locked around 20:50, so djay's
  lane can't be screenshotted. When you're back, unlock it: I'll do the pairs.
- **The CQ's tap delay** (for the app's CQ DELAY to follow the BPM): a Tap Delay in FX1 and a soft key set to its Tap
  Tempo, clicked in CQ MixPad (on the Mac or your phone). The app's CQ DELAY over USB MIDI already works (it sends
  Ip1/2 into FX1 at 0 dB and back to -inf).
- **The wave's height**: djay on your Mac draws a thin band; the app 0.8 of the lane. Your call.
