#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "library/basetrackcache.h"
#include "library/dao/trackschema.h"
#include "library/folders/foldertablemodel.h"
#include "library/folders/foldertree.h"
#include "library/onlinestatus.h"
#include "sources/localtrackcache.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "test/librarytest.h"
#include "track/track.h"

using djmantra::FolderNode;

TEST(FolderTreeTest, buildsTreeFromTrackFolders) {
    const QStringList roots{QStringLiteral("/m/Music"), QStringLiteral("/d/USB"),
            QStringLiteral("/e/Empty")};
    const QList<QPair<QString, int>> dirs{
            {QStringLiteral("/m/Music/house/Deep"), 3},
            {QStringLiteral("/m/Music/house"), 2},
            {QStringLiteral("/m/Music"), 1},
            {QStringLiteral("/m/Music/Ambient"), 4},
            {QStringLiteral("/d/USB/Techno"), 4},
            {QStringLiteral("/other/place"), 5}, // outside the library folders
    };
    const QList<FolderNode> tree = djmantra::buildFolderTree(roots, dirs);
    ASSERT_EQ(3, tree.size());
    // Sorted by name, case-insensitively
    EXPECT_EQ(QStringLiteral("Empty"), tree[0].label);
    EXPECT_EQ(0, tree[0].trackCount);
    EXPECT_TRUE(tree[0].children.isEmpty());
    EXPECT_EQ(QStringLiteral("Music"), tree[1].label);
    EXPECT_EQ(10, tree[1].trackCount); // with all subfolders
    ASSERT_EQ(2, tree[1].children.size());
    EXPECT_EQ(QStringLiteral("Ambient"), tree[1].children[0].label);
    EXPECT_EQ(QStringLiteral("house"), tree[1].children[1].label);
    EXPECT_EQ(5, tree[1].children[1].trackCount);
    ASSERT_EQ(1, tree[1].children[1].children.size());
    EXPECT_EQ(QStringLiteral("/m/Music/house/Deep"), tree[1].children[1].children[0].path);
    EXPECT_EQ(3, tree[1].children[1].children[0].trackCount);
    EXPECT_EQ(QStringLiteral("USB"), tree[2].label);
    EXPECT_EQ(4, tree[2].trackCount);
}

TEST(FolderTreeTest, similarNamesAreNotSubfolders) {
    const QList<FolderNode> tree = djmantra::buildFolderTree(
            {QStringLiteral("/m/Music")},
            {{QStringLiteral("/m/Music2"), 7}, {QStringLiteral("/m/Music/a"), 1}});
    ASSERT_EQ(1, tree.size());
    EXPECT_EQ(1, tree[0].trackCount);
}

class FolderTableModelTest : public LibraryTest {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_tmp.isValid());
        m_music = QDir(m_tmp.path()).filePath(QStringLiteral("drives/USB1/Music"));
        ASSERT_TRUE(QDir().mkpath(m_music));
        djmantra::LocalTrackCache::Settings settings;
        settings.mode = djmantra::LocalTrackCache::Mode::Off;
        settings.removablePrefixes = {QDir(m_tmp.path()).filePath(QStringLiteral("drives/"))};
        settings.excludedPrefixes = {};
        djmantra::LocalTrackCache::configure(settings);
        djmantra::OnlineStatus::invalidate();

        // The track source that "Tracks" (MixxxLibraryFeature) sets up in the app
        QStringList columns{LIBRARYTABLE_ID,
                LIBRARYTABLE_ARTIST,
                LIBRARYTABLE_TITLE,
                LIBRARYTABLE_MIXXXDELETED,
                TRACKLOCATIONSTABLE_LOCATION,
                TRACKLOCATIONSTABLE_FSDELETED};
        QStringList qualified;
        for (const auto& column : std::as_const(columns)) {
            qualified.append(mixxx::trackschema::tableForColumn(column) + QChar('.') + column);
        }
        QSqlQuery query(dbConnection());
        ASSERT_TRUE(query.exec(QStringLiteral(
                "CREATE TEMPORARY VIEW IF NOT EXISTS test_cache_view AS SELECT %1 FROM library "
                "INNER JOIN track_locations ON library.location = track_locations.id")
                                       .arg(qualified.join(QChar(',')))));
        m_pTrackSource = QSharedPointer<BaseTrackCache>(new BaseTrackCache(internalCollection(),
                QStringLiteral("test_cache_view"),
                LIBRARYTABLE_ID,
                columns,
                QStringList{LIBRARYTABLE_ARTIST, LIBRARYTABLE_TITLE},
                true));
        internalCollection()->connectTrackSource(m_pTrackSource);
    }
    void TearDown() override {
        djmantra::LocalTrackCache::configure(djmantra::LocalTrackCache::Settings{
                QString(), djmantra::LocalTrackCache::Mode::Off});
        djmantra::OnlineStatus::invalidate();
    }

    TrackPointer addTrack(const QString& relativePath) {
        const QString location = QDir(m_tmp.path()).filePath(relativePath);
        QDir().mkpath(QFileInfo(location).absolutePath());
        EXPECT_TRUE(QFile::copy(
                getTestDir().filePath(QStringLiteral("id3-test-data/empty.mp3")), location));
        return getOrAddTrackByLocation(location);
    }

    QTemporaryDir m_tmp;
    QString m_music;
    QSharedPointer<BaseTrackCache> m_pTrackSource;
};

