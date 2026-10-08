#pragma once

#include <QModelIndex>
#include <QPointer>

#include "library/folders/foldertablemodel.h"
#include "library/libraryfeature.h"

class TreeItemModel;
class WLibrarySidebar;
class WLibraryTextBrowser;

/// DJ Mantra: "Folders" in the sidebar, like djay's folder browser.
///
/// "+ Add folder" adds a folder to the library and scans it. The folder and
/// its subfolders then show up as a tree; clicking a folder lists all tracks
/// in it and its subfolders, like a playlist that fills itself.
///
/// A folder stays until it is removed explicitly: the tree is built from the
/// library, not from the disk, so a folder on an unplugged drive is still
/// there, marked offline, and its tracks are marked offline (red dot) until
/// the drive is back.
class FolderFeature final : public LibraryFeature {
    Q_OBJECT
  public:
    FolderFeature(Library* pLibrary, UserSettingsPointer pConfig);
    ~FolderFeature() override = default;

    QVariant title() override;
    TreeItemModel* sidebarModel() const override;
    void bindLibraryWidget(WLibrary* pLibraryWidget, KeyboardEventFilter* pKeyboard) override;
    void bindSidebarWidget(WLibrarySidebar* pSidebarWidget) override;

  public slots:
    void activate() override;
    void activateChild(const QModelIndex& index) override;
    void onRightClick(const QPoint& globalPos) override;
    void onRightClickChild(const QPoint& globalPos, const QModelIndex& index) override;

    void slotAddFolder();
    /// Rebuild the tree (after a scan, or a drive was plugged in or out).
    void slotRefresh();

  private slots:
    void slotLinkClicked(const QUrl& link);

  private:
    QString formatRootViewHtml() const;
    QString folderOf(const QModelIndex& index) const;
    /// Folders open in the sidebar, to keep them open across a rebuild.
    QStringList expandedFolders(const QModelIndex& parent = QModelIndex()) const;
    QModelIndex indexOfFolder(const QString& folder,
            const QModelIndex& parent = QModelIndex()) const;
    void removeFolder(const QString& folder);

    TreeItemModel* m_pSidebarModel;
    FolderTableModel m_tableModel;
    QPointer<WLibraryTextBrowser> m_pRootView;
    QPointer<WLibrarySidebar> m_pSidebarWidget;
    QString m_activeFolder;
};
