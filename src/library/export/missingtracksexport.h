#pragma once

#include <QDateTime>
#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QUrl>

namespace djmantra {

/// A song the user does not have as a playable file: either a library track
/// whose file was moved or deleted, or a streaming-playlist track without a
/// matching local file.
struct MissingTrack {
    QString artist;
    QString title;
    QString album;
    QString isrc;
    int durationSeconds = 0;
    /// Link to the song on the service it came from (e.g. a TIDAL track page).
    QUrl sourceUrl;
    /// Last known file location for library tracks.
    QString lastKnownLocation;
};

/// Stores whose search pages are linked for every missing song.
enum class Store {
    Bandcamp,
    Beatport,
    Qobuz,
    Tidal,
};

QString storeName(Store store);

/// Search page on the store for "artist title".
QUrl storeSearchUrl(Store store, const MissingTrack& track);

/// Human-readable list with links, one block per song.
QString formatMissingTracks(const QList<MissingTrack>& tracks,
        const QString& listName,
        const QDateTime& exportedAt = QDateTime::currentDateTime());

/// Write formatMissingTracks() to a UTF-8 text file.
bool writeMissingTracksFile(const QString& filePath,
        const QList<MissingTrack>& tracks,
        const QString& listName,
        QString* pErrorMessage = nullptr);

/// Library tracks whose files are missing (the "Missing Tracks" view),
/// sorted by artist and title.
QList<MissingTrack> queryLibraryMissingTracks(const QSqlDatabase& database);

} // namespace djmantra
