#!/data/data/com.termux/files/usr/bin/bash
# Why does Claude Code in Termux break down while DJ Mantra runs on the same
# phone? (Marko, 12.10.2026.) Logs every few seconds, until stopped:
#   - Android killing processes: phantom process killer, low memory killer,
#     ActivityManager kills / ANRs of Termux or DJ Mantra (logcat)
#   - free memory and memory pressure, the top CPU users
#   - whether Claude Code (node) and DJ Mantra are alive, their memory
# and once at the start: the phantom process settings, Termux battery state.
#
#   tools/android/watch_host.sh [OUT_DIR] [SERIAL]     (in Termux; Ctrl-C stops)
# Needs adb connected to this phone (SERIAL, default the only device).
OUT=${1:-$HOME/djmantra-watch/$(date -u +%Y-%m-%d_%H%M)}
SERIAL=${2:-}
A=(adb); [ -n "$SERIAL" ] && A=(adb -s "$SERIAL")
mkdir -p "$OUT"
echo "Logging to $OUT"

{
    echo "== $(date -u) start"
    echo "-- phantom process settings"
    "${A[@]}" shell /system/bin/device_config get activity_manager max_phantom_processes
    "${A[@]}" shell settings get global settings_enable_monitor_phantom_procs
    echo "-- Termux battery optimisation (whitelisted = not killed in the background)"
    "${A[@]}" shell dumpsys deviceidle whitelist | grep -i termux
    echo "-- memory"
    "${A[@]}" shell cat /proc/meminfo | head -5
    echo "-- Claude Code version"
    claude --version 2>&1
    echo "-- Claude Code debug logs"
    ls -lt "$HOME/.claude/debug" 2>/dev/null | head -5
} > "$OUT/start.txt" 2>&1

# Kills and crashes, as they happen
"${A[@]}" logcat -c
"${A[@]}" logcat -v time -b main,system,events,crash 2>/dev/null |
    grep --line-buffered -iE "PhantomProcess|lowmemorykiller|lmkd|am_kill|Killing [0-9]+:|killProcess|am_anr|ANR in|termux|com\.djmantra|FATAL|SIGKILL|signal 9|OutOfMemory|am_proc_died|am_low_memory" \
    > "$OUT/kills.log" &
LOGCAT=$!
trap 'kill $LOGCAT 2>/dev/null; echo "stopped: $OUT"; exit 0' INT TERM

while true; do
    {
        echo "== $(date -u +%H:%M:%S)"
        "${A[@]}" shell "grep -E 'MemAvailable|SwapFree' /proc/meminfo; cat /proc/pressure/memory 2>/dev/null | head -1"
        echo "-- alive: djmantra=$("${A[@]}" shell pidof com.djmantra.app) termux=$("${A[@]}" shell pidof com.termux) node=$(pgrep -f 'claude|node' | tr '\n' ' ')"
        ps -o pid,rss,etime,args -p "$(pgrep -d, -f 'claude|node' || echo 1)" 2>/dev/null | head -6
        "${A[@]}" shell top -b -n 1 -m 6 2>/dev/null | tail -7
        echo "-- adb processes in Termux: $(pgrep -c adb)"
    } >> "$OUT/watch.log" 2>&1
    sleep 5
done