TEST_F(FolderTableModelTest, folderConditionMatchesFolderAndSubfolders) {
    QSqlQuery query(dbConnection());
    ASSERT_TRUE(query.exec(QStringLiteral("CREATE TEMP TABLE dirs (directory TEXT)")));
    for (const char* dir : {"/m/Bob's", "/m/Bob's/a", "/m/Bob's/a/b", "/m/Bob's2", "/m/Bob", "/x"}) {
        query.prepare(QStringLiteral("INSERT INTO dirs VALUES (:d)"));
        query.bindValue(QStringLiteral(":d"), QString::fromUtf8(dir));
        ASSERT_TRUE(query.exec());
    }
    ASSERT_TRUE(query.exec(QStringLiteral("SELECT COUNT(*) FROM dirs WHERE ") +
            djmantra::folderCondition(QStringLiteral("directory"), QStringLiteral("/m/Bob's"))));
    ASSERT_TRUE(query.next());
    EXPECT_EQ(3, query.value(0).toInt());
}

TEST_F(FolderTableModelTest, listsFolderWithSubfoldersAndMarksOffline) {
    const TrackPointer pTop = addTrack(QStringLiteral("drives/USB1/Music/top.mp3"));
    const TrackPointer pSub = addTrack(QStringLiteral("drives/USB1/Music/House/sub.mp3"));
    addTrack(QStringLiteral("drives/USB1/Music2/other.mp3"));
    ASSERT_TRUE(pTop && pSub);

    // The last scan did not find the top track
    QSqlQuery query(dbConnection());
    query.prepare(QStringLiteral("UPDATE track_locations SET fs_deleted=1 WHERE location=:l"));
    query.bindValue(QStringLiteral(":l"), pTop->getLocation());
    ASSERT_TRUE(query.exec());

    FolderTableModel model(nullptr, trackCollectionManager());
    model.selectFolder(m_music);
    ASSERT_EQ(2, model.rowCount()); // missing tracks are listed (offline), Music2 is not
    const int locationColumn = model.fieldIndex(ColumnCache::COLUMN_TRACKLOCATIONSTABLE_LOCATION);
    int offline = 0;
    for (int row = 0; row < model.rowCount(); ++row) {
        const QModelIndex index = model.index(row, locationColumn);
        const bool isTop = model.data(index).toString() == pTop->getLocation();
        EXPECT_EQ(isTop, model.isOffline(index));
        offline += model.isOffline(index) ? 1 : 0;
        // Green/red dot next to the title
        EXPECT_FALSE(model.data(model.index(row, model.fieldIndex(
                                                           ColumnCache::COLUMN_LIBRARYTABLE_TITLE)),
                                  Qt::DecorationRole)
                             .isNull());
    }
    EXPECT_EQ(1, offline);

    model.selectFolder(QDir(m_music).filePath(QStringLiteral("House")));
    EXPECT_EQ(1, model.rowCount());

    // The drive is unplugged: everything on it is offline, nothing disappears
    const QString drive = QDir(m_tmp.path()).filePath(QStringLiteral("drives/USB1"));
    ASSERT_TRUE(QDir().rename(drive, drive + QStringLiteral(".away")));
    djmantra::OnlineStatus::invalidate();
    model.selectFolder(m_music);
    ASSERT_EQ(2, model.rowCount());
    for (int row = 0; row < model.rowCount(); ++row) {
        EXPECT_TRUE(model.isOffline(model.index(row, locationColumn)));
        EXPECT_TRUE(model.data(model.index(row, locationColumn), Qt::ToolTipRole)
                            .toString()
                            .startsWith(QStringLiteral("Offline")));
    }
    ASSERT_TRUE(QDir().rename(drive + QStringLiteral(".away"), drive));
    djmantra::OnlineStatus::invalidate();
    model.select();
    EXPECT_EQ(1, offline);
}
