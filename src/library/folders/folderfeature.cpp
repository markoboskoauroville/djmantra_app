#include "library/folders/folderfeature.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QSqlQuery>
#include <QStandardPaths>

#include "library/folders/foldertree.h"
#include "library/library.h"
#include "library/onlinestatus.h"
#include "library/sidebarmodel.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "library/treeitem.h"
#include "library/treeitemmodel.h"
#include "moc_folderfeature.cpp"
#include "sources/localtrackcache.h"
#include "widget/wlibrary.h"
#include "widget/wlibrarysidebar.h"
#include "widget/wlibrarytextbrowser.h"

namespace {

const QString kRootViewName = QStringLiteral("FOLDERS_HOME");
const QString kOfflineIcon = QStringLiteral(":/images/library/ic_library_cross_grey.svg");

QList<QPair<QString, int>> queryTrackDirectories(const QSqlDatabase& database) {
    QSqlQuery query(database);
    query.prepare(QStringLiteral(
            "SELECT track_locations.directory, COUNT(*) FROM library "
            "INNER JOIN track_locations ON library.location=track_locations.id "
            "WHERE library.mixxx_deleted=0 GROUP BY track_locations.directory"));
    QList<QPair<QString, int>> dirs;
    if (!query.exec()) {
        qWarning() << "FolderFeature: cannot read folders";
        return dirs;
    }
    while (query.next()) {
        dirs.append({query.value(0).toString(), query.value(1).toInt()});
    }
    return dirs;
}

bool isFolderOffline(const QString& folder) {
    // Offline when the folder's drive is not connected, or the folder itself
    // is not there (e.g. not mounted).
    return djmantra::OnlineStatus::isDriveOffline(folder + QStringLiteral("/x")) ||
            !QFileInfo(folder).isDir();
}

void appendNodes(TreeItem* pParent, const QList<djmantra::FolderNode>& nodes, bool parentOffline) {
    for (const auto& node : nodes) {
        const bool offline = parentOffline || isFolderOffline(node.path);
        QString label = QStringLiteral("%1 (%2)").arg(node.label).arg(node.trackCount);
        TreeItem* pItem = pParent->appendChild(label, node.path);
        if (offline) {
            pItem->setIcon(QIcon(kOfflineIcon));
        }
        appendNodes(pItem, node.children, offline);
    }
}

} // namespace

FolderFeature::FolderFeature(Library* pLibrary, UserSettingsPointer pConfig)
        : LibraryFeature(pLibrary, std::move(pConfig), QStringLiteral("computer")),
          m_pSidebarModel(new TreeItemModel(this)),
          m_tableModel(this, pLibrary->trackCollectionManager()) {
    connect(pLibrary->trackCollectionManager(),
            &TrackCollectionManager::libraryScanFinished,
            this,
            &FolderFeature::slotRefresh);
    slotRefresh();
}

QVariant FolderFeature::title() {
    return tr("Folders");
}

TreeItemModel* FolderFeature::sidebarModel() const {
    return m_pSidebarModel;
}

void FolderFeature::bindLibraryWidget(WLibrary* pLibraryWidget, KeyboardEventFilter* pKeyboard) {
    Q_UNUSED(pKeyboard);
    m_pRootView = new WLibraryTextBrowser(pLibraryWidget);
    m_pRootView->setOpenLinks(false);
    m_pRootView->setHtml(formatRootViewHtml());
    connect(m_pRootView, &WLibraryTextBrowser::anchorClicked, this, &FolderFeature::slotLinkClicked);
    pLibraryWidget->registerView(kRootViewName, m_pRootView);
}

void FolderFeature::bindSidebarWidget(WLibrarySidebar* pSidebarWidget) {
    m_pSidebarWidget = pSidebarWidget;
}

QString FolderFeature::formatRootViewHtml() const {
    QString html;
    html.append(QStringLiteral("<h2>%1</h2>").arg(tr("Folders")));
    html.append(QStringLiteral("<p>%1</p>")
                        .arg(tr("Add a music folder: it and all its subfolders appear here, "
                                "and each folder works like a playlist of its songs.")));
    html.append(QStringLiteral("<p>%1</p>")
                        .arg(tr("Folders stay in the library. When a drive is not connected, "
                                "its folders and songs are shown offline and come back "
                                "online when it is plugged in again.")));
    html.append(QStringLiteral("<p><a style=\"color:#0496FF;font-size:large;\" "
                               "href=\"add\">+ %1</a></p>")
                        .arg(tr("Add folder")));
    if (m_pLibrary) {
        const QStringList roots =
                m_pLibrary->trackCollectionManager()->internalCollection()->getRootDirStrings();
        if (!roots.isEmpty()) {
            html.append(QStringLiteral("<ul>"));
            for (const auto& root : roots) {
                html.append(QStringLiteral("<li>%1%2</li>")
                                    .arg(root.toHtmlEscaped(),
                                            isFolderOffline(root)
                                                    ? QStringLiteral(" – <i>%1</i>")
                                                              .arg(tr("offline"))
                                                    : QString()));
            }
            html.append(QStringLiteral("</ul>"));
        }
    }
    return html;
}

