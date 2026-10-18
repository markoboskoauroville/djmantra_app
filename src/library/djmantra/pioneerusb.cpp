#include "library/djmantra/pioneerusb.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <system_error>

// Ported from rbxport (https://github.com/chrisle/rbxport), Copyright Chris Le, GPL-2.0-or-later:
// crates/rbl-pdb/src/{build,rows,reference}.rs, crates/rbl-anlz/src/{write,encode}.rs and the layout
// of crates/rbl-export. The "[OBS ...]" notes there say which rekordbox file each byte was read from.
namespace djmantra::pioneer {

namespace fs = std::filesystem;

namespace {

// UTF-8 text <-> paths (C++20 made u8path / u8string char8_t)
fs::path toPath(const std::string& utf8) {
    return fs::path(std::u8string(utf8.begin(), utf8.end()));
}
std::string fromPath(const fs::path& path) {
    const std::u8string text = path.u8string();
    return std::string(text.begin(), text.end());
}

// ------------------------------------------------------------------ text

std::u16string utf16(const std::string& utf8) {
    std::u16string out;
    for (size_t i = 0; i < utf8.size();) {
        const unsigned char c = utf8[i];
        uint32_t cp = 0;
        int extra = 0;
        if (c < 0x80) {
            cp = c;
        } else if ((c >> 5) == 6) {
            cp = c & 0x1f;
            extra = 1;
        } else if ((c >> 4) == 14) {
            cp = c & 0x0f;
            extra = 2;
        } else if ((c >> 3) == 30) {
            cp = c & 0x07;
            extra = 3;
        } else {
            cp = 0xfffd;
        }
        ++i;
        for (int k = 0; k < extra && i < utf8.size(); ++k, ++i) {
            cp = (cp << 6) | (static_cast<unsigned char>(utf8[i]) & 0x3f);
        }
        if (cp >= 0x10000) {
            cp -= 0x10000;
            out.push_back(static_cast<char16_t>(0xd800 + (cp >> 10)));
            out.push_back(static_cast<char16_t>(0xdc00 + (cp & 0x3ff)));
        } else {
            out.push_back(static_cast<char16_t>(cp));
        }
    }
    return out;
}

bool isAscii(const std::string& text) {
    return std::all_of(text.begin(), text.end(), [](char c) { return static_cast<unsigned char>(c) < 0x80; });
}

void putU16(std::vector<uint8_t>& out, size_t at, uint16_t v) {
    if (at + 2 <= out.size()) {
        out[at] = v & 0xff;
        out[at + 1] = v >> 8;
    }
}
void putU32(std::vector<uint8_t>& out, size_t at, uint32_t v) {
    if (at + 4 <= out.size()) {
        for (int i = 0; i < 4; ++i) {
            out[at + i] = (v >> (8 * i)) & 0xff;
        }
    }
}
void appendU32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        out.push_back((v >> (8 * i)) & 0xff);
    }
}
void appendBe16(std::vector<uint8_t>& out, uint16_t v) {
    out.push_back(v >> 8);
    out.push_back(v & 0xff);
}
void appendBe32(std::vector<uint8_t>& out, uint32_t v) {
    for (int i = 3; i >= 0; --i) {
        out.push_back((v >> (8 * i)) & 0xff);
    }
}

// DeviceSQL strings: the short ASCII form (length byte "incremented, doubled, incremented"),
// the long ASCII form 0x40 and the UTF-16LE form 0x90, no terminator (a CDJ-2000NXS froze on one)
std::vector<uint8_t> shortAscii(const std::string& text) {
    std::vector<uint8_t> out;
    out.push_back(static_cast<uint8_t>(std::min<size_t>((text.size() + 1) * 2 + 1, 0xff)));
    out.insert(out.end(), text.begin(), text.end());
    return out;
}

std::vector<uint8_t> deviceSqlString(const std::string& text) {
    if (isAscii(text) && text.size() < 0x7e) {
        return shortAscii(text);
    }
    std::vector<uint8_t> out;
    if (isAscii(text)) {
        out.push_back(0x40);
        const uint16_t len = static_cast<uint16_t>(std::min<size_t>(4 + text.size(), 0xffff));
        out.push_back(len & 0xff);
        out.push_back(len >> 8);
        out.push_back(0);
        out.insert(out.end(), text.begin(), text.end());
        return out;
    }
    const std::u16string units = utf16(text);
    const uint16_t len = static_cast<uint16_t>(std::min<size_t>(4 + units.size() * 2, 0xffff));
    out.push_back(0x90);
    out.push_back(len & 0xff);
    out.push_back(len >> 8);
    out.push_back(0);
    for (char16_t unit : units) {
        out.push_back(unit & 0xff);
        out.push_back(unit >> 8);
    }
    return out;
}

// A long string starts on a four-byte boundary of the row, as rekordbox lays them out
size_t pushDeviceSqlString(std::vector<uint8_t>& row, const std::string& text) {
    const auto encoded = deviceSqlString(text);
    if (encoded.front() == 0x40 || encoded.front() == 0x90) {
        while (row.size() % 4) {
            row.push_back(0);
        }
    }
    const size_t at = row.size();
    row.insert(row.end(), encoded.begin(), encoded.end());
    return at;
}

// ------------------------------------------------------------------ pages (rbl-pdb build.rs)

constexpr size_t kPageSize = 4096;
constexpr size_t kPageHeader = 0x28;
constexpr size_t kRowGroup = 0x24;

size_t groupsFor(size_t rows) {
    return (rows + 15) / 16;
}

