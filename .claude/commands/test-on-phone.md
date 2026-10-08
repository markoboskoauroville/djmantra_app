---
description: Install the latest DJ Mantra CI build on the USB-connected phone, test it, and push the results
---

You are running on the user's own computer with their Android phone connected by USB.
Your job: test the latest DJ Mantra build on the phone and record the results in the repo,
so the cloud development session can read them and decide what to fix next.

## 1. Prepare
- Check the tools: `adb version`, `gh auth status`. If either is missing, tell the user how to
  install it (Android platform-tools; GitHub CLI + `gh auth login`) and stop.
- `git fetch origin claude/admiring-feynman-hym3vp && git checkout claude/admiring-feynman-hym3vp && git pull`
- `adb devices -l` must list exactly one device. If it shows `unauthorized`, ask the user to
  accept the USB debugging prompt on the phone.

## 2. Automatic smoke test
Run `tools/android/phone_test.sh` (add `--wait 30` on slow phones). It prints `REPORT=<path>`.
Read that report and the files it links (logcat, screenshot, MIDI and audio dumps).

## 3. Manual checks (ask the user, one at a time)
Use the checklist in `testing/TEST_PLAN.md` for the current milestone. For each item, tell the
user exactly what to do on the phone or controller, wait for their answer, and look at the
screenshot or logcat yourself where that answers it. Typical items:
- Does the app open, and what is on screen?
- Hercules DJControl Mix Ultra: switched on and in pairing mode; does it appear in the app / in `dumpsys midi`?
- Press PLAY, CUE, move the crossfader: does the app react? (`adb logcat -d | grep -i midi` helps)
- Audio: plug the USB DAC in, connect the Bluetooth speaker. Which outputs does the app list?
  Does sound come out of each?

Write the answers under `## Manual checks` in the report as a table: check | result
(pass / fail / not tested) | notes. Add a `## Problems` section with anything that failed,
including the exact error lines from logcat.

## 4. Publish the results
- Never commit secrets: check `git status` and the diff; only the new `testing/results/` files
  should be added.
- `git add testing/results/ && git commit -m "Phone test: <phone model>, <one-line outcome>"`
- `git push origin claude/admiring-feynman-hym3vp`
- Tell the user it's pushed and give a 3-line summary. They then tell the cloud session to
  read the newest file in `testing/results/`.

Do not change any source code in this command; fixes are made in the cloud session.
