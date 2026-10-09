#!/usr/bin/env bash
# DJ Mantra test on every connected Android device (phones and emulators).
#
#   tools/android/device_test.sh [--apk-dir DIR] [--sound] [--serial SERIAL]...
#
# For each device in `adb devices` (or each --serial): installs the APK for its
# ABI (uninstalling first if an old build has another signature), runs the
# launch test (tools/android/smoke_test.sh) and writes the results to
# testing/results/<stamp>_<device>/ plus one table, testing/results/<stamp>_devices.md.
#
# --sound: copies a 440 Hz test tone to the device's Music folder, starts the
#   app with "--play <tone>" and fails unless deck 1 plays and sound (not
#   silence) reaches the output. The tone plays through whatever Android
#   routes media to: the speaker when nothing is plugged in.
# --apk-dir: a folder with the APKs (*arm64*.apk, *x86_64*.apk). Default: the
#   android-latest release, downloaded with gh.
#
# Needs adb, python3 and (without --apk-dir) gh logged in.
# (bash 3.2 on macOS: no set -u, empty arrays would count as unset)
set -o pipefail

REPO="markoboskoauroville/djmantra_app"
PACKAGE=com.djmantra.app
APK_DIR=""
SOUND=""
SERIALS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --apk-dir) APK_DIR="$2"; shift 2 ;;
        --sound) SOUND=1; shift ;;
        --serial) SERIALS+=("$2"); shift 2 ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
done

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

if [ -z "$APK_DIR" ]; then
    APK_DIR="$(mktemp -d)"
    gh release download android-latest -R "$REPO" -p '*.apk' -D "$APK_DIR" ||
        { echo "Could not download the android-latest release" >&2; exit 1; }
fi
ls "$APK_DIR"/*.apk > /dev/null 2>&1 || { echo "No APK in $APK_DIR" >&2; exit 1; }
BUILD="$(ls "$APK_DIR"/*.apk | head -1 | xargs basename | sed -E 's/^DJMantra-([^-]+-[0-9a-f]+).*/\1/')"