// Rows grow forward from the header, their offsets backwards from the page's end, sixteen to a
// group, each group with its presence bits
class PageBuilder {
  public:
    explicit PageBuilder(uint32_t pageType)
            : m_pageType(pageType) {
    }
    bool fits(size_t len) const {
        return freeSpace() >= len + 8;
    }
    void push(const std::vector<uint8_t>& row) {
        while (m_heap.size() % 4) {
            m_heap.push_back(0);
        }
        m_offsets.push_back(static_cast<uint16_t>(m_heap.size()));
        m_heap.insert(m_heap.end(), row.begin(), row.end());
    }
    size_t rows() const {
        return m_offsets.size();
    }
    size_t freeSpace() const {
        const size_t index = std::max<size_t>(1, groupsFor(m_offsets.size())) * kRowGroup;
        const size_t used = kPageHeader + m_heap.size() + index;
        return used >= kPageSize ? 0 : kPageSize - used;
    }
    std::vector<uint8_t> finish() const {
        std::vector<uint8_t> page(kPageSize, 0);
        const size_t n = m_offsets.size();
        // a page of more than 255 rows names only its last row, as rekordbox heads one (rbxport #349)
        const bool lastRowMarked = n > 0xff;
        putU32(page, 0x08, m_pageType);
        const uint32_t rows = static_cast<uint32_t>(n);
        const uint32_t packed = (rows & 0x1fff) | (rows & 0x7ff) << 13;
        page[0x18] = packed & 0xff;
        page[0x19] = (packed >> 8) & 0xff;
        page[0x1a] = (packed >> 16) & 0xff;
        page[0x1b] = 0x24; // an ordinary data page
        const size_t used = (m_heap.size() + 3) / 4 * 4;
        const size_t reserved = kPageHeader + used + 2 * n + 4 * groupsFor(n);
        putU16(page, 0x1c, static_cast<uint16_t>(reserved >= kPageSize ? 0 : kPageSize - reserved));
        putU16(page, 0x1e, static_cast<uint16_t>(used));
        if (lastRowMarked) {
            putU16(page, 0x20, 1);
            putU16(page, 0x22, static_cast<uint16_t>(n - 1));
        } else {
            putU16(page, 0x20, static_cast<uint16_t>(n));
        }
        std::copy(m_heap.begin(), m_heap.end(), page.begin() + kPageHeader);
        const size_t groups = groupsFor(n);
        for (size_t group = 0; group < groups; ++group) {
            const size_t base = kPageSize - group * kRowGroup;
            const size_t inGroup = group + 1 < groups ? 16 : n - group * 16;
            uint16_t present = 0;
            for (size_t row = 0; row < inGroup; ++row) {
                present |= static_cast<uint16_t>(1u << row);
                putU16(page, base - (6 + 2 * row), m_offsets[group * 16 + row]);
            }
            const uint16_t second = !lastRowMarked ? present
                    : group + 1 == groups          ? static_cast<uint16_t>(1u << (inGroup - 1))
                                                   : 0;
            putU16(page, base - 4, present);
            putU16(page, base - 2, second);
        }
        return page;
    }

  private:
    uint32_t m_pageType;
    std::vector<uint8_t> m_heap;
    std::vector<uint16_t> m_offsets;
};

// The page that heads every table: flags 0x64, 0x1fff in both row words, 0x03ec, then a body
// naming itself, its first data page (or 0x03ffffff) and 1004 copies of ff 1f f8 ff
std::vector<uint8_t> indexPage(uint32_t pageIndex, uint32_t pageType, uint32_t nextPage, uint32_t firstData) {
    std::vector<uint8_t> page(kPageSize, 0);
    putU32(page, 0x04, pageIndex);
    putU32(page, 0x08, pageType);
    putU32(page, 0x0c, nextPage);
    putU32(page, 0x10, 1);
    page[0x1b] = 0x64;
    putU16(page, 0x20, 0x1fff);
    putU16(page, 0x22, 0x1fff);
    putU16(page, 0x24, 0x03ec);
    putU32(page, kPageHeader, pageIndex);
    putU32(page, kPageHeader + 4, firstData);
    putU32(page, kPageHeader + 8, 0x03ffffff);
    size_t at = kPageHeader + 18;
    for (int i = 0; i < 1004 && at + 4 <= page.size(); ++i, at += 4) {
        page[at] = 0xff;
        page[at + 1] = 0x1f;
        page[at + 2] = 0xf8;
        page[at + 3] = 0xff;
    }
    if (at + 2 <= page.size()) {
        page[at] = 0xff;
        page[at + 1] = 0x1f;
    }
    return page;
}

struct Table {
    uint32_t type;
    std::vector<std::vector<uint8_t>> pages;
    bool hasRows;
};

// Rows that open with a subtype word carry their slot on the page times 32 (tracks, artists,
// albums, property); NXS-era players read it as the row's cursor position
Table makeTable(uint32_t type, const std::vector<std::vector<uint8_t>>& rows, bool numbered = false) {
    Table table{type, {}, !rows.empty()};
    PageBuilder page(type);
    for (auto row : rows) {
        if (!page.fits(row.size())) {
            table.pages.push_back(page.finish());
            page = PageBuilder(type);
        }
        if (numbered) {
            putU16(row, 2, static_cast<uint16_t>(page.rows() * 32));
        }
        page.push(row);
    }
    table.pages.push_back(page.finish());
    return table;
}

