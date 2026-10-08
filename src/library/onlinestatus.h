#pragma once

#include <QString>

namespace djmantra {

/// Online/offline state of track files, like a video editor shows it for
/// media: a file is offline when its drive is not connected (or the file
/// was not found by the last scan). Offline tracks stay in the library and
/// are online again as soon as their drive is back.
class OnlineStatus {
  public:
    /// The drive of `location` is not reachable. Cheap enough to call while
    /// painting: drive checks are cached for a second.
    static bool isDriveOffline(const QString& location);

    /// Forget the cached drive checks (a drive was plugged in or out).
    static void invalidate();
};

} // namespace djmantra
