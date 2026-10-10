#include "widget/djmantra/mantraui.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>
#include <QSvgRenderer>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "util/androidwindow.h"
#include "moc_mantraui.cpp"

namespace djmantra::ui {

namespace icons {
const char* const kClose =
        "M19 6.41L17.59 5 12 10.59 6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 12 "
        "13.41 17.59 19 19 17.59 13.41 12z";
const char* const kBack = "M20 11H7.83l5.59-5.59L12 4l-8 8 8 8 1.41-1.41L7.83 13H20v-2z";
const char* const kFolder =
        "M10 4H4c-1.1 0-1.99.9-1.99 2L2 18c0 1.1.9 2 2 2h16c1.1 0 2-.9 "
        "2-2V8c0-1.1-.9-2-2-2h-8l-2-2z";
const char* const kAdd = "M19 13h-6v6h-2v-6H5v-2h6V5h2v6h6v2z";
const char* const kMore =
        "M12 8c1.1 0 2-.9 2-2s-.9-2-2-2-2 .9-2 2 .9 2 2 2zm0 2c-1.1 0-2 .9-2 2s.9 "
        "2 2 2 2-.9 2-2-.9-2-2-2zm0 6c-1.1 0-2 .9-2 2s.9 2 2 2 2-.9 2-2-.9-2-2-2z";
const char* const kDropDown = "M7 10l5 5 5-5z";
const char* const kNote =
        "M12 3v10.55c-.59-.34-1.27-.55-2-.55-2.21 0-4 1.79-4 4s1.79 4 4 4 4-1.79 "
        "4-4V7h4V3h-6z";
const char* const kQueue =
        "M3 13h2v-2H3v2zm0 4h2v-2H3v2zm0-8h2V7H3v2zm4 4h14v-2H7v2zm0 4h14v-2H7v2zM7 "
        "7v2h14V7H7z";
const char* const kHistory =
        "M11.99 2C6.47 2 2 6.48 2 12s4.47 10 9.99 10C17.52 22 22 17.52 22 12S17.52 2 "
        "11.99 2zM12 20c-4.42 0-8-3.58-8-8s3.58-8 8-8 8 3.58 8 8-3.58 8-8 8zm.5-13H11v6l5.25 "
        "3.15.75-1.23-4.5-2.67z";
const char* const kSettings =
        "M19.14 12.94c.04-.3.06-.61.06-.94 0-.32-.02-.64-.07-.94l2.03-1.58c.18-.14.23-.41."
        "12-.61l-1.92-3.32c-.12-.22-.37-.29-.59-.22l-2.39.96c-.5-.38-1.03-.7-1.62-.94l-.36-"
        "2.54c-.04-.24-.24-.41-.48-.41h-3.84c-.24 0-.43.17-.47.41l-.36 2.54c-.59.24-1.13.57-"
        "1.62.94l-2.39-.96c-.22-.08-.47 0-.59.22L2.74 8.87c-.12.21-.08.47.12.61l2.03 1.58c-"
        ".05.3-.09.63-.09.94s.02.64.07.94l-2.03 1.58c-.18.14-.23.41-.12.61l1.92 3.32c.12.22."
        "37.29.59.22l2.39-.96c.5.38 1.03.7 1.62.94l.36 2.54c.05.24.24.41.48.41h3.84c.24 0 .44-"
        ".17.47-.41l.36-2.54c.59-.24 1.13-.56 1.62-.94l2.39.96c.22.08.47 0 .59-.22l1.92-3.32c."
        "12-.22.07-.47-.12-.61l-2.01-1.58zM12 15.6c-1.98 0-3.6-1.62-3.6-3.6s1.62-3.6 3.6-3.6 "
        "3.6 1.62 3.6 3.6-1.62 3.6-3.6 3.6z";
const char* const kSound =
        "M7 18h2V6H7v12zm4 4h2V2h-2v20zm-8-8h2v-4H3v4zm12 4h2V6h-2v12zm4-8v4h2v-4h-2z";
const char* const kController =
        "M5 2c0-.55-.45-1-1-1s-1 .45-1 1v4H1v6h6V6H5V2zm4 14c0 1.3.84 2.4 2 2.82V23h2v-4.18c"
        "1.16-.41 2-1.51 2-2.82v-2H9v2zm-8 0c0 1.3.84 2.4 2 2.82V23h2v-4.18C6.16 18.4 7 17.3 "
        "7 16v-2H1v2zM21 6V2c0-.55-.45-1-1-1s-1 .45-1 1v4h-2v6h6V6h-2zm-8-4c0-.55-.45-1-1-1s"
        "-1 .45-1 1v4H9v6h6V6h-2V2zm4 14c0 1.3.84 2.4 2 2.82V23h2v-4.18c1.16-.41 2-1.51 2-"
        "2.82v-2h-6v2z";
const char* const kAdvanced =
        "M3 17v2h6v-2H3zM3 5v2h10V5H3zm10 16v-2h8v-2h-8v-2h-2v6h2zM7 9v2H3v2h4v2h2V9H7zm14 "
        "4v-2H11v2h10zm-6-4h2V7h4V5h-4V3h-2v6z";
const char* const kInfo =
        "M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 15h-2v-6h2v6zm0-"
        "8h-2V7h2v2z";
const char* const kHelp =
        "M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm1 17h-2v-2h2v2zm2."
        "07-7.75l-.9.92C13.45 12.9 13 13.5 13 15h-2v-.5c0-1.1.45-2.1 1.17-2.83l1.24-1.26c.37-"
        ".36.59-.86.59-1.41 0-1.1-.9-2-2-2s-2 .9-2 2H8c0-2.21 1.79-4 4-4s4 1.79 4 4c0 .88-.36 "
        "1.68-.93 2.25z";
const char* const kVolumeDown =
        "M18.5 12c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-4.02zM5 9v6h4l5 5V4L9 "
        "9H5z";
const char* const kVolumeUp =
        "M3 9v6h4l5 5V4L7 9H3zm13.5 3c0-1.77-1.02-3.29-2.5-4.03v8.05c1.48-.73 2.5-2.25 2.5-"
        "4.02zM14 3.23v2.06c2.89.86 5 3.54 5 6.71s-2.11 5.85-5 6.71v2.06c4.01-.91 7-4.49 7-8."
        "77s-2.99-7.86-7-8.77z";
const char* const kRecord = "M12 6a6 6 0 1 0 0 12a6 6 0 1 0 0-12z";
const char* const kChevron = "M10 6L8.59 7.41 13.17 12l-4.58 4.59L10 18l6-6z";
} // namespace icons

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

Switch::Switch(QWidget* pParent)
        : QAbstractButton(pParent) {
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::NoFocus);
}

