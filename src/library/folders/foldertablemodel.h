#pragma once

#include "library/basesqltablemodel.h"

/// DJ Mantra: the tracks of a library folder and all its subfolders, like a
/// playlist that fills itself. Offline tracks (drive not connected, or not
/// found by the last scan) are listed too, with an offline mark.
class FolderTableModel final : public BaseSqlTableModel {
    Q_OBJECT
  public:
    FolderTableModel(QObject* pParent, TrackCollectionManager* pTrackCollectionManager);
    ~FolderTableModel() final = default;

    void selectFolder(const QString& folder);
    const QString& folder() const {
        return m_folder;
    }

    bool isColumnInternal(int column) final;
    Capabilities getCapabilities() const final;
    QString modelKey(bool noSearch) const final;
    bool showsOnlineStatus() const final {
        return true;
    }

  private:
    QString m_folder;
};
