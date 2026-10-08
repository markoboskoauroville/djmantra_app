# Test plan

Checks per milestone (see [docs/ANDROID_PORT.md](../docs/ANDROID_PORT.md)). `/test-on-phone`
runs the checks for the newest milestone that has an APK.

## M3 – first APK (engine compiles, no audio/MIDI yet)
- [ ] APK installs
- [ ] App starts and is still running after 20 s (no crash in logcat)
- [ ] What is on screen? (screenshot)
- [ ] Android lists the phone's audio outputs (speaker, USB DAC, Bluetooth) in `audio_policy.txt`

## M4 – audio
- [ ] App lists audio outputs: phone speaker, USB DAC (FiiO KA11 / ESR), Bluetooth speaker
- [ ] A track loads and plays on the phone speaker
- [ ] Plays on the USB DAC
- [ ] Plays on the Bluetooth speaker
- [ ] Charging through the USB-C splitter while playing on the DAC

## Video files (from M4)
- [ ] An MP4 music video appears in the library and plays its audio
- [ ] Its cover art is a frame from the video (or the embedded cover)
- [ ] An MKV or WebM file plays too
- [ ] Jumping to a cue point inside a video file lands on the beat

## External drives (USB stick, USB disk, SD card)
Setup: an OTG USB stick or disk with a music folder; the phone connected to adb over Wi-Fi
(`adb tcpip 5555`, `adb connect <ip>:5555`) so the phone's USB-C port is free for the drive.
- [ ] First start: the app asks for "All files access" and opens the setting; after allowing it,
      `/storage/<id>/` is readable (`adb shell ls /storage`)
- [ ] Plugging in a new drive asks "Add its music to the library?"; "Add" scans it
- [ ] Loading a song from the drive: logcat shows `LocalTrackCache ... Copied ... KiB`
      (note how long a 10 MB MP3 and a large video take to load)
- [ ] **Pull the drive out while the song plays: playback continues to the end, no gap**
- [ ] The library still lists the drive's songs (not "missing") while it is out, also after a rescan
- [ ] Plug it back in: no error, a rescan runs by itself ~3 s later, nothing marked missing
- [ ] Unplug for 1–2 s *while a song is loading*: the load finishes once the drive is back
- [ ] With the drive out, load a song that was played before: it loads from the copy
- [ ] Cache size: `adb shell du -sh /data/data/com.djmantra.app/files/track-cache` (as `run-as`)

## Folders (like djay)
- [ ] Folders → "+ Add folder": pick the USB stick's music folder; the tree shows it and its
      subfolders with song counts; tapping a folder lists all its songs (incl. subfolders)
- [ ] Songs have green dots; pull the stick out: within ~3 s the dots turn red, the folders get
      the offline icon, nothing disappears; plug it back in: green again
- [ ] Restart the app with the stick out: the folders are still there (offline)

## M5 – MIDI
- [ ] Mix Ultra over Bluetooth appears in `dumpsys midi` and in the app
- [ ] Button presses show up in logcat
- [ ] LEDs on the controller react to the app

## M6–M8 – UI, mapping, dual output
- [ ] Touch UI usable on the phone screen (decks, mixer, pads, library)
- [ ] Every Mix Ultra control does what its label says
- [ ] MIDI learn: reassign a button and it sticks after restart
- [ ] Master on Bluetooth + cue on USB headphones at the same time
- [ ] Headphone delay setting brings cue in sync with the Bluetooth speaker
