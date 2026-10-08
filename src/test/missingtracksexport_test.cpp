#include "library/export/missingtracksexport.h"

#include <QDir>
#include <algorithm>
#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "library/trackcollectionmanager.h"
#include "test/librarytest.h"
#include "track/track.h"

using djmantra::MissingTrack;
using djmantra::Store;

namespace {

MissingTrack makeTrack() {
    MissingTrack track;
    track.artist = QStringLiteral("Simon & Garfunkel");
    track.title = QStringLiteral("Mrs. Robinson #1");
    track.album = QStringLiteral("Bookends");
    track.durationSeconds = 244;
    return track;
}

} // namespace

class MissingTracksExportTest : public LibraryTest {};

TEST_F(MissingTracksExportTest, searchUrlsAreFullyEncoded) {
    const MissingTrack track = makeTrack();
    EXPECT_EQ(QStringLiteral(
                      "https://bandcamp.com/search?q=Simon%20%26%20Garfunkel%20Mrs."
                      "%20Robinson%20%231"),
            djmantra::storeSearchUrl(Store::Bandcamp, track).toString(QUrl::FullyEncoded));
    EXPECT_EQ(QStringLiteral(
                      "https://www.qobuz.com/de-de/search?q=Simon%20%26%20Garfunkel%20Mrs."
                      "%20Robinson%20%231"),
            djmantra::storeSearchUrl(Store::Qobuz, track).toString(QUrl::FullyEncoded));
    // The query decodes back to the original text.
    EXPECT_EQ(QStringLiteral("q=Simon & Garfunkel Mrs. Robinson #1"),
            djmantra::storeSearchUrl(Store::Beatport, track).query(QUrl::FullyDecoded));
}

TEST_F(MissingTracksExportTest, unicodeNamesSurvive) {
    MissingTrack track;
    track.artist = QStringLiteral("Björk");
    track.title = QStringLiteral("Jóga");
    const QUrl url = djmantra::storeSearchUrl(Store::Tidal, track);
    EXPECT_EQ(QStringLiteral("q=Björk Jóga"), url.query(QUrl::FullyDecoded));
    EXPECT_TRUE(url.toString(QUrl::FullyEncoded).contains(QStringLiteral("Bj%C3%B6rk")));
}

TEST_F(MissingTracksExportTest, formatListsEverySongWithLinks) {
    MissingTrack tidalTrack = makeTrack();
    tidalTrack.isrc = QStringLiteral("USSM16800379");
    tidalTrack.sourceUrl = QUrl(QStringLiteral("https://tidal.com/browse/track/1234"));
    MissingTrack libraryTrack;
    libraryTrack.artist = QStringLiteral("Daft Punk");
    libraryTrack.title = QStringLiteral("One More Time");
    libraryTrack.lastKnownLocation = QStringLiteral("/music/daft punk/one more time.mp3");

    const QString text = djmantra::formatMissingTracks({tidalTrack, libraryTrack},
            QStringLiteral("Friday set"),
            QDateTime(QDate(2026, 10, 8), QTime(21, 5)));

    EXPECT_TRUE(text.startsWith(QStringLiteral(
            "DJ Mantra – missing songs\nList: Friday set\nExported: 2026-10-08 21:05\n"
            "Songs: 2\n")));
    EXPECT_TRUE(text.contains(QStringLiteral(
            "1. Simon & Garfunkel – Mrs. Robinson #1 (Bookends) [4:04]\n")));
    EXPECT_TRUE(text.contains(QStringLiteral("   ISRC: USSM16800379\n")));
    EXPECT_TRUE(text.contains(QStringLiteral("   Link: https://tidal.com/browse/track/1234\n")));
    EXPECT_TRUE(text.contains(QStringLiteral("2. Daft Punk – One More Time\n")));
    EXPECT_TRUE(text.contains(
            QStringLiteral("   Was at: /music/daft punk/one more time.mp3\n")));
    // Four store searches per song
    EXPECT_EQ(8, text.count(QStringLiteral(" search: https://")));
    EXPECT_TRUE(text.contains(QStringLiteral(
            "   Beatport search: https://www.beatport.com/search?q=Daft%20Punk%20One%20More%20Time\n")));
}

TEST_F(MissingTracksExportTest, writesUtf8File) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("missing.txt"));
    MissingTrack track;
    track.artist = QStringLiteral("Björk");
    track.title = QStringLiteral("Jóga");
    QString error;
    ASSERT_TRUE(djmantra::writeMissingTracksFile(path, {track}, QStringLiteral("Test"), &error))
            << error.toStdString();
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    const QString content = QString::fromUtf8(file.readAll());
    EXPECT_TRUE(content.contains(QStringLiteral("1. Björk – Jóga\n")));
}

TEST_F(MissingTracksExportTest, writeFailureReportsError) {
    QString error;
    EXPECT_FALSE(djmantra::writeMissingTracksFile(
            QStringLiteral("/nonexistent-dir/x/missing.txt"), {makeTrack()}, QStringLiteral("T"), &error));
    EXPECT_FALSE(error.isEmpty());
}

