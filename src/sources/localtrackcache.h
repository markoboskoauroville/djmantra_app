#pragma once

#include <QString>
#include <QStringList>
#include <chrono>
#include <functional>

namespace djmantra {

/// Copies of tracks that live on external drives (USB stick, SD card, USB disk),
/// kept on internal storage.
///
/// A deck reads a track only from its local copy. The copy is made completely
/// before the track is played, so pulling the drive out, a loose cable or a
/// drive that sleeps for a moment cannot interrupt playback. If the drive
/// drops out while the copy is being made, copying waits for it to come back
/// and continues where it stopped.
///
/// A copy is used as long as the original file is unchanged (same size and
/// modification time) or the original is unreachable. Least recently used
/// copies are deleted when the cache is full.
///
/// All functions are thread-safe. The cache is disabled until configure() is
/// called with a directory.
class LocalTrackCache {
  public:
    enum class Mode {
        Off,
        /// Only files on external drives (see Settings::removablePrefixes)
        RemovableOnly,
        All,
    };

    struct Settings {
        QString directory;
        Mode mode = Mode::RemovableOnly;
        qint64 maxBytes = qint64{8} * 1024 * 1024 * 1024;
        /// Path prefixes of external drives. A drive is the first directory
        /// below the prefix, e.g. /storage/1A2B-3C4D/ for /storage/.
        QStringList removablePrefixes = defaultRemovablePrefixes();
        /// Paths below these prefixes are never cached (internal storage).
        QStringList excludedPrefixes = defaultExcludedPrefixes();
        /// How long to wait for a drive that dropped out.
        std::chrono::milliseconds reconnectTimeout{30000};
        std::chrono::milliseconds pollInterval{250};
        /// Space left free on internal storage.
        qint64 reserveBytes = qint64{300} * 1024 * 1024;
    };

    struct Result {
        /// File to read the audio from: the local copy, or the original if
        /// the file is not cached.
        QString readPath;
        bool fromCache = false;
        /// Why the file is not cached (empty if it is, or need not be).
        QString error;
    };

    static QStringList defaultRemovablePrefixes();
    static QStringList defaultExcludedPrefixes();

    static void configure(const Settings& settings);
    static Settings settings();
    static bool isEnabled();

    /// True if the file is on a drive that is cached in this mode.
    static bool shouldCache(const QString& path);

    /// The drive directory of a path on an external drive, e.g.
    /// /storage/1A2B-3C4D for /storage/1A2B-3C4D/Music/a.mp3; empty otherwise.
    static QString driveRoot(const QString& path);

    /// The drive is there: its directory exists, or for placeholders (see
    /// ExternalContent) the volume is mounted.
    static bool isDriveReachable(const QString& driveRoot);

    /// True if `path` is on a drive (or below a library root directory) that
    /// is not reachable right now: unplugged, or not mounted yet.
    static bool isOnUnreachableDrive(const QString& path, const QStringList& libraryRoots);

    /// Where the copy of `path` is (or would be) stored.
    static QString copyPath(const QString& path);

    /// The valid local copy of `path` if there is one, otherwise `path`.
    /// Never copies; cheap enough to call from any thread.
    static QString readPath(const QString& path);

    /// Make sure the file is copied. Blocks while copying, and while waiting
    /// for a drive that dropped out (up to reconnectTimeout). Call it from a
    /// worker thread, never from the GUI or audio thread.
    static Result ensureCached(const QString& path,
            const std::function<bool()>& isCancelled = {});

    /// Total size of all copies.
    static qint64 usedBytes();

    /// Delete all copies.
    static void clear();
};

} // namespace djmantra
