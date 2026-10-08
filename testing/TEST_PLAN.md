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
