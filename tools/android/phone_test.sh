#!/usr/bin/env bash
# DJ Mantra phone smoke test.
#
# Downloads the latest APK built by the "Android" GitHub Actions workflow,
# installs it on the USB-connected phone, launches it and records what
# happened in testing/results/<timestamp>_<device>.md (+ log/screenshot files).
#
# Requirements: adb (Android platform-tools), gh (GitHub CLI, logged in),
# a phone with USB debugging enabled and authorised for this computer.
#
# Usage: tools/android/phone_test.sh [--branch BRANCH] [--apk PATH] [--wait SECONDS]

set -euo pipefail

REPO="markoboskoauroville/djmantra_app"
BRANCH="claude/admiring-feynman-hym3vp"
APK=""
WAIT=20
PKG_PATTERN='djmantra|mixxx'

while [ $# -gt 0 ]; do
    case "$1" in
        --branch) BRANCH="$2"; shift 2 ;;
        --apk) APK="$2"; shift 2 ;;
        --wait) WAIT="$2"; shift 2 ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
done

ROOT="$(git rev-parse --show-toplevel)"
cd "$ROOT"

need() { command -v "$1" >/dev/null 2>&1 || { echo "Missing tool: $1 ($2)" >&2; exit 1; }; }
need adb "install Android platform-tools"
[ -n "$APK" ] || need gh "install GitHub CLI and run: gh auth login"

# --- Device -----------------------------------------------------------------
if ! adb get-state >/dev/null 2>&1; then
    echo "No phone found. Plug it in, enable USB debugging and accept the prompt on the phone." >&2
    adb devices -l >&2
    exit 1
fi
MODEL="$(adb shell getprop ro.product.model | tr -d '\r')"
ANDROID_VER="$(adb shell getprop ro.build.version.release | tr -d '\r')"
SDK="$(adb shell getprop ro.build.version.sdk | tr -d '\r')"
ABI="$(adb shell getprop ro.product.cpu.abi | tr -d '\r')"
DEVICE_SLUG="$(echo "$MODEL" | tr -c 'A-Za-z0-9' '-' | sed 's/-*$//')"

STAMP="$(date +%Y-%m-%d_%H%M)"
OUT="testing/results/${STAMP}_${DEVICE_SLUG}"
mkdir -p "$OUT"
REPORT="${OUT}.md"

# --- APK ----------------------------------------------------------------------
RUN_ID="local"
COMMIT="$(git rev-parse --short HEAD)"
if [ -z "$APK" ]; then
    case "$ABI" in
        arm64-v8a) ARTIFACT="djmantra-arm64-v8a-apk" ;;
        x86_64) ARTIFACT="djmantra-x86_64-apk" ;;
        *) echo "Unsupported phone ABI: $ABI" >&2; exit 1 ;;
    esac
    RUN_JSON="$(gh run list -R "$REPO" -w Android -b "$BRANCH" -s success -L 1 --json databaseId,headSha)"
    RUN_ID="$(echo "$RUN_JSON" | python3 -c 'import sys,json;r=json.load(sys.stdin);print(r[0]["databaseId"] if r else "")')"
    COMMIT="$(echo "$RUN_JSON" | python3 -c 'import sys,json;r=json.load(sys.stdin);print(r[0]["headSha"][:7] if r else "")')"
    if [ -z "$RUN_ID" ]; then
        echo "No successful Android CI run on $BRANCH yet." >&2
        exit 1
    fi
    DL="$(mktemp -d)"
    gh run download "$RUN_ID" -R "$REPO" -n "$ARTIFACT" -D "$DL"
    APK="$(find "$DL" -name '*.apk' | head -1)"
fi
[ -f "$APK" ] || { echo "APK not found: $APK" >&2; exit 1; }

# --- Install & launch ----------------------------------------------------------
INSTALL_LOG="$(adb install -r -g "$APK" 2>&1 || true)"
PKG="$(adb shell pm list packages | tr -d '\r' | sed 's/^package://' | grep -E "$PKG_PATTERN" | head -1 || true)"

LAUNCHED="no"; ALIVE="no"; CRASH="none found"
if [ -n "$PKG" ]; then
    adb logcat -c || true
    adb shell monkey -p "$PKG" -c android.intent.category.LAUNCHER 1 >/dev/null 2>&1 && LAUNCHED="yes"
    sleep "$WAIT"
    PID="$(adb shell pidof "$PKG" | tr -d '\r' || true)"
    [ -n "$PID" ] && ALIVE="yes (pid $PID)"
    adb exec-out screencap -p > "$OUT/screen.png" || true
    adb logcat -d -v time > "$OUT/logcat-full.txt" || true
    # Keep the committed log small: app, Qt, audio, MIDI and crash lines only.
    grep -E -i "$PKG|mixxx|qt|libc|DEBUG|AndroidRuntime|FATAL|oboe|aaudio|midi|bluetooth" \
        "$OUT/logcat-full.txt" | tail -n 3000 > "$OUT/logcat.txt" || true
    rm -f "$OUT/logcat-full.txt"
    if grep -qE 'FATAL EXCEPTION|Fatal signal|SIGSEGV|SIGABRT' "$OUT/logcat.txt"; then
        CRASH="$(grep -m1 -E 'FATAL EXCEPTION|Fatal signal|SIGSEGV|SIGABRT' "$OUT/logcat.txt")"
    fi
fi

adb shell dumpsys midi > "$OUT/midi.txt" 2>&1 || true
adb shell dumpsys media.audio_policy > "$OUT/audio_policy.txt" 2>&1 || true
MIDI_DEVICES="$(grep -iE 'name=|PROPERTY_NAME|manufacturer' "$OUT/midi.txt" | sed 's/^ *//' | sort -u | head -20 || true)"
AUDIO_DEVICES="$(grep -oE 'AUDIO_DEVICE_OUT_[A-Z_]+' "$OUT/audio_policy.txt" | sort -u | tr '\n' ' ' || true)"

# --- Report ------------------------------------------------------------------
cat > "$REPORT" <<MD
# Phone test ${STAMP}

| | |
|---|---|
| Phone | ${MODEL} (Android ${ANDROID_VER}, API ${SDK}, ${ABI}) |
| Build | commit \`${COMMIT}\`, CI run ${RUN_ID} |
| Package | \`${PKG:-not installed}\` |
| Launched | ${LAUNCHED} |
| Still running after ${WAIT}s | ${ALIVE} |
| Crash | ${CRASH} |

## Install output
\`\`\`
${INSTALL_LOG}
\`\`\`

## MIDI devices seen by Android
\`\`\`
${MIDI_DEVICES:-none}
\`\`\`

## Audio outputs known to Android
${AUDIO_DEVICES:-none}

## Files
- [screenshot](${STAMP}_${DEVICE_SLUG}/screen.png)
- [logcat (filtered)](${STAMP}_${DEVICE_SLUG}/logcat.txt)
- [dumpsys midi](${STAMP}_${DEVICE_SLUG}/midi.txt)
- [audio policy](${STAMP}_${DEVICE_SLUG}/audio_policy.txt)

## Manual checks
<!-- Filled in by /test-on-phone from the user's answers -->
MD

echo "REPORT=$REPORT"
