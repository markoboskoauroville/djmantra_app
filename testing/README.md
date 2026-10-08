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
