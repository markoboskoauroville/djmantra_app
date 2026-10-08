#include "sources/videocoverimage.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
} // extern "C"

#include <QSet>
#include <algorithm>
#include <cstring>
#include <memory>

#include "util/logger.h"

namespace mixxx {

namespace {

const Logger kLogger("VideoCoverImage");

// Upper bound for decoded frames while looking for a non-black frame.
constexpr int kMaxFramesToInspect = 48;
// Upper bound for packets read while looking for decodable frames.
constexpr int kMaxPacketsToRead = 2000;
// Average brightness (0..255) below which a frame counts as black.
constexpr int kBlackFrameThreshold = 16;

struct FormatContextCloser {
    void operator()(AVFormatContext* p) const {
        avformat_close_input(&p);
    }
};
struct CodecContextFreer {
    void operator()(AVCodecContext* p) const {
        avcodec_free_context(&p);
    }
};
struct FrameFreer {
    void operator()(AVFrame* p) const {
        av_frame_free(&p);
    }
};
struct PacketFreer {
    void operator()(AVPacket* p) const {
        av_packet_free(&p);
    }
};
using FormatContextPtr = std::unique_ptr<AVFormatContext, FormatContextCloser>;
using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextFreer>;
using FramePtr = std::unique_ptr<AVFrame, FrameFreer>;
using PacketPtr = std::unique_ptr<AVPacket, PacketFreer>;

QImage frameToImage(const AVFrame* pFrame, int maxSize) {
    if (pFrame->width <= 0 || pFrame->height <= 0) {
        return {};
    }
    int width = pFrame->width;
    int height = pFrame->height;
    if (width > maxSize || height > maxSize) {
        if (width >= height) {
            height = std::max(1, height * maxSize / width);
            width = maxSize;
        } else {
            width = std::max(1, width * maxSize / height);
            height = maxSize;
        }
    }
    SwsContext* pSws = sws_getContext(pFrame->width,
            pFrame->height,
            static_cast<AVPixelFormat>(pFrame->format),
            width,
            height,
            AV_PIX_FMT_RGB24,
            SWS_BILINEAR,
            nullptr,
            nullptr,
            nullptr);
    if (!pSws) {
        kLogger.warning() << "Unsupported pixel format" << pFrame->format;
        return {};
    }
    // swscale's SIMD code may write past the end of a row, so scale into an
    // aligned, padded buffer and copy the rows into the QImage afterwards.
    uint8_t* dstData[4] = {};
    int dstLinesize[4] = {};
    if (av_image_alloc(dstData, dstLinesize, width, height, AV_PIX_FMT_RGB24, 32) < 0) {
        sws_freeContext(pSws);
        return {};
    }
    sws_scale(pSws,
            pFrame->data,
            pFrame->linesize,
            0,
            pFrame->height,
            dstData,
            dstLinesize);
    sws_freeContext(pSws);
    QImage image(width, height, QImage::Format_RGB888);
    for (int y = 0; y < height; ++y) {
        std::memcpy(image.scanLine(y), dstData[0] + y * dstLinesize[0], width * 3);
    }
    av_freep(&dstData[0]);
    return image;
}

int averageBrightness(const QImage& image) {
    // Sample a coarse grid; exact values don't matter.
    const int stepX = std::max(1, image.width() / 16);
    const int stepY = std::max(1, image.height() / 16);
    qint64 sum = 0;
    int count = 0;
    for (int y = 0; y < image.height(); y += stepY) {
        for (int x = 0; x < image.width(); x += stepX) {
            const QRgb rgb = image.pixel(x, y);
            sum += (qRed(rgb) + qGreen(rgb) + qBlue(rgb)) / 3;
            ++count;
        }
    }
    return count > 0 ? static_cast<int>(sum / count) : 0;
}

CodecContextPtr openDecoder(const AVStream* pStream) {
    const AVCodec* pCodec = avcodec_find_decoder(pStream->codecpar->codec_id);
    if (!pCodec) {
        return {};
    }
    CodecContextPtr pCodecContext(avcodec_alloc_context3(pCodec));
    if (!pCodecContext ||
            avcodec_parameters_to_context(pCodecContext.get(), pStream->codecpar) < 0) {
        return {};
    }
    // A single frame is needed; avoid frame-threading start-up delay.
    pCodecContext->thread_count = 1;
    if (avcodec_open2(pCodecContext.get(), pCodec, nullptr) < 0) {
        return {};
    }
    return pCodecContext;
}

QImage decodeAttachedPicture(const AVStream* pStream, int maxSize) {
    CodecContextPtr pCodecContext = openDecoder(pStream);
    FramePtr pFrame(av_frame_alloc());
    if (!pCodecContext || !pFrame) {
        return {};
    }
    if (avcodec_send_packet(pCodecContext.get(), &pStream->attached_pic) < 0) {
        return {};
    }
    avcodec_send_packet(pCodecContext.get(), nullptr); // flush
    if (avcodec_receive_frame(pCodecContext.get(), pFrame.get()) < 0) {
        return {};
    }
    return frameToImage(pFrame.get(), maxSize);
}

QImage decodeFirstVideoFrame(AVFormatContext* pFormatContext,
        const AVStream* pStream,
        int maxSize) {
    CodecContextPtr pCodecContext = openDecoder(pStream);
    FramePtr pFrame(av_frame_alloc());
    PacketPtr pPacket(av_packet_alloc());
    if (!pCodecContext || !pFrame || !pPacket) {
        return {};
    }
    QImage firstImage;
    int inspectedFrames = 0;
    bool flushing = false;
    for (int packets = 0; packets < kMaxPacketsToRead; ++packets) {
        if (!flushing) {
            const int readResult = av_read_frame(pFormatContext, pPacket.get());
            if (readResult < 0) {
                // End of file: drain the frames still buffered in the decoder.
                flushing = true;
                avcodec_send_packet(pCodecContext.get(), nullptr);
            } else if (pPacket->stream_index != pStream->index) {
                av_packet_unref(pPacket.get());
                continue;
            } else {
                const int sendResult = avcodec_send_packet(pCodecContext.get(), pPacket.get());
                av_packet_unref(pPacket.get());
                if (sendResult < 0 && sendResult != AVERROR(EAGAIN)) {
                    continue;
                }
            }
        }
        while (true) {
            const int receiveResult = avcodec_receive_frame(pCodecContext.get(), pFrame.get());
            if (receiveResult == AVERROR(EAGAIN)) {
                break;
            }
            if (receiveResult < 0) {
                // AVERROR_EOF after flushing, or a decoding error.
                return firstImage;
            }
            QImage image = frameToImage(pFrame.get(), maxSize);
            av_frame_unref(pFrame.get());
            if (image.isNull()) {
                continue;
            }
            if (averageBrightness(image) >= kBlackFrameThreshold) {
                return image;
            }
            if (firstImage.isNull()) {
                firstImage = std::move(image);
            }
            if (++inspectedFrames >= kMaxFramesToInspect) {
                return firstImage;
            }
        }
    }
    return firstImage;
}

} // namespace

bool isVideoFileSuffix(const QString& suffix) {
    static const QSet<QString> kVideoSuffixes = {
            QStringLiteral("3g2"),
            QStringLiteral("3gp"),
            QStringLiteral("avi"),
            QStringLiteral("flv"),
            QStringLiteral("m4v"),
            QStringLiteral("mkv"),
            QStringLiteral("mov"),
            QStringLiteral("mp4"),
            QStringLiteral("webm"),
    };
    return kVideoSuffixes.contains(suffix.toLower());
}

QImage extractVideoCoverImage(const QString& localFilePath, int maxSize) {
    AVFormatContext* pRawFormatContext = nullptr;
    const QByteArray path = localFilePath.toUtf8();
    if (avformat_open_input(&pRawFormatContext, path.constData(), nullptr, nullptr) < 0) {
        return {};
    }
    FormatContextPtr pFormatContext(pRawFormatContext);
    if (avformat_find_stream_info(pFormatContext.get(), nullptr) < 0) {
        return {};
    }

    // 1. An embedded cover picture (e.g. MKV/MP4 cover attachment).
    for (unsigned int i = 0; i < pFormatContext->nb_streams; ++i) {
        const AVStream* pStream = pFormatContext->streams[i];
        if ((pStream->disposition & AV_DISPOSITION_ATTACHED_PIC) &&
                pStream->attached_pic.size > 0) {
            QImage image = decodeAttachedPicture(pStream, maxSize);
            if (!image.isNull()) {
                return image;
            }
        }
    }

    // 2. A frame from the main video track.
    const int streamIndex = av_find_best_stream(
            pFormatContext.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (streamIndex < 0) {
        return {}; // audio-only file
    }
    const AVStream* pStream = pFormatContext->streams[streamIndex];
    if (pStream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
        return {}; // already tried above
    }
    QImage image = decodeFirstVideoFrame(pFormatContext.get(), pStream, maxSize);
    if (image.isNull()) {
        kLogger.info() << "No decodable video frame in" << localFilePath;
    }
    return image;
}

} // namespace mixxx
