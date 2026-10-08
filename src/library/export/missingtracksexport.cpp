#include "library/export/missingtracksexport.h"

#include <QFile>
#include <QSaveFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>

#include "util/logger.h"

namespace djmantra {

namespace {

const mixxx::Logger kLogger("MissingTracksExport");

constexpr Store kStores[] = {Store::Tidal, Store::Bandcamp, Store::Beatport, Store::Qobuz};

QString searchTerms(const MissingTrack& track) {
    return QStringList{track.artist.trimmed(), track.title.trimmed()}.join(QChar(' ')).trimmed();
}

QString formatDuration(int seconds) {
    if (seconds <= 0) {
        return {};
    }
    return QStringLiteral("%1:%2")
            .arg(seconds / 60)
            .arg(seconds % 60, 2, 10, QChar('0'));
}

QString displayName(const MissingTrack& track) {
    QString name = track.artist.trimmed();
    if (!name.isEmpty() && !track.title.trimmed().isEmpty()) {
        name += QStringLiteral(" – ");
    }
    name += track.title.trimmed();
    if (name.isEmpty()) {
        name = QStringLiteral("(unknown)");
    }
    if (!track.album.trimmed().isEmpty()) {
        name += QStringLiteral(" (%1)").arg(track.album.trimmed());
    }
    const QString duration = formatDuration(track.durationSeconds);
    if (!duration.isEmpty()) {
        name += QStringLiteral(" [%1]").arg(duration);
    }
    return name;
}

} // namespace

QString storeName(Store store) {
    switch (store) {
    case Store::Bandcamp:
        return QStringLiteral("Bandcamp");
    case Store::Beatport:
        return QStringLiteral("Beatport");
    case Store::Qobuz:
        return QStringLiteral("Qobuz");
    case Store::Tidal:
        return QStringLiteral("TIDAL");
    }
    return {};
}

QUrl storeSearchUrl(Store store, const MissingTrack& track) {
    QUrl url;
    switch (store) {
    case Store::Bandcamp:
        url = QUrl(QStringLiteral("https://bandcamp.com/search"));
        break;
    case Store::Beatport:
        url = QUrl(QStringLiteral("https://www.beatport.com/search"));
        break;
    case Store::Qobuz:
        // German store (downloads in up to 24-bit)
        url = QUrl(QStringLiteral("https://www.qobuz.com/de-de/search"));
        break;
    case Store::Tidal:
        url = QUrl(QStringLiteral("https://listen.tidal.com/search"));
        break;
    }
    // Fully percent-encode so that "&", "#", "+" etc. in names survive.
    url.setQuery(QStringLiteral("q=") +
                    QString::fromLatin1(QUrl::toPercentEncoding(searchTerms(track))),
            QUrl::StrictMode);
    return url;
}

QString formatMissingTracks(const QList<MissingTrack>& tracks,
        const QString& listName,
        const QDateTime& exportedAt) {
    QString text;
    QTextStream out(&text);
    out << "DJ Mantra – missing songs\n";
    out << "List: " << listName << '\n';
    out << "Exported: " << exportedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm")) << '\n';
    out << "Songs: " << tracks.size() << "\n\n";
    int number = 0;
    for (const auto& track : tracks) {
        out << ++number << ". " << displayName(track) << '\n';
        if (!track.isrc.trimmed().isEmpty()) {
            out << "   ISRC: " << track.isrc.trimmed() << '\n';
        }
        if (!track.lastKnownLocation.isEmpty()) {
            out << "   Was at: " << track.lastKnownLocation << '\n';
        }
        if (track.sourceUrl.isValid() && !track.sourceUrl.isEmpty()) {
            out << "   Link: " << track.sourceUrl.toString(QUrl::FullyEncoded) << '\n';
        }
        if (!searchTerms(track).isEmpty()) {
            for (const auto store : kStores) {
                out << "   " << storeName(store) << " search: "
                    << storeSearchUrl(store, track).toString(QUrl::FullyEncoded) << '\n';
            }
        }
        out << '\n';
    }
    out.flush();
    return text;
}

bool writeMissingTracksFile(const QString& filePath,
        const QList<MissingTrack>& tracks,
        const QString& listName,
        QString* pErrorMessage) {
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (pErrorMessage) {
            *pErrorMessage = file.errorString();
        }
        return false;
    }
    file.write(formatMissingTracks(tracks, listName).toUtf8());
    if (!file.commit()) {
        if (pErrorMessage) {
            *pErrorMessage = file.errorString();
        }
        return false;
    }
    return true;
}

QList<MissingTrack> queryLibraryMissingTracks(const QSqlDatabase& database) {
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
            "SELECT library.artist, library.title, library.album, "
            "library.duration, track_locations.location "
            "FROM library "
            "INNER JOIN track_locations ON library.location=track_locations.id "
            "WHERE library.mixxx_deleted=0 AND track_locations.fs_deleted=1 "
            "ORDER BY library.artist COLLATE NOCASE, library.title COLLATE NOCASE"));
    QList<MissingTrack> tracks;
    if (!query.exec()) {
        kLogger.warning() << "Failed to query missing tracks:" << query.lastError();
        return tracks;
    }
    while (query.next()) {
        MissingTrack track;
        track.artist = query.value(0).toString();
        track.title = query.value(1).toString();
        track.album = query.value(2).toString();
        track.durationSeconds = qRound(query.value(3).toDouble());
        track.lastKnownLocation = query.value(4).toString();
        tracks.append(std::move(track));
    }
    return tracks;
}

} // namespace djmantra
