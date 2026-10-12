# Round 12 on the phone — 12.10.2026 (Nothing A142, Android 16)

Tester: local phone session (Claude Code in Termux/proot, adb over Wireless debugging 192.168.1.30:38589).
Times below are phone time (CEST, UTC+2).

## Diagnosis first: what killed Claude Code at 02:21–02:23

**Android's phantom process killer.** It trimmed Termux's child processes at 02:23:23, and memory pressure made it worse.
Neither DJ Mantra nor the low-memory killer killed Claude Code directly.

### The evidence

1. The previous session (transcript `32badc32…`) wrote its last line at **02:21:48**. That line started a
   Bash call ("I'll scan for the wireless-debugging port myself"). Its result never came back.
2. From 02:21:42 on, logcat shows a new `timeout`/`bash`/`loader` process exec'd **every second** inside Termux.
   This is the port scan, written as one shell process per port (21 `timeout` execs in 02:20–02:23).
3. **02:23:23**, ActivityManager (all 10 lines are in `host/logcat-kills.txt`):
   ```
   I/ActivityManager: Killing PhantomProcessRecord {6b4bd 19704:19657:bash/u0a245}: Trimming phantom processes
   I/ActivityManager: Killing PhantomProcessRecord {51abab2 19725:19657:runsvdir/u0a245}: Trimming phantom processes
   ... runsv x3, svlogd x3 ...
   I/ActivityManager: Killing PhantomProcessRecord {f94be75 28511:19657:bash/u0a245}: Trimming phantom processes
   I/ActivityManager: Killing PhantomProcessRecord {7daee0a 28533:19657:bash/u0a245}: Trimming phantom processes
   ```
   19657 is `com.termux`. 19704 is the first shell of the Termux session that ran proot + Claude Code.
   Once that shell was gone, the session closed, and proot (`--kill-on-exit`) took Claude Code down with it.
   The termux-services supervisors (runsvdir/runsv/svlogd) were killed too.
4. **The phantom limit is at Android's default:** `device_config get activity_manager max_phantom_processes` → `null`,
   `settings get global settings_enable_monitor_phantom_procs` → `null`. So the cap is 32 processes across all
   apps, and the monitor is on. Termux alone sits at **18 processes** at rest (`host/termux-ps-0238.txt`: proot, claude,
   2×bash, 2×adb, runsvdir + 3 runsv + 3 svlogd, termux-api, sh, timeout…). Anything that fans out (a port scan,
   `ui_test.py` with its adb children, the watcher) pushes it over.
5. **Memory pressure at the same moment.** DJ Mantra was running (pss 491 MB at 02:18:57; it died as a cached app
   at 02:27:26). Swap was **78 % used** (3.2 of 4 GB), load average 16, and lmkd kept killing other apps:
   ```
   02:16:42 lowmemorykiller: Kill 'org.mozilla.firefox:tab…' … reason: device is not responding
   02:21:47 lowmemorykiller: Kill 'com.google.android.gms.unstable' … reason: device is not responding
   02:33:34 lowmemorykiller: Kill 'org.mozilla.firefox:gpu…' … reason: device is in direct reclaim and low on memory
   ```
   "Device is not responding" one second before the session's last write means the phone was thrashing.
   That explains the earlier "Claude Code freezes / is very slow" reports. The kill itself was the phantom trim.
6. **The Termux app itself was not killed tonight.** `dumpsys activity exit-info com.termux` (`host/termux-exit-info.txt`):
   the last kill was 11.10 18:23 (`TOO MANY EMPTY PROCS`). Earlier ones were LOW_MEMORY (10.10, 2.10, 29.9),
   one SIGNALED status=9 (5.10) and several TOO MANY EMPTY PROCS. Termux and Termux:API are on the
   deviceidle whitelist (battery unrestricted).

### Not available

- `~/.claude/debug/` has only one file, from 13.9. Claude Code was not started with `--debug`, so no debug log
  exists for tonight's sessions.
