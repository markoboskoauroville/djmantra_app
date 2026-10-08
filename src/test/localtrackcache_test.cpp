#include "sources/localtrackcache.h"

#include <gtest/gtest.h>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QThread>
#include <thread>

#include "library/scanner/libraryscanner.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "sources/soundsourceproxy.h"
#include "test/librarytest.h"
#include "track/track.h"
#include "util/volumewatcher.h"

using djmantra::LocalTrackCache;

namespace {

QByteArray readAll(const QString& path) {
    QFile file(path);
    EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << path.toStdString();
    return file.readAll();
}

void writeFile(const QString& path, const QByteArray& data) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    ASSERT_EQ(data.size(), file.write(data));
}

} // namespace

/// A fake external drive: <tmp>/drives/USB1 below the prefix <tmp>/drives/,
/// and the cache in <tmp>/cache.
class LocalTrackCacheTest : public LibraryTest {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_tmp.isValid());
        m_drives = QDir(m_tmp.path()).filePath(QStringLiteral("drives"));
        m_drive = QDir(m_drives).filePath(QStringLiteral("USB1"));
        m_cacheDir = QDir(m_tmp.path()).filePath(QStringLiteral("cache"));
        ASSERT_TRUE(QDir().mkpath(m_drive));
        configure(LocalTrackCache::Mode::RemovableOnly, qint64{64} * 1024 * 1024);
    }

    void TearDown() override {
        LocalTrackCache::configure(LocalTrackCache::Settings{QString(), LocalTrackCache::Mode::Off});
    }

    void configure(LocalTrackCache::Mode mode,
            qint64 maxBytes,
            std::chrono::milliseconds reconnectTimeout = std::chrono::milliseconds(3000)) {
        LocalTrackCache::Settings settings;
        settings.directory = m_cacheDir;
        settings.mode = mode;
        settings.maxBytes = maxBytes;
        settings.removablePrefixes = {m_drives + QChar('/')};
        settings.excludedPrefixes = {};
        settings.reconnectTimeout = reconnectTimeout;
        settings.pollInterval = std::chrono::milliseconds(20);
        settings.reserveBytes = 0;
        LocalTrackCache::configure(settings);
    }

    QString onDrive(const QString& relative) const {
        return QDir(m_drive).filePath(relative);
    }

    void unplug() {
        ASSERT_TRUE(QDir().rename(m_drive, m_drive + QStringLiteral(".away")));
    }
    void replug() {
        ASSERT_TRUE(QDir().rename(m_drive + QStringLiteral(".away"), m_drive));
    }

    QTemporaryDir m_tmp;
    QString m_drives;
    QString m_drive;
    QString m_cacheDir;
};

TEST_F(LocalTrackCacheTest, driveRootAndWhatIsCached) {
    EXPECT_EQ(m_drive, LocalTrackCache::driveRoot(onDrive(QStringLiteral("Music/a.mp3"))));
    EXPECT_TRUE(LocalTrackCache::shouldCache(onDrive(QStringLiteral("a.mp3"))));
    // Not on an external drive
    EXPECT_TRUE(LocalTrackCache::driveRoot(QDir(m_tmp.path()).filePath("a.mp3")).isEmpty());
    EXPECT_FALSE(LocalTrackCache::shouldCache(QDir(m_tmp.path()).filePath("a.mp3")));
    // The drive directory itself is not a file on the drive
    EXPECT_TRUE(LocalTrackCache::driveRoot(m_drive).isEmpty());
    // Copies are never cached again
    EXPECT_FALSE(LocalTrackCache::shouldCache(QDir(m_cacheDir).filePath("x.mp3")));

    configure(LocalTrackCache::Mode::All, 1024);
    EXPECT_TRUE(LocalTrackCache::shouldCache(QDir(m_tmp.path()).filePath("a.mp3")));
    configure(LocalTrackCache::Mode::Off, 1024);
    EXPECT_FALSE(LocalTrackCache::shouldCache(onDrive(QStringLiteral("a.mp3"))));
}

