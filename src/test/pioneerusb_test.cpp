#include "library/djmantra/pioneerusb.h"

#include <gtest/gtest.h>
#include <kaitai/kaitaistream.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <unistd.h>

#include "rekordbox_anlz.h"
#include "rekordbox_pdb.h"

// DJ Mantra V92: the Pioneer USB export, read back with the rekordbox parsers Mixxx itself uses to
// import a stick (lib/rekordbox-metadata, Kaitai), so what we write is what a reader of rekordbox
// exports understands
namespace {

using djmantra::pioneer::Column;
using djmantra::pioneer::Export;
using djmantra::pioneer::Playlist;
using djmantra::pioneer::Track;

std::string text(rekordbox_pdb_t::device_sql_string_t* s) {
    if (auto* a = dynamic_cast<rekordbox_pdb_t::device_sql_short_ascii_t*>(s->body())) {
        return a->text();
    }
    if (auto* a = dynamic_cast<rekordbox_pdb_t::device_sql_long_ascii_t*>(s->body())) {
        return a->text();
    }
    if (auto* u = dynamic_cast<rekordbox_pdb_t::device_sql_long_utf16le_t*>(s->body())) {
        const std::string raw = u->text();
        std::string out;
        for (size_t i = 0; i + 1 < raw.size(); i += 2) { // test names are Latin-1
            const unsigned cp = static_cast<unsigned char>(raw[i]) | static_cast<unsigned char>(raw[i + 1]) << 8;
            if (cp < 0x80) {
                out.push_back(static_cast<char>(cp));
            } else {
                out.push_back(static_cast<char>(0xc0 | cp >> 6));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
            }
        }
        return out;
    }
    return "?";
}

Track track(const std::string& path, const std::string& title, const std::string& artist, double bpm, int seconds) {
    Track t;
    t.sourcePath = path;
    t.title = title;
    t.artist = artist;
    t.album = "Mantra";
    t.genre = "House";
    t.key = "8A";
    t.bpm = bpm;
    t.durationSec = seconds;
    for (int b = 0; b * 60.0 / bpm < seconds; ++b) {
        t.beats.emplace_back(0.1 + b * 60.0 / bpm, b % 4 + 1);
    }
    for (int c = 0; c < seconds * 150; ++c) {
        t.waveform.push_back(Column{static_cast<uint8_t>(c % 256), 120, 60, 200});
    }
    return t;
}

struct Read {
    std::map<uint32_t, std::string> titles, paths, analyses, artists, playlists, keys;
    std::map<uint32_t, uint32_t> trackArtist;
    std::map<uint32_t, std::map<uint32_t, uint32_t>> entries; // playlist -> position -> track
    int colors = 0;
};

Read readPdb(const std::string& file) {
    std::ifstream in(file, std::ios::binary);
    kaitai::kstream stream(&in);
    rekordbox_pdb_t db(&stream);
    Read out;
    for (const auto& table : *db.tables()) {
        const uint32_t last = table->last_page()->index();
        rekordbox_pdb_t::page_ref_t* ref = table->first_page();
        for (int guard = 0; guard < 1000; ++guard) {
            rekordbox_pdb_t::page_t* page = ref->body();
            if (page->is_data_page()) {
                for (const auto& group : *page->row_groups()) {
                    for (const auto& row : *group->rows()) {
                        if (!row->present()) {
                            continue;
                        }
                        switch (table->type()) {
                        case rekordbox_pdb_t::PAGE_TYPE_TRACKS: {
                            auto* t = static_cast<rekordbox_pdb_t::track_row_t*>(row->body());
                            out.titles[t->id()] = text(t->title());
                            out.paths[t->id()] = text(t->file_path());
                            out.analyses[t->id()] = text(t->analyze_path());
                            out.trackArtist[t->id()] = t->artist_id();
                        } break;
                        case rekordbox_pdb_t::PAGE_TYPE_ARTISTS: {
                            auto* a = static_cast<rekordbox_pdb_t::artist_row_t*>(row->body());
                            out.artists[a->id()] = text(a->name());
                        } break;
                        case rekordbox_pdb_t::PAGE_TYPE_KEYS: {
                            auto* k = static_cast<rekordbox_pdb_t::key_row_t*>(row->body());
                            out.keys[k->id()] = text(k->name());
                        } break;
                        case rekordbox_pdb_t::PAGE_TYPE_COLORS:
                            ++out.colors;
                            break;
                        case rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_TREE: {
                            auto* p = static_cast<rekordbox_pdb_t::playlist_tree_row_t*>(row->body());
                            out.playlists[p->id()] = text(p->name());
                        } break;
                        case rekordbox_pdb_t::PAGE_TYPE_PLAYLIST_ENTRIES: {
                            auto* e = static_cast<rekordbox_pdb_t::playlist_entry_row_t*>(row->body());
                            out.entries[e->playlist_id()][e->entry_index()] = e->track_id();
                        } break;
                        default:
                            break;
                        }
                    }
                }
            }
            if (ref->index() == last) {
                break;
            }
            ref = page->next_page();
        }
    }
    return out;
}

class PioneerUsbTest : public testing::Test {
  protected:
    void SetUp() override {
        m_dir = std::filesystem::temp_directory_path() / ("djmantra_pioneer_" + std::to_string(::getpid()));
        std::filesystem::remove_all(m_dir);
        std::filesystem::create_directories(m_dir / "src");
    }
    void TearDown() override {
        std::filesystem::remove_all(m_dir);
    }
    std::string audio(const std::string& name) {
        const auto path = m_dir / "src" / name;
        std::ofstream(path, std::ios::binary) << std::string(1000, 'x');
        return path.string();
    }
    std::filesystem::path m_dir;
};

TEST_F(PioneerUsbTest, AnalysisDirectoryMatchesRekordbox) {
    // folders rekordbox itself chose for these files (rbxport's crates/rbl-export tests)
    EXPECT_EQ(djmantra::pioneer::analysisDirectory("/Contents/Hosanna, Westend/Drum Death - Extended "
                                                   "Mix/20130688_drum_death_(extended_mix).mp3"),
            "/PIONEER/USBANLZ/P002/0002583E");
    EXPECT_EQ(djmantra::pioneer::analysisDirectory("/Contents/Meduza, Aya Anne, GENESI (ITA)/Freak "
                                                   "EP/20063130_freak_(feat._aya_anne)_(feat._aya_a.mp3"),
            "/PIONEER/USBANLZ/P018/00008F82");
}

TEST_F(PioneerUsbTest, FatNames) {
    EXPECT_EQ(djmantra::pioneer::fatName("AC/DC: Live?", 64), "AC_DC_ Live_");
    EXPECT_EQ(djmantra::pioneer::fatName("dots...", 64), "dots");
    EXPECT_EQ(djmantra::pioneer::fatName(std::string(200, 'a') + ".mp3", 20), std::string(16, 'a') + ".mp3");
}

TEST_F(PioneerUsbTest, StickReadsBack) {
    Export library;
    library.date = "2026-10-18";
    library.tracks.push_back(track(audio("one.mp3"), "Om Shanti", "Marko", 122, 30));
    library.tracks.push_back(track(audio("two.flac"), "Čarobna Noć", "Ðuro Šimić", 128, 20));
    library.tracks.push_back(track(audio("three.wav"), "Third", "Marko", 118.5, 10));
    library.playlists.push_back(Playlist{"Ecstatic Dance", {2, 0, 1}});
    library.playlists.push_back(Playlist{"Warm up", {1}});
    const std::string root = (m_dir / "stick").string();
    const auto result = djmantra::pioneer::write(root, library);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.tracks, 3);