// The file as rekordbox lays one out: the header page listing the tables, then per table its
// index page and first data page (or one zeroed "empty candidate" page for a table without rows);
// then, in write order (property first), each table's further pages and its empty candidate. The
// header's sequence is one past the newest page's, so a player that opens the file to write its
// history does not roll our pages back.
std::vector<uint8_t> renderFile(std::vector<Table> tables) {
    std::vector<std::vector<uint8_t>> pages;
    uint32_t next = 1;
    uint32_t sequence = 1;
    struct Entry {
        uint32_t type, first, last, candidate;
    };
    std::vector<Entry> entries;
    std::vector<std::pair<size_t, std::vector<std::vector<uint8_t>>>> held;
    for (size_t t = 0; t < tables.size(); ++t) {
        Table& table = tables[t];
        const uint32_t index = next++;
        if (table.hasRows) {
            const uint32_t firstData = next++;
            pages.push_back(indexPage(index, table.type, firstData, firstData));
            std::vector<uint8_t> first = table.pages.front();
            putU32(first, 0x04, firstData);
            pages.push_back(first);
            held.emplace_back(t, std::vector<std::vector<uint8_t>>(table.pages.begin() + 1, table.pages.end()));
            entries.push_back({table.type, index, firstData, 0xffffffffu});
        } else {
            const uint32_t candidate = next++;
            pages.push_back(indexPage(index, table.type, candidate, 0x03ffffff));
            pages.emplace_back(kPageSize, 0);
            entries.push_back({table.type, index, index, candidate});
        }
    }
    std::stable_sort(held.begin(), held.end(), [&](const auto& a, const auto& b) {
        const auto rank = [&](size_t t) { return entries[t].type == 19 ? 0u : 1u + entries[t].type; };
        return rank(a.first) < rank(b.first);
    });
    for (auto& [t, rest] : held) {
        const uint32_t firstData = entries[t].last;
        // pages[] position of a page is its index - 1 (the header is page 0)
        size_t lastAt = firstData - 1;
        ++sequence;
        putU32(pages[lastAt], 0x10, sequence);
        uint32_t lastIndex = firstData;
        for (auto& page : rest) {
            const uint32_t index = next++;
            putU32(page, 0x04, index);
            ++sequence;
            putU32(page, 0x10, sequence);
            putU32(pages[lastAt], 0x0c, index);
            pages.push_back(page);
            lastAt = pages.size() - 1;
            lastIndex = index;
        }
        const uint32_t candidate = next++;
        pages.emplace_back(kPageSize, 0);
        putU32(pages[lastAt], 0x0c, candidate);
        entries[t].last = lastIndex;
        entries[t].candidate = candidate;
    }
    // zeroed pages at the end are not written: rekordbox's files stop at the last page with data
    while (!pages.empty() && std::all_of(pages.back().begin(), pages.back().end(), [](uint8_t b) { return b == 0; })) {
        pages.pop_back();
    }
    std::vector<uint8_t> out(kPageSize, 0);
    putU32(out, 0x04, kPageSize);
    putU32(out, 0x08, static_cast<uint32_t>(tables.size()));
    putU32(out, 0x0c, next);
    putU32(out, 0x10, 1);
    putU32(out, 0x14, sequence + 1);
    for (size_t i = 0; i < entries.size(); ++i) {
        const size_t at = 28 + i * 16;
        putU32(out, at, entries[i].type);
        putU32(out, at + 4, entries[i].candidate);
        putU32(out, at + 8, entries[i].first);
        putU32(out, at + 12, entries[i].last);
    }
    for (const auto& page : pages) {
        out.insert(out.end(), page.begin(), page.end());
    }
    return out;
}

// ------------------------------------------------------------------ rows (rbl-pdb rows.rs)

uint16_t audioFileType(const std::string& name) {
    std::string ext = fs::path(name).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ext == ".mp3") {
        return 1;
    }
    if (ext == ".m4a" || ext == ".mp4" || ext == ".aac") {
        return 4;
    }
    if (ext == ".flac") {
        return 5;
    }
    if (ext == ".wav") {
        return 11;
    }
    if (ext == ".aif" || ext == ".aiff") {
        return 12;
    }
    return 0;
}

struct TrackRow {
    uint32_t id = 0, artist = 0, album = 0, genre = 0, key = 0;
    const Track* track = nullptr;
    std::string filename, path, analyzePath, dateAdded;
    uint32_t fileSize = 0;
};

// A track row: 0x5e fixed bytes, then 21 string offsets (relative to the row), then the strings.
// Subtype 0x24 and the trailer words are what a CDJ-3000 needs to list the tracks at all.
std::vector<uint8_t> trackRow(const TrackRow& in) {
    constexpr size_t kFixed = 0x5e;
    constexpr size_t kStrings = 21;
    std::vector<std::string> strings(kStrings);
    strings[10] = in.dateAdded;
    strings[14] = in.analyzePath;
    strings[16] = in.track->comment;
    strings[17] = in.track->title;
    strings[19] = in.filename;
    strings[20] = in.path;
    std::vector<uint8_t> row(kFixed + kStrings * 2, 0);
    putU16(row, 0x00, 0x24);
    putU32(row, 0x08, static_cast<uint32_t>(in.track->sampleRate));
    putU32(row, 0x10, in.fileSize);
    putU32(row, 0x20, in.key);
    putU32(row, 0x30, static_cast<uint32_t>(std::max(0, in.track->bitrateKbps)));
    putU32(row, 0x38, static_cast<uint32_t>(std::lround(in.track->bpm * 100)));
    putU32(row, 0x3c, in.genre);
    putU32(row, 0x40, in.album);
    putU32(row, 0x44, in.artist);
    putU32(row, 0x48, in.id);
    putU16(row, 0x50, static_cast<uint16_t>(std::clamp(in.track->year, 0, 0xffff)));
    putU16(row, 0x52, 16);
    putU16(row, 0x54, static_cast<uint16_t>(std::clamp(in.track->durationSec, 0, 0xffff)));
    putU16(row, 0x56, 0x29);
    row[0x59] = static_cast<uint8_t>(std::clamp(in.track->rating, 0, 5));
    putU16(row, 0x5a, audioFileType(in.filename));
    putU16(row, 0x5c, 3);
    std::array<uint16_t, kStrings> offsets{};
    for (size_t slot = 0; slot < kStrings; ++slot) {
        offsets[slot] = static_cast<uint16_t>(pushDeviceSqlString(row, strings[slot]));
    }
    for (size_t slot = 0; slot < kStrings; ++slot) {
        putU16(row, kFixed + slot * 2, offsets[slot]);
    }
    return row;
}

