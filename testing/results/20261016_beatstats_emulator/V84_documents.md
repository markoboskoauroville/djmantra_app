# V84 on the cloud emulator: documents in Documents/DJ Mantra (16.10.2026)

The published V84 (c96b141, x86_64) on the android-30 ATD emulator. Before the first start, old files in Music/djmantra_app:
BeatStats/charts.json, BeatStats/captures/*.jpg (3), BeatStats/pages/page_old.html, Download_Queue/TIDAL.txt,
Room_Corrections/Living room.json, and Playlists/keep.m3u (must stay).

First start, the app's log:

    Documents: 5 files /sdcard/Music/djmantra_app/BeatStats -> /sdcard/Documents/DJ Mantra/BeatStats
    Documents: 1 files /sdcard/Music/djmantra_app/Download_Queue -> /sdcard/Documents/DJ Mantra/Download_Queue
    Documents: 1 files /sdcard/Music/djmantra_app/Room_Corrections -> /sdcard/Documents/DJ Mantra/Room_Corrections

After it: all 7 files in Documents/DJ Mantra/... with their subfolders (captures/, pages/), the three old folders gone,
Music/djmantra_app holds only Playlists (kept). BeatStats READ CHART on a page with no songs then wrote
`Documents/DJ Mantra/BeatStats/pages/page_20261016_144724.html` (log `page saved`). **PASS.**

(The full app answers slowly on this software-only emulator: Android showed "DJ Mantra isn't responding" during its start;
"Wait" and it carried on.)
