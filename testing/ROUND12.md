# Round 12 on the phone (for the local phone session, 12.10.2026)

Build: the Android CI run of `fbb6ceb` or later on `ccr-eb7d1019-d3kubd` (artifact `*phone-arm64*.apk`).
Output: `testing/results/<UTC stamp>_r12_phone/REPORT.md` with a table
"what | expected | what happened | pass/fail", screenshots, logs; push to `ccr-eb7d1019-d3kubd`,
then message the cloud session "DJ mantra app testing" (session_01Ggm1Tq5a9mnY3wAZMMS9jh).
Songs: `tools/android/fetch_test_songs.sh --dest /sdcard/Music` (69 MP3s, skips those already there).

1. **GPU** (new: waveforms and VU meters on the GPU, Marko's request). Start the app, then
   `adb logcat -d | grep -E "Android: drawing on the|GPU did not start|OpenGL driver version|Waveforms: GPU|FATAL|SIGSEGV" > $OUT/gpu-log.txt`.
   Expect "Android: drawing on the GPU (OpenGL ES)". A crash: `adb logcat -d -b crash > $OUT/crash.txt`, report at once.
2. Two of Marko's songs playing: screenshots of the waveforms tab, the mixer (VU meters move) and
   One Deck. Waveforms smooth, right colours, filling their box?
3. CPU load, both decks playing: `adb shell top -b -n 3 -d 2 | grep -i djmantra > $OUT/cpu-gpu.txt`;
   Settings → "Draw on the GPU" off, restart, same songs: `... > $OUT/cpu-cpu.txt`; switch it back on.
4. Deck counter: a tap switches TIME (1:23 / 5:40 / -4:17) ↔ BARS & BEATS (17.3  next -6.2  4/4) on
   both decks; bar.beat moves while playing. Screenshots.
5. Camelot: keys shown as 8A / 5B; the badge between the decks (colour, text) with two songs; a tap
   opens the wheel; Settings → Camelot wheel. Screenshots.
6. `python3 tools/android/ui_test.py --serial $SERIAL --out $OUT` (round 11 failed: deck 2 hot cue
   pads, the p_mixer / p_deck selectors, landscape mode buttons and pads).
7. Mix: two songs, SYNC on deck 2, crossfader left → right; One Deck: play, next, previous, a row tap.
8. **No "Mixxx" on screen** (d594f12): on every screen visited, `adb shell uiautomator dump /sdcard/ui.xml && adb shell grep -c -i mixxx /sdcard/ui.xml`
   (Qt widgets may not show in the dump: then look at the screenshots). Press Android's back while a deck plays:
   the box must say "Exit DJ Mantra?". Settings → About may say "Mixxx" only in the Engine line (licence credit).

## Before the round: why Claude Code breaks down while DJ Mantra runs (Marko, 12.10.2026)

Claude Code (Termux) and DJ Mantra run on the same phone. Suspects: Android's phantom process killer
(kills Termux's child processes, like Claude Code, when many run or they use CPU) and the low memory
killer (kills background apps while a heavy app is in front). To find out:

1. `termux-wake-lock` (Termux keeps running with the screen off or another app in front).
2. In a second Termux session (swipe from the left → NEW SESSION):
   `tools/android/watch_host.sh ~/djmantra-watch/r12 $SERIAL` — leave it running for the whole round.
3. Start Claude Code with `claude --debug` from now on (its log goes to `~/.claude/debug/`).
4. After the round (or after a breakdown): copy `~/djmantra-watch/r12/` and the newest file of
   `~/.claude/debug/` into `$OUT/host/`, and the end of the session transcript
   (`ls -t ~/.claude/projects/*/*.jsonl | head -1`, last 200 lines) as `$OUT/host/transcript-tail.jsonl`.
   Report in REPORT.md: did Termux / node die, when, and the lines of `kills.log` at that time.
