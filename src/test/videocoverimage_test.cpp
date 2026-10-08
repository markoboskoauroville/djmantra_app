// DJ Mantra: playing video files by their audio track, with a video frame
// as cover art. Test media in id3-test-data/ (see README there):
//   video-cover-test.mkv/.mp4  2 s, solid red 160x90 H.264 + 44.1 kHz AAC
//   video-fade-test.webm       1 s black then 1 s green VP9 + 48 kHz Opus

#ifdef __VIDEO_COVER_ART__

#include "sources/videocoverimage.h"

#include <QImage>

#include "library/coverartutils.h"
#include "sources/audiosourcestereoproxy.h"
#include "sources/soundsourceproxy.h"
#include "test/librarytest.h"
#include "track/track.h"
#include "util/samplebuffer.h"

namespace {

bool isRed(QRgb rgb) {
    return qRed(rgb) > 200 && qGreen(rgb) < 60 && qBlue(rgb) < 60;
}

bool isGreen(QRgb rgb) {
    return qGreen(rgb) > 100 && qRed(rgb) < 60 && qBlue(rgb) < 60;
}

} // namespace

// LibraryTest provides the GlobalTrackCache and the registered SoundSources.
class VideoCoverImageTest : public LibraryTest {
  protected:
    QString testFile(const QString& name) const {
        return getTestDir().filePath(QStringLiteral("id3-test-data/") + name);
    }

    // Decode the whole file and return the number of frames read.
    SINT decodeAllFrames(const QString& filePath,
            mixxx::audio::SampleRate expectedSampleRate) {
        auto pTrack = Track::newTemporary(filePath);
        SoundSourceProxy proxy(pTrack);
        mixxx::AudioSource::OpenParams openParams;
        openParams.setChannelCount(mixxx::audio::ChannelCount(2));
        auto pAudioSource = proxy.openAudioSource(openParams);
        if (!pAudioSource) {
            ADD_FAILURE() << "Failed to open" << filePath.toStdString();
            return 0;
        }
        EXPECT_EQ(expectedSampleRate, pAudioSource->getSignalInfo().getSampleRate());
        EXPECT_EQ(mixxx::audio::ChannelCount(2),
                pAudioSource->getSignalInfo().getChannelCount());

        constexpr SINT kChunkFrames = 4096;
        mixxx::SampleBuffer buffer(
                pAudioSource->getSignalInfo().frames2samples(kChunkFrames));
        SINT framesRead = 0;
        auto frameIndex = pAudioSource->frameIndexMin();
        while (frameIndex < pAudioSource->frameIndexMax()) {
            const auto range = mixxx::IndexRange::forward(frameIndex,
                    std::min(kChunkFrames, pAudioSource->frameIndexMax() - frameIndex));
            const auto readRange = pAudioSource->readSampleFrames(
                                                       mixxx::WritableSampleFrames(range,
                                                               mixxx::SampleBuffer::WritableSlice(
                                                                       buffer)))
                                           .frameIndexRange();
            if (readRange.empty()) {
                break;
            }
            framesRead += readRange.length();
            frameIndex = readRange.end();
        }
        return framesRead;
    }
};

TEST_F(VideoCoverImageTest, videoFileSuffixes) {
    for (const auto& suffix : {"mp4", "MP4", "m4v", "mov", "mkv", "webm", "avi", "flv", "3gp"}) {
        EXPECT_TRUE(mixxx::isVideoFileSuffix(QString::fromLatin1(suffix))) << suffix;
    }
    for (const auto& suffix : {"mp3", "m4a", "mka", "flac", "wav", "ogg", "opus", ""}) {
        EXPECT_FALSE(mixxx::isVideoFileSuffix(QString::fromLatin1(suffix))) << suffix;
    }
}

TEST_F(VideoCoverImageTest, videoFileTypesAreSupported) {
    for (const auto& suffix : {"mp4", "m4v", "mov", "mkv", "mka", "webm", "avi", "flv"}) {
        EXPECT_TRUE(SoundSourceProxy::isFileSuffixSupported(QString::fromLatin1(suffix)))
                << suffix;
    }
}

TEST_F(VideoCoverImageTest, firstFrameOfMkv) {
    const QImage image = mixxx::extractVideoCoverImage(testFile("video-cover-test.mkv"));
    ASSERT_FALSE(image.isNull());
    EXPECT_EQ(QSize(160, 90), image.size());
    EXPECT_TRUE(isRed(image.pixel(80, 45)));
}

TEST_F(VideoCoverImageTest, firstFrameOfMp4) {
    const QImage image = mixxx::extractVideoCoverImage(testFile("video-cover-test.mp4"));
    ASSERT_FALSE(image.isNull());
    EXPECT_EQ(QSize(160, 90), image.size());
    EXPECT_TRUE(isRed(image.pixel(80, 45)));
}

TEST_F(VideoCoverImageTest, skipsBlackFadeIn) {
    const QImage image = mixxx::extractVideoCoverImage(testFile("video-fade-test.webm"));
    ASSERT_FALSE(image.isNull());
    EXPECT_TRUE(isGreen(image.pixel(80, 45)));
}

TEST_F(VideoCoverImageTest, scalesDownLargeFrames) {
    const QImage image = mixxx::extractVideoCoverImage(testFile("video-cover-test.mkv"), 64);
    ASSERT_FALSE(image.isNull());
    EXPECT_EQ(QSize(64, 36), image.size());
}

TEST_F(VideoCoverImageTest, attachedPictureIsPreferred) {
    // cover-test.wav carries reference_cover.png as an attached picture.
    const QImage image = mixxx::extractVideoCoverImage(testFile("cover-test.wav"));
    ASSERT_FALSE(image.isNull());
    const QImage reference = QImage(testFile("reference_cover.png"))
                                     .convertToFormat(QImage::Format_RGB888);
    EXPECT_EQ(reference, image);
}

TEST_F(VideoCoverImageTest, noImageForAudioOnlyOrMissingFiles) {
    EXPECT_TRUE(mixxx::extractVideoCoverImage(testFile("artist.mp3")).isNull());
    EXPECT_TRUE(mixxx::extractVideoCoverImage(testFile("does-not-exist.mkv")).isNull());
}

TEST_F(VideoCoverImageTest, coverArtOfVideoFilesComesFromVideoTrack) {
    // Through the same path the library uses when adding tracks.
    for (const auto& name : {"video-cover-test.mkv", "video-cover-test.mp4"}) {
        const QImage image = CoverArtUtils::extractEmbeddedCover(
                mixxx::FileAccess(mixxx::FileInfo(testFile(QString::fromLatin1(name)))));
        ASSERT_FALSE(image.isNull()) << name;
        EXPECT_TRUE(isRed(image.pixel(image.width() / 2, image.height() / 2))) << name;
    }
}

TEST_F(VideoCoverImageTest, audioTrackOfVideoFilesPlays) {
    // 2 s of audio; allow for encoder priming/padding.
    const SINT mkvFrames = decodeAllFrames(
            testFile("video-cover-test.mkv"), mixxx::audio::SampleRate(44100));
    EXPECT_NEAR(2 * 44100, mkvFrames, 4096);
    const SINT mp4Frames = decodeAllFrames(
            testFile("video-cover-test.mp4"), mixxx::audio::SampleRate(44100));
    EXPECT_NEAR(2 * 44100, mp4Frames, 4096);
    const SINT webmFrames = decodeAllFrames(
            testFile("video-fade-test.webm"), mixxx::audio::SampleRate(48000));
    EXPECT_NEAR(2 * 48000, webmFrames, 4096);
}

#endif // __VIDEO_COVER_ART__
