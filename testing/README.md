# Testing loop

DJ Mantra is developed in a cloud session and tested on a real phone by Claude Code running on
your computer. The two meet in this folder.

```
cloud session                       GitHub                       your computer + phone
-------------                       ------                       ---------------------
change code, push      ──────▶  CI builds the APK   ──────▶  /test-on-phone
                                                             installs, tests, asks you
read results, fix      ◀──────  testing/results/*.md ◀────  commits + pushes results
```

## On your computer (once)
1. Install Claude Code, [Android platform-tools](https://developer.android.com/tools/releases/platform-tools)
   (`adb`) and the [GitHub CLI](https://cli.github.com/) (`gh auth login`).
2. Clone the repo and check out the development branch:
   ```bash
   git clone https://github.com/markoboskoauroville/djmantra_app
   cd djmantra_app
   git checkout claude/admiring-feynman-hym3vp
   ```
3. On the phone: Settings → About phone → tap *Build number* 7 times, then Developer options →
   enable *USB debugging*. Connect by USB and accept the prompt.

## Each test round
In the repo folder, start `claude` and run:

```
/test-on-phone
```

Then tell the cloud session: **"read the latest phone test results"**.

Without Claude Code, `tools/android/phone_test.sh` runs the automatic part on its own.

## Remote test loop (from 9.10.2026)

Device tests are run by the local session **Claude Code local** on the Mac, driven by the cloud
session over Remote Control. Marko does not need to be there.

1. The cloud session checks the local one is connected; if not, it asks Marko to start Remote
   Control on the Mac.
2. It sends the commands. The usual round, in the djmantra_app clone on the Mac:
   ```bash
   git pull origin claude/admiring-feynman-hym3vp
   adb devices -l                       # Nothing Phone 2, Pixel 7, Pixel 7 emulator
   tools/android/device_test.sh --sound # every device: install, launch, tone through the speaker
   git add testing/results && git commit -m "Device test <stamp>" && git push
   ```
3. The local session messages the result table back to "DJ APP cloud" and stops; the cloud
   session reads `testing/results/<stamp>_devices.md` and the per-device folders, fixes, pushes,
   and sends the next round when the new android-latest release is up.

Devices: Nothing Phone 2 and Pixel 7 on USB, Pixel 7 emulator (arm64) on the Mac. Sound goes
through the phone speaker; USB-C audio, the Mix Ultra and USB sticks are the last phase, with
Marko present.

## The real Pixel 7

No lock since 10.10.2026 (Marko: "remove the lock algorithm because now we have only one session
running"): adb straight to it, like the emulator (`emulator-5554`).