void FolderFeature::slotRefresh() {
    djmantra::OnlineStatus::invalidate();
    if (!m_pLibrary) {
        return;
    }
    TrackCollection* pCollection = m_pLibrary->trackCollectionManager()->internalCollection();
    const QStringList roots = pCollection->getRootDirStrings();
    const auto tree = djmantra::buildFolderTree(roots, queryTrackDirectories(pCollection->database()));
    const QStringList expanded = expandedFolders();
    auto pRoot = TreeItem::newRoot(this);
    appendNodes(pRoot.get(), tree, false);
    m_pSidebarModel->setRootItem(std::move(pRoot));
    if (m_pSidebarWidget) {
        auto* pSidebarModel = qobject_cast<SidebarModel*>(m_pSidebarWidget->model());
        if (pSidebarModel) {
            for (const auto& folder : expanded) {
                const QModelIndex index = indexOfFolder(folder);
                if (index.isValid()) {
                    m_pSidebarWidget->expand(pSidebarModel->translateChildIndex(index));
                }
            }
            const QModelIndex active = indexOfFolder(m_activeFolder);
            if (active.isValid()) {
                m_pSidebarWidget->selectChildIndex(active, true);
            }
        }
    }
    if (m_pRootView) {
        m_pRootView->setHtml(formatRootViewHtml());
    }
    if (!m_activeFolder.isEmpty()) {
        m_tableModel.select(); // online/offline marks and new tracks
    }
}

void FolderFeature::activate() {
    m_activeFolder.clear();
    if (m_pRootView) {
        m_pRootView->setHtml(formatRootViewHtml());
    }
    emit switchToView(kRootViewName);
    emit disableSearch();
    emit enableCoverArtDisplay(true);
}

QString FolderFeature::folderOf(const QModelIndex& index) const {
    const TreeItem* pItem = static_cast<TreeItem*>(index.internalPointer());
    return pItem ? pItem->getData().toString() : QString();
}

QStringList FolderFeature::expandedFolders(const QModelIndex& parent) const {
    QStringList folders;
    if (!m_pSidebarWidget) {
        return folders;
    }
    auto* pSidebarModel = qobject_cast<SidebarModel*>(m_pSidebarWidget->model());
    if (!pSidebarModel) {
        return folders;
    }
    for (int row = 0; row < m_pSidebarModel->rowCount(parent); ++row) {
        const QModelIndex index = m_pSidebarModel->index(row, 0, parent);
        if (m_pSidebarWidget->isExpanded(pSidebarModel->translateChildIndex(index))) {
            folders.append(folderOf(index));
            folders.append(expandedFolders(index));
        }
    }
    return folders;
}

QModelIndex FolderFeature::indexOfFolder(const QString& folder, const QModelIndex& parent) const {
    if (folder.isEmpty()) {
        return {};
    }
    for (int row = 0; row < m_pSidebarModel->rowCount(parent); ++row) {
        const QModelIndex index = m_pSidebarModel->index(row, 0, parent);
        const QString path = folderOf(index);
        if (path == folder) {
            return index;
        }
        if (folder.startsWith(path + QChar('/'))) {
            return indexOfFolder(folder, index);
        }
    }
    return {};
}

void FolderFeature::activateChild(const QModelIndex& index) {
    const QString folder = folderOf(index);
    if (folder.isEmpty()) {
        return;
    }
    m_activeFolder = folder;
    emit saveModelState();
    m_tableModel.selectFolder(folder);
    emit showTrackModel(&m_tableModel);
    emit enableCoverArtDisplay(true);
}

void FolderFeature::onRightClick(const QPoint& globalPos) {
    QMenu menu(m_pSidebarWidget);
    menu.addAction(tr("Add folder..."), this, &FolderFeature::slotAddFolder);
    menu.exec(globalPos);
}

void FolderFeature::onRightClickChild(const QPoint& globalPos, const QModelIndex& index) {
    const QString folder = folderOf(index);
    QMenu menu(m_pSidebarWidget);
    menu.addAction(tr("Add folder..."), this, &FolderFeature::slotAddFolder);
    menu.addAction(tr("Rescan"), this, [this] {
        if (m_pLibrary) {
            m_pLibrary->trackCollectionManager()->startLibraryScan();
        }
    });
    const QStringList roots = m_pLibrary
            ? m_pLibrary->trackCollectionManager()->internalCollection()->getRootDirStrings()
            : QStringList();
    if (roots.contains(folder)) {
        menu.addSeparator();
        menu.addAction(tr("Remove from library..."), this, [this, folder] {
            removeFolder(folder);
        });
    }
    menu.exec(globalPos);
}

void FolderFeature::slotLinkClicked(const QUrl& link) {
    if (link.path() == QStringLiteral("add")) {
        slotAddFolder();
    }
}

void FolderFeature::slotAddFolder() {
    if (!m_pLibrary) {
        return;
    }
    const QString folder = QFileDialog::getExistingDirectory(nullptr,
            tr("Add music folder"),
            QStandardPaths::writableLocation(QStandardPaths::MusicLocation));
    if (folder.isEmpty()) {
        return;
    }
    if (!m_pLibrary->requestAddDir(folder)) {
        return;
    }
    slotRefresh(); // shows the folder at once; its songs after the scan
    m_pLibrary->trackCollectionManager()->startLibraryScan();
}

void FolderFeature::removeFolder(const QString& folder) {
    if (!m_pLibrary) {
        return;
    }
    const auto answer = QMessageBox::question(nullptr,
            tr("Remove folder"),
            tr("Remove %1 from the library?\n\nIts songs are hidden from the "
               "library. Playlists and crates keep them.")
                    .arg(QDir::toNativeSeparators(folder)));
    if (answer != QMessageBox::Yes) {
        return;
    }
    if (m_pLibrary->requestRemoveDir(folder, LibraryRemovalType::HideTracks)) {
        if (m_activeFolder == folder) {
            m_activeFolder.clear();
            activate();
        }
        slotRefresh();
    }
}
