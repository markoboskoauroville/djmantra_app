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

## In the cloud session, before every push (Marko 14.10.2026: "Running everything is in mantra manifest")

Never ask Marko for an SDK. The cloud session sets it up itself, as `mantra_manifest/modules/cloud-android.md`
describes:

```bash
curl -s -H "Accept: application/vnd.github.raw" \
  https://api.github.com/repos/markoboskoauroville/mantra_manifest/contents/tools/cloud-android/setup.sh \
  -o /tmp/setup.sh && bash /tmp/setup.sh          # Android SDK in /opt/android-sdk, about 1 min
javac -nowarn -d /tmp/jc -cp /opt/android-sdk/platforms/android-36/android.jar \
  $(find packaging/android/package/src -name "*.java")   # the app's Java: must print nothing
python3 tools/qml/check_djmantra_qml.py                   # every QML screen loads
```

The app's screens are not tested in the cloud: a software-only emulator (no `/dev/kvm`) never draws a Qt Quick app
(testing-on-a-budget.md). Screens and sound are tested on the Mac's emulator and the Pixel 7 (below). Java code
that only talks to the network (SunoBridge, BeatStatsActivity.isrc) can be run here with plain Java and a stub
`android.util.Log`.

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

## Test songs (11.10.2026)

Marko's 69 test MP3s (about 880 MB) are in a public Google Drive folder
(`1pgo5Zyxl-t4uVHfLtMMXOSCO9uQpFOT_`), listed in `testing/songs/drive-songs.tsv`.
`tools/android/fetch_test_songs.sh` downloads them into the phone's `/sdcard/Music` (run in Termux on
the phone) or, with `--serial`, pushes them there over adb. Use them for the load, play and mix tests.
