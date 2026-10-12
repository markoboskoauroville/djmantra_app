"""rekordbox's BPM, beat grid and key as a reference (Marko, 12.10.2026: "not
only phrase, bpm and key"). Nothing of this goes into the app.

    python3 -I rekordbox_xml.py rekordbox.xml out.json

rekordbox: File > Export Collection in xml format. Out: per audio file name the
average BPM, the first beat and its bar position (the grid's "1"), the tempo
markers and the key (rekordbox's Tonality, e.g. "Am", "F#m", "Db") with its
Camelot code.
"""
import json
import os
import sys
import urllib.parse
import xml.etree.ElementTree as ET

# musical key -> Camelot
CAMELOT = {}
for i, (minor, major) in enumerate(zip(
        ["Abm", "Ebm", "Bbm", "Fm", "Cm", "Gm", "Dm", "Am", "Em", "Bm", "F#m", "Dbm"],
        ["B", "F#", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E"])):
    CAMELOT[minor] = f"{i + 1}A"
    CAMELOT[major] = f"{i + 1}B"
SAME = {"G#m": "Abm", "D#m": "Ebm", "A#m": "Bbm", "Gbm": "F#m", "C#m": "Dbm", "Gb": "F#", "C#": "Db",
        "G#": "Ab", "D#": "Eb", "A#": "Bb", "Cb": "B"}


def camelot(key):
    key = (key or "").strip()
    return CAMELOT.get(SAME.get(key, key), "")


def main(xml_path, out_path):
    result = {}
    for track in ET.parse(xml_path).getroot().iter("TRACK"):
        location = track.get("Location")
        if not location:
            continue  # a playlist entry, not a collection track
        name = os.path.basename(urllib.parse.unquote(urllib.parse.urlparse(location).path))
        tempos = [{"s": float(t.get("Inizio", 0)), "bpm": float(t.get("Bpm", 0)),
                   "beat_in_bar": int(t.get("Battito", 1))} for t in track.findall("TEMPO")]
        key = track.get("Tonality", "")
        result[name] = {
            "bpm": float(track.get("AverageBpm", 0) or 0),
            "key": key,
            "camelot": camelot(key),
            "first_beat_s": tempos[0]["s"] if tempos else None,
            "first_beat_in_bar": tempos[0]["beat_in_bar"] if tempos else None,
            "tempo_markers": tempos,
            "duration_s": float(track.get("TotalTime", 0) or 0),
        }
    json.dump(result, open(out_path, "w"), indent=1, ensure_ascii=False)
    print(f"{len(result)} tracks (BPM, grid, key) -> {out_path}")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