TEST_F(LocalTrackCacheTest, copiesAndReusesTheCopy) {
    const QString song = onDrive(QStringLiteral("Music/Song One.MP3"));
    const QByteArray data(3 * 1024 * 1024 + 17, 'x');
    writeFile(song, data);
    EXPECT_EQ(song, LocalTrackCache::readPath(song)); // not copied yet

    const auto result = LocalTrackCache::ensureCached(song);
    ASSERT_TRUE(result.fromCache) << result.error.toStdString();
    EXPECT_TRUE(result.readPath.startsWith(m_cacheDir));
    EXPECT_TRUE(result.readPath.endsWith(QStringLiteral(".mp3"))); // decoder by extension
    EXPECT_EQ(data, readAll(result.readPath));
    EXPECT_EQ(result.readPath, LocalTrackCache::readPath(song));
    EXPECT_EQ(data.size(), LocalTrackCache::usedBytes());

    // Second time: same copy
    const auto again = LocalTrackCache::ensureCached(song);
    EXPECT_TRUE(again.fromCache);
    EXPECT_EQ(result.readPath, again.readPath);
}

TEST_F(LocalTrackCacheTest, changedOriginalIsCopiedAgain) {
    const QString song = onDrive(QStringLiteral("a.flac"));
    writeFile(song, QByteArray(1000, 'a'));
    const auto first = LocalTrackCache::ensureCached(song);
    ASSERT_TRUE(first.fromCache);

    writeFile(song, QByteArray(2000, 'b'));
    // Stale copy is not used
    EXPECT_EQ(song, LocalTrackCache::readPath(song));
    const auto second = LocalTrackCache::ensureCached(song);
    ASSERT_TRUE(second.fromCache);
    EXPECT_EQ(QByteArray(2000, 'b'), readAll(second.readPath));
}

TEST_F(LocalTrackCacheTest, unpluggedDriveUsesTheCopy) {
    const QString song = onDrive(QStringLiteral("Music/a.mp3"));
    writeFile(song, QByteArray(5000, 'z'));
    const auto cached = LocalTrackCache::ensureCached(song);
    ASSERT_TRUE(cached.fromCache);

    unplug();
    EXPECT_TRUE(LocalTrackCache::isOnUnreachableDrive(song, {}));
    EXPECT_EQ(cached.readPath, LocalTrackCache::readPath(song));
    const auto offline = LocalTrackCache::ensureCached(song);
    EXPECT_TRUE(offline.fromCache);
    EXPECT_EQ(cached.readPath, offline.readPath);
    EXPECT_EQ(QByteArray(5000, 'z'), readAll(offline.readPath));

    replug();
    EXPECT_FALSE(LocalTrackCache::isOnUnreachableDrive(song, {}));
    EXPECT_EQ(cached.readPath, LocalTrackCache::readPath(song));
}

TEST_F(LocalTrackCacheTest, waitsForADriveThatDroppedOut) {
    const QString song = onDrive(QStringLiteral("Music/b.mp3"));
    writeFile(song, QByteArray(200000, 'q'));
    unplug();

    // The drive comes back after 300 ms, while a deck is loading the song
    std::thread replugLater([this] {
        QThread::msleep(300);
        replug();
    });
    QElapsedTimer timer;
    timer.start();
    const auto result = LocalTrackCache::ensureCached(song);
    replugLater.join();
    EXPECT_TRUE(result.fromCache) << result.error.toStdString();
    EXPECT_GE(timer.elapsed(), 250);
    EXPECT_EQ(QByteArray(200000, 'q'), readAll(result.readPath));
}

TEST_F(LocalTrackCacheTest, givesUpWhenTheDriveStaysAway) {
    configure(LocalTrackCache::Mode::RemovableOnly, 1 << 20, std::chrono::milliseconds(200));
    const QString song = onDrive(QStringLiteral("c.mp3"));
    writeFile(song, QByteArray(100, 'c'));
    unplug();
    const auto result = LocalTrackCache::ensureCached(song);
    EXPECT_FALSE(result.fromCache);
    EXPECT_FALSE(result.error.isEmpty());
    replug();
}

