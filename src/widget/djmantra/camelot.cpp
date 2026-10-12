#include "widget/djmantra/camelot.h"

#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include "control/controlproxy.h"
#include "moc_camelot.cpp"
#include "track/keyutils.h"
#include "widget/djmantra/mantraui.h"

using mixxx::track::io::key::ChromaticKey;

namespace djmantra::camelot {

namespace {

const QColor kDeck1(0x3a, 0xd1, 0x3a);
const QColor kDeck2(0xe0, 0x4a, 0xd0);
const QColor kAmber(0xff, 0xb3, 0x00);
const QColor kRed(0xe5, 0x39, 0x35);

// The wheel's colour of a number, round the hue circle as on printed wheels
QColor segmentColor(int number, bool major) {
    const int hue = ((number - 1) * 30 + 150) % 360;
    return QColor::fromHsv(hue, major ? 150 : 200, major ? 235 : 205);
}

} // namespace

Code fromKey(ChromaticKey key) {
    Code code;
    if (key == mixxx::track::io::key::INVALID) {
        return code;
    }
    // Camelot (Mixxx's "Lancelot") is Open Key turned back by 5
    int number = KeyUtils::keyToOpenKeyNumber(key) - 5;
    if (number < 1) {
        number += 12;
    }
    code.number = number;
    code.major = KeyUtils::keyIsMajor(key);
    return code;
}

ChromaticKey toKey(Code code) {
    if (!code.isValid()) {
        return mixxx::track::io::key::INVALID;
    }
    int openKey = code.number + 5;
    if (openKey > 12) {
        openKey -= 12;
    }
    return KeyUtils::openKeyNumberToKey(openKey, code.major);
}

QString text(Code code) {
    if (!code.isValid()) {
        return QStringLiteral("-");
    }
    return QString::number(code.number) + (code.major ? QChar('B') : QChar('A'));
}

QString musicalName(Code code) {
    if (!code.isValid()) {
        return QString();
    }
    return KeyUtils::keyToString(toKey(code), KeyUtils::KeyNotation::Traditional);
}

Match match(Code from, Code to, QString* pWhy) {
    QString why;
    Match result = Match::Clash;
    if (!from.isValid() || !to.isValid()) {
        result = Match::None;
    } else {
        const int delta = (to.number - from.number + 12) % 12;
        if (from.major == to.major) {
            switch (delta) {
            case 0:
                result = Match::Same;
                why = QObject::tr("same key");
                break;
            case 1:
                result = Match::Compatible;
                why = QObject::tr("+1 energy up");
                break;
            case 11:
                result = Match::Compatible;
                why = QObject::tr("-1 energy down");
                break;
            case 2:
                result = Match::Risky;
                why = QObject::tr("+2 boost");
                break;
            case 10:
                result = Match::Risky;
                why = QObject::tr("-2 drop");
                break;
            case 7:
                result = Match::Risky;
                why = QObject::tr("+7 energy boost");
                break;
            default:
                why = QObject::tr("clash");
                break;
            }
        } else if (delta == 0) {
            result = Match::Compatible;
            why = from.major ? QObject::tr("B > A: to minor") : QObject::tr("A > B: to major");
        } else if (delta == 1 || delta == 11) {
            result = Match::Risky;
            why = QObject::tr("diagonal");
        } else {
            why = QObject::tr("clash");
        }
    }
    if (pWhy) {
        *pWhy = why;
    }
    return result;
}

QColor color(Match match) {
    switch (match) {
    case Match::Same:
    case Match::Compatible:
        return kDeck1;
    case Match::Risky:
        return kAmber;
    case Match::Clash:
        return kRed;
    case Match::None:
        break;
    }
    return ui::kSubText;
}

QString matchName(Match match) {
    switch (match) {
    case Match::Same:
        return QObject::tr("Same key");
    case Match::Compatible:
        return QObject::tr("Compatible");
    case Match::Risky:
        return QObject::tr("Risky");
    case Match::Clash:
        return QObject::tr("Clash");
    case Match::None:
        break;
    }
    return QObject::tr("No key");
}

Wheel::Wheel(QWidget* pParent)
        : QWidget(pParent) {
    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
}

void Wheel::setDeckKeys(Code deck1, Code deck2) {
    m_deck1 = deck1;
    m_deck2 = deck2;
    update();
}

QSize Wheel::sizeHint() const {
    return QSize(320, 320);
}

void Wheel::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal side = qMin(width(), height()) - 8;
    const QPointF centre(width() / 2.0, height() / 2.0);
    const qreal outer = side / 2;
    const qreal middle = outer * 0.66;
    const qreal inner = outer * 0.34;
    const auto ring = [&](qreal radius) {
        return QRectF(centre.x() - radius, centre.y() - radius, 2 * radius, 2 * radius);
    };
    const bool anyDeck = m_deck1.isValid() || m_deck2.isValid();
    const auto lit = [&](Code code) {
        for (const Code& deck : {m_deck1, m_deck2}) {
            const Match m = match(deck, code);
            if (m == Match::Same || m == Match::Compatible) {
                return true;
            }
        }
        return false;
    };
    QFont font = painter.font();
    for (const bool major : {true, false}) {
        const qreal r1 = major ? outer : middle;
        const qreal r2 = major ? middle : inner;
        font.setPixelSize(qMax(9, static_cast<int>((r1 - r2) * (major ? 0.30 : 0.34))));
        font.setBold(true);
        painter.setFont(font);
        for (int number = 1; number <= 12; ++number) {
            const Code code{number, major};
            // Qt's angles: degrees counter-clockwise from 3 o'clock; 12 on top, clockwise
            const qreal mid = 90 - (number % 12) * 30;
            QPainterPath path;
            path.arcMoveTo(ring(r1), mid - 15);
            path.arcTo(ring(r1), mid - 15, 30);
            path.arcTo(ring(r2), mid + 15, -30);
            path.closeSubpath();
            QColor fill = segmentColor(number, major);
            if (anyDeck && !lit(code)) {
                fill = fill.darker(320);
            }
            painter.setPen(QPen(ui::kGround, 2));
            painter.setBrush(fill);
            painter.drawPath(path);
            // The label in the middle of the segment
            const qreal labelRadius = (r1 + r2) / 2;
            const QPointF at(centre.x() + labelRadius * qCos(qDegreesToRadians(mid)),
                    centre.y() - labelRadius * qSin(qDegreesToRadians(mid)));
            const QRectF box(at.x() - 30, at.y() - 12, 60, 24);
            painter.setPen(anyDeck && !lit(code) ? ui::kSubText : QColor(0x11, 0x12, 0x15));
            painter.drawText(box, Qt::AlignCenter, text(code));
        }
    }
    // The decks' keys ringed in their colours
    for (int deck = 0; deck < 2; ++deck) {
        const Code code = deck == 0 ? m_deck1 : m_deck2;
        if (!code.isValid()) {
            continue;
        }
        const qreal r1 = code.major ? outer : middle;
        const qreal r2 = code.major ? middle : inner;
        const qreal mid = 90 - (code.number % 12) * 30;
        // Deck 2 a little inside deck 1's ring when both have the same key
        const qreal inset = (deck == 1 && code == m_deck1) ? 4 : 1.5;
        QPainterPath path;
        path.arcMoveTo(ring(r1 - inset), mid - 15 + inset / 4);
        path.arcTo(ring(r1 - inset), mid - 15 + inset / 4, 30 - inset / 2);
        path.arcTo(ring(r2 + inset), mid + 15 - inset / 4, -(30 - inset / 2));
        path.closeSubpath();
        painter.setPen(QPen(deck == 0 ? kDeck1 : kDeck2, 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }
    // In the middle: "A minor / B major"
    font.setPixelSize(qMax(9, static_cast<int>(inner * 0.28)));
    font.setBold(false);
    painter.setFont(font);
    painter.setPen(ui::kSubText);
    painter.drawText(ring(inner), Qt::AlignCenter, QObject::tr("A minor\nB major"));
}

namespace {

ChromaticKey deckKey(int deck) {
    ControlProxy key(QStringLiteral("[Channel%1]").arg(deck),
            QStringLiteral("key"),
            nullptr,
            ControlFlag::NoAssertIfMissing);
    return KeyUtils::keyFromNumericValue(key.get());
}

QWidget* legendRow(const QColor& colour, const QString& title, const QString& text) {
    auto* pLabel = new QLabel(QStringLiteral("<span style='color:%1; font-weight:bold'>%2</span>"
                                             "<br><span style='color:%3'>%4</span>")
                                      .arg(colour.name(), title, ui::kSubText.name(), text));
    pLabel->setWordWrap(true);
    pLabel->setTextFormat(Qt::RichText);
    pLabel->setStyleSheet(QStringLiteral("background: transparent; font-size: 14px;"));
    return pLabel;
}

} // namespace

void showWheelScreen(QWidget* pWindow) {
    auto* pScreen = new ui::Screen(pWindow, QObject::tr("Camelot wheel"));
    const Code deck1 = fromKey(deckKey(1));
    const Code deck2 = fromKey(deckKey(2));
    if (deck1.isValid() || deck2.isValid()) {
        pScreen->addSection(QObject::tr("Now"));
        QString why;
        const Match m = match(deck1, deck2, &why);
        pScreen->addWidget(legendRow(color(m),
                QObject::tr("Deck 1 %1 (%2)  >  Deck 2 %3 (%4)")
                        .arg(text(deck1), musicalName(deck1), text(deck2), musicalName(deck2)),
                matchName(m) + (why.isEmpty() ? QString() : QStringLiteral(": ") + why)));
    }
    auto* pWheel = new Wheel();
    pWheel->setDeckKeys(deck1, deck2);
    pWheel->setMinimumHeight(260);
    pScreen->addWidget(pWheel);
    pScreen->addSection(QObject::tr("How keys match"));
    pScreen->addWidget(legendRow(kDeck1,
            QObject::tr("Same key, ±1, A ↔ B: compatible"),
            QObject::tr("8A goes with 8A, 7A, 9A and 8B. +1 lifts the energy, -1 calms it; "
                        "A ↔ B with the same number switches between minor and major.")));
    pScreen->addWidget(legendRow(kAmber,
            QObject::tr("+2, +7, diagonal: risky"),
            QObject::tr("8A > 10A, or 8A > 3A (+7): an energy boost that can work on a short "
                        "blend; 8A > 9B (diagonal) only with care.")));
    pScreen->addWidget(legendRow(kRed,
            QObject::tr("Anything else: clash"),
            QObject::tr("Change the song, or blend on drums only (EQ the mids out).")));
    pScreen->addWidget(legendRow(ui::kSubText,
            QObject::tr("Inner ring A = minor, outer ring B = major"),
            QObject::tr("The deck keys are ringed: deck 1 green, deck 2 magenta. The keys "
                        "include pitch changes (with key lock off the key moves with the tempo).")));
    pScreen->show();
    pScreen->raise();
    pScreen->setFocus();
}

KeyMatchBadge::KeyMatchBadge(QWidget* pParent)
        : QWidget(pParent),
          m_pKey1(new ControlProxy(QStringLiteral("[Channel1]"),
                  QStringLiteral("key"),
                  this,
                  ControlFlag::NoAssertIfMissing)),
          m_pKey2(new ControlProxy(QStringLiteral("[Channel2]"),
                  QStringLiteral("key"),
                  this,
                  ControlFlag::NoAssertIfMissing)),
          m_pPlay1(new ControlProxy(QStringLiteral("[Channel1]"),
                  QStringLiteral("play"),
                  this,
                  ControlFlag::NoAssertIfMissing)),
          m_pPlay2(new ControlProxy(QStringLiteral("[Channel2]"),
                  QStringLiteral("play"),
                  this,
                  ControlFlag::NoAssertIfMissing)) {
    for (auto* pProxy : {m_pKey1, m_pKey2, m_pPlay1, m_pPlay2}) {
        pProxy->connectValueChanged(this, [this](double) { update(); });
    }
    setAttribute(Qt::WA_OpaquePaintEvent, false);
}

QSize KeyMatchBadge::sizeHint() const {
    return QSize(118, 34);
}

QSize KeyMatchBadge::minimumSizeHint() const {
    return QSize(96, 34);
}

void KeyMatchBadge::paintEvent(QPaintEvent*) {
    const Code key1 = fromKey(KeyUtils::keyFromNumericValue(m_pKey1->get()));
    const Code key2 = fromKey(KeyUtils::keyFromNumericValue(m_pKey2->get()));
    // From the playing deck to the other one (deck 1 > 2 when both or none play)
    const bool reverse = m_pPlay2->get() > 0 && m_pPlay1->get() <= 0;
    const Code from = reverse ? key2 : key1;
    const Code to = reverse ? key1 : key2;
    QString why;
    const Match m = match(from, to, &why);
    const QColor colour = color(m);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF box = QRectF(rect()).adjusted(1, 1, -1, -1);
    painter.setPen(QPen(colour, 1.5));
    painter.setBrush(ui::kTile);
    painter.drawRoundedRect(box, 8, 8);

    QFont font = painter.font();
    font.setPixelSize(13);
    font.setBold(true);
    painter.setFont(font);
    const QRectF top(box.left(), box.top() + 1, box.width(), box.height() * 0.55);
    const QRectF bottom(box.left(), top.bottom() - 1, box.width(), box.height() - top.height());
    painter.setPen(reverse ? kDeck2 : kDeck1);
    const QString left = text(from);
    const QString right = text(to);
    const QFontMetrics metrics(font);
    const QString arrow = QStringLiteral("  >  ");
    const int total = metrics.horizontalAdvance(left + arrow + right);
    qreal x = box.center().x() - total / 2.0;
    painter.drawText(QRectF(x, top.top(), total, top.height()), Qt::AlignLeft | Qt::AlignVCenter, left);
    x += metrics.horizontalAdvance(left);
    painter.setPen(ui::kSubText);
    painter.drawText(QRectF(x, top.top(), total, top.height()), Qt::AlignLeft | Qt::AlignVCenter, arrow);
    x += metrics.horizontalAdvance(arrow);
    painter.setPen(reverse ? kDeck1 : kDeck2);
    painter.drawText(QRectF(x, top.top(), total, top.height()), Qt::AlignLeft | Qt::AlignVCenter, right);

    font.setPixelSize(10);
    font.setBold(false);
    painter.setFont(font);
    painter.setPen(colour);
    painter.drawText(bottom, Qt::AlignHCenter | Qt::AlignTop, m == Match::None ? tr("key") : why);
}

void KeyMatchBadge::mousePressEvent(QMouseEvent* pEvent) {
    if (pEvent->button() == Qt::LeftButton) {
        showWheelScreen(window());
    }
}

} // namespace djmantra::camelot
