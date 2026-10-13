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
