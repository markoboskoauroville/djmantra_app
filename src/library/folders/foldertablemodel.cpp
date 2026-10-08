#include "library/folders/foldertablemodel.h"

#include <QCryptographicHash>
#include <QSqlError>
#include <QSqlQuery>

#include "library/dao/trackschema.h"
#include "library/folders/foldertree.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "mixer/playerinfo.h"
#include "moc_foldertablemodel.cpp"

namespace {

const QString kModelName = QStringLiteral("folder:");

} // namespace

FolderTableModel::FolderTableModel(
        QObject* pParent, TrackCollectionManager* pTrackCollectionManager)
        : BaseSqlTableModel(pParent, pTrackCollectionManager, "mixxx.db.model.folder") {
}

void FolderTableModel::selectFolder(const QString& folder) {
    m_folder = folder;
    // One view per folder, like playlists
    const QString tableName = QStringLiteral("folder_") +
            QString::fromLatin1(QCryptographicHash::hash(folder.toUtf8(),
                    QCryptographicHash::Sha1)
                                        .toHex()
                                        .left(16));
    QSqlQuery query(m_database);
    const QString statement =
            QStringLiteral(
                    "CREATE TEMPORARY VIEW IF NOT EXISTS %1 AS "
                    "SELECT library.%2 FROM library "
                    "INNER JOIN track_locations ON library.location=track_locations.id "
                    "WHERE library.mixxx_deleted=0 AND %3")
                    .arg(tableName,
                            LIBRARYTABLE_ID,
                            djmantra::folderCondition(
                                    QStringLiteral("track_locations.directory"), folder));
    if (!query.exec(statement)) {
        qWarning() << "FolderTableModel: cannot create view" << query.lastError();
        return;
    }
    setTable(tableName,
            LIBRARYTABLE_ID,
            QStringList{LIBRARYTABLE_ID},
            m_pTrackCollectionManager->internalCollection()->getTrackSource());
    // Folder order: by file location, like a file browser
    setDefaultSort(fieldIndex(ColumnCache::COLUMN_TRACKLOCATIONSTABLE_LOCATION),
            Qt::AscendingOrder);
    setSearch(QString());
    select();
}

bool FolderTableModel::isColumnInternal(int column) {
    return column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_ID) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_URL) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_CUEPOINT) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_WAVESUMMARYHEX) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_SAMPLERATE) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_MIXXXDELETED) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_HEADERPARSED) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PLAYED) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_KEY_ID) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_BPM_LOCK) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_CHANNELS) ||
            column == fieldIndex(ColumnCache::COLUMN_TRACKLOCATIONSTABLE_FSDELETED) ||
            (PlayerInfo::instance().numPreviewDecks() == 0 &&
                    column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_PREVIEW)) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_SOURCE) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_TYPE) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_LOCATION) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_COLOR) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_DIGEST) ||
            column == fieldIndex(ColumnCache::COLUMN_LIBRARYTABLE_COVERART_HASH);
}

TrackModel::Capabilities FolderTableModel::getCapabilities() const {
    return Capability::AddToTrackSet |
            Capability::AddToAutoDJ |
            Capability::EditMetadata |
            Capability::LoadToDeck |
            Capability::LoadToSampler |
            Capability::LoadToPreviewDeck |
            Capability::Hide |
            Capability::ResetPlayed |
            Capability::Analyze |
            Capability::Properties |
            Capability::Sorting;
}

QString FolderTableModel::modelKey(bool noSearch) const {
    if (noSearch) {
        return kModelName + m_folder;
    }
    return kModelName + m_folder + QStringLiteral("#") + currentSearch();
}
