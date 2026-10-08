#include "sources/externalcontent.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <sys/stat.h>

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>

#include "library/onlinestatus.h"
#include "sources/localtrackcache.h"
#include "sources/soundsourceproxy.h"
#include "test/librarytest.h"
#include "track/track.h"

using djmantra::ExternalContent;
using djmantra::ExternalContentProvider;
using djmantra::LocalTrackCache;

namespace {

const QString kVolume = QStringLiteral("AAAA-1111");

/// Serves a directory as if it were a USB stick read through the Storage
/// Access Framework.
class FakeProvider : public ExternalContentProvider {
  public:
    explicit FakeProvider(QString root)
            : m_root(std::move(root)) {
    }
    bool mounted = true;
    bool listFails = false;

    QList<Volume> volumes() override {
        return {Volume{kVolume, QStringLiteral("SanDisk USB"), mounted, true}};
    }
    bool list(const QString& volumeId, QList<Entry>* pEntries) override {
        if (volumeId != kVolume || !mounted || listFails) {
            return false;
        }
        QDirIterator it(m_root, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QFileInfo info(it.next());
            pEntries->append(Entry{QDir(m_root).relativeFilePath(info.absoluteFilePath()),
                    info.size(),
                    info.lastModified().toMSecsSinceEpoch()});
        }
        return true;
    }
    int openForReading(const QString& volumeId, const QString& relativePath) override {
        if (volumeId != kVolume || !mounted) {
            return -1;
        }
        return ::open(QDir(m_root).filePath(relativePath).toLocal8Bit().constData(), O_RDONLY);
    }

  private:
    QString m_root;
};

QByteArray readAll(const QString& path) {
    QFile file(path);
    EXPECT_TRUE(file.open(QIODevice::ReadOnly)) << path.toStdString();
    return file.readAll();
}

} // namespace

class ExternalContentTest : public LibraryTest {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_tmp.isValid());
        m_drive = QDir(m_tmp.path()).filePath(QStringLiteral("stick"));
        m_mirror = QDir(m_tmp.path()).filePath(QStringLiteral("app/drives"));
        ASSERT_TRUE(QDir().mkpath(m_drive + QStringLiteral("/Music/House")));
        const QString flac = getTestDir().filePath(QStringLiteral("id3-test-data/cover-test.flac"));
        ASSERT_TRUE(QFile::copy(flac, m_drive + QStringLiteral("/Music/a.flac")));
        ASSERT_TRUE(QFile::copy(flac, m_drive + QStringLiteral("/Music/House/b.flac")));
        QFile notes(m_drive + QStringLiteral("/notes.txt"));
        ASSERT_TRUE(notes.open(QIODevice::WriteOnly));
        notes.write("not music");
        notes.close();

        m_pProvider = std::make_shared<FakeProvider>(m_drive);
        ExternalContent::setProvider(m_pProvider, m_mirror);

        LocalTrackCache::Settings settings;
        settings.directory = QDir(m_tmp.path()).filePath(QStringLiteral("app/track-cache"));
        settings.mode = LocalTrackCache::Mode::RemovableOnly;
        settings.removablePrefixes = {};
        settings.excludedPrefixes = {};
        settings.reconnectTimeout = std::chrono::milliseconds(300);
        settings.pollInterval = std::chrono::milliseconds(20);
        settings.reserveBytes = 0;
        LocalTrackCache::configure(settings);
        djmantra::OnlineStatus::invalidate();
    }
    void TearDown() override {
        ExternalContent::setProvider(nullptr, QString());
        LocalTrackCache::configure(LocalTrackCache::Settings{QString(), LocalTrackCache::Mode::Off});
        djmantra::OnlineStatus::invalidate();
    }

    QString placeholder(const QString& relative) const {
        return ExternalContent::volumeDirectory(kVolume) + QChar('/') + relative;
    }
    ExternalContent::SyncResult sync() const {
        return ExternalContent::syncVolume(kVolume, {QStringLiteral("flac"), QStringLiteral("mp3")});
    }

    QTemporaryDir m_tmp;
    QString m_drive;
    QString m_mirror;
    std::shared_ptr<FakeProvider> m_pProvider;
};

TEST_F(ExternalContentTest, placeholdersMirrorTheDrive) {
    const auto first = sync();
    ASSERT_TRUE(first.ok);
    EXPECT_EQ(2, first.added);
    const QString a = placeholder(QStringLiteral("Music/a.flac"));
    const QFileInfo real(m_drive + QStringLiteral("/Music/a.flac"));
    const QFileInfo info(a);
    ASSERT_TRUE(info.exists());
    EXPECT_EQ(real.size(), info.size());
    EXPECT_EQ(real.lastModified().toMSecsSinceEpoch(), info.lastModified().toMSecsSinceEpoch());
    EXPECT_FALSE(QFileInfo::exists(placeholder(QStringLiteral("notes.txt")))); // not music
    // Sparse: takes (almost) no space
    struct stat st;
    ASSERT_EQ(0, ::stat(a.toLocal8Bit().constData(), &st));
    EXPECT_LT(static_cast<qint64>(st.st_blocks) * 512, real.size());

    EXPECT_TRUE(ExternalContent::isPlaceholder(a));
    QString volumeId;
    QString relative;
    ASSERT_TRUE(ExternalContent::splitPlaceholder(a, &volumeId, &relative));
    EXPECT_EQ(kVolume, volumeId);
    EXPECT_EQ(QStringLiteral("Music/a.flac"), relative);
    EXPECT_EQ(QStringLiteral("SanDisk USB/Music"),
            ExternalContent::displayPath(placeholder(QStringLiteral("Music"))));

    // Nothing changed: nothing to do
    const auto again = sync();
    EXPECT_EQ(0, again.added + again.updated + again.removed);

    // A file changed, one deleted
    QFile change(real.absoluteFilePath());
    ASSERT_TRUE(change.open(QIODevice::Append));
    change.write("x");
    change.close();
    ASSERT_TRUE(QFile::remove(m_drive + QStringLiteral("/Music/House/b.flac")));
    const auto third = sync();
    EXPECT_EQ(1, third.updated);
    EXPECT_EQ(1, third.removed);
    EXPECT_EQ(real.size() + 1, QFileInfo(a).size());
    EXPECT_FALSE(QFileInfo::exists(placeholder(QStringLiteral("Music/House/b.flac"))));
}