TEST_F(MissingTracksExportTest, queryReturnsOnlyTracksWithMissingFiles) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString sourceFile = getTestDir().filePath(QStringLiteral("id3-test-data/empty.mp3"));
    auto addTrack = [&](const QString& fileName, const QString& artist, const QString& title) {
        const QString location = QDir(dir.path()).filePath(fileName);
        EXPECT_TRUE(QFile::copy(sourceFile, location));
        const TrackPointer pTrack = getOrAddTrackByLocation(location);
        EXPECT_TRUE(pTrack);
        QSqlQuery update(dbConnection());
        update.prepare(QStringLiteral(
                "UPDATE library SET artist=:artist, title=:title, duration=125.4 "
                "WHERE id=:id"));
        update.bindValue(QStringLiteral(":artist"), artist);
        update.bindValue(QStringLiteral(":title"), title);
        update.bindValue(QStringLiteral(":id"), pTrack->getId().toVariant());
        EXPECT_TRUE(update.exec());
        return pTrack->getLocation();
    };
    const QString missingB = addTrack(QStringLiteral("b.mp3"), QStringLiteral("Bravo"), QStringLiteral("Two"));
    addTrack(QStringLiteral("present.mp3"), QStringLiteral("Present"), QStringLiteral("Here"));
    const QString missingA = addTrack(QStringLiteral("a.mp3"), QStringLiteral("alpha"), QStringLiteral("One"));

    QSqlQuery query(dbConnection());
    query.prepare(QStringLiteral(
            "UPDATE track_locations SET fs_deleted=1 WHERE location IN (:a, :b)"));
    query.bindValue(QStringLiteral(":a"), missingA);
    query.bindValue(QStringLiteral(":b"), missingB);
    ASSERT_TRUE(query.exec());

    const QList<MissingTrack> tracks = djmantra::queryLibraryMissingTracks(dbConnection());
    ASSERT_EQ(2, tracks.size());
    // Sorted by artist, case-insensitively
    EXPECT_EQ(QStringLiteral("alpha"), tracks[0].artist);
    EXPECT_EQ(QStringLiteral("One"), tracks[0].title);
    EXPECT_EQ(missingA, tracks[0].lastKnownLocation);
    EXPECT_EQ(125, tracks[0].durationSeconds);
    EXPECT_EQ(QStringLiteral("Bravo"), tracks[1].artist);
    EXPECT_EQ(missingB, tracks[1].lastKnownLocation);
}

TEST_F(MissingTracksExportTest, formatMarkedListsFileAndRating) {
    MissingTrack track;
    track.artist = QStringLiteral("Daft Punk");
    track.title = QStringLiteral("One More Time");
    track.durationSeconds = 320;
    track.rating = 4;
    track.fileLocation = QStringLiteral("/music/one more time.mp3");

    const QString text = djmantra::formatMarkedTracks({track},
            QStringLiteral("Library – marked tracks"),
            QDateTime(QDate(2026, 10, 8), QTime(21, 5)));

    EXPECT_TRUE(text.startsWith(QStringLiteral(
            "DJ Mantra – marked songs\nList: Library – marked tracks\n")));
    EXPECT_TRUE(text.contains(QStringLiteral("1. Daft Punk – One More Time [5:20] ★ 4\n")));
    EXPECT_TRUE(text.contains(QStringLiteral("   File: /music/one more time.mp3\n")));
    EXPECT_FALSE(text.contains(QStringLiteral("Was at:")));
    EXPECT_EQ(4, text.count(QStringLiteral(" search: https://")));
}

TEST_F(MissingTracksExportTest, exportMarkIsSavedAndQueried) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString sourceFile = getTestDir().filePath(QStringLiteral("id3-test-data/empty.mp3"));
    auto addTrack = [&](const QString& fileName, const QString& artist, bool marked) {
        const QString location = QDir(dir.path()).filePath(fileName);
        EXPECT_TRUE(QFile::copy(sourceFile, location));
        const TrackPointer pTrack = getOrAddTrackByLocation(location);
        EXPECT_TRUE(pTrack);
        pTrack->setArtist(artist);
        pTrack->setTitle(QStringLiteral("Song"));
        pTrack->setRating(3);
        pTrack->setExportMark(marked);
        EXPECT_TRUE(pTrack->getExportMark() == marked);
        EXPECT_EQ(TrackCollectionManager::SaveTrackResult::Saved,
                trackCollectionManager()->saveTrack(pTrack));
        return pTrack->getId();
    };
    const TrackId markedB = addTrack(QStringLiteral("b.mp3"), QStringLiteral("Bravo"), true);
    addTrack(QStringLiteral("unmarked.mp3"), QStringLiteral("Unmarked"), false);
    const TrackId markedA = addTrack(QStringLiteral("a.mp3"), QStringLiteral("alpha"), true);

    const QList<MissingTrack> tracks = djmantra::queryLibraryMarkedTracks(dbConnection());
    ASSERT_EQ(2, tracks.size());
    EXPECT_EQ(QStringLiteral("alpha"), tracks[0].artist);
    EXPECT_EQ(3, tracks[0].rating);
    EXPECT_TRUE(tracks[0].fileLocation.endsWith(QStringLiteral("a.mp3")));
    EXPECT_TRUE(tracks[0].lastKnownLocation.isEmpty());
    EXPECT_EQ(QStringLiteral("Bravo"), tracks[1].artist);

    QList<TrackId> ids = djmantra::queryLibraryMarkedTrackIds(dbConnection());
    std::sort(ids.begin(), ids.end());
    QList<TrackId> expected{markedA, markedB};
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(expected, ids);

    // The mark is loaded back from the database
    const TrackPointer pLoaded = trackCollectionManager()->getTrackById(markedA);
    ASSERT_TRUE(pLoaded);
    EXPECT_TRUE(pLoaded->getExportMark());
}
