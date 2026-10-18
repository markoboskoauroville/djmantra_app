#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/// DJ Mantra V92 (Marko 18.10.2026: "add pioneer export so i can export my playlists to usb stick
/// compatible with pioneer players"): a rekordbox "Device Library" on a USB stick, as Pioneer DJ /
/// AlphaTheta players read it (CDJ-2000NXS2, CDJ-3000, XDJ-RX, XDJ-XZ, XDJ-AZ ...):
///
///   /PIONEER/rekordbox/export.pdb        the DeviceSQL database: tracks, artists, albums, genres,
///                                        keys, colours, playlists, browse columns
///   /PIONEER/USBANLZ/P###/########/ANLZ0000.DAT / .EXT / .2EX
///                                        per track: the beat grid, the waveforms, the cue lists
///   /Contents/<artist>/<album>/<file>    the audio, copied
///
/// Ported from rbxport (https://github.com/chrisle/rbxport, Chris Le, GPL-2.0-or-later): the page
/// and row layouts of crates/rbl-pdb (build.rs, rows.rs, reference.rs), the ANLZ sections of
/// crates/rbl-anlz (write.rs, encode.rs) and the stick layout of crates/rbl-export, each measured by
/// its author against rekordbox's own exports and player firmware. Not written: exportLibrary.db
/// (OneLibrary, SQLCipher), artwork, hot cues, history; players read the Device Library without
/// them. Plain C++ (no Qt), so the format is tested alone (src/test/pioneerusb_test.cpp).
namespace djmantra::pioneer {

/// One column of the scrolling waveform (150 a second), every value 0..255
struct Column {
    uint8_t low = 0;
    uint8_t mid = 0;
    uint8_t high = 0;
    uint8_t peak = 0;
};

struct Track {
    std::string sourcePath; ///< the audio file to copy (UTF-8)
    std::string title, artist, album, genre, comment, key; ///< UTF-8; key as shown (e.g. "Am" or "8A")
    double bpm = 0;
    int durationSec = 0;
    int sampleRate = 44100;
    int bitrateKbps = 0;
    int year = 0;
    int rating = 0; ///< 0..5
    /// every beat (seconds) and its number in the bar, 1..4
    std::vector<std::pair<double, int>> beats;
    std::vector<Column> waveform; ///< 150 columns a second; empty: flat previews
};

struct Playlist {
    std::string name;
    std::vector<size_t> tracks; ///< indices into the export's tracks, in order
};

struct Export {
    std::vector<Track> tracks;
    std::vector<Playlist> playlists;
    std::string deviceName = "DJ Mantra";
    std::string date; ///< YYYY-MM-DD, the export's day
};

/// Where a track's analysis lives: rekordbox's hash of the audio path on the stick (CDJ-3000
/// firmware 3.20, as rbxport found it; matches rekordbox's own exports)
std::string analysisDirectory(const std::string& audioPathOnStick);

/// FAT32-safe names for the Contents folders and files
std::string fatName(const std::string& name, size_t maxBytes);

/// The database alone (for tests): tracks with their paths on the stick already set in
/// `stickPaths` (audio) and `analysisPaths` (the .DAT), one per track
std::vector<uint8_t> buildPdb(const Export& library,
        const std::vector<std::string>& stickPaths,
        const std::vector<std::string>& analysisPaths);

/// The three analysis files of one track; `stickPath` is its audio path on the stick
std::array<std::vector<uint8_t>, 3> buildAnalysis(const Track& track, const std::string& stickPath);

struct Result {
    bool ok = false;
    std::string error;
    int tracks = 0;
    int playlists = 0;
    int64_t bytes = 0;
};

/// Writes the whole Device Library under `root` (the stick's top folder, or any folder to copy onto
/// one). Existing files of the same name are replaced; other files on the stick are left alone.
/// `progress(done, total, title)` after each track; return false from it to stop.
Result write(const std::string& root,
        const Export& library,
        const std::function<bool(int, int, const std::string&)>& progress = {});

} // namespace djmantra::pioneer