    const Read db = readPdb(root + "/PIONEER/rekordbox/export.pdb");
    ASSERT_EQ(db.titles.size(), 3u);
    EXPECT_EQ(db.titles.at(1), "Om Shanti");
    EXPECT_EQ(db.titles.at(2), "Čarobna Noć");
    EXPECT_EQ(db.paths.at(1), "/Contents/Marko/Mantra/one.mp3");
    EXPECT_EQ(db.artists.at(db.trackArtist.at(2)), "Ðuro Šimić");
    EXPECT_EQ(db.trackArtist.at(1), db.trackArtist.at(3));
    EXPECT_EQ(db.keys.at(1), "8A");
    EXPECT_EQ(db.colors, 8);
    EXPECT_EQ(db.playlists.at(1), "Ecstatic Dance");
    EXPECT_EQ(db.playlists.at(2), "Warm up");
    ASSERT_EQ(db.entries.at(1).size(), 3u);
    EXPECT_EQ(db.entries.at(1).at(1), 3u);
    EXPECT_EQ(db.entries.at(1).at(2), 1u);
    EXPECT_EQ(db.entries.at(1).at(3), 2u);
    EXPECT_EQ(db.entries.at(2).at(1), 2u);

    for (uint32_t id = 1; id <= 3; ++id) {
        EXPECT_TRUE(std::filesystem::exists(root + db.paths.at(id))) << db.paths.at(id);
        const std::string dat = root + db.analyses.at(id);
        ASSERT_TRUE(std::filesystem::exists(dat)) << dat;
        std::string ext = dat;
        ext.replace(ext.size() - 3, 3, "EXT");
        std::string twoEx = dat;
        twoEx.replace(twoEx.size() - 3, 3, "2EX");
        for (const std::string& file : {dat, ext, twoEx}) {
            std::ifstream in(file, std::ios::binary);
            kaitai::kstream stream(&in);
            rekordbox_anlz_t anlz(&stream);
            EXPECT_GE(anlz.sections()->size(), 3u) << file;
            if (file == dat) {
                bool grid = false;
                for (const auto& section : *anlz.sections()) {
                    if (section->fourcc() == rekordbox_anlz_t::SECTION_TAGS_BEAT_GRID) {
                        auto* beats = static_cast<rekordbox_anlz_t::beat_grid_tag_t*>(section->body());
                        ASSERT_GT(beats->num_beats(), 10u);
                        const auto& first = beats->beats()->at(0);
                        EXPECT_EQ(first->beat_number(), 1);
                        EXPECT_EQ(first->time(), 100u);
                        EXPECT_EQ(beats->beats()->at(1)->beat_number(), 2);
                        grid = true;
                    }
                }
                EXPECT_TRUE(grid);
            }
        }
    }
}

