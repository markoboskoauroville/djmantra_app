#include "library/missing_hidden/dlgmissing.h"

#include <QDir>
#include <QFileDialog>
#include <QItemSelection>
#include <QMessageBox>

#include "controllers/keyboard/keyboardeventfilter.h"
#include "library/export/missingtracksexport.h"
#include "library/library.h"
#include "library/missing_hidden/missingtablemodel.h"
#include "library/trackcollection.h"
#include "library/trackcollectionmanager.h"
#include "moc_dlgmissing.cpp"
#include "util/assert.h"
#include "widget/wlibrary.h"
#include "widget/wtracktableview.h"

DlgMissing::DlgMissing(
        WLibrary* parent,
        UserSettingsPointer pConfig,
        Library* pLibrary,
        KeyboardEventFilter* pKeyboard)
        : QWidget(parent),
          Ui::DlgMissing(),
          m_pTrackTableView(
                  new WTrackTableView(
                          this,
                          pConfig,
                          pLibrary,
                          parent->getTrackTableBackgroundColorOpacity())),
          m_pLibrary(pLibrary) {
    setupUi(this);
    m_pTrackTableView->installEventFilter(pKeyboard);

    // Install our own trackTable
    QBoxLayout* box = qobject_cast<QBoxLayout*>(layout());
    VERIFY_OR_DEBUG_ASSERT(box) { //Assumes the form layout is a QVBox/QHBoxLayout!
    } else {
        box->removeWidget(m_pTrackTablePlaceholder);
        m_pTrackTablePlaceholder->hide();
        box->insertWidget(1, m_pTrackTableView);
    }

    m_pMissingTableModel = new MissingTableModel(this, pLibrary->trackCollectionManager());
    m_pTrackTableView->loadTrackModel(m_pMissingTableModel);

    connect(btnPurge, &QPushButton::clicked, m_pTrackTableView, &WTrackTableView::slotPurge);
    connect(btnSelect, &QPushButton::clicked, this, &DlgMissing::selectAll);
    connect(btnExport, &QPushButton::clicked, this, &DlgMissing::exportList);
    connect(m_pTrackTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &DlgMissing::selectionChanged);
    connect(m_pTrackTableView, &WTrackTableView::trackSelected, this, &DlgMissing::trackSelected);

    connect(pLibrary, &Library::setTrackTableFont, m_pTrackTableView, &WTrackTableView::setTrackTableFont);
    connect(pLibrary, &Library::setTrackTableRowHeight, m_pTrackTableView, &WTrackTableView::setTrackTableRowHeight);
    connect(pLibrary, &Library::setSelectedClick, m_pTrackTableView, &WTrackTableView::setSelectedClick);
}

DlgMissing::~DlgMissing() {
    // Delete m_pTrackTableView before the table model. This is because the
    // table view saves the header state using the model.
    delete m_pTrackTableView;
    delete m_pMissingTableModel;
}

void DlgMissing::onShow() {
    m_pMissingTableModel->select();
    activateButtons(false);
}

void DlgMissing::onSearch(const QString& text) {
    m_pMissingTableModel->search(text);
}

QString DlgMissing::currentSearch() {
    return m_pMissingTableModel->currentSearch();
}

void DlgMissing::selectAll() {
    m_pTrackTableView->selectAll();
}

void DlgMissing::exportList() {
    const QList<djmantra::MissingTrack> tracks = djmantra::queryLibraryMissingTracks(
            m_pLibrary->trackCollectionManager()->internalCollection()->database());
    if (tracks.isEmpty()) {
        QMessageBox::information(this, tr("Export list"), tr("No tracks are missing."));
        return;
    }
    const QString filePath = QFileDialog::getSaveFileName(this,
            tr("Export missing tracks"),
            QDir::home().filePath(QStringLiteral("missing-tracks.txt")),
            tr("Text files (*.txt)"));
    if (filePath.isEmpty()) {
        return;
    }
    QString error;
    if (!djmantra::writeMissingTracksFile(
                filePath, tracks, tr("Library – tracks with missing files"), &error)) {
        QMessageBox::warning(this,
                tr("Export list"),
                tr("Could not save %1:\n%2").arg(filePath, error));
    }
}

void DlgMissing::activateButtons(bool enable) {
    btnPurge->setEnabled(enable);
}

void DlgMissing::selectionChanged(const QItemSelection &selected,
                                  const QItemSelection &deselected) {
    Q_UNUSED(deselected);
    activateButtons(!selected.indexes().isEmpty());
}

bool DlgMissing::hasFocus() const {
    return m_pTrackTableView->hasFocus();
}

void DlgMissing::saveCurrentViewState() {
    m_pTrackTableView->saveCurrentViewState();
};

bool DlgMissing::restoreCurrentViewState() {
    return m_pTrackTableView->restoreCurrentViewState();
};

void DlgMissing::setFocus() {
    m_pTrackTableView->setFocus();
}