if [ ${#SERIALS[@]} -eq 0 ]; then
    while read -r serial state _; do
        [ "$state" = device ] && SERIALS+=("$serial")
    done < <(adb devices | tail -n +2)
fi
[ ${#SERIALS[@]} -gt 0 ] || { echo "No device found (adb devices)" >&2; adb devices -l >&2; exit 1; }

STAMP="$(date +%Y-%m-%d_%H%M)"
TABLE="testing/results/${STAMP}_devices.md"
TONE=""
if [ -n "$SOUND" ]; then
    TONE="$(mktemp -d)/djmantra-test-tone.wav"
    # 90 s, 440 Hz, stereo, at -12 dB, with short fades
    python3 - "$TONE" <<'PY'
import math, struct, sys, wave
rate, seconds, amp = 44100, 90, 0.25
with wave.open(sys.argv[1], "wb") as w:
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(rate)
    frames = bytearray()
    n = rate * seconds
    fade = rate // 20
    for i in range(n):
        g = min(1.0, i / fade, (n - i) / fade)
        v = int(32767 * amp * g * math.sin(2 * math.pi * 440 * i / rate))
        frames += struct.pack("<hh", v, v)
    w.writeframes(bytes(frames))
PY
fi

{
    echo "# Device test ${STAMP}"
    echo
    echo "Build: \`${BUILD}\`$( [ -n "$SOUND" ] && echo ", with the sound test (440 Hz tone on deck 1)")"
    echo
    echo "| Device | Android | Result | Sound output | Audio route | Window |"
    echo "|---|---|---|---|---|---|"
} > "$TABLE"

overall=0
for serial in "${SERIALS[@]}"; do
    export ANDROID_SERIAL="$serial"
    model="$(adb shell getprop ro.product.model | tr -d '\r')"
    release="$(adb shell getprop ro.build.version.release | tr -d '\r')"
    sdk="$(adb shell getprop ro.build.version.sdk | tr -d '\r')"
    abi="$(adb shell getprop ro.product.cpu.abi | tr -d '\r')"
    slug="$(echo "$model" | tr -c 'A-Za-z0-9' '-' | sed 's/-*$//')"
    [ "$(adb shell getprop ro.kernel.qemu | tr -d '\r')" = 1 ] && slug="${slug}-emulator"
    out="testing/results/${STAMP}_${slug}"
    mkdir -p "$out"
    echo "=== $model ($serial, Android $release, $abi) ==="

    case "$abi" in
        arm64-v8a) apk="$(ls "$APK_DIR"/*arm64*.apk | head -1)" ;;
        x86_64) apk="$(ls "$APK_DIR"/*x86_64*.apk | head -1)" ;;
        *) apk="" ;;
    esac
    if [ -z "$apk" ]; then
        echo "| $model | $release | **no APK for $abi** | | | |" >> "$TABLE"
        overall=1
        continue
    fi

    # An older build signed with another key cannot be updated in place
    if ! inst="$(adb install -r -g "$apk" 2>&1)"; then
        if echo "$inst" | grep -q "INSTALL_FAILED_UPDATE_INCOMPATIBLE\|signatures do not match"; then
            echo "Other signature installed: uninstalling the old build"
            adb uninstall "$PACKAGE" > /dev/null
        fi
    fi

    smoke_args=""
    expect_play=""
    if [ -n "$SOUND" ]; then
        adb push "$TONE" /sdcard/Music/djmantra-test-tone.wav > /dev/null
        # Media volume about half way, so the speaker is audible but not loud
        max="$(adb shell cmd media_session volume --stream 3 --get 2>/dev/null |
            sed -nE 's/.*range \[[0-9]+\.\.([0-9]+)\].*/\1/p' | tr -d '\r')"
        [ -n "$max" ] && adb shell cmd media_session volume --stream 3 --set $((max / 2)) > /dev/null 2>&1
        smoke_args="--play /sdcard/Music/djmantra-test-tone.wav"
        expect_play=1
    fi

    SMOKE_ARGS="$smoke_args" SMOKE_EXPECT_PLAY="$expect_play" SMOKE_NO_SCREENSHOT_TEXT=1 \
        tools/android/smoke_test.sh "$apk" "$out"
    status=$?
    [ $status = 0 ] || overall=1

    adb shell dumpsys media.audio_flinger > "$out/audio_flinger.txt" 2>&1 || true
    route="$(grep -oE "AUDIO_DEVICE_OUT_[A-Z_]+" "$out/audio_flinger.txt" | sort | uniq -c |
        sort -rn | head -3 | awk '{print $2}' | sed 's/AUDIO_DEVICE_OUT_//' | paste -sd ',' -)"
    soundline="$(grep -m1 "Sound output open" "$out/logcat.txt" | sed 's/.*Sound output open: *//' | cut -c1-90)"
    level="$(grep "Output level: peak" "$out/logcat.txt" | tail -1 | sed 's/.*peak */peak /')"
    window="$(grep -m1 "main window ready" "$out/logcat.txt" | sed 's/.*full screen: *//' | cut -c1-60)"
    result="$(grep -m1 "^### Launch test:" "$out/summary.txt" | sed 's/### Launch test: //')"
    [ $status = 0 ] && verdict="**pass**" || verdict="**fail** ($result)"
    echo "| $model | $release (API $sdk) | $verdict | ${soundline:-none}${level:+; $level} | ${route:-?} | ${window:-?} |" >> "$TABLE"
    unset ANDROID_SERIAL
done

{
    echo
    echo "Per device: \`summary.txt\` (verdict and app log), \`logcat.txt\`, \`screen.png\`,"
    echo "\`audio_flinger.txt\`; after a crash \`crash.txt\`, after a hang \`hang.txt\`."
} >> "$TABLE"
echo
cat "$TABLE"
exit $overall
