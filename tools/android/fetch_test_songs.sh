#!/usr/bin/env bash
# Downloads Marko's test songs (public Google Drive folder
# 1pgo5Zyxl-t4uVHfLtMMXOSCO9uQpFOT_, list in testing/songs/drive-songs.tsv)
# and puts them in the phone's Music folder, for DJ Mantra tests.
#
#   tools/android/fetch_test_songs.sh [--dest DIR] [--serial SERIAL] [--limit N]
#
# --dest: where to save (default: /sdcard/Music when it is writable, as in
#   Termux on the phone after termux-setup-storage; else ./test-songs).
# --serial: also `adb push` the songs to that device's /sdcard/Music
#   (for a computer with the phone on USB).
# Files already there with the right size are skipped. Needs curl.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
LIST="$ROOT/testing/songs/drive-songs.tsv"
DEST="" SERIAL="" LIMIT=0
while [ $# -gt 0 ]; do
    case "$1" in
        --dest) DEST="$2"; shift 2 ;;
        --serial) SERIAL="$2"; shift 2 ;;
        --limit) LIMIT="$2"; shift 2 ;;
        *) echo "Unknown option $1" >&2; exit 2 ;;
    esac
done
if [ -z "$DEST" ]; then
    if [ -w /sdcard/Music ]; then DEST=/sdcard/Music; else DEST="$PWD/test-songs"; fi
fi
mkdir -p "$DEST"
ok=0 fail=0 n=0
while IFS=$'\t' read -r id name; do
    [ -n "$id" ] || continue
    n=$((n + 1)); [ "$LIMIT" -gt 0 ] && [ "$n" -gt "$LIMIT" ] && break
    out="$DEST/$name"
    if [ -s "$out" ] && ! head -c 15 "$out" | grep -qi '<!doctype\|<html'; then
        echo "have  $name"; ok=$((ok + 1)); continue
    fi
    if curl -fsSL --retry 3 -o "$out.part" \
        "https://drive.usercontent.google.com/download?id=$id&export=download&confirm=t" &&
        ! head -c 15 "$out.part" | grep -qi '<!doctype\|<html'; then
        mv "$out.part" "$out"; echo "got   $name ($(wc -c < "$out") bytes)"; ok=$((ok + 1))
    else
        rm -f "$out.part"; echo "FAIL  $name" >&2; fail=$((fail + 1))
    fi
done < "$LIST"
if [ -n "$SERIAL" ]; then
    adb -s "$SERIAL" shell mkdir -p /sdcard/Music
    adb -s "$SERIAL" push "$DEST"/*.mp3 /sdcard/Music/ > /dev/null && echo "pushed to $SERIAL:/sdcard/Music"
    adb -s "$SERIAL" shell am broadcast -a android.intent.action.MEDIA_SCANNER_SCAN_FILE -d file:///sdcard/Music > /dev/null 2>&1
fi
echo "Songs in $DEST: $ok ok, $fail failed"
[ "$fail" -eq 0 ]