- The session transcripts (`~/.claude/projects/…/*.jsonl`) were **not copied** into this repo. The tester's
  permission guard blocked pushing raw session transcripts, which can hold secrets and private
  messages. The relevant facts from them are quoted above (last-write times). If Marko wants the raw tails,
  he can allow it.
- The earlier session `57ab9b50…` (ended 02:08 after starting `ui_test.py`) cannot be checked against logcat.
  Logcat starts at 00:53, but nothing was killed between 02:00 and 02:16. That session ended with a
  `continued-in`, not a kill.

### Suggested fixes (for Marko and the cloud session to decide; nothing was changed on the phone)

- Lift the phantom limit over adb (the three commands in the manifest, `termux-proot-working.md` §
  "Long sessions die at their desk"). This alone would have saved tonight's session.
- Or, without changing Android: stop the termux-services that aren't needed (7 of the 18 processes), and never
  write a scan or loop as one process per item (use one Python process with asyncio).
- Close Firefox tabs and other heavy apps before a DJ Mantra round. With 78 % of swap used, everything is slow.

### Fix applied (Marko approved it, 12.10.2026 02:45 CEST)

```
adb shell settings put global settings_enable_monitor_phantom_procs false   → rc 0
adb shell settings get global settings_enable_monitor_phantom_procs         → false
```
To undo it: the same `settings put` command with `true`. `max_phantom_processes` was not changed (still the default).
No port scan from the earlier session was left running; it died with that session at 02:23.

The watcher (`tools/android/watch_host.sh ~/djmantra-watch/r12 192.168.1.30:38589`) has been running since 00:39:59 UTC.
Its output is copied to `host/watch/` at the end of the round.

## Marko's new request (12.10.2026, 03:05): lay the interface out around the selfie camera hole, no clipped text

> "Pay attention on a camera, selfie camera hole. User interface part are under this selfie camera and they have hole
> in it. You need to spread the user interface around this hole. Hole is giving the problem. And you also need to in
> the screenshot look carefully if some text is going out of its display size, that's not allowed or acceptable behavior."

**The hole on this phone** (Nothing A142, 1084×2412, portrait), from `adb shell dumpsys window`:
`mDisplayCutout=DisplayCutout{insets=Rect(0, 126 - 0, 0) … boundingRect … Rect(454, 0 - 540, 126)}`,
`cutoutSpec={M540.2,33C524.1,33…`. So it is a round hole centred at x≈540, starting at y=33 (≈ x 511–571, y 31–91),
and Android asks apps to keep the top **126 px** (the cutout inset) clear. Screenshots never show the hole.
`cutout-overlay-top.png` draws it (red box) and the 126 px line (yellow) over `02-mixer-both.png`:

- **The hole sits on the key badge, right on "8A" and the "c" of "clash".** A reading under the hole is a reading with a hole in it.
- The whole top strip (dj logo, key badge) is inside the 126 px cutout band. The artist line touches its edge.
- The app draws full screen (`main window ready, full screen: true size QSize(413, 919)` = the whole glass) and does not
  pad for `displayCutout()` insets.

**What to change** (manifest `modules/system-bars.md`): the top row keeps out of the hole. Either the whole
interface starts below the cutout inset (126 px here; ask Qt/Android for it at run time, never a constant, since it
moves on rotation and is different on every phone), or the top row is split into a left part and a right part with
the hole's width free between them (logo + badge left of x≈500, the rest right of x≈580). In landscape the hole is on
a side and the same rule applies to that side.

**Text going out of its box (not acceptable), seen in this round's screenshots:**

| where | what is seen | screenshot |
|---|---|---|
| deck 1 header, TIME mode | `0:31 / 5:56 / -5:2` — the end of the remaining time is cut by the key "8A" | `02-mixer-both.png`, `cutout-overlay-top.png` |
| deck 2 header, TIME mode | `04 / 5:11 / -5:06` — the start ("0:") is cut off | same |
| deck 1 header, BARS mode | `101.1  next -77.` — cut by the key | step 3 screenshots |
| deck 2 artist | `Maxim Sheremetev, C…` — ellipsis, stays inside its box (allowed, but the line could use the space the key badge leaves) | `02-mixer-both.png` |

