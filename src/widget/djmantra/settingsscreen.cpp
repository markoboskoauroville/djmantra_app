#include "widget/djmantra/settingsscreen.h"

#include "widget/djmantra/camelot.h"

#include <QDesktopServices>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QSlider>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtDebug>

#include "control/controlproxy.h"
#include "util/androidwindow.h"
#include "moc_settingsscreen.cpp"
#include "util/versionstore.h"

namespace djmantra {

using namespace ui;

namespace {

const QString kWebsite = QStringLiteral("https://djmantra.pages.dev");

QLabel* iconLabel(const char* path, int size, QWidget* pParent) {
    auto* pLabel = new QLabel(pParent);
    pLabel->setPixmap(icon(path, kText, size).pixmap(size, size));
    pLabel->setFixedSize(size, size);
    return pLabel;
}

} // namespace

SettingsScreen::SettingsScreen(QWidget* pWindow, const SettingsActions& actions)
        : Screen(pWindow, tr("Settings")),
          m_pMainGain(new ControlProxy(QStringLiteral("[Master]"),
                  QStringLiteral("gain"),
                  this,
                  ControlFlag::NoAssertIfMissing)),
          m_pHeadSplit(new ControlProxy(QStringLiteral("[Master]"),
                  QStringLiteral("headSplit"),
                  this,
                  ControlFlag::NoAssertIfMissing)) {
    // Main Volume: the master gain, its knob position from 0 to 1
    addSection(tr("Main Volume"));
    auto* pVolume = new QWidget(this);
    auto* pVolumeLayout = new QHBoxLayout(pVolume);
    pVolumeLayout->setContentsMargins(0, 4, 0, 4);
    pVolumeLayout->setSpacing(18);
    auto* pSlider = new QSlider(Qt::Horizontal, pVolume);
    pSlider->setRange(0, 1000);
    pSlider->setFixedHeight(44);
    pSlider->setFocusPolicy(Qt::NoFocus);
    if (m_pMainGain->valid()) {
        pSlider->setValue(qRound(m_pMainGain->getParameter() * 1000));
        connect(pSlider, &QSlider::valueChanged, this, [this](int value) {
            m_pMainGain->setParameter(value / 1000.0);
            qInfo().noquote() << "Settings main gain" << m_pMainGain->get();
        });
    }
    pVolumeLayout->addWidget(iconLabel(icons::kVolumeDown, 24, pVolume));
    pVolumeLayout->addWidget(pSlider, 1);
    pVolumeLayout->addWidget(iconLabel(icons::kVolumeUp, 24, pVolume));
    addWidget(pVolume);

    // Pre-Cueing: the deck you listen to on the left, the mix on the right
    addSection(tr("Pre-Cueing"));
    auto* pSplit = addSwitchRow(tr("Split Output"),
            tr("Pre-listen in the left ear, the mix in the right: one output "
               "with a splitter cable."));
    if (m_pHeadSplit->valid()) {
        pSplit->setChecked(m_pHeadSplit->get() > 0);
        connect(pSplit, &QAbstractButton::toggled, this, [this](bool on) {
            m_pHeadSplit->set(on ? 1.0 : 0.0);
            qInfo().noquote() << "Settings split output" << m_pHeadSplit->get();
        });
    }

    auto* pQuick = addSwitchRow(tr("New interface (GPU)"),
            tr("DJ Mantra's new screen, drawn completely by the graphics chip: smoother "
               "waveforms, less battery. Close and open the app to switch."));
    pQuick->setChecked(actions.quick);
    if (actions.setQuick) {
        const auto setQuick = actions.setQuick;
        connect(pQuick, &QAbstractButton::toggled, this, [setQuick](bool on) {
            setQuick(on);
            qInfo().noquote() << "Settings new interface" << on;
        });
    }

    auto* pGpu = addSwitchRow(tr("Draw on the GPU (test)"),
            tr("Waveforms on the graphics chip. Still being tuned: it can make the "
               "screen slow. After a restart."));
    pGpu->setChecked(actions.gpu);
    if (actions.setGpu) {
        const auto setGpu = actions.setGpu;
        connect(pGpu, &QAbstractButton::toggled, this, [setGpu](bool on) {
            setGpu(on);
            qInfo().noquote() << "Settings GPU" << on;
        });
    }

    addSection(tr("Settings"));
    addRow(icons::kSound, tr("Sound"), tr("Output, latency"), actions.sound);
    addRow(icons::kNote, tr("Library"), tr("Song folders"), actions.library);
    addRow(icons::kInfo,
            tr("Camelot wheel"),
            tr("Which keys mix well: 8A, 9A, 8B ..."),
            [this] { camelot::showWheelScreen(m_pWindow); });
    addRow(icons::kController,
            tr("Virtual controller"),
            tr("Choose what each button, fader and knob does"),
            actions.controller);
    addRow(icons::kSettings, tr("MIDI Devices"), tr("The controller over USB or Bluetooth"),
            actions.midi);
    addRow(icons::kAdvanced, tr("Advanced"), tr("Every setting"), actions.advanced);

    addSection(tr("Support"));
    addRow(icons::kHelp, tr("Online Help"), QString(), [] {
        QDesktopServices::openUrl(QUrl(kWebsite));
    });
    addRow(icons::kInfo, tr("About"), QString(), [this] { showAbout(); });

    addFooter(tr("Version %1").arg(QStringLiteral(DJMANTRA_VERSION)));

    show();
    raise();
    setFocus();
    qInfo() << "Settings screen open" << geometry() << "visible" << isVisible();
}

void SettingsScreen::showAbout() {
    auto* pAbout = new Screen(m_pWindow, tr("About"));
    pAbout->addSection(tr("DJ Mantra"));
    pAbout->addRow(nullptr,
            tr("Version"),
            QStringLiteral(DJMANTRA_VERSION) + QStringLiteral(" (") +
                    VersionStore::gitVersion() + QStringLiteral(")"),
            nullptr);
    pAbout->addRow(nullptr, tr("Engine"),
            // The licence credit keeps the engine's name (outside tr(): the
            // BrandTranslator would put DJ Mantra in)
            tr("%1 %2, free software (GPL)")
                    .arg(QStringLiteral("Mixxx"), VersionStore::version()),
            nullptr);
    pAbout->addRow(nullptr, tr("Website"), kWebsite, [] {
        QDesktopServices::openUrl(QUrl(kWebsite));
    });
    pAbout->show();
    pAbout->raise();
    pAbout->setFocus();
}

namespace {
// The menu sheet starts under the skin's top row: the selfie camera hole sits
// in its middle (round 14: the sheet's close button was under the hole)
constexpr int kSheetTop = 44;
} // namespace

MainMenu::MainMenu(QWidget* pWindow, const Actions& actions)
        : QWidget(pWindow),
          m_pWindow(pWindow),
          m_pRecording(new ControlProxy(QStringLiteral("[Recording]"),
                  QStringLiteral("status"),
                  this,
                  ControlFlag::NoAssertIfMissing)) {
    setAttribute(Qt::WA_DeleteOnClose);
    setFocusPolicy(Qt::StrongFocus);
    setStyleSheet(QStringLiteral(
            "QLabel { color: %1; background: transparent; }"
            "QToolButton { background: transparent; border: none; color: %1; font-size: 16px; }"
            "QToolButton:pressed { background-color: %2; border-radius: 12px; }"
            "QToolButton#Tile { font-size: 14px; font-weight: 600; border-radius: 18px; }"
            "QToolButton#Tile:pressed { border: 3px solid white; }")
                    .arg(kText.name(), kPressed.name()));

    // The sheet hangs from the top; the rest of the screen is dimmed and
    // closes the menu when tapped
    m_pSheet = new QWidget(this);
    auto* pLayout = new QVBoxLayout(m_pSheet);
    pLayout->setContentsMargins(20, 14, 20, 22);
    pLayout->setSpacing(14);

    auto* pClose = new QToolButton(m_pSheet);
    pClose->setIcon(icon(icons::kClose, kText, 26));
    pClose->setIconSize(QSize(26, 26));
    pClose->setFixedSize(52, 52);
    pClose->setStyleSheet(QStringLiteral(
            "QToolButton { border: 2px solid %1; border-radius: 26px; }")
                    .arg(kText.name()));
    connect(pClose, &QToolButton::clicked, this, &QWidget::close);
    pLayout->addWidget(pClose, 0, Qt::AlignHCenter);

    // no "DJ Mantra" title on the sheet (Marko, 12.10.2026: no logo text)

    // Large tiles, coloured like djay's mode tiles
    auto* pTiles = new QHBoxLayout();
    pTiles->setSpacing(16);
    auto tile = [this](const char* path,
                        const QString& text,
                        const QString& gradient,
                        const std::function<void()>& action) {
        auto* pTile = new QToolButton(m_pSheet);
        pTile->setObjectName(QStringLiteral("Tile"));
        pTile->setText(text);
        pTile->setIcon(icon(path, Qt::white, 34));
        pTile->setIconSize(QSize(34, 34));
        pTile->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        pTile->setFixedSize(108, 108);
        pTile->setStyleSheet(QStringLiteral("QToolButton#Tile { background: %1; }").arg(gradient));
        connect(pTile, &QToolButton::clicked, this, [this, action] {
            close();
            if (action) {
                action();
            }
        });
        return pTile;
    };
    pTiles->addStretch(1);
    pTiles->addWidget(tile(icons::kNote,
            tr("LIBRARY"),
            QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #19C6FF, stop:1 #1E6BFF)"),
            actions.library));
    pTiles->addWidget(tile(icons::kController,
            tr("CONTROLLER"),
            QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #FF9F1C, stop:1 #F0542D)"),
            actions.controller));
    pTiles->addWidget(tile(icons::kNote,
            actions.inOneDeck ? tr("TWO DECKS") : tr("ONE DECK"),
            QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #FFD400, stop:1 #FF9800)"),
            actions.oneDeck));
    pTiles->addStretch(1);
    pLayout->addLayout(pTiles);

    // Small actions: REC and Settings
    auto* pRow = new QHBoxLayout();
    pRow->setSpacing(8);
    auto* pRec = new QToolButton(m_pSheet);
    pRec->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pRec->setIconSize(QSize(34, 34));
    pRec->setFixedHeight(56);
    auto updateRec = [this, pRec] {
        const bool recording = m_pRecording->valid() && m_pRecording->get() >= 2;
        pRec->setText(recording ? tr("STOP REC") : tr("REC"));
        QPixmap pixmap(34 * 3, 34 * 3);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.scale(3, 3);
        painter.setPen(QPen(kText, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QPointF(17, 17), 15, 15);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0xFF, 0x30, 0x4A));
        if (recording) {
            painter.drawRoundedRect(QRectF(11, 11, 12, 12), 2, 2);
        } else {
            painter.drawEllipse(QPointF(17, 17), 9, 9);
        }
        painter.end();
        pixmap.setDevicePixelRatio(3);
        pRec->setIcon(QIcon(pixmap));
    };
    updateRec();
    m_pRecording->connectValueChanged(this, [updateRec](double) { updateRec(); });
    connect(pRec, &QToolButton::clicked, this, [] {
        ControlProxy toggle(QStringLiteral("[Recording]"),
                QStringLiteral("toggle_recording"),
                nullptr,
                ControlFlag::NoAssertIfMissing);
        if (toggle.valid()) {
            toggle.set(1);
            toggle.set(0);
        }
    });
    auto* pSettings = new QToolButton(m_pSheet);
    pSettings->setText(tr("Settings"));
    pSettings->setIcon(icon(icons::kSettings, kText, 34));
    pSettings->setIconSize(QSize(34, 34));
    pSettings->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pSettings->setFixedHeight(56);
    const auto settingsAction = actions.settings;
    connect(pSettings, &QToolButton::clicked, this, [this, settingsAction] {
        // round 12: "a tap on Settings just closed the menu"; logged to see where it stops
        qInfo() << "Menu: Settings tapped";
        close();
        if (settingsAction) {
            settingsAction();
        }
    });
    pRow->addStretch(1);
    pRow->addWidget(pRec);
    pRow->addSpacing(24);
    pRow->addWidget(pSettings);
    pRow->addStretch(1);
    pLayout->addLayout(pRow);