QSize Switch::sizeHint() const {
    return QSize(56, 36);
}

void Switch::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF track(8, height() / 2.0 - 8, 40, 16);
    painter.setPen(Qt::NoPen);
    painter.setBrush(isChecked() ? kAccent.darker(140) : QColor(0x3A, 0x3C, 0x40));
    painter.drawRoundedRect(track, 8, 8);
    const qreal x = isChecked() ? track.right() - 10 : track.left() + 10;
    painter.setBrush(isChecked() ? kAccent : QColor(0x55, 0x57, 0x5C));
    painter.drawEllipse(QPointF(x, height() / 2.0), 11, 11);
}

namespace {

/// A tappable row: the whole row reacts, like Android's list items
class Row : public QWidget {
  public:
    Row(std::function<void()> onTap, QWidget* pParent)
            : QWidget(pParent),
              m_onTap(std::move(onTap)),
              m_pressed(false) {
        setAttribute(Qt::WA_StyledBackground);
    }

  protected:
    void mousePressEvent(QMouseEvent*) override {
        m_pressed = true;
        update();
    }
    void mouseReleaseEvent(QMouseEvent* pEvent) override {
        const bool tap = m_pressed && rect().contains(pEvent->position().toPoint());
        m_pressed = false;
        update();
        if (tap && m_onTap) {
            m_onTap();
        }
    }
    void paintEvent(QPaintEvent*) override {
        if (m_pressed) {
            QPainter(this).fillRect(rect(), kPressed);
        }
    }

