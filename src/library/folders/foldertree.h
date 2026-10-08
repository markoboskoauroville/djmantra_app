#pragma once

#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

namespace djmantra {

/// A folder in the "Folders" sidebar tree.
struct FolderNode {
    QString path;
    QString label;
    /// Tracks in this folder and all its subfolders.
    int trackCount = 0;
    QList<FolderNode> children;
};

/// Build the folder tree for the library folders (`roots`) from the folders
/// the library has tracks in (`trackDirs`: folder, number of tracks).
///
/// The tree comes from the database, not from the file system, so it is the
/// same whether a drive is plugged in or not. Folders without tracks (at any
/// depth) are left out; the roots are always there. Children are sorted by
/// name, case-insensitively.
QList<FolderNode> buildFolderTree(const QStringList& roots,
        const QList<QPair<QString, int>>& trackDirs);

/// SQL condition for "in this folder or a subfolder of it", on the column
/// `directoryColumn`. The folder is embedded as an escaped string literal.
QString folderCondition(const QString& directoryColumn, const QString& folder);

} // namespace djmantra