Probably the same fix for all three: the time/bars label needs a minimum width for its longest text (`-10:00`,
`999.4  next -99.4`), or a smaller font, and the key label must not take its space.

## Round 12 steps

Build: Android run 38188934192 of b1f26f9 (same app code as fbb6ceb + d594f12), `djmantra-arm64-v8a.apk`,
sha256 `02633b30f6e0a9aaac0217e7704e9398ffdbdb17aec4ea22f2f53ee0bef64e6a`, installed with `adb install -r` at 02:56.

| # | what | expected | what happened | pass/fail |
|---|---|---|---|---|
| 1 | GPU at start | "Android: drawing on the GPU (OpenGL ES)", no crash | `02:57:11.467 Android: drawing on the GPU (OpenGL ES)`; `Waveforms: GPU (all-shader) instead of the saved type 10`. No "OpenGL driver version" line was logged. Crash buffer empty. `gpu-log.txt`, `01-start.png` | PASS |
| 2a | Two songs playing | both decks play | Started with `--play /sdcard/Music/Addis.mp3 /sdcard/Music/Dune.mp3` (media volume set to 0 at 3 a.m.; it was 2/16). **Only deck 1 started** (log: `Deck 1 playing`, nothing for deck 2). Deck 2 needed a tap on its play button | FAIL (minor) |
| 2b | Mixer: VU meters move | VU strips between the faders move | **No VU meter at all.** The skin has two 12f `<VuMeter>`s between the channel faders (`skin.xml:622`, `:634`), but nothing is drawn there with both decks playing: `02-mixer-both.png`, `02-mixer-again.png`, crop `02-mixer-vu-crop.png`. No VU/GL warning in the log. Probably fbb6ceb (VU meters on the GPU) | **FAIL** |
| 2c | Waveforms tab | smooth, right colours, fill their box | Both decks scroll; red/pink/blue colours, red play line in the middle, beat lines: `02-waveforms.png`, `02-waveforms-b.png`. Addis (deck 1) fills only ~20 % of its box width, Dune ~60 %. Could be the song's level, not a bug: compare in step 3 with the CPU renderer | PASS (look at the width) |
| 2d | One Deck | layout right | dj logo → ONE DECK: cover, title/artist/BPM, time, key 8A, full-width waveform, ⏮ ⏸ ⏭, CUE, CONTINUOUS (green), Files/Queue/History: `02-onedeck.png`, menu `02-menu.png` | PASS |
| 2e | Deck header time | `1:23 / 5:40 / -4:17` | **Cut off on both decks:** deck 1 `0:44 / 5:56 / -5:` (end clipped by the key), deck 2 `18 / 5:11 / -4:53` (the start "0:" is clipped) | FAIL (cosmetic) |
| 3a | CPU, both decks, GPU on | (measure) | pid 9292, waveforms tab: **92.3 / 89.5 / 70.5 %CPU**; mixer tab: 85.1 / 78.5 / 60.0 %CPU (`cpu-gpu.txt`, measured with `top -p PID`, because `grep djmantra` also matched the grep's own shell). gfxinfo: 23 frames, 90th percentile 250 ms | (measured) |
| 3b | Settings → "Draw on the GPU" off | Settings opens | **Not reached yet.** From the waveforms tab, the dj logo did not open the menu (5 tries over ~1 min), and a tab tap was ignored too. A minute later the mixer tab responded and the menu opened from there, but a tap on "Settings" / its gear closed the menu without opening Settings (twice, 03:04–03:06). Then the logo stopped opening the menu again. uiautomator showed no hidden dialog. Paused here: Marko stopped the round before the restart for the camera hole request | BLOCKED |