TEST_F(LocalTrackCacheTest, missingFileOnPresentDriveFailsAtOnce) {
    QElapsedTimer timer;
    timer.start();
    const auto result = LocalTrackCache::ensureCached(onDrive(QStringLiteral("nope.mp3")));
    EXPECT_FALSE(result.fromCache);
    EXPECT_LT(timer.elapsed(), 1000);
}

TEST_F(LocalTrackCacheTest, cancelStopsTheWait) {
    const QString song = onDrive(QStringLiteral("d.mp3"));
    writeFile(song, QByteArray(100, 'd'));
    unplug();
    QElapsedTimer timer;
    timer.start();
    const auto result = LocalTrackCache::ensureCached(song, [&timer] { return timer.elapsed() > 100; });
    EXPECT_FALSE(result.fromCache);
    EXPECT_LT(timer.elapsed(), 2000);
    replug();
}

TEST_F(LocalTrackCacheTest, evictsLeastRecentlyUsed) {
    configure(LocalTrackCache::Mode::RemovableOnly, 2500);
    const QString a = onDrive(QStringLiteral("a.mp3"));
    const QString b = onDrive(QStringLiteral("b.mp3"));
    const QString c = onDrive(QStringLiteral("c.mp3"));
    writeFile(a, QByteArray(1000, 'a'));
    writeFile(b, QByteArray(1000, 'b'));
    writeFile(c, QByteArray(1000, 'c'));
    ASSERT_TRUE(LocalTrackCache::ensureCached(a).fromCache);
    QThread::msleep(1100); // file times have 1 s resolution on some file systems
    ASSERT_TRUE(LocalTrackCache::ensureCached(b).fromCache);
    QThread::msleep(1100);
    ASSERT_TRUE(LocalTrackCache::ensureCached(a).fromCache); // a used again
    QThread::msleep(1100);
    ASSERT_TRUE(LocalTrackCache::ensureCached(c).fromCache);

    EXPECT_NE(a, LocalTrackCache::readPath(a));
    EXPECT_EQ(b, LocalTrackCache::readPath(b)); // evicted
    EXPECT_NE(c, LocalTrackCache::readPath(c));
    EXPECT_LE(LocalTrackCache::usedBytes(), 2500);

    // Larger than the whole cache: played from the drive
    const QString big = onDrive(QStringLiteral("big.mp3"));
    writeFile(big, QByteArray(3000, 'x'));
    const auto result = LocalTrackCache::ensureCached(big);
    EXPECT_FALSE(result.fromCache);
    EXPECT_EQ(big, result.readPath);
}

TEST_F(LocalTrackCacheTest, decodesFromTheCopyWhileTheDriveIsUnplugged) {
    const QString song = onDrive(QStringLiteral("Music/cover-test.flac"));
    QDir().mkpath(QFileInfo(song).absolutePath());
    ASSERT_TRUE(QFile::copy(
            getTestDir().filePath(QStringLiteral("id3-test-data/cover-test.flac")), song));
    const auto cached = LocalTrackCache::ensureCached(song);
    ASSERT_TRUE(cached.fromCache);
    unplug();

    const TrackPointer pTrack = Track::newTemporary(mixxx::FileAccess(mixxx::FileInfo(song)));
    mixxx::AudioSource::OpenParams params;
    params.setChannelCount(mixxx::audio::ChannelCount::stereo());
    const auto pAudioSource =
            SoundSourceProxy(pTrack, LocalTrackCache::readPath(song)).openAudioSource(params);
    ASSERT_TRUE(pAudioSource);
    EXPECT_FALSE(pAudioSource->frameIndexRange().empty());
    mixxx::SampleBuffer buffer(pAudioSource->getSignalInfo().frames2samples(1024));
    const auto read = pAudioSource->readSampleFrames(mixxx::WritableSampleFrames(
            mixxx::IndexRange::forward(pAudioSource->frameIndexRange().start(), 1024),
            mixxx::SampleBuffer::WritableSlice(buffer)));
    EXPECT_EQ(1024, read.frameIndexRange().length());
    replug();
}