// A library larger than one page: every track still reads back, the pages chain
TEST_F(PioneerUsbTest, ManyPages) {
    Export library;
    library.date = "2026-10-18";
    Playlist all{"All", {}};
    for (int i = 0; i < 300; ++i) {
        Track t;
        t.sourcePath = "/nonexistent";
        t.title = "Track number " + std::to_string(i);
        t.artist = "Artist " + std::to_string(i % 40);
        t.album = "Album " + std::to_string(i % 25);
        t.bpm = 120;
        library.tracks.push_back(t);
        all.tracks.push_back(i);
    }
    library.playlists.push_back(all);
    std::vector<std::string> paths, analyses;
    for (int i = 0; i < 300; ++i) {
        paths.push_back("/Contents/a/b/" + std::to_string(i) + ".mp3");
        analyses.push_back(djmantra::pioneer::analysisDirectory(paths.back()) + "/ANLZ0000.DAT");
    }
    const auto pdb = djmantra::pioneer::buildPdb(library, paths, analyses);
    const auto file = m_dir / "export.pdb";
    std::ofstream(file, std::ios::binary).write(reinterpret_cast<const char*>(pdb.data()), pdb.size());
    const Read db = readPdb(file.string());
    EXPECT_EQ(db.titles.size(), 300u);
    EXPECT_EQ(db.titles.at(300), "Track number 299");
    EXPECT_EQ(db.artists.size(), 40u);
    EXPECT_EQ(db.entries.at(1).size(), 300u);
    EXPECT_EQ(db.entries.at(1).at(300), 300u);
}

} // namespace