    m_pWindow->installEventFilter(this);
    setGeometry(m_pWindow->rect());
    // Android shows the system bars again over a new screen (round 7)
    AndroidWindow::hideSystemBars();
    m_pSheet->setGeometry(0, kSheetTop, width(), m_pSheet->sizeHint().height());
    show();
    raise();
    setFocus();
}

bool MainMenu::eventFilter(QObject* pObject, QEvent* pEvent) {
    if (pObject == m_pWindow && pEvent->type() == QEvent::Resize) {
        setGeometry(m_pWindow->rect());
        m_pSheet->setGeometry(0, kSheetTop, width(), m_pSheet->sizeHint().height());
    }
    return QWidget::eventFilter(pObject, pEvent);
}

void MainMenu::keyPressEvent(QKeyEvent* pEvent) {
    if (pEvent->key() == Qt::Key_Back || pEvent->key() == Qt::Key_Escape) {
        close();
        pEvent->accept();
        return;
    }
    QWidget::keyPressEvent(pEvent);
}

void MainMenu::mousePressEvent(QMouseEvent* pEvent) {
    // Accepted, so the release comes here too
    pEvent->accept();
}

void MainMenu::mouseReleaseEvent(QMouseEvent* pEvent) {
    // A tap below the sheet closes the menu
    if (!m_pSheet->geometry().contains(pEvent->position().toPoint())) {
        close();
    }
}

void MainMenu::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(0, 0, 0, 140));
    QPainterPath sheet;
    const QRectF sheetRect = m_pSheet->geometry();
    sheet.addRoundedRect(sheetRect.adjusted(0, -40, 0, 0), 28, 28);
    painter.fillPath(sheet, QColor(0x0E, 0x0F, 0x11));
}

} // namespace djmantra
