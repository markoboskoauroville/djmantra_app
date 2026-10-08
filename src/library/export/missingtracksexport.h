#pragma once

#include <QDateTime>
#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QUrl>

#include "track/trackid.h"

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
    /// File of a track that is present (marked-tracks export).
    QString fileLocation;
    /// Star rating 1–5, 0 = not rated.
    int rating = 0;
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

/// Same layout for the tracks marked for export ("Export" column).
QString formatMarkedTracks(const QList<MissingTrack>& tracks,
        const QString& listName,
        const QDateTime& exportedAt = QDateTime::currentDateTime());

/// Write formatMissingTracks() to a UTF-8 text file.
bool writeMissingTracksFile(const QString& filePath,
        const QList<MissingTrack>& tracks,
        const QString& listName,
        QString* pErrorMessage = nullptr);

/// Write formatMarkedTracks() to a UTF-8 text file.
bool writeMarkedTracksFile(const QString& filePath,
        const QList<MissingTrack>& tracks,
        const QString& listName,
        QString* pErrorMessage = nullptr);

/// Library tracks whose files are missing (the "Missing Tracks" view),
/// sorted by artist and title.
QList<MissingTrack> queryLibraryMissingTracks(const QSqlDatabase& database);

/// Library tracks with the export mark set, sorted by artist and title.
/// Reads the database: save tracks that are loaded in decks first.
QList<MissingTrack> queryLibraryMarkedTracks(const QSqlDatabase& database);

/// Ids of the library tracks with the export mark set.
QList<TrackId> queryLibraryMarkedTrackIds(const QSqlDatabase& database);

} // namespace djmantra
