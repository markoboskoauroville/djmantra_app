"""Rekordbox's own phrases as the reference point (Marko, 12.10.2026: "just to
create a reference point"; nothing of this goes into the app).

    python3 -I rekordbox_phrases.py <USB>/PIONEER/USBANLZ  out.json

Rekordbox (Preferences > Analysis: Phrase on) analyses the songs; an export to
a USB stick writes per song ANLZ0000.DAT (beat grid PQTZ, file path PPTH) and
ANLZ0000.EXT (phrases PSSI, masked in newer versions). Format: Deep Symmetry's
description, as in lib/rekordbox-metadata/rekordbox_anlz.ksy. Out: per audio
file name its phrases with start beat, time (s) and kind, and the BPM.
"""
import json
import os
import struct
import sys

KINDS = {
    1: {1: "Intro", 2: "Up", 3: "Down", 5: "Chorus", 6: "Outro"},  # high mood
    2: {1: "Intro", 2: "Verse 1", 3: "Verse 2", 4: "Verse 3", 5: "Verse 4", 6: "Verse 5", 7: "Verse 6",
        8: "Bridge", 9: "Chorus", 10: "Outro"},  # mid
    3: {1: "Intro", 2: "Verse 1", 3: "Verse 1", 4: "Verse 1", 5: "Verse 2", 6: "Verse 2", 7: "Verse 2",
        8: "Bridge", 9: "Chorus", 10: "Outro"},  # low
}
MASK = [0xCB, 0xE1, 0xEE, 0xFA, 0xE5, 0xEE, 0xAD, 0xEE, 0xE9, 0xD2, 0xE9, 0xEB, 0xE1, 0xE9, 0xF3, 0xE8,
        0xE9, 0xF4, 0xE1]


def tags(path):
    data = open(path, "rb").read()
    if data[:4] != b"PMAI":
        return {}
    pos = struct.unpack(">I", data[4:8])[0]
    out = {}
    while pos + 12 <= len(data):
        fourcc = data[pos:pos + 4].decode("latin-1")
        len_tag = struct.unpack(">I", data[pos + 8:pos + 12])[0]
        if len_tag < 12:
            break
        out.setdefault(fourcc, data[pos + 12:pos + len_tag])
        pos += len_tag
    return out


def beat_grid(body):
    n = struct.unpack(">I", body[8:12])[0]
    beats = []
    for i in range(n):
        number, tempo, ms = struct.unpack(">HHI", body[12 + 8 * i:20 + 8 * i])
        beats.append((number, tempo / 100.0, ms / 1000.0))
    return beats


def song_path(body):
    n = struct.unpack(">I", body[0:4])[0]
    return body[4:4 + n - 2].decode("utf-16-be") if n > 1 else ""


def phrases(body):
    entry_bytes, count = struct.unpack(">IH", body[0:6])
    raw = bytearray(body[6:])
    if struct.unpack(">H", raw[0:2])[0] > 20:  # masked
        for i in range(len(raw)):
            raw[i] ^= (MASK[i % len(MASK)] + count) & 0xFF
    mood, = struct.unpack(">H", raw[0:2])
    end_beat, = struct.unpack(">H", raw[8:10])
    out = []
    for i in range(count):
        e = raw[14 + i * entry_bytes:14 + (i + 1) * entry_bytes]
        index, beat, kind = struct.unpack(">HHH", e[0:6])
        out.append({"index": index, "beat": beat, "kind": KINDS.get(mood, KINDS[2]).get(kind, str(kind))})
    return mood, end_beat, out


def main(root, out_path):
    result = {}
    for folder, _, files in os.walk(root):
        if "ANLZ0000.EXT" not in files or "ANLZ0000.DAT" not in files:
            continue
        dat = tags(os.path.join(folder, "ANLZ0000.DAT"))
        ext = tags(os.path.join(folder, "ANLZ0000.EXT"))
        if "PSSI" not in ext or "PQTZ" not in dat:
            continue
        grid = beat_grid(dat["PQTZ"])
        mood, end_beat, ph = phrases(ext["PSSI"])
        for p in ph:  # beat numbers count the grid's beats from 1
            p["s"] = round(grid[min(len(grid), max(1, p["beat"])) - 1][2], 3)
        name = os.path.basename(song_path(dat.get("PPTH", b"\0\0\0\0")))
        bpm = sorted(b[1] for b in grid)[len(grid) // 2] if grid else 0
        result[name] = {"bpm": bpm, "mood": {1: "high", 2: "mid", 3: "low"}.get(mood, str(mood)),
                        "end_beat": end_beat, "phrases": ph}
    json.dump(result, open(out_path, "w"), indent=1, ensure_ascii=False)
    print(f"{len(result)} songs with phrases -> {out_path}")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