std::vector<uint8_t> namedRow(uint32_t id, const std::string& name) {
    std::vector<uint8_t> row;
    appendU32(row, id);
    const auto text = deviceSqlString(name);
    row.insert(row.end(), text.begin(), text.end());
    return row;
}

std::vector<uint8_t> keyRow(uint32_t id, const std::string& name) {
    std::vector<uint8_t> row;
    appendU32(row, id);
    appendU32(row, id);
    const auto text = deviceSqlString(name);
    row.insert(row.end(), text.begin(), text.end());
    return row;
}

std::vector<uint8_t> colorRow(uint16_t id, const std::string& name) {
    std::vector<uint8_t> row(8, 0);
    row[4] = static_cast<uint8_t>(id);
    putU16(row, 5, id);
    const auto text = deviceSqlString(name);
    row.insert(row.end(), text.begin(), text.end());
    return row;
}

std::vector<uint8_t> artistRow(uint32_t id, const std::string& name) {
    std::vector<uint8_t> row(10, 0);
    putU16(row, 0, 0x60);
    putU32(row, 4, id);
    row[8] = 0x03;
    row[9] = static_cast<uint8_t>(pushDeviceSqlString(row, name));
    return row;
}

std::vector<uint8_t> albumRow(uint32_t id, uint32_t artistId, const std::string& name) {
    std::vector<uint8_t> row(0x16, 0);
    putU16(row, 0, 0x80);
    putU32(row, 0x08, artistId);
    putU32(row, 0x0c, id);
    row[0x14] = 0x03;
    row[0x15] = static_cast<uint8_t>(pushDeviceSqlString(row, name));
    return row;
}

std::vector<uint8_t> playlistRow(uint32_t id, uint32_t parent, uint32_t sortOrder, const std::string& name) {
    std::vector<uint8_t> row;
    appendU32(row, parent);
    appendU32(row, 0);
    appendU32(row, sortOrder);
    appendU32(row, id);
    appendU32(row, 0); // a playlist, not a folder
    const auto text = deviceSqlString(name);
    row.insert(row.end(), text.begin(), text.end());
    return row;
}

std::vector<uint8_t> playlistEntryRow(uint32_t position, uint32_t trackId, uint32_t playlistId) {
    std::vector<uint8_t> row;
    appendU32(row, position);
    appendU32(row, trackId);
    appendU32(row, playlistId);
    return row;
}

// The property row: the track count, the date, dbVersion "1000" and the device name
std::vector<uint8_t> propertyRow(uint32_t contents, const std::string& date, const std::string& device) {
    std::vector<uint8_t> row{0x80, 0x02, 0x00, 0x00};
    appendU32(row, contents);
    row.insert(row.end(), {0x00, 0x00, 0x00, 0x00});
    const auto day = shortAscii(date.size() == 10 ? date : std::string("2026-01-01"));
    row.insert(row.end(), day.begin(), day.end());
    row.insert(row.end(), {0x19, 0x1e});
    const auto version = shortAscii("1000");
    row.insert(row.end(), version.begin(), version.end());
    const auto name = deviceSqlString(device);
    row.insert(row.end(), name.begin(), name.end());
    row.insert(row.end(), 8, 0);
    while (row.size() % 4) {
        row.push_back(0);
    }
    return row;
}

// Rows rekordbox writes into every export whatever it holds (rbl-pdb reference.rs, read from
// rekordbox 7.2.11 exports): the browse columns (type 16), as UTF-16 names in 0xfffa / 0xfffb
std::vector<std::vector<uint8_t>> columnRows() {
    const std::pair<uint16_t, const char*> columns[] = {{0x80, "GENRE"}, {0x81, "ARTIST"}, {0x82, "ALBUM"},
            {0x83, "TRACK"}, {0x85, "BPM"}, {0x86, "RATING"}, {0x87, "YEAR"}, {0x88, "REMIXER"},
            {0x89, "LABEL"}, {0x8a, "ORIGINAL ARTIST"}, {0x8b, "KEY"}, {0x8d, "CUE"}, {0x8e, "COLOR"},
            {0x92, "TIME"}, {0x93, "BITRATE"}, {0x94, "FILE NAME"}, {0x84, "PLAYLIST"},
            {0x98, "HOT CUE BANK"}, {0x95, "HISTORY"}, {0x91, "SEARCH"}, {0x96, "COMMENTS"},
            {0x8c, "DATE ADDED"}, {0x97, "DJ PLAY COUNT"}, {0x90, "FOLDER"}, {0xa1, "DEFAULT"},
            {0xa2, "ALPHABET"}, {0xaa, "MATCHING"}};
    std::vector<std::vector<uint8_t>> rows;
    uint16_t id = 1;
    for (const auto& [code, name] : columns) {
        std::vector<uint8_t> row;
        row.push_back(id & 0xff);
        row.push_back(id >> 8);
        row.push_back(code & 0xff);
        row.push_back(code >> 8);
        const size_t units = std::char_traits<char>::length(name) + 2;
        row.push_back(0x90);
        row.push_back(static_cast<uint8_t>(4 + units * 2));
        row.push_back(0);
        row.push_back(0);
        row.insert(row.end(), {0xfa, 0xff});
        for (const char* c = name; *c; ++c) {
            row.push_back(static_cast<uint8_t>(*c));
            row.push_back(0);
        }
        row.insert(row.end(), {0xfb, 0xff});
        while (row.size() % 4) {
            row.push_back(0);
        }
        rows.push_back(row);
        ++id;
    }
    return rows;
}

