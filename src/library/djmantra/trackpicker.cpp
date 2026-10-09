#include "library/djmantra/trackpicker.h"

#include <QCollator>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRegularExpression>
#include <QScroller>
#include <QStandardPaths>
#include <QStyledItemDelegate>
#include <QSvgRenderer>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtDebug>

#include "moc_trackpicker.cpp"
#include "sources/soundsourceproxy.h"
#include "util/androidwindow.h"

namespace djmantra {

namespace {

// Colours of the picker (Marko's reference: a dark Android file picker)
const QColor kGround(0x11, 0x12, 0x15);
const QColor kBar(0x1E, 0x1F, 0x22);
const QColor kTile(0x26, 0x27, 0x2B);
const QColor kText(0xF2, 0xF2, 0xF2);
const QColor kSubText(0x9A, 0x9C, 0xA1);
const QColor kAccent(0x2D, 0x8C, 0xFF);

const QString kGroup = QStringLiteral("[DJMantra]");
const QString kRootsKey = QStringLiteral("picker_roots");
const QString kQueueKey = QStringLiteral("picker_queue");
const QString kHistoryKey = QStringLiteral("picker_history");
const int kHistoryMax = 100;

// Item data
const int kPathRole = Qt::UserRole;
const int kKindRole = Qt::UserRole + 1;
const int kSubtitleRole = Qt::UserRole + 2;
enum Kind {
    KindUp,
    KindFolder,
    KindRoot,
    KindSong
};

// Material icons (Apache 2.0), 24x24 view box
const char* kIconClose =
        "M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 "
        "13.41 17.59 19 19 17.59 13.41 12z";
const char* kIconFolder =
        "M10 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 "
        "2-2V8c0-1.1-.9-2-2-2h-8l-2-2z";
const char* kIconAdd = "M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z";
const char* kIconMore =
        "M12 8c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm0 2c-1.1 0-2 .9-2 2s.9 "
        "2 2 2 2-.9 2-2-.9-2-2-2zm0 6c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2z";
const char* kIconDropDown = "M7 10l5 5 5-5z";
const char* kIconNote =
        "M12 3v10.55c-.59-.34-1.27-.55-2-.55-2.21 0-4 1.79-4 4s1.79 4 4 4 4-1.79 "
        "4-4V7h4V3h-6z";
const char* kIconQueue =
        "M3 13h2v-2H3v2zm0 4h2v-2H3v2zm0-8h2V7H3v2zm4 4h14v-2H7v2zm0 4h14v-2H7v2zM7 "
        "7v2h14V7H7z";
const char* kIconHistory =
        "M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 "
        "11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 "
        "3.15.75-1.23-4.5-2.67z";
const char* kIconBack =
        "M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z";

void paintIcon(QPainter* pPainter, const char* path, const QRectF& rect, const QColor& color) {
    const QByteArray svg = QByteArrayLiteral(
                                   "<svg xmlns='http://www.w3.org/2000/svg' "
                                   "viewBox='0 0 24 24'><path fill='") +
            color.name().toLatin1() + QByteArrayLiteral("' d='") + QByteArray(path) +
            QByteArrayLiteral("'/></svg>");
    QSvgRenderer renderer(svg);
    renderer.render(pPainter, rect);
}

QIcon icon(const char* path, const QColor& color, int size) {
    // Drawn at 3x so it stays sharp on phone screens
    QPixmap pixmap(size * 3, size * 3);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    paintIcon(&painter, path, QRectF(0, 0, size * 3, size * 3), color);
    painter.end();
    pixmap.setDevicePixelRatio(3);
    return QIcon(pixmap);
}

QToolButton* barButton(const char* path, QWidget* pParent) {
    auto* pButton = new QToolButton(pParent);
    pButton->setIcon(icon(path, kText, 26));
    pButton->setIconSize(QSize(26, 26));
    pButton->setFixedSize(52, 52);
    pButton->setAutoRaise(true);
    pButton->setFocusPolicy(Qt::NoFocus);
    return pButton;
}

QString sizeText(qint64 bytes) {
    return QStringLiteral("%1 MB").arg(bytes / 1048576.0, 0, 'f', 1);
}

/// A row: a rounded tile with the icon, the name, a grey subtitle and ⋮
class RowDelegate : public QStyledItemDelegate {
  public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex& index) const override {
        return QSize(100, index.data(kKindRole).toInt() == KindUp ? 56 : 72);
    }