TEST_F(ExternalContentTest, nothingRemovedWhenTheDriveCannotBeListed) {
    ASSERT_TRUE(sync().ok);
    m_pProvider->listFails = true;
    ASSERT_TRUE(QFile::remove(m_drive + QStringLiteral("/Music/a.flac")));
    EXPECT_FALSE(sync().ok);
    EXPECT_TRUE(QFileInfo::exists(placeholder(QStringLiteral("Music/a.flac"))));
}

TEST_F(ExternalContentTest, readerReadsTheRealContent) {
    ASSERT_TRUE(sync().ok);
    const QString a = placeholder(QStringLiteral("Music/a.flac"));
    QString linkPath;
    {
        ExternalContent::Reader reader(a);
        EXPECT_TRUE(reader.isPlaceholder());
        ASSERT_FALSE(reader.path().isEmpty());
        EXPECT_TRUE(reader.path().endsWith(QStringLiteral(".flac"))); // decoder by extension
        EXPECT_EQ(readAll(m_drive + QStringLiteral("/Music/a.flac")), readAll(reader.path()));
        linkPath = reader.path();
    }
    EXPECT_FALSE(QFileInfo::exists(linkPath)); // cleaned up

    m_pProvider->mounted = false;
    ExternalContent::Reader offline(a);
    EXPECT_TRUE(offline.path().isEmpty());

    // Other files are read as they are
    ExternalContent::Reader plain(m_drive + QStringLiteral("/notes.txt"));
    EXPECT_FALSE(plain.isPlaceholder());
    EXPECT_EQ(m_drive + QStringLiteral("/notes.txt"), plain.path());
}

TEST_F(ExternalContentTest, decodesAndImportsThroughThePlaceholder) {
    ASSERT_TRUE(sync().ok);
    const QString a = placeholder(QStringLiteral("Music/a.flac"));

    // Metadata comes from the real file (the placeholder has no content)
    const TrackPointer pTrack = getOrAddTrackByLocation(a);
    ASSERT_TRUE(pTrack);
    EXPECT_GT(pTrack->getDuration(), 0.0);

    mixxx::AudioSource::OpenParams params;
    params.setChannelCount(mixxx::audio::ChannelCount::stereo());
    const auto pFromPlaceholder = SoundSourceProxy(pTrack).openAudioSource(params);
    ASSERT_TRUE(pFromPlaceholder);
    const TrackPointer pReal = Track::newTemporary(
            mixxx::FileAccess(mixxx::FileInfo(m_drive + QStringLiteral("/Music/a.flac"))));
    const auto pFromReal = SoundSourceProxy(pReal).openAudioSource(params);
    ASSERT_TRUE(pFromReal);
    EXPECT_EQ(pFromReal->frameIndexRange(), pFromPlaceholder->frameIndexRange());
}

TEST_F(ExternalContentTest, deckCopyAndOfflineWhenUnplugged) {
    ASSERT_TRUE(sync().ok);
    const QString a = placeholder(QStringLiteral("Music/a.flac"));
    EXPECT_EQ(ExternalContent::volumeDirectory(kVolume), LocalTrackCache::driveRoot(a));
    EXPECT_TRUE(LocalTrackCache::shouldCache(a));
    EXPECT_FALSE(djmantra::OnlineStatus::isDriveOffline(a));

    const auto cached = LocalTrackCache::ensureCached(a);
    ASSERT_TRUE(cached.fromCache) << cached.error.toStdString();
    EXPECT_EQ(readAll(m_drive + QStringLiteral("/Music/a.flac")), readAll(cached.readPath));

    // Unplugged: the placeholder is still there, but the drive is offline
    m_pProvider->mounted = false;
    djmantra::OnlineStatus::invalidate();
    EXPECT_TRUE(djmantra::OnlineStatus::isDriveOffline(a));
    EXPECT_TRUE(LocalTrackCache::isOnUnreachableDrive(a, {}));
    EXPECT_EQ(cached.readPath, LocalTrackCache::readPath(a));
    EXPECT_EQ(cached.readPath, LocalTrackCache::ensureCached(a).readPath);

    // Not copied before and unplugged: fails after the reconnect wait
    const QString b = placeholder(QStringLiteral("Music/House/b.flac"));
    EXPECT_FALSE(LocalTrackCache::ensureCached(b).fromCache);
}

TEST_F(ExternalContentTest, rejectsPathsOutsideTheDrive) {
    class Evil : public FakeProvider {
      public:
        using FakeProvider::FakeProvider;
        bool list(const QString&, QList<Entry>* pEntries) override {
            pEntries->append(Entry{QStringLiteral("../../escape.flac"), 10, 0});
            pEntries->append(Entry{QStringLiteral("/abs.flac"), 10, 0});
            return true;
        }
    };
    ExternalContent::setProvider(std::make_shared<Evil>(m_drive), m_mirror);
    const auto result = sync();
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(0, result.added);
    EXPECT_FALSE(QFileInfo::exists(QDir(m_tmp.path()).filePath(QStringLiteral("app/escape.flac"))));
}
