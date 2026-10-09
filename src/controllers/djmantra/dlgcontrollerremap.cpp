#include "controllers/djmantra/dlgcontrollerremap.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScroller>
#include <QVBoxLayout>
#include <algorithm>

#include "controllers/controlpickermenu.h"
#include "controllers/djmantra/controlleroverrides.h"
#include "moc_dlgcontrollerremap.cpp"

namespace djmantra {

namespace {

const QColor kAmber(0xE8, 0xA3, 0x3D);
const QColor kText(0xF2, 0xDD, 0xB4);

const char* kStyle = R"(
QDialog, QWidget#RemapTop { background-color: #0B0D10; color: #F2DDB4; }
QLabel { color: #F2DDB4; font-size: 15px; }
QLabel#RemapTitle { font-size: 18px; font-weight: bold; }
QPushButton { color: #F2DDB4; background-color: #16171A; border: 2px solid #33383E;
              border-radius: 8px; padding: 6px 14px; font-size: 15px; font-weight: bold;
              min-height: 30px; }
QPushButton:checked { color: #0B0D10; background-color: #E8A33D; border-color: #E8A33D; }
QLineEdit { color: #F2DDB4; background-color: #16171A; border: 2px solid #33383E;
            border-radius: 8px; padding: 6px 10px; font-size: 16px; min-height: 30px; }
QListWidget { color: #F2DDB4; background-color: #0E0F11; border: none; font-size: 16px; }
QListWidget::item { padding: 10px 6px; border-bottom: 1px solid #26282C; }
QListWidget::item:selected { background-color: #E8A33D; color: #0B0D10; }
QMenu { background-color: #16171A; color: #F2DDB4; font-size: 17px; border: 1px solid #33383E; }
QMenu::item { padding: 12px 24px; }
QMenu::item:selected { background-color: #E8A33D; color: #0B0D10; }
)";

bool isRound(const ControllerLayoutControl& control) {
    return control.shape == QLatin1String("round") || control.shape == QLatin1String("knob") ||
            control.shape == QLatin1String("encoder") || control.shape == QLatin1String("jog") ||
            control.shape == QLatin1String("led");
}

QString functionName(ControlPickerMenu* pPicker, const QString& target) {
    const ConfigKey key = ConfigKey::parseCommaSeparated(target);
    const QString title = pPicker->controlTitleForConfigKey(key);
    return title.isEmpty() ? target : title;
}

void fillScreen(QDialog* pDialog) {
#if defined(Q_OS_ANDROID)
    pDialog->setWindowState(Qt::WindowFullScreen);
#else
    pDialog->resize(1000, 600);
#endif
}

} // namespace

ControllerMapView::ControllerMapView(ControlPickerMenu* pPicker, QWidget* pParent)
        : QWidget(pParent),
          m_pPicker(pPicker),
          m_shift(false) {
    m_photo.load(ControllerOverrides::instance().imagePath());
    setMinimumSize(320, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ControllerMapView::setShift(bool shift) {
    m_shift = shift;
    update();
}

QRectF ControllerMapView::imageRect() const {
    const auto& overrides = ControllerOverrides::instance();
    const double aspect = overrides.imageHeight() > 0
            ? overrides.imageWidth() / overrides.imageHeight()
            : 16.0 / 9.0;
    double w = width();
    double h = w / aspect;
    if (h > height()) {
        h = height();
        w = h * aspect;
    }
    return QRectF((width() - w) / 2, (height() - h) / 2, w, h);
}

QRectF ControllerMapView::controlRect(const ControllerLayoutControl& control) const {
    const auto& overrides = ControllerOverrides::instance();
    const QRectF image = imageRect();
    const double unit = image.width() / std::max(overrides.imageWidth(), 1.0);
    // at least a fingertip: small buttons get a bigger touch area
    const double w = std::max(control.w * unit, 26.0);
    const double h = std::max(control.h * unit, 26.0);
    return QRectF(image.x() + control.x * unit - w / 2, image.y() + control.y * unit - h / 2, w, h);
}

void ControllerMapView::paintEvent(QPaintEvent* /*pEvent*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(0x0B, 0x0D, 0x10));
    const QRectF image = imageRect();
    if (!m_photo.isNull()) {
        painter.setOpacity(0.55);
        painter.drawPixmap(image, m_photo, m_photo.rect());
        painter.setOpacity(1.0);
    }
    const auto& overrides = ControllerOverrides::instance();
    QFont font = painter.font();
    font.setPixelSize(std::max(10, static_cast<int>(image.height() / 40)));
    font.setBold(true);
    painter.setFont(font);
    for (const auto& control : overrides.controls()) {
        if (control.inputs.isEmpty()) {
            continue;
        }
        QStringList chosen;
        for (const auto& input : control.inputs) {
            const QString target = overrides.target(input.key, m_shift);
            if (!target.isEmpty()) {
                chosen.append(functionName(m_pPicker, target));
            }
        }
        const QRectF r = controlRect(control);
        const QColor color = chosen.isEmpty() ? QColor(255, 255, 255, 150) : kAmber;
        painter.setPen(QPen(color, chosen.isEmpty() ? 1.5 : 3.0));
        painter.setBrush(chosen.isEmpty() ? QColor(0, 0, 0, 40) : QColor(232, 163, 61, 70));
        if (isRound(control)) {
            painter.drawEllipse(r);
        } else {
            painter.drawRoundedRect(r, 4, 4);
        }
        if (!chosen.isEmpty()) {
            painter.setPen(Qt::white);
            const QRectF textRect(r.center().x() - 80, r.bottom() + 1, 160, font.pixelSize() * 2.6);
            painter.drawText(textRect,
                    Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                    chosen.join(QStringLiteral(" / ")));
        }
    }
}

void ControllerMapView::mouseReleaseEvent(QMouseEvent* pEvent) {
    const auto& controls = ControllerOverrides::instance().controls();
    const QPointF pos = pEvent->position();
    // the smallest control under the finger (a button on a jog wheel wins)
    int best = -1;
    double bestArea = 0;
    for (int i = 0; i < controls.size(); ++i) {
        if (controls[i].inputs.isEmpty()) {
            continue;
        }
        const QRectF r = controlRect(controls[i]);
        if (r.contains(pos)) {
            const double area = r.width() * r.height();
            if (best < 0 || area < bestArea) {
                best = i;
                bestArea = area;
            }
        }
    }
    if (best >= 0) {
        emit controlTapped(best);
    }
}

DlgControllerRemap::DlgControllerRemap(QWidget* pParent)
        : QDialog(pParent),
          m_pPicker(new ControlPickerMenu(this)) {
    setWindowTitle(tr("Controller"));
    setStyleSheet(QString::fromLatin1(kStyle));
    setAttribute(Qt::WA_DeleteOnClose);

    auto* pTop = new QWidget(this);
    pTop->setObjectName(QStringLiteral("RemapTop"));
    auto* pTopLayout = new QHBoxLayout(pTop);
    auto* pTitle = new QLabel(tr("Hercules DJControl Mix Ultra"), pTop);
    pTitle->setObjectName(QStringLiteral("RemapTitle"));
    m_pShift = new QPushButton(tr("SHIFT"), pTop);
    m_pShift->setCheckable(true);
    auto* pReset = new QPushButton(tr("Reset all"), pTop);
    auto* pClose = new QPushButton(tr("Done"), pTop);
    pTopLayout->addWidget(pTitle);
    pTopLayout->addStretch();
    pTopLayout->addWidget(m_pShift);
    pTopLayout->addWidget(pReset);
    pTopLayout->addWidget(pClose);

    m_pView = new ControllerMapView(m_pPicker, this);
    m_pStatus = new QLabel(this);
    m_pStatus->setWordWrap(true);

    auto* pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(8, 8, 8, 8);
    pLayout->addWidget(pTop);
    pLayout->addWidget(m_pView, 1);
    pLayout->addWidget(m_pStatus);

    connect(m_pView, &ControllerMapView::controlTapped, this, &DlgControllerRemap::slotControlTapped);
    connect(m_pShift, &QPushButton::toggled, m_pView, &ControllerMapView::setShift);
    connect(m_pShift, &QPushButton::toggled, this, [this] { updateStatus(); });
    connect(pClose, &QPushButton::clicked, this, &QDialog::accept);
    connect(pReset, &QPushButton::clicked, this, [this] {
        ControllerOverrides::instance().clearAll();
        m_pView->update();
        updateStatus();
    });
    updateStatus();
    fillScreen(this);
}

void DlgControllerRemap::updateStatus() {
    const int count = ControllerOverrides::instance().count();
    m_pStatus->setText(
            (m_pShift->isChecked() ? tr("Functions with SHIFT held. ") : QString()) +
            tr("Tap a button, pad, fader, knob or jog wheel to choose what it does. "
               "Amber: your choice (%1 in all); the others follow the controller mapping.")
                    .arg(count));
}

void DlgControllerRemap::slotControlTapped(int index) {
    const auto& control = ControllerOverrides::instance().controls().at(index);
    if (control.inputs.size() == 1) {
        chooseFunction(index, 0);
        return;
    }
    // a jog wheel or the browser encoder: which part?
    QMenu menu(this);
    for (int i = 0; i < control.inputs.size(); ++i) {
        QAction* pAction = menu.addAction(control.inputs[i].label);
        pAction->setData(i);
    }
    QAction* pChosen = menu.exec(m_pView->mapToGlobal(m_pView->rect().center()));
    if (pChosen) {
        chooseFunction(index, pChosen->data().toInt());
    }
}

void DlgControllerRemap::chooseFunction(int controlIndex, int inputIndex) {
    auto& overrides = ControllerOverrides::instance();
    const auto& control = overrides.controls().at(controlIndex);
    const auto& input = control.inputs.at(inputIndex);
    const bool shift = m_pShift->isChecked();

    QDialog dialog(this);
    dialog.setStyleSheet(QString::fromLatin1(kStyle));
    auto* pLayout = new QVBoxLayout(&dialog);
    const QString deck = control.deck == QLatin1String("master")
            ? QString()
            : tr("deck %1 ").arg(control.deck);
    auto* pTitle = new QLabel(deck + input.label + (shift ? tr(" with SHIFT") : QString()), &dialog);
    pTitle->setObjectName(QStringLiteral("RemapTitle"));
    const QString current = overrides.target(input.key, shift);
    auto* pCurrent = new QLabel(current.isEmpty()
                    ? tr("Now: the controller mapping")
                    : tr("Now: %1").arg(functionName(m_pPicker, current)),
            &dialog);
    auto* pSearch = new QLineEdit(&dialog);
    pSearch->setPlaceholderText(tr("Search: play, volume, crossfader, hotcue, loop, filter..."));
    auto* pList = new QListWidget(&dialog);
    QScroller::grabGesture(pList->viewport(), QScroller::LeftMouseButtonGesture);
    pList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    auto* pDefault = new QListWidgetItem(tr("↺ Back to the controller mapping"), pList);
    pDefault->setData(Qt::UserRole, QString());
    for (const ConfigKey& key : m_pPicker->controlsAvailable()) {
        const QString title = m_pPicker->controlTitleForConfigKey(key);
        auto* pItem = new QListWidgetItem(
                (title.isEmpty() ? key.item : title) + QStringLiteral("  ·  ") + key.group,
                pList);
        pItem->setData(Qt::UserRole, QString(key.group + QLatin1Char(',') + key.item));
        pItem->setToolTip(m_pPicker->descriptionForConfigKey(key));
    }
    connect(pSearch, &QLineEdit::textChanged, pList, [pList](const QString& text) {
        for (int i = 1; i < pList->count(); ++i) {
            QListWidgetItem* pItem = pList->item(i);
            pItem->setHidden(!text.isEmpty() &&
                    !pItem->text().contains(text, Qt::CaseInsensitive) &&
                    !pItem->toolTip().contains(text, Qt::CaseInsensitive));
        }
    });
    auto* pCancel = new QPushButton(tr("Cancel"), &dialog);
    connect(pCancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(pList, &QListWidget::itemClicked, &dialog, [&dialog, pList](QListWidgetItem* pItem) {
        pList->setCurrentItem(pItem);
        dialog.accept();
    });

    auto* pTop = new QHBoxLayout();
    pTop->addWidget(pTitle);
    pTop->addStretch();
    pTop->addWidget(pCancel);
    pLayout->addLayout(pTop);
    pLayout->addWidget(pCurrent);
    pLayout->addWidget(pSearch);
    pLayout->addWidget(pList, 1);
    fillScreen(&dialog);

    if (dialog.exec() != QDialog::Accepted || !pList->currentItem()) {
        return;
    }
    const QString chosen = pList->currentItem()->data(Qt::UserRole).toString();
    if (chosen.isEmpty()) {
        overrides.clear(input.key, shift);
    } else {
        overrides.setTarget(input.key, shift, chosen);
    }
    m_pView->update();
    updateStatus();
}

} // namespace djmantra
