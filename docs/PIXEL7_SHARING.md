# Sharing the Pixel 7 between two projects

Marko's real **Pixel 7** (adb over **wireless debugging**, from Marko's Mac) is used by two
projects at the same time:

- **DJ Mantra** (this repo): the cloud session "DJ APP cloud" sends test rounds to the
  local session "Claude Code local" on the Mac, which runs adb.
- **NDI camera app**: its own Claude session(s). The Pixel 7 is also the camera pointed at
  the Hercules Mix Ultra controller.

One phone, one app in front at a time: a DJ Mantra test (app on screen, taps, screenshots,
sound) and a camera test (camera app on screen, camera streaming) cannot run together. So the
phone is **taken and given back** through a lock on the Mac.

## The lock (on Marko's Mac)

- Script: `~/pixel7/pixel7-lock.sh` (source: `tools/android/pixel7_lock.sh` in this repo).
- State: `~/pixel7/lock/info` (who, what, until when). History: `~/pixel7/log.txt`.

```bash
~/pixel7/pixel7-lock.sh status                          # FREE or BUSY owner=... until=...
~/pixel7/pixel7-lock.sh take djmantra 30 "ui test r8"   # exit 0 = yours, exit 1 = busy
~/pixel7/pixel7-lock.sh wait camera 20 "ndi stream"     # waits (checks every minute), then takes
~/pixel7/pixel7-lock.sh extend djmantra 15              # need more time
~/pixel7/pixel7-lock.sh give djmantra                   # done: give it back
```

## Rules for every session

1. **No adb command to the Pixel 7 without holding the lock** (the emulator `emulator-5554`
   needs no lock: it is not shared).
2. Take it for a realistic time (most rounds: 15–30 min), `extend` if needed, **`give` as soon
   as you are done**, also when the run fails. Never hold it while waiting for a CI build.
3. A lock whose time ran out more than 10 minutes ago is stale; `take` then takes it over and
   logs it.
4. When you give the phone back, leave it as you found it: your app closed
   (`adb shell am force-stop <package>`), screen on, home screen.
5. Busy? Don't wait idle in a loop for long: work on something else and check again later
   (or use `wait` in a background job).
6. Camera pictures of the controller (its LEDs) for DJ Mantra are taken under the
   **djmantra** lock, with the camera app the DJ Mantra session uses; the camera project's own
   app is not touched.
7. Find the phone with `adb devices -l | grep -i "pixel_7\|Pixel 7"`; over wireless debugging
   its serial looks like `adb-<serial>-xxxx._adb-tls-connect._tcp` or `192.168.x.x:port`. If it
   is missing, `adb mdns services` lists it; if it is gone for good, ask Marko to turn wireless
   debugging on again (one action).

## Prompt for the camera project's session

Paste this into the NDI camera app's chat:

> We now share Marko's Pixel 7 with the DJ Mantra project (another Claude session tests an
> Android DJ app on the same phone over wireless debugging). Only one project can use the phone
> at a time, so there is a lock on Marko's Mac:
> `~/pixel7/pixel7-lock.sh` (state in `~/pixel7/lock/info`, history in `~/pixel7/log.txt`).
> Before any adb command to the Pixel 7, run `~/pixel7/pixel7-lock.sh take camera <minutes>
> "<what>"`. Exit 0 means the phone is yours; exit 1 prints who has it and until when, so do
> something else and try again later (or `~/pixel7/pixel7-lock.sh wait camera <minutes>
> "<what>"`, which waits and then takes it). Take it only for as long as you need (15–30
> min), `extend camera <minutes>` if needed, and `give camera` as soon as you are done or a
> run fails. Never hold it while you wait for a build. When you give it back, stop your app,
> leave the screen on and on the home screen. A lock whose time ran out more than 10 minutes
> ago is stale and may be taken over. Never touch the DJ Mantra app (`com.djmantra.app`). If
> your session can't run commands on Marko's Mac directly, ask the session that runs your
> adb commands to follow these same rules. The full rules are in
> https://github.com/markoboskoauroville/djmantra_app/blob/claude/admiring-feynman-hym3vp/docs/PIXEL7_SHARING.md