  private:
    std::function<void()> m_onTap;
    bool m_pressed;
};

/// The row's icon, drawn in the text colour
class IconLabel : public QWidget {
  public:
    IconLabel(const char* path, QWidget* pParent)
            : QWidget(pParent),
              m_path(path) {
        setFixedSize(32, 32);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

  protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        paintIcon(&painter, m_path, QRectF(2, 2, 28, 28), kText);
    }

  private:
    const char* m_path;
};

} // namespace

namespace {

/// The sheet's widget: the window dimmed, the items in a panel at the bottom
class SheetWidget : public QWidget {
  public:
    SheetWidget(QWidget* pWindow,
            const QString& title,
            const QList<std::shared_ptr<Sheet::Item>>& items)
            : QWidget(pWindow),
              m_pWindow(pWindow) {
        setAttribute(Qt::WA_DeleteOnClose);
        setFocusPolicy(Qt::StrongFocus);
        setStyleSheet(QStringLiteral(
                "QLabel { color: %1; background: transparent; font-size: 18px; }"
                "QLabel#SheetTitle { color: %2; font-size: 15px; }")
                        .arg(kText.name(), kSubText.name()));
        m_pPanel = new QWidget(this);
        auto* pLayout = new QVBoxLayout(m_pPanel);
        pLayout->setContentsMargins(0, 18, 0, 18);
        pLayout->setSpacing(0);
        if (!title.isEmpty()) {
            auto* pTitle = new QLabel(title, m_pPanel);
            pTitle->setObjectName(QStringLiteral("SheetTitle"));
            pTitle->setContentsMargins(24, 0, 24, 10);
            pLayout->addWidget(pTitle);
        }
        for (const auto& pItem : items) {
            const auto action = pItem->action;
            auto* pRow = new Row(
                    [this, action] {
                        close();
                        if (action) {
                            // after the sheet is gone (the action may open a screen)
                            QTimer::singleShot(0, m_pWindow, action);
                        }
                    },
                    m_pPanel);
            auto* pRowLayout = new QHBoxLayout(pRow);
            pRowLayout->setContentsMargins(24, 16, 24, 16);
            auto* pText = new QLabel(pItem->text, pRow);
            pText->setAttribute(Qt::WA_TransparentForMouseEvents);
            pRowLayout->addWidget(pText, 1);
            if (pItem->checkable && pItem->checked) {
                auto* pCheck = new QLabel(QStringLiteral("✓"), pRow);
                pCheck->setStyleSheet(QStringLiteral("color: %1;").arg(kAccent.name()));
                pCheck->setAttribute(Qt::WA_TransparentForMouseEvents);
                pRowLayout->addWidget(pCheck);
            }
            pLayout->addWidget(pRow);
        }
        m_pWindow->installEventFilter(this);
        place();
        // Android shows the system bars again over a new screen (round 7)
        AndroidWindow::hideSystemBars();
        show();
        raise();
        setFocus();
    }

  protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override {
        if (pObject == m_pWindow && pEvent->type() == QEvent::Resize) {
            place();
        }
        return QWidget::eventFilter(pObject, pEvent);
    }
    void mousePressEvent(QMouseEvent* pEvent) override {
        pEvent->accept();
    }
    void mouseReleaseEvent(QMouseEvent* pEvent) override {
        // a tap above the panel closes the sheet
        if (!m_pPanel->geometry().contains(pEvent->position().toPoint())) {
            close();
        }
    }
    void keyPressEvent(QKeyEvent* pEvent) override {
        if (pEvent->key() == Qt::Key_Back || pEvent->key() == Qt::Key_Escape) {
            close();
            pEvent->accept();
            return;
        }
        QWidget::keyPressEvent(pEvent);
    }
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(0, 0, 0, 140));
        QPainterPath panel;
        panel.addRoundedRect(QRectF(m_pPanel->geometry()).adjusted(0, 0, 0, 40), 24, 24);
        painter.fillPath(panel, QColor(0x2B, 0x2D, 0x31));
    }

  private:
    void place() {
        setGeometry(m_pWindow->rect());
        const int width = qMin(m_pWindow->width(), 640);
        const int height = qMin(m_pPanel->sizeHint().height(), m_pWindow->height() - 40);
        m_pPanel->setGeometry((m_pWindow->width() - width) / 2,
                m_pWindow->height() - height,
                width,
                height);
    }

    QWidget* m_pWindow;
    QWidget* m_pPanel;
};

} // namespace