TEST_F(LocalTrackCacheTest, volumeWatcherReportsDrives) {
    QStringList present{QStringLiteral("/storage/AAAA-1111")};
    djmantra::VolumeWatcher watcher(nullptr, [&present] { return present; }, 1000000);
    QSignalSpy attached(&watcher, &djmantra::VolumeWatcher::driveAttached);
    QSignalSpy detached(&watcher, &djmantra::VolumeWatcher::driveDetached);
    watcher.start();
    watcher.poll();
    EXPECT_EQ(0, attached.count()); // present at start: not "attached"

    present.append(QStringLiteral("/storage/BBBB-2222"));
    watcher.poll();
    ASSERT_EQ(1, attached.count());
    EXPECT_EQ(QStringLiteral("/storage/BBBB-2222"), attached.at(0).at(0).toString());

    present.removeFirst();
    watcher.poll();
    ASSERT_EQ(1, detached.count());
    EXPECT_EQ(QStringLiteral("/storage/AAAA-1111"), detached.at(0).at(0).toString());
    EXPECT_EQ(QStringList{QStringLiteral("/storage/BBBB-2222")}, watcher.drives());
}

namespace {

class Scanner {
  public:
    Scanner(const mixxx::DbConnectionPoolPtr& pool, const UserSettingsPointer& config)
            : m_scanner(pool, config) {
        m_scanner.start();
    }
    ~Scanner() {
        m_scanner.quit();
        m_scanner.wait();
    }
    bool scan() {
        QSignalSpy finished(&m_scanner, &LibraryScanner::scanFinished);
        m_scanner.scan();
        return finished.wait(30000);
    }

  private:
    LibraryScanner m_scanner;
};

} // namespace

TEST_F(LocalTrackCacheTest, scanKeepsTracksOfAnUnpluggedDrive) {
    const QString music = onDrive(QStringLiteral("Music"));
    const QString source = getTestDir().filePath(QStringLiteral("id3-test-data/cover-test.flac"));
    QDir().mkpath(music);
    ASSERT_TRUE(QFile::copy(source, QDir(music).filePath(QStringLiteral("one.flac"))));
    ASSERT_TRUE(QFile::copy(source, QDir(music).filePath(QStringLiteral("two.flac"))));
    // A library folder on internal storage, for comparison
    const QString internal = QDir(m_tmp.path()).filePath(QStringLiteral("internal"));
    QDir().mkpath(internal);
    ASSERT_TRUE(QFile::copy(source, QDir(internal).filePath(QStringLiteral("three.flac"))));

    ASSERT_EQ(DirectoryDAO::AddResult::Ok,
            trackCollectionManager()->addDirectory(mixxx::FileInfo(music)));
    ASSERT_EQ(DirectoryDAO::AddResult::Ok,
            trackCollectionManager()->addDirectory(mixxx::FileInfo(internal)));

    auto countMissing = [this] {
        QSqlQuery query(dbConnection());
        EXPECT_TRUE(query.exec(QStringLiteral(
                "SELECT COUNT(*) FROM track_locations WHERE fs_deleted=1")));
        EXPECT_TRUE(query.next());
        return query.value(0).toInt();
    };
    auto countTracks = [this] {
        QSqlQuery query(dbConnection());
        EXPECT_TRUE(query.exec(QStringLiteral(
                "SELECT COUNT(*) FROM library WHERE mixxx_deleted=0")));
        EXPECT_TRUE(query.next());
        return query.value(0).toInt();
    };

    Scanner scanner(dbConnectionPooler(), config());
    ASSERT_TRUE(scanner.scan());
    EXPECT_EQ(3, countTracks());
    EXPECT_EQ(0, countMissing());

    // Drive unplugged, and a file really deleted on internal storage
    unplug();
    ASSERT_TRUE(QFile::remove(QDir(internal).filePath(QStringLiteral("three.flac"))));
    ASSERT_TRUE(scanner.scan());
    EXPECT_EQ(3, countTracks());
    EXPECT_EQ(1, countMissing()); // only the deleted file

    // Plugged in again: all there
    replug();
    ASSERT_TRUE(scanner.scan());
    EXPECT_EQ(1, countMissing());
    EXPECT_EQ(3, countTracks());
}
