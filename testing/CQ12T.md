# Allen & Heath CQ-12T: the app on USB multitrack (Marko 13.10.2026)

Goal: the phone's DJ app plays into the CQ-12T over USB in **Multitrack** streaming mode and uses its **first 4
USB tracks**: tracks 1-2 = MASTER (the dance floor), tracks 3-4 = HEADPHONES (pre-listen). On the mixer
(Configuration -> USB) there are two streaming modes: Stereo (any app) and Multitrack.

How the app does it today (since V7): Settings -> MASTER / HEADPHONES lists every USB output with its channel count;
on a 4+ channel output it shows the pairs "1-2", "3-4" ... and APPLY opens one Oboe (AAudio) stream with 4 channels
(src/soundio/sounddeviceoboe.cpp). Open question: does Android pass 4 channels to USB tracks 1-4 as they are (a
channel index mask) or treat them as quad surround and remix?
**The CQ-12T is on the Mac's LAN too (Marko 13.10.2026), and Allen & Heath's CQ app on the Mac controls it
(CQ-MixPad): use that app (screenshots / clicks) for the USB streaming mode, the channels' input sources (USB 1-4),
the routing and the meters, instead of asking Marko.** Voice to Marko (MANTRA_VOICE) only for what the app cannot
do (moving a cable, a hardware switch), one action at a time.

## A. On the Mac (the CQ-12T on the Mac's USB now)
1. Mixer in Stereo mode, then in Multitrack mode (Marko switches: Configuration -> USB -> streaming mode). For each:
   `system_profiler SPAudioDataType`, `system_profiler SPUSBDataType`, and the output / input channel counts and
   sample rates in Audio MIDI Setup (screenshot). Push to testing/results/<date>_cq12t/mac.md.
2. Which sample rates does the CQ offer over USB (44.1 / 48 / 96 kHz)? The app runs at 44.1 kHz unless told otherwise.

## B. On the Pixel (the CQ-12T on the phone's USB-C)
1. Before unplugging the Mac's cable: `adb tcpip 5555`, note the phone's Wi-Fi IP, `adb connect <ip>:5555`, check
   `adb devices` lists it over Wi-Fi. Then by voice: Marko moves the CQ-12T's USB cable to the Pixel (Multitrack mode).
2. `adb shell dumpsys media.audio_policy | grep -A30 -i usb` and `adb shell dumpsys media.audio_flinger` -> what
   Android sees: channel masks / counts, sample rates of the USB output profile. Push.
3. App (V9 or newest): Settings -> MASTER: "USB: <CQ name>" shows its channels? Choose it, channels 1-2;
   HEADPHONES: the same USB output, channels 3-4; APPLY. Log: `Sound output open ...` (channels, rate),
   `adb logcat -d | grep -iE "Sound|oboe|AAudio|AudioTrack"`.
4. `adb shell dumpsys media.audio_flinger | grep -iE "channel|mask|Output thread|Track"` while playing: the track's
   channel mask (index mask 0xf = tracks 1-4 as they are; a positional quad mask = remixed).
5. Play deck 1 (on air) and press deck 2's headphone button with deck 2 playing. On the CQ-12T (camera): the USB
   tracks 1-2 show deck 1's level, 3-4 deck 2's. If the mixer needs its channels' sources set to USB 1-4, ask Marko by
   voice, one channel at a time. Listen: master on the speakers, the pre-listen on the CQ's headphones.
6. Report what works, what does not, with the dumpsys lines; testing/results/<date>_cq12t/REPORT.md.