std::vector<std::vector<uint8_t>> constantRows(const std::vector<std::array<uint8_t, 8>>& data) {
    std::vector<std::vector<uint8_t>> rows;
    for (const auto& r : data) {
        rows.emplace_back(r.begin(), r.end());
    }
    return rows;
}

const std::vector<std::array<uint8_t, 8>> kHistoryPlaylists = {{0x01, 0x00, 0x01, 0x00, 0x63, 0x01, 0x00, 0x00},
        {0x05, 0x00, 0x06, 0x00, 0x05, 0x01, 0x00, 0x00}, {0x06, 0x00, 0x07, 0x00, 0x63, 0x01, 0x00, 0x00},
        {0x07, 0x00, 0x08, 0x00, 0x63, 0x01, 0x00, 0x00}, {0x08, 0x00, 0x09, 0x00, 0x63, 0x01, 0x00, 0x00},
        {0x09, 0x00, 0x0a, 0x00, 0x63, 0x01, 0x00, 0x00}, {0x0a, 0x00, 0x0b, 0x00, 0x63, 0x01, 0x00, 0x00},
        {0x0d, 0x00, 0x0f, 0x00, 0x63, 0x01, 0x00, 0x00}, {0x0e, 0x00, 0x13, 0x00, 0x04, 0x01, 0x00, 0x00},
        {0x0f, 0x00, 0x14, 0x00, 0x06, 0x01, 0x00, 0x00}, {0x10, 0x00, 0x15, 0x00, 0x63, 0x01, 0x00, 0x00},
        {0x12, 0x00, 0x17, 0x00, 0x63, 0x01, 0x00, 0x00}, {0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x01, 0x00},
        {0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x02, 0x00}, {0x04, 0x00, 0x04, 0x00, 0x01, 0x00, 0x03, 0x00},
        {0x0b, 0x00, 0x0c, 0x00, 0x63, 0x00, 0x04, 0x00}, {0x11, 0x00, 0x05, 0x00, 0x63, 0x00, 0x05, 0x00},
        {0x13, 0x00, 0x16, 0x00, 0x63, 0x00, 0x06, 0x00}, {0x14, 0x00, 0x12, 0x00, 0x63, 0x00, 0x07, 0x00},
        {0x1b, 0x00, 0x1a, 0x00, 0x63, 0x02, 0x08, 0x00}, {0x18, 0x00, 0x11, 0x00, 0x63, 0x00, 0x09, 0x00},
        {0x16, 0x00, 0x1b, 0x00, 0x63, 0x00, 0x0a, 0x00}};
const std::vector<std::array<uint8_t, 8>> kHistoryEntries = {{0x01, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00},
        {0x15, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x00}, {0x0e, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00},
        {0x08, 0x00, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00}, {0x09, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00},
        {0x0a, 0x00, 0x0b, 0x00, 0x01, 0x00, 0x00, 0x00}, {0x0f, 0x00, 0x0d, 0x00, 0x01, 0x00, 0x00, 0x00},
        {0x0d, 0x00, 0x0f, 0x00, 0x01, 0x00, 0x00, 0x00}, {0x17, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00},
        {0x16, 0x00, 0x11, 0x00, 0x01, 0x00, 0x00, 0x00}, {0x19, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00},
        {0x1a, 0x00, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00}, {0x02, 0x00, 0x02, 0x00, 0x00, 0x03, 0x00, 0x00},
        {0x03, 0x00, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00}, {0x05, 0x00, 0x04, 0x00, 0x00, 0x05, 0x00, 0x00},
        {0x06, 0x00, 0x05, 0x00, 0x00, 0x06, 0x00, 0x00}, {0x0b, 0x00, 0x0c, 0x00, 0x00, 0x07, 0x00, 0x00}};

// Names into ids from 1, in first-seen order; "" has no row (id 0)
struct Intern {
    std::map<std::string, uint32_t> ids;
    std::vector<std::string> order;
    uint32_t id(const std::string& name) {
        if (name.empty()) {
            return 0;
        }
        const auto it = ids.find(name);
        if (it != ids.end()) {
            return it->second;
        }
        order.push_back(name);
        return ids[name] = static_cast<uint32_t>(order.size());
    }
};

// ------------------------------------------------------------------ analysis (rbl-anlz)

struct Section {
    const char* tag;
    std::vector<uint8_t> header;
    std::vector<uint8_t> payload;
};

