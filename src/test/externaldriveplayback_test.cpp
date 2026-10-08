#include <gtest/gtest.h>

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtMath>

#include "sources/localtrackcache.h"
#include "test/signalpathtest.h"

using djmantra::LocalTrackCache;

namespace {

/// 16-bit stereo sine WAV
void writeSineWav(const QString& path, int seconds) {
    constexpr int kRate = 44100;
    const quint32 frames = static_cast<quint32>(kRate * seconds);
    const quint32 dataBytes = frames * 4;
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("RIFF", 4);
    out << quint32(36 + dataBytes);
    out.writeRawData("WAVEfmt ", 8);
    out << quint32(16) << quint16(1) << quint16(2) << quint32(kRate) << quint32(kRate * 4)
        << quint16(4) << quint16(16);
    out.writeRawData("data", 4);
    out << dataBytes;
    for (quint32 i = 0; i < frames; ++i) {
        const auto sample = static_cast<qint16>(12000 * qSin(2 * M_PI * 440.0 * i / kRate));
        out << sample << sample;
    }
}

bool isSilent(const CSAMPLE* pBuffer, int size) {
    for (int i = 0; i < size; ++i) {
        if (std::abs(pBuffer[i]) > 0.001f) {
            return false;
        }
    }
    return true;
}

} // namespace

class ExternalDrivePlaybackTest : public BaseSignalPathTest {
  protected:
    void SetUp() override {
        ASSERT_TRUE(m_tmp.isValid());
        m_drives = QDir(m_tmp.path()).filePath(QStringLiteral("drives"));
        const QString drive = QDir(m_drives).filePath(QStringLiteral("USB1"));
        ASSERT_TRUE(QDir().mkpath(drive));
        m_song = QDir(drive).filePath(QStringLiteral("sine.wav"));
        writeSineWav(m_song, 20);
    }
    void TearDown() override {
        LocalTrackCache::configure(LocalTrackCache::Settings{QString(), LocalTrackCache::Mode::Off});
    }

    void configureCache(LocalTrackCache::Mode mode) {
        LocalTrackCache::Settings settings;
        settings.directory = QDir(m_tmp.path()).filePath(QStringLiteral("cache"));
        settings.mode = mode;
        settings.removablePrefixes = {m_drives + QChar('/')};
        settings.excludedPrefixes = {};
        settings.reserveBytes = 0;
        LocalTrackCache::configure(settings);
    }

    /// Load, play 1 s, make the original unreadable (as if the drive was
    /// pulled out), play on. Returns the share of non-silent buffers after.
    double playThroughDropout() {
        loadTrack(m_pMixerDeck1, Track::newTemporary(m_song));
        ControlObject::set(ConfigKey(m_sGroup1, "play"), 1.0);
        auto playBuffers = [this](int count) {
            int audible = 0;
            for (int i = 0; i < count; ++i) {
                ProcessBuffer();
                // Let the reader thread fetch the next chunks
                QTest::qSleep(1);
                if (!isSilent(m_pEngineMixer->getMainBuffer(), kProcessBufferSize)) {
                    ++audible;
                }
            }
            return audible;
        };
        playBuffers(100);
        {
            // The drive is gone: the file cannot be read any more
            QFile file(m_song);
            EXPECT_TRUE(file.resize(0));
        }
        const int count = 600; // several seconds, far beyond the read-ahead
        return static_cast<double>(playBuffers(count)) / count;
    }

    QTemporaryDir m_tmp;
    QString m_drives;
    QString m_song;
};

TEST_F(ExternalDrivePlaybackTest, keepsPlayingWhenTheDriveIsPulledOut) {
    configureCache(LocalTrackCache::Mode::RemovableOnly);
    EXPECT_GT(playThroughDropout(), 0.95);
}

TEST_F(ExternalDrivePlaybackTest, withoutCacheTheDropoutIsAudible) {
    // Shows that the test above really tests the cache
    configureCache(LocalTrackCache::Mode::Off);
    EXPECT_LT(playThroughDropout(), 0.5);
}
