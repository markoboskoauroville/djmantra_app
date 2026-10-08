#pragma once

#include <QImage>
#include <QString>

namespace mixxx {

/// File suffixes (lower case, without dot) of video container formats.
/// For these files only the audio track is played, and a frame of the
/// video track can serve as cover art.
bool isVideoFileSuffix(const QString& suffix);

/// Extract a cover image from the video track of a media file.
///
/// Uses the attached picture stream (embedded cover) if the container has
/// one. Otherwise the first video frames are decoded and the first frame
/// that is not (almost) black is returned, so a fade-in from black does
/// not end up as cover art. If all of those frames are dark, the very
/// first frame is returned.
///
/// The image is scaled down, preserving the aspect ratio, so that neither
/// side exceeds maxSize pixels.
///
/// Returns a null image if the file has no video track or decoding fails.
/// This function is thread-safe.
QImage extractVideoCoverImage(const QString& localFilePath, int maxSize = 1024);

} // namespace mixxx