std::vector<uint8_t> render(const std::vector<Section>& sections) {
    const std::array<uint8_t, 16> extra{0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
    size_t body = 0;
    for (const auto& s : sections) {
        body += 12 + s.header.size() + s.payload.size();
    }
    std::vector<uint8_t> out{'P', 'M', 'A', 'I'};
    appendBe32(out, 12 + extra.size());
    appendBe32(out, static_cast<uint32_t>(12 + extra.size() + body));
    out.insert(out.end(), extra.begin(), extra.end());
    for (const auto& s : sections) {
        out.insert(out.end(), s.tag, s.tag + 4);
        appendBe32(out, static_cast<uint32_t>(12 + s.header.size()));
        appendBe32(out, static_cast<uint32_t>(12 + s.header.size() + s.payload.size()));
        out.insert(out.end(), s.header.begin(), s.header.end());
        out.insert(out.end(), s.payload.begin(), s.payload.end());
    }
    return out;
}

// PPTH: the audio path on the stick, UTF-16BE with a terminator
Section pathSection(const std::string& path) {
    Section s{"PPTH", {}, {}};
    for (char16_t unit : utf16(path)) {
        appendBe16(s.payload, unit);
    }
    s.payload.insert(s.payload.end(), {0, 0});
    appendBe32(s.header, static_cast<uint32_t>(s.payload.size()));
    return s;
}

std::vector<Column> resample(const std::vector<Column>& columns, size_t n) {
    std::vector<Column> out(n);
    if (columns.empty()) {
        return out;
    }
    for (size_t bucket = 0; bucket < n; ++bucket) {
        const size_t start = bucket * columns.size() / n;
        const size_t end = std::min(columns.size(), std::max(start + 1, (bucket + 1) * columns.size() / n));
        Column loudest;
        for (size_t i = start; i < end; ++i) {
            loudest.low = std::max(loudest.low, columns[i].low);
            loudest.mid = std::max(loudest.mid, columns[i].mid);
            loudest.high = std::max(loudest.high, columns[i].high);
            loudest.peak = std::max(loudest.peak, columns[i].peak);
        }
        out[bucket] = loudest;
    }
    return out;
}

// rbl-anlz encode.rs: the heights and colours rbxport measured against rekordbox 7.2.11's own
constexpr uint32_t kBandFullScale = 44;
uint8_t band(uint8_t v) {
    return static_cast<uint8_t>(v * kBandFullScale / 255);
}
uint32_t bandHeight(Column c, uint32_t ceiling) {
    const uint32_t loudest = std::max({band(c.low), band(c.mid), band(c.high)});
    return std::min(ceiling, loudest * ceiling / kBandFullScale);
}
uint32_t detailHeight(Column c) {
    constexpr uint32_t kFull = 255 * 255;
    return std::min<uint32_t>(31, (31 * c.peak * c.peak + kFull / 2) / kFull);
}
uint8_t monoByte(Column c, uint32_t height) {
    const uint32_t total = c.low + c.mid + c.high;
    const uint32_t whiteness = total ? (c.high * 7 + total / 2) / total : 0;
    return static_cast<uint8_t>((std::min<uint32_t>(whiteness, 7) << 5) | std::min<uint32_t>(height, 31));
}

Section previewSection(const char* tag, const std::vector<uint8_t>& data) {
    Section s{tag, {}, data};
    appendBe32(s.header, static_cast<uint32_t>(data.size()));
    appendBe32(s.header, 0x00010000);
    return s;
}

Section scrollSection(const char* tag, uint32_t stride, const std::vector<uint8_t>& data) {
    Section s{tag, {}, data};
    appendBe32(s.header, stride);
    appendBe32(s.header, static_cast<uint32_t>(stride ? data.size() / stride : 0));
    const std::string t(tag);
    if (t != "PWV6") { // PWV6 has a 20-byte header, not the 24-byte scroll header
        appendBe32(s.header, t == "PWV5" ? 0x00960305u : t == "PWV4" ? 0u : 0x00960000u);
    }
    return s;
}

void cueLists(std::vector<Section>& sections, bool extended) {
    for (uint32_t kind : {1u, 0u}) {
        Section s{"PCOB", {}, {}};
        appendBe32(s.header, kind);
        appendBe16(s.header, 0);
        appendBe16(s.header, 0);
        appendBe32(s.header, 0xffffffffu);
        sections.push_back(s);
    }
    if (extended) {
        for (uint32_t kind : {1u, 0u}) {
            Section s{"PCO2", {}, {}};
            appendBe32(s.header, kind);
            appendBe16(s.header, 0);
            appendBe16(s.header, 0);
            sections.push_back(s);
        }
    }
}

// FAT32 rules for one name
std::string fatSafe(const std::string& name) {
    std::string out;
    for (unsigned char c : name) {
        out.push_back(c < 0x20 || std::string("/\\:*?\"<>|").find(static_cast<char>(c)) != std::string::npos
                        ? '_'
                        : static_cast<char>(c));
    }
    while (!out.empty() && (out.back() == '.' || out.back() == ' ')) {
        out.pop_back();
    }
    return out.empty() ? std::string("Unknown") : out;
}

} // namespace

std::string fatName(const std::string& name, size_t maxBytes) {
    std::string safe = fatSafe(name);
    if (safe.size() <= maxBytes) {
        return safe;
    }
    std::string stem = safe, ext;
    const size_t dot = safe.rfind('.');
    if (dot != std::string::npos && dot > 0 && safe.size() - dot < 10) {
        stem = safe.substr(0, dot);
        ext = safe.substr(dot);
    }
    size_t end = std::min(stem.size(), maxBytes > ext.size() ? maxBytes - ext.size() : 1);
    while (end > 0 && (static_cast<unsigned char>(stem[end]) & 0xc0) == 0x80) {
        --end; // not inside a UTF-8 character
    }
    stem = stem.substr(0, end);
    while (!stem.empty() && (stem.back() == '.' || stem.back() == ' ')) {
        stem.pop_back();
    }
    return (stem.empty() ? std::string("Unknown") : stem) + ext;
}

std::string analysisDirectory(const std::string& audioPathOnStick) {
    uint32_t hash = 0;
    for (char16_t unit : utf16(audioPathOnStick)) {
        if (unit == 0) {
            break;
        }
        hash = hash * 0x34f5501du + static_cast<uint32_t>(unit) * 0x93b6u;
    }
    hash %= 0x00030d43u;
    uint32_t shard = 0;
    const int bits[] = {0, 2, 6, 7, 9, 13, 16};
    for (int i = 0; i < 7; ++i) {
        shard |= ((hash >> bits[i]) & 1u) << i;
    }
    char text[64];
    std::snprintf(text, sizeof(text), "/PIONEER/USBANLZ/P%03X/%08X", shard, hash);
    return text;
}

