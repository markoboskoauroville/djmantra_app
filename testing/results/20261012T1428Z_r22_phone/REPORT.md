# Round 22 on the phone (Nothing Phone (2a), A142, Android 16, 1084x2412)

Build: run 38260844240, commit 94fc08d (the builder's head after 98e3fae, 5a23541, 2df39cf). Started by the tester's build watcher, 12.10.2026.
Round 21 was skipped at the builder's request (its fixes are in this build).

## Part 0: smoke - PASS
- Ready in 6 s, no qml error. `AudioRouter: 9 output(s) added`; `Sound output open: "Phone audio output (speaker, USB or Bluetooth)" (clock) 48000 Hz, 2 channels, 1024 frames per callback, burst 256, API AAudio, device 3`; `Sound buffer: 3072 of 4096 frames`.
- It started in landscape (`interface ready 919x413`): the phone was lying sideways with auto-rotate on. Locked to portrait for the test with `cmd window user-rotation lock 0` (`settings put system user_rotation 0` alone did not turn it); auto-rotate is given back at the end.
