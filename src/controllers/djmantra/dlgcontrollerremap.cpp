#include "controllers/djmantra/dlgcontrollerremap.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QApplication>
#include <QPointer>
#include <QPushButton>
#include <QScroller>
#include <QVBoxLayout>
#include <algorithm>

#include "controllers/controlpickermenu.h"
#include "controllers/djmantra/controlleroverrides.h"
#include "moc_dlgcontrollerremap.cpp"

namespace djmantra {

namespace {

// Controller Mapper (Mantra) colours, orange and green swapped (Marko, 9.10.2026)
const QColor kGround(0x0B, 0x0D, 0x10);
const QColor kBody(0x0E, 0x0F, 0x11);
const QColor kCap(0x16, 0x17, 0x1A);
const QColor kCapRing(255, 255, 255, 15);
const QColor kGroove(0x26, 0x28, 0x2C);
const QColor kSand(0xF2, 0xDD, 0xB4);
const QColor kDimSand(242, 221, 180, 115);
const QColor kOrange(0xE8, 0xA3, 0x3D);
const QColor kGreen(0x33, 0xD1, 0x7A);

const char* kStyle = R"(
QDialog, QWidget#RemapTop { background-color: #0B0D10; color: #F2DDB4; }
QLabel { color: #F2DDB4; font-size: 13px; }
QLabel#RemapTitle { font-size: 18px; font-weight: bold; }
QLabel#RemapSelected { color: #33D17A; font-size: 16px; font-weight: bold; }
QPushButton { color: #F2DDB4; background-color: #16171A; border: 2px solid #33383E;
              border-radius: 8px; padding: 6px 14px; font-size: 15px; font-weight: bold;
              min-height: 30px; }
QPushButton:checked { color: #0B0D10; background-color: #33D17A; border-color: #33D17A; }
QPushButton:disabled { color: #33383E; border-color: #26282C; }
QLineEdit { color: #F2DDB4; background-color: #16171A; border: 2px solid #33383E;
            border-radius: 8px; padding: 6px 10px; font-size: 16px; min-height: 30px; }
QListWidget { color: #F2DDB4; background-color: #0E0F11; border: none; font-size: 16px; }
QListWidget::item { padding: 10px 6px; border-bottom: 1px solid #26282C; }
QListWidget::item:selected { background-color: #33D17A; color: #0B0D10; }
QMenu { background-color: #16171A; color: #F2DDB4; font-size: 17px; border: 1px solid #33383E; }
QMenu::item { padding: 12px 24px; }
QMenu::item:selected { background-color: #33D17A; color: #0B0D10; }
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
          m_shift(false),
          m_selected(-1) {
    setMinimumSize(320, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ControllerMapView::setShift(bool shift) {
    m_shift = shift;
    update();
}

void ControllerMapView::setSelected(int index) {
    m_selected = index;
    update();
}

// The controller's body fills the view (a small margin for the labels under
// the bottom row)
QRectF ControllerMapView::area() const {
    return ControllerOverrides::instance().outline().adjusted(-0.3, -0.3, 0.3, 0.6);
}

double ControllerMapView::unit() const {
    const QRectF a = area();
    return std::min(width() / std::max(a.width(), 1.0), height() / std::max(a.height(), 1.0));
}

QPointF ControllerMapView::origin() const {
    const QRectF a = area();
    const double u = unit();
    return QPointF((width() - a.width() * u) / 2 - a.x() * u,
            (height() - a.height() * u) / 2 - a.y() * u);
}

QRectF ControllerMapView::controlRect(const ControllerLayoutControl& control) const {
    const double u = unit();
    const QPointF o = origin();
    return QRectF(o.x() + (control.x - control.w / 2) * u,
            o.y() + (control.y - control.h / 2) * u,
            control.w * u,
            control.h * u);
}

// Drawn like Marko's Controller Mapper (Mantra): the controller in simple
// shapes, dark caps, monospaced labels. Orange: a control; green: the
// selected one (tapped here or pressed on the controller); white text under a
// control: the function chosen for it.
void ControllerMapView::paintEvent(QPaintEvent* /*pEvent*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), kGround);
    const auto& overrides = ControllerOverrides::instance();
    const double u = unit();
    const QPointF o = origin();
    const QRectF body = overrides.outline();
    p.setPen(QPen(QColor(255, 255, 255, 13), 1));
    p.setBrush(kBody);
    p.drawRoundedRect(QRectF(o.x() + body.x() * u, o.y() + body.y() * u,
                              body.width() * u, body.height() * u),
            u * 0.5,
            u * 0.5);

    QFont mono(QStringLiteral("monospace"));
    mono.setStyleHint(QFont::Monospace);
    auto setFont = [&](double size, bool bold = false) {
        mono.setPixelSize(std::max(7, static_cast<int>(size)));
        mono.setBold(bold);
        p.setFont(mono);
    };
    // the deck names, as on the controller
    setFont(u * 0.42);
    p.setPen(kDimSand);
    p.drawText(QPointF(o.x() + 3.9 * u, o.y() + 11.8 * u), QStringLiteral("deck 1"));
    p.drawText(QPointF(o.x() + 22.0 * u, o.y() + 11.8 * u), QStringLiteral("deck 2"));

    const auto& controls = overrides.controls();
    for (int i = 0; i < controls.size(); ++i) {
        const auto& c = controls[i];
        const QRectF r = controlRect(c);
        const bool selected = i == m_selected;
        const QColor col = selected ? kGreen : kOrange;
        QStringList chosen;
        for (const auto& input : c.inputs) {
            const QString target = overrides.target(input.key, m_shift);
            if (!target.isEmpty()) {
                chosen.append(functionName(m_pPicker, target));
            }
        }
        const QColor ring = selected ? kGreen : (chosen.isEmpty() ? kCapRing : kOrange);
        const double lw = selected ? std::max(2.0, u * 0.1) : 1.0;
        p.setPen(Qt::NoPen);
        if (c.shape == QLatin1String("led")) {
            p.setBrush(QColor(0xFF, 0x3B, 0x30));
            p.drawEllipse(r);
        } else if (c.shape == QLatin1String("round")) {
            p.setBrush(selected ? QColor(51, 209, 122, 140) : kCap);
            p.setPen(QPen(ring, lw));
            p.drawEllipse(r);
            setFont(u * 0.36);
            p.setPen(col);
            p.drawText(r, Qt::AlignCenter, c.label);
        } else if (c.shape == QLatin1String("knob") || c.shape == QLatin1String("encoder")) {
            p.setBrush(selected ? QColor(51, 209, 122, 90) : kCap);
            p.setPen(QPen(c.shape == QLatin1String("encoder") ? QColor(242, 221, 180, 64)
                                                             : QColor(255, 255, 255, 15),
                    c.shape == QLatin1String("encoder") ? u * 0.12 : 1));
            p.drawEllipse(r);
            if (selected || !chosen.isEmpty()) {
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(ring, lw));
                p.drawEllipse(r.adjusted(-u * 0.12, -u * 0.12, u * 0.12, u * 0.12));
            }
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            const double iw = std::max(2.0, u * 0.08);
            p.drawRect(QRectF(r.center().x() - iw / 2, r.top() + r.height() * 0.12,
                    iw, r.height() * 0.36));
            setFont(u * 0.26, true);
            p.setPen(col);
            p.drawText(QRectF(r.center().x() - 2 * u, r.bottom() + u * 0.05, 4 * u, u * 0.5),
                    Qt::AlignHCenter | Qt::AlignTop,
                    c.label);
        } else if (c.shape == QLatin1String("fader-v") || c.shape == QLatin1String("fader-h")) {
            const bool vertical = c.shape == QLatin1String("fader-v");
            p.setBrush(kGroove);
            const double g = std::max(3.0, u * 0.12);
            p.drawRoundedRect(vertical ? QRectF(r.center().x() - g / 2, r.top(), g, r.height())
                                       : QRectF(r.left(), r.center().y() - g / 2, r.width(), g),
                    2,
                    2);
            const QRectF capRect = vertical
                    ? QRectF(r.center().x() - u * 0.6, r.center().y() - u * 0.27, u * 1.2, u * 0.55)
                    : QRectF(r.center().x() - u * 0.27, r.center().y() - u * 0.55, u * 0.55, u * 1.1);
            p.setBrush(selected ? QColor(51, 209, 122, 128) : kCap);
            p.setPen(QPen(selected ? kGreen : QColor(255, 255, 255, 26), lw));
            p.drawRoundedRect(capRect, 3, 3);
            p.setPen(QPen(col, 2));
            if (vertical) {
                p.drawLine(QPointF(capRect.left() + 2, capRect.center().y()),
                        QPointF(capRect.right() - 2, capRect.center().y()));
            } else {
                p.drawLine(QPointF(capRect.center().x(), capRect.top() + 2),
                        QPointF(capRect.center().x(), capRect.bottom() - 2));
            }
            if (!c.label.isEmpty()) {
                setFont(u * 0.26, true);
                p.drawText(QRectF(r.center().x() - 2 * u, r.bottom() + u * 0.1, 4 * u, u * 0.5),
                        Qt::AlignHCenter | Qt::AlignTop,
                        c.label);
            }
        } else if (c.shape == QLatin1String("jog")) {
            p.setBrush(QColor(0x12, 0x13, 0x16));
            p.setPen(QPen(selected ? kGreen : QColor(255, 255, 255, 20), selected ? u * 0.3 : u * 0.25));
            p.drawEllipse(r.adjusted(u * 0.12, u * 0.12, -u * 0.12, -u * 0.12));
            const QRectF top = r.adjusted(u * 0.75, u * 0.75, -u * 0.75, -u * 0.75);
            p.setBrush(selected ? QColor(51, 209, 122, 46) : kCap);
            p.setPen(QPen(QColor(255, 255, 255, 15), 1));
            p.drawEllipse(top);
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawEllipse(QRectF(r.center().x() - u * 0.6, r.center().y() - u * 0.6, u * 1.2, u * 1.2));
            p.setBrush(kSand);
            p.drawEllipse(QRectF(r.center().x() - u * 0.17, top.top() + u * 0.35, u * 0.35, u * 0.35));
        } else {
            // pads and buttons
            const bool pad = c.shape == QLatin1String("pad");
            const double radius = u * (pad ? 0.15 : 0.12);
            p.setBrush(selected ? QColor(51, 209, 122, 150) : kCap);
            p.setPen(QPen(ring, lw));
            p.drawRoundedRect(r, radius, radius);
            setFont(pad ? u * 0.3 : std::min(u * 0.3, r.height() * 0.55));
            p.setPen(col);
            p.drawText(pad ? r.adjusted(0, 0, -u * 0.15, -u * 0.1) : r,
                    pad ? (Qt::AlignRight | Qt::AlignBottom) : Qt::AlignCenter,
                    c.label);
        }
        if (!chosen.isEmpty()) {
            // the chosen function, in white under the control
            setFont(u * 0.24, true);
            p.setPen(Qt::white);
            p.drawText(QRectF(r.center().x() - 2.2 * u, r.bottom() + u * 0.45, 4.4 * u, u * 0.9),
                    Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                    chosen.join(QStringLiteral(" / ")));
        }
    }
}

void ControllerMapView::mouseReleaseEvent(QMouseEvent* pEvent) {
    const auto& controls = ControllerOverrides::instance().controls();
    const QPointF pos = pEvent->position();
    // the smallest control under the finger, with a fingertip's margin
    const double margin = std::max(6.0, unit() * 0.25);
    int best = -1;
    double bestArea = 0;
    for (int i = 0; i < controls.size(); ++i) {
        if (controls[i].inputs.isEmpty()) {
            continue;
        }
        const QRectF r = controlRect(controls[i]).adjusted(-margin, -margin, margin, margin);
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
    // the selected control: what it does now, and CHANGE
    auto* pSelectedRow = new QHBoxLayout();
    m_pSelected = new QLabel(this);
    m_pSelected->setObjectName(QStringLiteral("RemapSelected"));
    m_pSelected->setWordWrap(true);
    m_pChange = new QPushButton(tr("CHANGE"), this);
    m_pChange->setEnabled(false);
    pSelectedRow->addWidget(m_pSelected, 1);
    pSelectedRow->addWidget(m_pChange);

    auto* pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(6, 2, 6, 4);
    pLayout->setSpacing(2);
    pTopLayout->setContentsMargins(4, 2, 4, 2);
    pLayout->addWidget(pTop);
    pLayout->addWidget(m_pView, 1);
    pLayout->addLayout(pSelectedRow);
    pLayout->addWidget(m_pStatus);

    connect(m_pView, &ControllerMapView::controlTapped, this, &DlgControllerRemap::slotControlTapped);
    connect(m_pChange, &QPushButton::clicked, this, [this] {
        if (m_pView->selected() >= 0) {
            slotControlTapped(m_pView->selected());
        }
    });
    // A press or move on the real controller selects that control here
    QPointer<DlgControllerRemap> pSelf(this);
    ControllerOverrides::instance().setMidiObserver([pSelf](int status, int data1) {
        QMetaObject::invokeMethod(
                qApp,
                [pSelf, status, data1] {
                    if (pSelf) {
                        pSelf->selectFromMidi(status, data1);
                    }
                },
                Qt::QueuedConnection);
    });
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

DlgControllerRemap::~DlgControllerRemap() {
    ControllerOverrides::instance().setMidiObserver({});
}

void DlgControllerRemap::selectFromMidi(int status, int data1) {
    int control;
    int input;
    if (ControllerOverrides::instance().findInput(status, data1, &control, &input)) {
        // SHIFT layer messages switch the view to SHIFT
        const auto& inputs = ControllerOverrides::instance().controls().at(control).inputs;
        bool shift = false;
        for (const auto& midi : inputs.at(input).midiShift) {
            if (midi.first == status && midi.second == data1) {
                shift = true;
            }
        }
        if (shift != m_pShift->isChecked()) {
            m_pShift->setChecked(shift);
        }
        select(control);
    }
}

void DlgControllerRemap::select(int index) {
    m_pView->setSelected(index);
    const auto& overrides = ControllerOverrides::instance();
    const auto& control = overrides.controls().at(index);
    QStringList lines;
    for (const auto& input : control.inputs) {
        const QString target = overrides.target(input.key, m_pShift->isChecked());
        lines.append(input.label + QStringLiteral(": ") +
                (target.isEmpty() ? tr("controller mapping")
                                  : functionName(m_pPicker, target)));
    }
    const QString deck = control.deck == QLatin1String("master")
            ? QString()
            : tr("Deck %1 · ").arg(control.deck);
    m_pSelected->setText(deck + lines.join(QStringLiteral("   ")));
    m_pChange->setEnabled(true);
}

void DlgControllerRemap::updateStatus() {
    const int count = ControllerOverrides::instance().count();
    m_pStatus->setText(
            (m_pShift->isChecked() ? tr("Functions with SHIFT held. ") : QString()) +
            tr("Press a control on the Mix Ultra or tap it here · green: selected · "
               "white: your choice (%1)")
                    .arg(count));
}

void DlgControllerRemap::slotControlTapped(int index) {
    select(index);
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
    select(controlIndex);
    updateStatus();
}

} // namespace djmantra