std::array<std::vector<uint8_t>, 3> buildAnalysis(const Track& track, const std::string& stickPath) {
    // the beat grid, PQTZ: per beat its number, the tempo x100 and the time in ms
    Section grid{"PQTZ", {}, {}};
    appendBe32(grid.header, 0);
    appendBe32(grid.header, 0x00080000);
    appendBe32(grid.header, static_cast<uint32_t>(track.beats.size()));
    for (size_t i = 0; i < track.beats.size(); ++i) {
        double bpm = track.bpm;
        if (i + 1 < track.beats.size() && track.beats[i + 1].first > track.beats[i].first) {
            bpm = 60.0 / (track.beats[i + 1].first - track.beats[i].first);
        }
        appendBe16(grid.payload, static_cast<uint16_t>(std::clamp(track.beats[i].second, 1, 4)));
        appendBe16(grid.payload, static_cast<uint16_t>(std::lround(std::clamp(bpm, 0.0, 600.0) * 100)));
        appendBe32(grid.payload, static_cast<uint32_t>(std::lround(std::max(0.0, track.beats[i].first) * 1000)));
    }
    const std::vector<Column>& columns = track.waveform;
    // A preview column of height 0 reads as "not analysed" on Nexus players: at least 2 (PWAV), 1 (PWV2)
    std::vector<uint8_t> pwav, pwv2;
    for (const Column& c : resample(columns, 400)) {
        const uint8_t b = monoByte(c, bandHeight(c, 25));
        pwav.push_back(static_cast<uint8_t>((b & 0xe0) | std::max<uint8_t>(b & 0x1f, 2)));
    }
    for (const Column& c : resample(columns, 100)) {
        pwv2.push_back(static_cast<uint8_t>(std::clamp<uint32_t>(bandHeight(c, 15), 1, 15)));
    }
    std::vector<uint8_t> pwv3, pwv5, pwv4, pwv6, pwv7;
    for (const Column& c : columns) {
        pwv3.push_back(monoByte(c, detailHeight(c)));
        const uint32_t low = 2u * c.low, mid = 2u * c.mid, high = 3u * c.high;
        const uint32_t strongest = std::max({low, mid, high});
        const auto share = [strongest](uint32_t v, uint32_t full) {
            return strongest ? (2 * full * v + strongest) / (2 * strongest) : 0;
        };
        const uint16_t word = static_cast<uint16_t>((share(low, 7) << 13) | (share(mid, 5) << 10) |
                (share(high, 7) << 7) | (detailHeight(c) << 2));
        appendBe16(pwv5, word);
        pwv7.insert(pwv7.end(), {static_cast<uint8_t>(c.low >> 1), static_cast<uint8_t>(c.mid >> 1),
                                        static_cast<uint8_t>(c.high >> 1)});
    }
    for (const Column& c : resample(columns, 1200)) {
        pwv4.insert(pwv4.end(), {static_cast<uint8_t>(c.peak >> 1), 0, 0, static_cast<uint8_t>(c.low >> 1),
                                        static_cast<uint8_t>((c.mid >> 1) * 4 / 5),
                                        static_cast<uint8_t>((c.high >> 1) * 13 / 20)});
        pwv6.insert(pwv6.end(), {band(c.low), band(c.mid), band(c.high)});
    }
    std::vector<Section> dat{pathSection(stickPath)};
    Section vbr{"PVBR", {}, std::vector<uint8_t>(401 * 4, 0)};
    appendBe32(vbr.header, 0);
    dat.push_back(vbr);
    dat.push_back(grid);
    dat.push_back(previewSection("PWAV", pwav));
    dat.push_back(previewSection("PWV2", pwv2));
    cueLists(dat, false);
    std::vector<Section> ext{pathSection(stickPath), scrollSection("PWV3", 1, pwv3)};
    cueLists(ext, true);
    ext.push_back(scrollSection("PWV5", 2, pwv5));
    ext.push_back(scrollSection("PWV4", 6, pwv4));
    std::vector<Section> twoEx{pathSection(stickPath), scrollSection("PWV7", 3, pwv7), scrollSection("PWV6", 3, pwv6)};
    return {render(dat), render(ext), render(twoEx)};
}

