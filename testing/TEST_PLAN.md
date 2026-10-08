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

## Newest milestone: M5 Android MIDI + Mix Ultra mapping + beat lights (CI run #29 or newer)
Logcat for all of it: `adb logcat -s DJMantraMIDI:* DJMantraStorage:* AndroidRuntime:E`
(the controller's every incoming message is logged as `in 91 07 7F` etc.).

**Start**
- [ ] The app starts and stays running (no `libomp.so` crash). **Launch time**: tap → first screen,
      on the Pixel 7 and the emulator (`adb shell am start -W com.djmantra.app/...` gives TotalTime)
- [ ] First start asks for **"All files access"** (explanation, then the system screen); after
      allowing it, coming back to the app works
- [ ] First start asks for **Bluetooth** ("Nearby devices"); allow it

**USB stick through the folder picker (SAF)**: see "External drives" above; the key checks:
- [ ] Plugging the stick in offers it; "Add to library" opens **Android's folder picker on the stick**
- [ ] After "Allow" the stick's music is in Folders; restart: no picker again

**Mix Ultra over USB-C** (adb over Wi-Fi, controller in the phone's USB-C port)
- [ ] logcat `DJMantraMIDI: Opening Guillemot Corporation DJControl Mix Ultra type 1` and
      `Connected (usb)`; the screen stays on while connected
- [ ] The knobs' positions arrive at once (B0 7F 7F): `in B1 00 ..` etc. right after connecting
- [ ] Every control from the capture shows up as `in ...` with the captured message (spot-check
      PLAY 91 07, SHIFT 91 04, jog B1 0A, a pad in each mode)
- [ ] Unplug and plug back in: `Disconnected (usb)`, then `Connected (usb)` again, LEDs light up again

**Mix Ultra over Bluetooth** (unplugged, on battery; NOT paired in Android Settings)
- [ ] logcat `Looking for the controller over Bluetooth`, `Found DJControl Mix Ultra <address>`,
      `Connected (bluetooth)` within ~10 s, without any pairing dialog
- [ ] Switch the controller off and on: it reconnects by itself (`Found ... ` with the same address)
- [ ] Messages arrive like over USB; note any delay you can feel (jog, PLAY)

**Mapping and LEDs follow the app** (with two tracks loaded; sound needs M4)
- [ ] PLAY / CUE / SYNC / PFL LEDs follow the decks (play_indicator, cue_indicator, sync, pfl)
- [ ] LOAD 1/2 loads the selected track; the browser knob moves in the library; press = open
- [ ] Faders, EQ, filter, GAIN (SHIFT + HIGH), master, crossfader move the app's controls
- [ ] Jog: touch + turn scratches, turning without touch bends the pitch, SHIFT + top seeks;
      **deck 1 SHIFT + jog**: which message comes (B4 0A expected)
- [ ] Tempo fader: which direction is faster (report it)
- [ ] **Each pad mode** (HOT CUE, LOOP, FX, NEURAL MIX, and with SHIFT: PITCH PLAY, BOUNCE LOOP,
      SLICER, SAMPLER): the pads do what the script header says; note anything that feels wrong
- [ ] **Beat lights**: on a playing deck one pad lights per beat, running 1→8, **in every mode**;
      tell which modes show it and in which colour (photo per mode). SHIFT + STEMS turns them off/on
- [ ] SLICER: pad 5 jumps to beat 5 of the phrase and the light continues from 5;
      SHIFT + SLICER pad 1 makes the current beat "1"
- [ ] Two decks: with both main cues on a "1", the lights of both decks show the same number
      when the songs are aligned

## M6–M8 – UI, mapping, dual output
- [ ] Touch UI usable on the phone screen (decks, mixer, pads, library)
- [ ] Every Mix Ultra control does what its label says
- [ ] MIDI learn: reassign a button and it sticks after restart
- [ ] Master on Bluetooth + cue on USB headphones at the same time
- [ ] Headphone delay setting brings cue in sync with the Bluetooth speaker