Sheet::Item* Sheet::addAction(
        const QString& text, QObject* pContext, std::function<void()> action) {
    Q_UNUSED(pContext);
    auto pItem = std::make_shared<Item>();
    pItem->text = text;
    pItem->action = std::move(action);
    m_items.append(pItem);
    return pItem.get();
}

void Sheet::show(QWidget* pWindow, const QString& title) {
    new SheetWidget(pWindow, title, m_items);
}

Screen::Screen(QWidget* pWindow, const QString& title)
        : QWidget(pWindow),
          m_pWindow(pWindow) {
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAutoFillBackground(true);
    QPalette background = palette();
    background.setColor(QPalette::Window, kSurface);
    setPalette(background);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(QStringLiteral(
            "QLabel { color: %1; background: transparent; font-size: 18px; }"
            "QLabel#ScreenTitle { font-size: 22px; font-weight: 500; }"
            "QLabel#Section { color: %2; font-size: 17px; font-weight: 600;"
            "  padding: 28px 20px 10px 20px; }"
            "QLabel#Subtitle { color: %3; font-size: 15px; }"
            "QLabel#Footer { color: %3; font-size: 15px; padding: 32px 20px 8px 20px; }"
            "QToolButton { background: transparent; border: none; }"
            "QToolButton:pressed { background-color: %4; border-radius: 26px; }"
            "QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }"
            "QSlider::groove:horizontal { height: 3px; background: #44464B; border-radius: 1px; }"
            "QSlider::sub-page:horizontal { height: 3px; background: %2; border-radius: 1px; }"
            "QSlider::handle:horizontal { background: white; width: 26px; height: 26px;"
            "  margin: -12px 0px; border-radius: 13px; }")
                    .arg(kText.name(), kAccent.name(), kSubText.name(), kPressed.name()));

    auto* pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(0);

    auto* pTop = new QWidget(this);
    pTop->setFixedHeight(68);
    auto* pTopLayout = new QHBoxLayout(pTop);
    pTopLayout->setContentsMargins(6, 0, 16, 0);
    auto* pBack = new QToolButton(pTop);
    pBack->setIcon(icon(icons::kBack, kText, 28));
    pBack->setIconSize(QSize(28, 28));
    pBack->setFixedSize(52, 52);
    pBack->setFocusPolicy(Qt::NoFocus);
    connect(pBack, &QToolButton::clicked, this, &QWidget::close);
    auto* pTitle = new QLabel(title, pTop);
    pTitle->setObjectName(QStringLiteral("ScreenTitle"));
    pTopLayout->addWidget(pBack);
    pTopLayout->addSpacing(14);
    pTopLayout->addWidget(pTitle, 1);
    pLayout->addWidget(pTop);

    auto* pScroll = new QScrollArea(this);
    pScroll->setWidgetResizable(true);
    pScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pScroll->setFrameShape(QFrame::NoFrame);
    QScroller::grabGesture(pScroll->viewport(), QScroller::LeftMouseButtonGesture);
    auto* pContent = new QWidget(pScroll);
    m_pColumn = new QVBoxLayout(pContent);
    m_pColumn->setContentsMargins(0, 8, 0, 24);
    m_pColumn->setSpacing(0);
    pScroll->setWidget(pContent);
    // Android moved only part of the page when scrolling (round 8: the rows
    // slid under a slider that stayed put): repaint the whole page each step
    connect(pScroll->verticalScrollBar(), &QScrollBar::valueChanged, pContent, [pContent] {
        pContent->update();
    });
    pLayout->addWidget(pScroll, 1);

    // Android shows the system bars again over a new screen (round 7)
    AndroidWindow::hideSystemBars();
    m_pWindow->installEventFilter(this);
    setGeometry(m_pWindow->rect());
}

