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
focus=$(adb shell dumpsys window 2>/dev/null | grep -m1 -E "mCurrentFocus" | tr -d '\r')

# A hang: the stacks of all threads (emulator images allow adb root)
if [ "$result" = "timeout" ]; then
    pid=$(adb shell pidof "$PACKAGE" | tr -d '\r')
    adb root > /dev/null 2>&1 && sleep 3 && adb wait-for-device
    adb shell debuggerd -b "$pid" > "$OUT/stacks.txt" 2>&1 || true
fi

# Critical errors the app reported (before and after its own logging starts)
grep -E " E default *:|critical \[" "$OUT/logcat.txt" > "$OUT/critical.txt" || true

NDK=${ANDROID_NDK_HOME:-$(ls -d "${ANDROID_SDK_ROOT:-$ANDROID_HOME}"/ndk/* 2>/dev/null | sort -V | tail -1)}
if [ -n "$SYMBOLS" ] && [ -x "$NDK/ndk-stack" ]; then
    if [ "$result" = "crashed" ]; then
        "$NDK/ndk-stack" -sym "$SYMBOLS" -i "$OUT/logcat.txt" > "$OUT/crash.txt" 2>&1 || true
    fi
    if [ -s "$OUT/stacks.txt" ]; then
        # ndk-stack reads one backtrace at a time: the main Qt thread's
        awk '/^"qtMainLoopThrea"/,/^$/' "$OUT/stacks.txt" > "$OUT/main-thread.txt"
        { echo "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***"
          echo "backtrace:"
          grep -E "^ +#[0-9]+ pc" "$OUT/main-thread.txt"; } > "$OUT/main-thread-bt.txt"
        "$NDK/ndk-stack" -sym "$SYMBOLS" -i "$OUT/main-thread-bt.txt" > "$OUT/hang.txt" 2>&1 || true
    fi
fi

echo "### Launch test: $result"
echo
echo "focused window: ${focus:-?}"
echo
grep -E "DJMantra|default *:" "$OUT/logcat.txt" | grep -v SchemaManager | tail -80 | sed 's/^/    /'
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
if [ -s "$OUT/hang.txt" ]; then
    echo
    echo "Main thread while hanging (symbolized):"
    sed 's/^/    /' "$OUT/hang.txt" | head -80
elif [ -s "$OUT/main-thread.txt" ]; then
    echo
    echo "Main thread while hanging:"
    sed 's/^/    /' "$OUT/main-thread.txt" | head -60
fi
[ "$fail" = 0 ] && echo && echo "OK: started, main window ready, alive ${SETTLE}s later, no critical errors"
exit $fail
