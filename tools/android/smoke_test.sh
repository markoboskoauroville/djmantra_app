#!/bin/bash
# DJ Mantra launch test: installs the APK on a running emulator (or phone),
# starts it and fails if it crashes, shows a critical error or does not reach
# its main window.
#
#   tools/android/smoke_test.sh <apk> <out-dir> [<dir with unstripped .so>]
#
# Writes logcat.txt, screen.png and, after a crash, a symbolized backtrace
# (crash.txt, via the NDK's ndk-stack) to <out-dir>.
set -u

APK=$1
OUT=$2
SYMBOLS=${3:-}
PACKAGE=com.djmantra.app
ACTIVITY=org.qtproject.qt.android.bindings.QtActivity
READY="DJ Mantra main window ready"
TIMEOUT=${SMOKE_TIMEOUT:-150}
# How long it must stay alive after the main window is ready
SETTLE=${SMOKE_SETTLE:-15}

mkdir -p "$OUT"
exec > >(tee "$OUT/summary.txt") 2>&1
adb wait-for-device
adb install -r -g "$APK" || { echo "FAIL: install"; exit 1; }
adb shell dumpsys package "$PACKAGE" | grep -m1 versionName | sed 's/^ */installed: /'
adb logcat -c
adb shell am start -W -n "$PACKAGE/$ACTIVITY"

result=""
ready_at=""
for ((t = 0; t < TIMEOUT; t += 3)); do
    sleep 3
    adb logcat -d > "$OUT/logcat.txt"
    if grep -qE "Fatal signal|FATAL EXCEPTION|>>> $PACKAGE <<<" "$OUT/logcat.txt"; then
        result="crashed"
        break
    fi
    if [ -z "$(adb shell pidof "$PACKAGE" | tr -d '\r')" ]; then
        result="exited"
        break
    fi
    if [ -z "$ready_at" ] && grep -q "$READY" "$OUT/logcat.txt"; then
        ready_at=$t
        echo "main window ready after ${t}s"
    fi
    if [ -n "$ready_at" ] && ((t - ready_at >= SETTLE)); then
        result="ok"
        break
    fi
done
[ -n "$result" ] || result="timeout"

sleep 2
adb logcat -d > "$OUT/logcat.txt"
adb exec-out screencap -p > "$OUT/screen.png" 2>/dev/null || true

# Critical errors the app reported (before and after its own logging starts)
grep -E " E default *:|critical \[" "$OUT/logcat.txt" > "$OUT/critical.txt" || true

if [ "$result" = "crashed" ] && [ -n "$SYMBOLS" ]; then
    NDK=${ANDROID_NDK_HOME:-$(ls -d "${ANDROID_SDK_ROOT:-$ANDROID_HOME}"/ndk/* 2>/dev/null | sort -V | tail -1)}
    if [ -x "$NDK/ndk-stack" ]; then
        "$NDK/ndk-stack" -sym "$SYMBOLS" -i "$OUT/logcat.txt" > "$OUT/crash.txt" 2>&1 || true
    fi
fi

echo "### Launch test: $result"
echo
grep -E "DJMantra|default *:" "$OUT/logcat.txt" | tail -40 | sed 's/^/    /'
fail=0
if [ "$result" != "ok" ]; then
    fail=1
    echo
    echo "FAIL: $result (ready line seen: ${ready_at:-no})"
fi
if [ -s "$OUT/critical.txt" ]; then
    fail=1
    echo
    echo "FAIL: critical errors:"
    sed 's/^/    /' "$OUT/critical.txt"
fi
if [ -s "$OUT/crash.txt" ]; then
    echo
    echo "Symbolized crash:"
    sed 's/^/    /' "$OUT/crash.txt" | head -80
fi
[ "$fail" = 0 ] && echo && echo "OK: started, main window ready, alive ${SETTLE}s later, no critical errors"
exit $fail