void Screen::addSection(const QString& title) {
    auto* pLabel = new QLabel(title, this);
    pLabel->setObjectName(QStringLiteral("Section"));
    m_pColumn->addWidget(pLabel);
}

QWidget* Screen::addRow(const char* iconPath,
        const QString& title,
        const QString& subtitle,
        std::function<void()> onTap) {
    auto* pRow = new Row(std::move(onTap), this);
    auto* pLayout = new QHBoxLayout(pRow);
    pLayout->setContentsMargins(20, 14, 20, 14);
    pLayout->setSpacing(22);
    if (iconPath) {
        pLayout->addWidget(new IconLabel(iconPath, pRow));
    }
    auto* pText = new QVBoxLayout();
    pText->setSpacing(2);
    auto* pTitle = new QLabel(title, pRow);
    pTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
    pText->addWidget(pTitle);
    if (!subtitle.isEmpty()) {
        auto* pSubtitle = new QLabel(subtitle, pRow);
        pSubtitle->setObjectName(QStringLiteral("Subtitle"));
        pSubtitle->setWordWrap(true);
        pSubtitle->setAttribute(Qt::WA_TransparentForMouseEvents);
        pText->addWidget(pSubtitle);
    }
    pLayout->addLayout(pText, 1);
    m_pColumn->addWidget(pRow);
    return pRow;
}

Switch* Screen::addSwitchRow(const QString& title, const QString& description) {
    auto* pSwitch = new Switch(this);
    auto* pRow = new Row([pSwitch] { pSwitch->toggle(); }, this);
    auto* pLayout = new QVBoxLayout(pRow);
    pLayout->setContentsMargins(20, 14, 12, 14);
    pLayout->setSpacing(6);
    auto* pTop = new QHBoxLayout();
    auto* pTitle = new QLabel(title, pRow);
    pTitle->setAttribute(Qt::WA_TransparentForMouseEvents);
    pTop->addWidget(pTitle, 1);
    pSwitch->setParent(pRow);
    pTop->addWidget(pSwitch);
    pLayout->addLayout(pTop);
    if (!description.isEmpty()) {
        auto* pDescription = new QLabel(description, pRow);
        pDescription->setObjectName(QStringLiteral("Subtitle"));
        pDescription->setWordWrap(true);
        pDescription->setAttribute(Qt::WA_TransparentForMouseEvents);
        pLayout->addWidget(pDescription);
    }
    m_pColumn->addWidget(pRow);
    return pSwitch;
}

void Screen::addWidget(QWidget* pWidget) {
    auto* pBox = new QWidget(this);
    auto* pLayout = new QHBoxLayout(pBox);
    pLayout->setContentsMargins(20, 8, 20, 8);
    pWidget->setParent(pBox);
    pLayout->addWidget(pWidget);
    m_pColumn->addWidget(pBox);
}

void Screen::addFooter(const QString& text) {
    m_pColumn->addStretch(1);
    auto* pLabel = new QLabel(text, this);
    pLabel->setObjectName(QStringLiteral("Footer"));
    pLabel->setAlignment(Qt::AlignCenter);
    m_pColumn->addWidget(pLabel);
}

void Screen::paintEvent(QPaintEvent*) {
    // The style sheet replaces the palette: the background is painted here
    QPainter(this).fillRect(rect(), kSurface);
}

bool Screen::eventFilter(QObject* pObject, QEvent* pEvent) {
    if (pObject == m_pWindow && pEvent->type() == QEvent::Resize) {
        setGeometry(m_pWindow->rect());
    }
    return QWidget::eventFilter(pObject, pEvent);
}

void Screen::keyPressEvent(QKeyEvent* pEvent) {
    if (pEvent->key() == Qt::Key_Back || pEvent->key() == Qt::Key_Escape) {
        close();
        pEvent->accept();
        return;
    }
    QWidget::keyPressEvent(pEvent);
}

} // namespace djmantra::ui
