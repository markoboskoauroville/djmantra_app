#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <memory>

namespace djmantra {

/// Drives that apps cannot read through file paths: on Android a USB stick
/// is mounted where only the system can read it, and apps reach it through
/// the Storage Access Framework (a "tree" the user grants once).
///
/// The library works on file paths. So every audio file on such a drive gets
/// a placeholder below the mirror directory, at
///   <mirror>/<volume id>/<path on the drive>
/// with the same name, size and modification time but no content (a sparse
/// file). Library scans, the Folders tree, sorting and online/offline marks
/// all work on the placeholders unchanged. Whenever the content is needed
/// (tags, analysis, the deck's local copy), it is opened through the
/// provider and read as /proc/self/fd/<n>.
///
/// On desktop systems there is no provider and nothing changes.
class ExternalContentProvider {
  public:
    struct Volume {
        QString id;    // e.g. 6AB3-73C5
        QString label; // e.g. "SanDisk USB"
        bool mounted = false;
        bool granted = false; // the user granted a tree on it
    };
    struct Entry {
        QString relativePath; // from the drive's root, '/' separated
        qint64 size = 0;
        qint64 modifiedMs = 0;
    };

    virtual ~ExternalContentProvider() = default;

    virtual QList<Volume> volumes() = 0;
    /// All files below the granted trees of the volume. False if the drive
    /// could not be read completely (then nothing is removed).
    virtual bool list(const QString& volumeId, QList<Entry>* pEntries) = 0;
    /// A file descriptor for reading, owned by the caller; -1 on failure.
    virtual int openForReading(const QString& volumeId, const QString& relativePath) = 0;
};

class ExternalContent {
  public:
    static void setProvider(std::shared_ptr<ExternalContentProvider> pProvider,
            const QString& mirrorDirectory);
    static bool isEnabled();
    static QString mirrorDirectory();

    /// The placeholder directory of a volume: <mirror>/<volume id>
    static QString volumeDirectory(const QString& volumeId);
    /// `path` is a placeholder (below the mirror directory).
    static bool isPlaceholder(const QString& path);
    /// Volume id and path on the drive of a placeholder.
    static bool splitPlaceholder(const QString& path, QString* pVolumeId, QString* pRelativePath);

    static QList<ExternalContentProvider::Volume> volumes();
    static bool isVolumeMounted(const QString& volumeId);
    /// The volume of a directory below the mirror (<mirror>/<id>[/...]).
    static QString volumeIdOfDirectory(const QString& directory);
    /// Placeholder paths shown with the drive's name: "SanDisk USB/Music".
    static QString displayPath(const QString& path);

    struct SyncResult {
        bool ok = false;
        int added = 0;
        int updated = 0;
        int removed = 0;
    };
    /// Bring the placeholders of a volume up to date with the drive. Only
    /// audio/video files (by extension) get placeholders. If the drive cannot
    /// be listed completely, nothing is removed.
    static SyncResult syncVolume(const QString& volumeId, const QStringList& fileSuffixes);

    /// Opens a placeholder's content for reading while it lives.
    /// path() is a readable path: a link to /proc/self/fd/<n> with the file's
    /// extension for a placeholder whose drive is there, the path itself for
    /// any other file, empty when the placeholder's drive cannot be read.
    class Reader {
      public:
        explicit Reader(const QString& path);
        ~Reader();
        Reader(const Reader&) = delete;
        Reader& operator=(const Reader&) = delete;

        const QString& path() const {
            return m_path;
        }
        bool isPlaceholder() const {
            return m_isPlaceholder;
        }

      private:
        int m_fd = -1;
        bool m_isPlaceholder = false;
        QString m_path;
        QString m_link;
    };
};

} // namespace djmantra