std::vector<uint8_t> buildPdb(const Export& library,
        const std::vector<std::string>& stickPaths,
        const std::vector<std::string>& analysisPaths) {
    Intern artists, albums, genres, keys;
    std::vector<std::vector<uint8_t>> trackRows;
    std::map<std::string, uint32_t> albumArtist;
    for (size_t i = 0; i < library.tracks.size(); ++i) {
        const Track& track = library.tracks[i];
        TrackRow row;
        row.id = static_cast<uint32_t>(i + 1);
        row.track = &track;
        row.artist = artists.id(track.artist);
        row.album = albums.id(track.album);
        if (row.album && !albumArtist.count(track.album)) {
            albumArtist[track.album] = row.artist;
        }
        row.genre = genres.id(track.genre);
        row.key = keys.id(track.key);
        row.path = i < stickPaths.size() ? stickPaths[i] : std::string();
        row.filename = fs::path(row.path).filename().string();
        row.analyzePath = i < analysisPaths.size() ? analysisPaths[i] : std::string();
        row.dateAdded = library.date;
        std::error_code error;
        const auto size = fs::file_size(toPath(track.sourcePath), error);
        row.fileSize = error ? 0 : static_cast<uint32_t>(std::min<uintmax_t>(size, 0xffffffffu));
        trackRows.push_back(trackRow(row));
    }
    std::vector<std::vector<uint8_t>> artistRows, albumRows, genreRows, keyRows, colorRows;
    for (size_t i = 0; i < artists.order.size(); ++i) {
        artistRows.push_back(artistRow(static_cast<uint32_t>(i + 1), artists.order[i]));
    }
    for (size_t i = 0; i < albums.order.size(); ++i) {
        albumRows.push_back(albumRow(static_cast<uint32_t>(i + 1), albumArtist[albums.order[i]], albums.order[i]));
    }
    for (size_t i = 0; i < genres.order.size(); ++i) {
        genreRows.push_back(namedRow(static_cast<uint32_t>(i + 1), genres.order[i]));
    }
    for (size_t i = 0; i < keys.order.size(); ++i) {
        keyRows.push_back(keyRow(static_cast<uint32_t>(i + 1), keys.order[i]));
    }
    const char* colors[] = {"Pink", "Red", "Orange", "Yellow", "Green", "Aqua", "Blue", "Purple"};
    for (uint16_t i = 0; i < 8; ++i) {
        colorRows.push_back(colorRow(i + 1, colors[i]));
    }
    std::vector<std::vector<uint8_t>> playlistRows, entryRows;
    for (size_t p = 0; p < library.playlists.size(); ++p) {
        const uint32_t id = static_cast<uint32_t>(p + 1);
        playlistRows.push_back(playlistRow(id, 0, static_cast<uint32_t>(p), library.playlists[p].name));
        uint32_t position = 0;
        for (size_t t : library.playlists[p].tracks) {
            if (t < library.tracks.size()) {
                entryRows.push_back(playlistEntryRow(++position, static_cast<uint32_t>(t + 1), id));
            }
        }
    }
    std::vector<Table> tables;
    tables.push_back(makeTable(0, trackRows, true));
    tables.push_back(makeTable(1, genreRows));
    tables.push_back(makeTable(2, artistRows, true));
    tables.push_back(makeTable(3, albumRows, true));
    tables.push_back(makeTable(4, {}));
    tables.push_back(makeTable(5, keyRows));
    tables.push_back(makeTable(6, colorRows));
    tables.push_back(makeTable(7, playlistRows));
    tables.push_back(makeTable(8, entryRows));
    for (uint32_t type = 9; type <= 15; ++type) {
        tables.push_back(makeTable(type, {}));
    }
    tables.push_back(makeTable(16, columnRows()));
    tables.push_back(makeTable(17, constantRows(kHistoryPlaylists)));
    tables.push_back(makeTable(18, constantRows(kHistoryEntries)));
    tables.push_back(makeTable(19,
            {propertyRow(static_cast<uint32_t>(library.tracks.size()), library.date, library.deviceName)},
            true));
    return renderFile(std::move(tables));
}

namespace {

bool writeFile(const fs::path& path, const std::vector<uint8_t>& data, std::string* error) {
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!out) {
        *error = "cannot write " + fromPath(path);
        return false;
    }
    return true;
}

} // namespace

Result write(const std::string& root,
        const Export& library,
        const std::function<bool(int, int, const std::string&)>& progress) {
    Result result;
    const fs::path base = toPath(root);
    std::vector<std::string> stickPaths, analysisPaths;
    std::map<std::string, int> taken;
    for (size_t i = 0; i < library.tracks.size(); ++i) {
        const Track& track = library.tracks[i];
        const std::string artist = fatName(track.artist.empty() ? "UnknownArtist" : track.artist, 64);
        const std::string album = fatName(track.album.empty() ? "UnknownAlbum" : track.album, 64);
        std::string file = fatName(fromPath(toPath(track.sourcePath).filename()), 128);
        std::string stick = "/Contents/" + artist + "/" + album + "/" + file;
        // two different songs with one name in one folder: the second gets " (2)"
        if (taken.count(stick) && taken[stick] >= 0) {
            const fs::path f = toPath(file);
            file = fromPath(f.stem()) + " (" + std::to_string(++taken[stick] + 1) + ")" + fromPath(f.extension());
            stick = "/Contents/" + artist + "/" + album + "/" + file;
        }
        taken[stick];
        stickPaths.push_back(stick);
        analysisPaths.push_back(analysisDirectory(stick) + "/ANLZ0000.DAT");
    }
    const int total = static_cast<int>(library.tracks.size());
    for (int i = 0; i < total; ++i) {
        const Track& track = library.tracks[i];
        const fs::path target = base / toPath(stickPaths[i].substr(1));
        std::error_code ec;
        fs::create_directories(target.parent_path(), ec);
        const auto sourceSize = fs::file_size(toPath(track.sourcePath), ec);
        if (ec) {
            result.error = "cannot read " + track.sourcePath;
            return result;
        }
        std::error_code same;
        if (!(fs::exists(target, same) && fs::file_size(target, same) == sourceSize)) {
            fs::copy_file(toPath(track.sourcePath), target, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                result.error = "cannot copy to " + fromPath(target) + ": " + ec.message();
                return result;
            }
        }
        result.bytes += static_cast<int64_t>(sourceSize);
        const auto files = buildAnalysis(track, stickPaths[i]);
        const fs::path dir = base / toPath(analysisDirectory(stickPaths[i]).substr(1));
        const char* names[] = {"ANLZ0000.DAT", "ANLZ0000.EXT", "ANLZ0000.2EX"};
        for (int k = 0; k < 3; ++k) {
            if (!writeFile(dir / names[k], files[k], &result.error)) {
                return result;
            }
        }
        ++result.tracks;
        if (progress && !progress(i + 1, total, track.title)) {
            result.error = "stopped";
            return result;
        }
    }
    if (!writeFile(base / "PIONEER" / "rekordbox" / "export.pdb", buildPdb(library, stickPaths, analysisPaths), &result.error)) {
        return result;
    }
    result.playlists = static_cast<int>(library.playlists.size());
    result.ok = true;
    return result;
}

} // namespace djmantra::pioneer
