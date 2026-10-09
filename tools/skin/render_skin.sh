#!/bin/bash
# Renders the desktop build with the DJ Mantra skin in a virtual X screen and
# saves a screenshot: the phone layouts can be checked without a phone.
#
#   tools/skin/render_skin.sh <mixxx binary> <width> <height> <out.png> [clicks]
#
# clicks: "x,y" taps and "dx,y" double taps after start, e.g. "455,100 d300,200"
# (landscape 914x411: the waveform tab, then a double tap on the waveforms).
# Needs Xvfb, xdotool and ImageMagick's import.
set -u
BIN=$1; W=$2; H=$3; OUT=$4; CLICKS=${5:-}
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
TMP="$(mktemp -d)"
printf '[Config]\nResizableSkin DJMantra\n\n[DJMantra]\nskin_migrated 1\n' > "$TMP/mixxx.cfg"
python3 - "$TMP" <<'PY'
import math, random, struct, sys, wave
def make(path, bpm, seconds, base):
    rate = 22050; beat = 60.0 / bpm; out = bytearray(); random.seed(bpm)
    for i in range(rate * seconds):
        t = i / rate; bt = t % beat
        kick = math.sin(2 * math.pi * (50 + 80 * math.exp(-bt * 30)) * bt) * math.exp(-bt * 8)
        hat = (random.random() * 2 - 1) * math.exp(-((t + beat / 2) % beat) * 40) * 0.3
        chord = sum(math.sin(2 * math.pi * f * t) for f in (base, base * 1.25, base * 1.5)) * 0.12
        s = int(max(-1, min(1, 0.6 * kick + hat + chord)) * 20000)
        out += struct.pack('<hh', s, s)
    w = wave.open(path, 'wb'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(rate)
    w.writeframes(bytes(out)); w.close()
make(sys.argv[1] + '/track_a.wav', 124, 60, 220)
make(sys.argv[1] + '/track_b.wav', 128, 60, 330)
PY
export DISPLAY=:$((90 + RANDOM % 9))
Xvfb "$DISPLAY" -screen 0 "${W}x${H}x24" > /dev/null 2>&1 &
XP=$!
sleep 1
# RENDER_ARGS: more options, e.g. "--play"; QT_SCALE_FACTOR=2.625 renders like a Pixel 7
"$BIN" --resource-path "$ROOT/res/" --settings-path "$TMP" ${RENDER_ARGS:-} "$TMP/track_a.wav" "$TMP/track_b.wav" \
    > "$TMP/run.log" 2>&1 &
MP=$!
sleep 12
# First start on desktop: music folder picker (Esc), no sound device (Continue),
# hidden menu bar notice (Hide)
xdotool key Escape; sleep 4; xdotool key Return; sleep 4; xdotool key Return; sleep 2
for wid in $(xdotool search --onlyvisible --name "Mixxx|DJ Mantra"); do
    xdotool windowmove "$wid" 0 0 windowsize "$wid" "$W" "$H"
done
sleep 4
for c in $CLICKS; do
    case $c in
        d*) xy=${c#d}; xdotool mousemove "${xy%,*}" "${xy#*,}" click --repeat 2 --delay 120 1 ;;
        *) xdotool mousemove "${c%,*}" "${c#*,}" click 1 ;;
    esac
    sleep 2
done
sleep 3
import -window root "$OUT"
kill $MP; sleep 1; kill -9 $MP 2> /dev/null; kill $XP
grep -E "Loaded skin|critical" "$TMP/run.log"
[ -n "${KEEP_LOG:-}" ] && cp "$TMP/run.log" "$KEEP_LOG"
rm -rf "$TMP"
