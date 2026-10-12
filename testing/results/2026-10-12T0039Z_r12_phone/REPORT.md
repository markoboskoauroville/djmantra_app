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

## Round 12 steps

Build: Android run 38188934192 of b1f26f9 (same app code as fbb6ceb + d594f12), `djmantra-arm64-v8a.apk`,
sha256 `02633b30f6e0a9aaac0217e7704e9398ffdbdb17aec4ea22f2f53ee0bef64e6a`, installed with `adb install -r` at 02:56.

| # | what | expected | what happened | pass/fail |
|---|---|---|---|---|
| 1 | GPU at start | "Android: drawing on the GPU (OpenGL ES)", no crash | `02:57:11.467 Android: drawing on the GPU (OpenGL ES)`; `Waveforms: GPU (all-shader) instead of the saved type 10`. No "OpenGL driver version" line was logged. Crash buffer empty. `gpu-log.txt`, `01-start.png` | PASS |
