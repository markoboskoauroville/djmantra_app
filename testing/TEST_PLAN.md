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

## External drives (USB stick, USB disk, SD card): Storage Access Framework
On a Pixel a USB stick is mounted where apps cannot read it by path (`/mnt/media_rw/<id>` only,
no `/storage/<id>`). The app reads it through the Storage Access Framework: the user grants the
stick once in Android's folder picker; the app keeps the permission and mirrors the files as
placeholders (same name, size, date, no content) in its own storage, reading the content
through the ContentResolver when needed.
Setup: OTG stick or disk with a music folder; adb over Wi-Fi (`adb tcpip 5555`,
`adb connect <ip>:5555`) so the USB-C port is free.
- [ ] Plug the stick in with the app running: within ~2 s it asks "A drive was connected: <name>.
      Add its music to the library?"; "Add to library" opens **Android's folder picker on the stick**
- [ ] Pick the stick's root or a music folder → "Allow": the Folders tree shows it (named after the
      stick) with song counts after the scan; logcat `ExternalContent ... placeholders: +N`
- [ ] Restart the app: no picker again (the permission is kept); `adb shell dumpsys activity
      providers | grep -i persisted` or Settings → Apps → DJ Mantra shows the access
- [ ] Load a song from the stick: logcat `LocalTrackCache ... Copied ... KiB` (note the load time
      for a 10 MB MP3 and for a large video)
- [ ] **Pull the stick out while the song plays: the deck keeps going to the end** (until M4 there is
      no sound: the position keeps moving, no "Failed to read" in logcat)
- [ ] With the stick out: songs red/offline, folders with the offline icon, nothing disappears, also
      after a rescan; a song played before still loads (from its copy)
- [ ] Plug it back in: green again within ~3 s, a rescan runs by itself, nothing marked missing
- [ ] Unplug for 1–2 s while a song is loading: the load finishes once the stick is back
- [ ] Files changed on the stick while away (add/delete one on the Mac): after replugging the
      library shows the new one and the deleted one is missing
- [ ] Internal storage still works: "+ Add folder" → pick a folder on the phone (Music) → it is added
      by its real path `/storage/emulated/0/...`; "All files access" must be allowed for that

## Folders (like djay)
- [ ] Folders → "+ Add folder" opens Android's folder picker (on the phone) or a folder dialog
      (desktop); the tree shows the folder and its subfolders with song counts; tapping a folder
      lists all its songs (incl. subfolders)
- [ ] Songs have green dots; pull the stick out: within ~3 s red, the folders get the offline icon,
      nothing disappears; plug it back in: green again
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