    void paint(QPainter* pPainter,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override {
        pPainter->save();
        pPainter->setRenderHint(QPainter::Antialiasing);
        const QRect rect = option.rect;
        if (option.state & QStyle::State_Sunken) {
            pPainter->fillRect(rect, kBar);
        }
        const int kind = index.data(kKindRole).toInt();
        const int tile = kind == KindUp ? 40 : 52;
        const QRectF tileRect(rect.left() + 12,
                rect.top() + (rect.height() - tile) / 2.0,
                tile,
                tile);
        const char* path = kIconNote;
        if (kind == KindUp) {
            path = kIconBack;
        } else if (kind != KindSong) {
            path = kIconFolder;
        }
        if (kind != KindUp) {
            QPainterPath tilePath;
            tilePath.addRoundedRect(tileRect, 10, 10);
            pPainter->fillPath(tilePath, kTile);
        }
        const qreal iconSize = tile * 0.55;
        paintIcon(pPainter,
                path,
                QRectF(tileRect.center().x() - iconSize / 2,
                        tileRect.center().y() - iconSize / 2,
                        iconSize,
                        iconSize),
                kText);

        const bool hasMenu = kind != KindUp;
        const int textLeft = static_cast<int>(tileRect.right()) + 16;
        const int textRight = rect.right() - (hasMenu ? 56 : 16);
        const QString subtitle = index.data(kSubtitleRole).toString();
        QFont font = option.font;
        font.setPixelSize(17);
        pPainter->setFont(font);
        pPainter->setPen(kText);
        const QFontMetrics metrics(font);
        const QString name = metrics.elidedText(
                index.data(Qt::DisplayRole).toString(), Qt::ElideMiddle, textRight - textLeft);
        if (subtitle.isEmpty()) {
            pPainter->drawText(QRect(textLeft, rect.top(), textRight - textLeft, rect.height()),
                    Qt::AlignVCenter | Qt::AlignLeft,
                    name);
        } else {
            pPainter->drawText(
                    QRect(textLeft, rect.top() + 12, textRight - textLeft, rect.height() / 2 - 8),
                    Qt::AlignBottom | Qt::AlignLeft,
                    name);
            QFont small = font;
            small.setPixelSize(13);
            pPainter->setFont(small);
            pPainter->setPen(kSubText);
            pPainter->drawText(QRect(textLeft,
                                       rect.top() + rect.height() / 2 + 4,
                                       textRight - textLeft,
                                       rect.height() / 2 - 12),
                    Qt::AlignTop | Qt::AlignLeft,
                    QFontMetrics(small).elidedText(
                            subtitle, Qt::ElideMiddle, textRight - textLeft));
        }
        if (hasMenu) {
            paintIcon(pPainter,
                    kIconMore,
                    QRectF(rect.right() - 44, rect.center().y() - 12, 24, 24),
                    kText);
        }
        pPainter->restore();
    }
};

} // namespace

TrackPicker::TrackPicker(UserSettingsPointer pConfig, QWidget* pWindow, const QString& group)
        : QWidget(pWindow),
          m_pConfig(pConfig),
          m_pWindow(pWindow),
          m_group(group),
          m_tab(Tab::Files),
          m_newestFirst(false),
          m_lastReleaseX(0) {
    setAttribute(Qt::WA_DeleteOnClose);
    // Opaque, so the widgets below (VU meters, waveforms) never paint through
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(true);
    QPalette background = palette();
    background.setColor(QPalette::Window, kGround);
    setPalette(background);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(QStringLiteral(
            "QWidget#PickerBar { background-color: %1; }"
            "QLabel { color: %2; background: transparent; }"
            "QLabel#PickerTitle { font-size: 21px; font-weight: 500; }"
            "QLabel#PickerEmpty { color: %3; font-size: 16px; }"
            "QToolButton { background: transparent; border: none; color: %2; }"
            "QToolButton:pressed { background-color: #2E3034; border-radius: 26px; }"
            "QToolButton#PickerTab { font-size: 14px; color: %2; padding-top: 6px;"
            "  border-radius: 0px; }"
            "QToolButton#PickerTab[active=\"true\"] { color: %4; }"
            "QListWidget { background-color: %5; border: none; outline: none; }"
            "QMenu { background-color: #2B2D31; color: %2; font-size: 16px;"
            "  border: none; padding: 6px 0px; }"
            "QMenu::item { padding: 12px 28px; }"
            "QMenu::item:selected { background-color: #3A3C41; }")
                    .arg(kBar.name(), kText.name(), kSubText.name(), kAccent.name(),
                            kGround.name()));

    auto* pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(0);

    // Top bar: close, folder source, title, add folder, more
    auto* pTop = new QWidget(this);
    pTop->setObjectName(QStringLiteral("PickerBar"));
    pTop->setAttribute(Qt::WA_StyledBackground);
    pTop->setFixedHeight(64);
    auto* pTopLayout = new QHBoxLayout(pTop);
    pTopLayout->setContentsMargins(8, 0, 8, 0);
    pTopLayout->setSpacing(4);
    m_pClose = barButton(kIconClose, pTop);
    m_pSource = barButton(kIconFolder, pTop);
    m_pSource->setIcon(QIcon());
    m_pSource->setFixedSize(64, 52);
    {
        // The source button: a white rounded folder tile and ▾
        QPixmap pixmap(64 * 3, 52 * 3);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(3, 3);
        QPainterPath tile;
        tile.addRoundedRect(QRectF(4, 8, 36, 36), 8, 8);
        painter.fillPath(tile, kText);
        paintIcon(&painter, kIconFolder, QRectF(10, 14, 24, 24), kAccent);
        paintIcon(&painter, kIconDropDown, QRectF(40, 14, 24, 24), kText);
        painter.end();
        pixmap.setDevicePixelRatio(3);
        m_pSource->setIcon(QIcon(pixmap));
        m_pSource->setIconSize(QSize(64, 52));
    }
    m_pTitle = new QLabel(pTop);
    m_pTitle->setObjectName(QStringLiteral("PickerTitle"));
    m_pAdd = barButton(kIconAdd, pTop);
    m_pMore = barButton(kIconMore, pTop);
    pTopLayout->addWidget(m_pClose);
    pTopLayout->addSpacing(8);
    pTopLayout->addWidget(m_pSource);
    pTopLayout->addSpacing(12);
    pTopLayout->addWidget(m_pTitle, 1);
    pTopLayout->addWidget(m_pAdd);
    pTopLayout->addWidget(m_pMore);
    pLayout->addWidget(pTop);

    // The list, scrolled with a finger
    m_pList = new QListWidget(this);
    m_pList->setItemDelegate(new RowDelegate(m_pList));
    m_pList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_pList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pList->setSelectionMode(QAbstractItemView::NoSelection);
    m_pList->setFocusPolicy(Qt::NoFocus);
    m_pList->setUniformItemSizes(false);
    QScroller::grabGesture(m_pList->viewport(), QScroller::LeftMouseButtonGesture);
    m_pList->viewport()->installEventFilter(this);
    m_pEmpty = new QLabel(m_pList);
    m_pEmpty->setObjectName(QStringLiteral("PickerEmpty"));
    m_pEmpty->setAlignment(Qt::AlignCenter);
    m_pEmpty->setWordWrap(true);
    m_pEmpty->hide();
    pLayout->addWidget(m_pList, 1);

    // Bottom tabs: Files | Queue | History
    auto* pBottom = new QWidget(this);
    pBottom->setObjectName(QStringLiteral("PickerBar"));
    pBottom->setAttribute(Qt::WA_StyledBackground);
    pBottom->setFixedHeight(76);
    auto* pBottomLayout = new QHBoxLayout(pBottom);
    pBottomLayout->setContentsMargins(0, 0, 0, 0);
    pBottomLayout->setSpacing(0);
    const QList<QPair<QString, const char*>> tabs = {
            {tr("Files"), kIconFolder},
            {tr("Queue"), kIconQueue},
            {tr("History"), kIconHistory},
    };
    for (int i = 0; i < tabs.size(); ++i) {
        auto* pTab = new QToolButton(pBottom);
        pTab->setObjectName(QStringLiteral("PickerTab"));
        pTab->setText(tabs[i].first);
        pTab->setProperty("iconPath", QByteArray(tabs[i].second));
        pTab->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        pTab->setIconSize(QSize(30, 30));
        pTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        pTab->setFocusPolicy(Qt::NoFocus);
        connect(pTab, &QToolButton::clicked, this, [this, i] {
            showTab(static_cast<Tab>(i));
        });
        pBottomLayout->addWidget(pTab);
        m_tabs.append(pTab);
    }
    pLayout->addWidget(pBottom);

    connect(m_pClose, &QToolButton::clicked, this, &QWidget::close);
    connect(m_pSource, &QToolButton::clicked, this, &TrackPicker::showSourceMenu);
    connect(m_pAdd, &QToolButton::clicked, this, &TrackPicker::addFolder);
    connect(m_pMore, &QToolButton::clicked, this, &TrackPicker::showMoreMenu);
    connect(m_pList, &QListWidget::itemClicked, this, [this](QListWidgetItem* pItem) {
        // ⋮ at the right end of the row opens the row's menu
        if (pItem->data(kKindRole).toInt() != KindUp &&
                m_lastReleaseX > m_pList->viewport()->width() - 64) {
            itemMenu(pItem);
        } else {
            itemTapped(pItem);
        }
    });

    // Over the whole window, following its size
    m_pWindow->installEventFilter(this);
    setGeometry(m_pWindow->rect());
    const QString lastFolder = m_pConfig->getValue(
            ConfigKey(kGroup, QStringLiteral("picker_last_folder")), QString());
    if (!lastFolder.isEmpty() && QFileInfo(lastFolder).isDir()) {
        m_path = lastFolder;
    }
    // Android shows the system bars again over a new screen (round 7)
    AndroidWindow::hideSystemBars();
    showTab(Tab::Files);
    show();
    raise();
    setFocus();
}

bool TrackPicker::eventFilter(QObject* pObject, QEvent* pEvent) {
    if (pObject == m_pWindow && pEvent->type() == QEvent::Resize) {
        setGeometry(m_pWindow->rect());
    } else if (pObject == m_pList->viewport() && pEvent->type() == QEvent::Resize) {
        placeEmptyText();
    } else if (pObject == m_pList->viewport() && pEvent->type() == QEvent::MouseButtonRelease) {
        m_lastReleaseX = static_cast<QMouseEvent*>(pEvent)->position().toPoint().x();
    }
    return QWidget::eventFilter(pObject, pEvent);
}

void TrackPicker::keyPressEvent(QKeyEvent* pEvent) {
    // Android's back: one folder up, then close
    if (pEvent->key() == Qt::Key_Back || pEvent->key() == Qt::Key_Escape) {
        if (m_tab == Tab::Files && !m_path.isEmpty()) {
            goUp();
        } else {
            close();
        }
        pEvent->accept();
        return;
    }
    QWidget::keyPressEvent(pEvent);
}

void TrackPicker::paintEvent(QPaintEvent* pEvent) {
    QPainter painter(this);
    painter.fillRect(rect(), kGround);
    // The active tab's blue line on top of the bottom bar
    if (!m_tabs.isEmpty()) {
        const QToolButton* pTab = m_tabs.value(static_cast<int>(m_tab));
        const QPoint topLeft = pTab->mapTo(this, QPoint(0, 0));
        painter.fillRect(QRect(topLeft.x(), topLeft.y(), pTab->width(), 3), kAccent);
    }
    QWidget::paintEvent(pEvent);
}

void TrackPicker::showTab(Tab tab) {
    m_tab = tab;
    for (int i = 0; i < m_tabs.size(); ++i) {
        const bool active = i == static_cast<int>(tab);
        m_tabs[i]->setProperty("active", active);
        m_tabs[i]->setIcon(icon(m_tabs[i]->property("iconPath").toByteArray().constData(),
                active ? kAccent : kText,
                30));
        m_tabs[i]->style()->unpolish(m_tabs[i]);
        m_tabs[i]->style()->polish(m_tabs[i]);
    }
    m_pSource->setVisible(tab == Tab::Files);
    m_pAdd->setVisible(tab == Tab::Files);
    refresh();
    update();
}

void TrackPicker::showFolder(const QString& path) {
    m_path = path;
    m_pConfig->setValue(ConfigKey(kGroup, QStringLiteral("picker_last_folder")), path);
    refresh();
}

void TrackPicker::goUp() {
    const QString root = m_path;
    for (const QString& rootPath : roots()) {
        if (QDir::cleanPath(rootPath) == QDir::cleanPath(m_path)) {
            showFolder(QString());
            return;
        }
    }
    QDir dir(root);
    if (!dir.cdUp() || dir.path() == m_path) {
        showFolder(QString());
        return;
    }
    showFolder(dir.path());
}

void TrackPicker::refresh() {
    m_pList->clear();
    QString empty;
    auto addItem = [this](const QString& name, const QString& path, int kind, const QString& sub) {
        auto* pItem = new QListWidgetItem(name, m_pList);
        pItem->setData(kPathRole, path);
        pItem->setData(kKindRole, kind);
        pItem->setData(kSubtitleRole, sub);
    };

    if (m_tab == Tab::Files) {
        if (m_path.isEmpty()) {
            m_pTitle->setText(tr("Folders"));
            for (const QString& root : roots()) {
                const QFileInfo info(root);
                addItem(info.fileName().isEmpty() ? root : info.fileName(),
                        root,
                        KindRoot,
                        QString());
            }
            if (m_pList->count() == 0) {
                empty = tr("No folders yet.\nTap + to add a folder with songs.");
            }
        } else {
            QDir dir(m_path);
            m_pTitle->setText(dir.dirName());
            addItem(tr("Back"), QString(), KindUp, QString());
            QFileInfoList folders = dir.entryInfoList(
                    QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
            QFileInfoList songs = dir.entryInfoList(QDir::Files | QDir::Readable);
            QCollator collator;
            collator.setNumericMode(true);
            collator.setCaseSensitivity(Qt::CaseInsensitive);
            auto byName = [&collator](const QFileInfo& a, const QFileInfo& b) {
                return collator.compare(a.fileName(), b.fileName()) < 0;
            };
            std::sort(folders.begin(), folders.end(), byName);
            if (m_newestFirst) {
                std::sort(songs.begin(), songs.end(), [](const QFileInfo& a, const QFileInfo& b) {
                    return a.lastModified() > b.lastModified();
                });
            } else {
                std::sort(songs.begin(), songs.end(), byName);
            }
            for (const QFileInfo& folder : std::as_const(folders)) {
                if (!folder.fileName().startsWith(QLatin1Char('.'))) {
                    addItem(folder.fileName(), folder.filePath(), KindFolder, QString());
                }
            }
            for (const QFileInfo& song : std::as_const(songs)) {
                if (SoundSourceProxy::isFileNameSupported(song.fileName())) {
                    addItem(song.completeBaseName(),
                            song.filePath(),
                            KindSong,
                            song.suffix().toUpper() + QStringLiteral(" · ") +
                                    sizeText(song.size()));
                }
            }
            if (m_pList->count() == 1) {
                empty = tr("No songs in this folder.");
            }
        }
    } else {
        const bool queue = m_tab == Tab::Queue;
        m_pTitle->setText(queue ? tr("Queue") : tr("History"));
        const QStringList paths = list(queue ? kQueueKey : kHistoryKey);
        for (const QString& path : paths) {
            const QFileInfo info(path);
            addItem(info.completeBaseName(), path, KindSong, info.dir().dirName());
        }
        if (paths.isEmpty()) {
            empty = queue ? tr("The queue is empty.\nIn Files, tap ⋮ on a song to add it.")
                          : tr("Songs you load show here.");
        }
    }

    // For the phone tests: what the list holds
    qInfo().noquote() << "TrackPicker" << m_pTitle->text() << "rows" << m_pList->count();
    const bool showEmpty = !empty.isEmpty();
    m_pEmpty->setText(empty);
    placeEmptyText();
    m_pEmpty->setVisible(showEmpty);
    m_pList->scrollToTop();
    // Android drew the new folder's rows only partly (round 7: the old rows
    // stayed half on screen): lay out and repaint the whole list now and
    // once more when the rows are in place
    m_pList->doItemsLayout();
    m_pList->viewport()->update();
    QTimer::singleShot(150, m_pList->viewport(), [this] {
        m_pList->viewport()->update();
        update();
    });
}

void TrackPicker::placeEmptyText() {
    m_pEmpty->setGeometry(24, 96, qMax(100, m_pList->viewport()->width() - 48), 120);
}

void TrackPicker::itemTapped(QListWidgetItem* pItem) {
    const QString path = pItem->data(kPathRole).toString();
    switch (pItem->data(kKindRole).toInt()) {
    case KindUp:
        goUp();
        break;
    case KindRoot:
    case KindFolder:
        showFolder(path);
        break;
    case KindSong:
        if (m_group.isEmpty()) {
            // The library (no deck chosen yet): ask which deck
            itemMenu(pItem);
            break;
        }
        if (m_tab == Tab::Queue) {
            QStringList queue = list(kQueueKey);
            queue.removeOne(path);
            setList(kQueueKey, queue);
        }
        load(path, m_group);
        break;
    }
}

void TrackPicker::itemMenu(QListWidgetItem* pItem) {
    const QString path = pItem->data(kPathRole).toString();
    const int kind = pItem->data(kKindRole).toInt();
    QMenu menu(this);
    if (kind == KindRoot) {
        menu.addAction(tr("Remove from Folders"), this, [this, path] {
            QStringList folders = list(kRootsKey);
            if (folders.isEmpty()) {
                folders = roots();
            }
            folders.removeOne(path);
            // An empty list would bring the default folders back
            setList(kRootsKey, folders.isEmpty() ? QStringList{QString()} : folders);
            refresh();
        });
    } else if (kind == KindFolder) {
        menu.addAction(tr("Add to Folders"), this, [this, path] {
            QStringList folders = roots();
            if (!folders.contains(path)) {
                folders.append(path);
            }
            setList(kRootsKey, folders);
        });
    } else if (kind == KindSong) {
        const QString first = m_group.isEmpty() ? QStringLiteral("[Channel1]") : m_group;
        const QString second = m_group.isEmpty() ? QStringLiteral("[Channel2]") : otherGroup();
        for (const QString& group : {first, second}) {
            menu.addAction(tr("Load to %1").arg(deckName(group)), this, [this, path, group] {
                if (m_tab == Tab::Queue) {
                    QStringList queue = list(kQueueKey);
                    queue.removeOne(path);
                    setList(kQueueKey, queue);
                }
                load(path, group);
            });
        }
        if (m_tab == Tab::Queue) {
            menu.addAction(tr("Remove from queue"), this, [this, path] {
                QStringList queue = list(kQueueKey);
                queue.removeOne(path);
                setList(kQueueKey, queue);
                refresh();
            });
        } else {
            menu.addAction(tr("Add to queue"), this, [this, path] {
                QStringList queue = list(kQueueKey);
                queue.removeOne(path);
                queue.append(path);
                setList(kQueueKey, queue);
            });
        }
    }
    if (!menu.isEmpty()) {
        menu.exec(m_pList->viewport()->mapToGlobal(
                QPoint(qMax(0, m_pList->viewport()->width() - 260),
                        m_pList->visualItemRect(pItem).center().y())));
    }
}

void TrackPicker::load(const QString& location, const QString& group) {
    QStringList history = list(kHistoryKey);
    history.removeOne(location);
    history.prepend(location);
    while (history.size() > kHistoryMax) {
        history.removeLast();
    }
    setList(kHistoryKey, history);
    emit loadRequested(location, group);
    close();
}

void TrackPicker::addFolder() {
    const QString start = m_path.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::MusicLocation)
            : m_path;
    const QString folder = QFileDialog::getExistingDirectory(this, tr("Add a folder"), start);
    if (folder.isEmpty()) {
        return;
    }
    QStringList folders = roots();
    if (!folders.contains(folder)) {
        folders.append(folder);
    }
    setList(kRootsKey, folders);
    showFolder(QString());
}

void TrackPicker::showSourceMenu() {
    QMenu menu(this);
    menu.addAction(tr("Folders"), this, [this] { showFolder(QString()); });
    QStringList storages;
#ifdef Q_OS_ANDROID
    storages.append(QStringLiteral("/storage/emulated/0"));
    // USB sticks and SD cards: /storage/XXXX-XXXX
    const QFileInfoList volumes = QDir(QStringLiteral("/storage"))
                                          .entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& volume : volumes) {
        if (volume.fileName() != QLatin1String("emulated") &&
                volume.fileName() != QLatin1String("self") && volume.isReadable()) {
            storages.append(volume.filePath());
        }
    }
#else
    storages.append(QDir::homePath());
#endif
    for (const QString& storage : std::as_const(storages)) {
        const QString name = storage == QLatin1String("/storage/emulated/0")
                ? tr("Phone storage")
                : QFileInfo(storage).fileName();
        menu.addAction(name, this, [this, storage] { showFolder(storage); });
    }
    menu.exec(m_pSource->mapToGlobal(QPoint(0, m_pSource->height())));
}

void TrackPicker::showMoreMenu() {
    QMenu menu(this);
    auto* pByName = menu.addAction(tr("Sort by name"), this, [this] {
        m_newestFirst = false;
        refresh();
    });
    pByName->setCheckable(true);
    pByName->setChecked(!m_newestFirst);
    auto* pNewest = menu.addAction(tr("Newest first"), this, [this] {
        m_newestFirst = true;
        refresh();
    });
    pNewest->setCheckable(true);
    pNewest->setChecked(m_newestFirst);
    if (m_tab == Tab::Queue) {
        menu.addSeparator();
        menu.addAction(tr("Clear the queue"), this, [this] {
            setList(kQueueKey, {});
            refresh();
        });
    } else if (m_tab == Tab::History) {
        menu.addSeparator();
        menu.addAction(tr("Clear the history"), this, [this] {
            setList(kHistoryKey, {});
            refresh();
        });
    }
    menu.exec(m_pMore->mapToGlobal(QPoint(m_pMore->width() - 220, m_pMore->height())));
}

QStringList TrackPicker::roots() const {
    QStringList folders = list(kRootsKey);
    if (folders.isEmpty()) {
        // First start: the phone's music and download folders
        for (const auto location :
                {QStandardPaths::MusicLocation, QStandardPaths::DownloadLocation}) {
            const QString path = QStandardPaths::writableLocation(location);
            if (!path.isEmpty() && QFileInfo(path).isDir() && !folders.contains(path)) {
                folders.append(path);
            }
        }
    }
    folders.removeAll(QString());
    return folders;
}

QStringList TrackPicker::list(const QString& key) const {
    const QString json = m_pConfig->getValue(ConfigKey(kGroup, key), QString());
    QStringList values;
    const QJsonArray array = QJsonDocument::fromJson(json.toUtf8()).array();
    for (const auto& value : array) {
        values.append(value.toString());
    }
    return values;
}

void TrackPicker::setList(const QString& key, const QStringList& values) {
    m_pConfig->setValue(ConfigKey(kGroup, key),
            QString::fromUtf8(QJsonDocument(QJsonArray::fromStringList(values))
                                      .toJson(QJsonDocument::Compact)));
}

QString TrackPicker::deckName(const QString& group) const {
    QString number = group;
    number.remove(QRegularExpression(QStringLiteral("[^0-9]")));
    return tr("Deck %1").arg(number);
}

QString TrackPicker::otherGroup() const {
    return m_group == QLatin1String("[Channel1]") ? QStringLiteral("[Channel2]")
                                                  : QStringLiteral("[Channel1]");
}

} // namespace djmantra
