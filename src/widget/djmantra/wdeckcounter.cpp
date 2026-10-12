#include "widget/djmantra/wdeckcounter.h"

#include <QMouseEvent>
#include <QPainter>
#include <algorithm>
#include <cmath>

#include "control/controlproxy.h"
#include "moc_wdeckcounter.cpp"
#include "util/duration.h"

WDeckCounter::WDeckCounter(const QString& group, QWidget* pParent)
        : WLabel(pParent),
          m_pTimeElapsed(proxy(group, QStringLiteral("time_elapsed"))),
          m_pTimeRemaining(proxy(group, QStringLiteral("time_remaining"))),
          m_pBeatInPhrase(proxy(group, QStringLiteral("beat_in_phrase"))),
          m_pBeatNumber(proxy(group, QStringLiteral("beat_number"))),
          m_pBeatsPerBar(proxy(group, QStringLiteral("beats_per_bar"))),
          m_pBeatUnit(proxy(group, QStringLiteral("beat_unit"))),
          m_pSectionBeatsLeft(proxy(group, QStringLiteral("section_beats_left"))),
          m_pSectionFound(proxy(group, QStringLiteral("section_found"))) {
    slotUpdate();
}

ControlProxy* WDeckCounter::proxy(const QString& group, const QString& item) {
    auto* pProxy = new ControlProxy(group, item, this, ControlFlag::NoAssertIfMissing);
    pProxy->connectValueChanged(this, &WDeckCounter::slotUpdate);
    return pProxy;
}

// static
QString WDeckCounter::barBeat(int number, int beatsPerBar) {
    if (beatsPerBar < 1) {
        beatsPerBar = 4;
    }
    // Floor division, so that the beat before "1" is the last beat of bar 0
    const int bar = static_cast<int>(std::floor(static_cast<double>(number) / beatsPerBar));
    const int beat = number - bar * beatsPerBar;
    return QString::number(bar + 1) + QChar('.') + QString::number(beat + 1);
}

// static
QString WDeckCounter::barsBeatsLeft(int beats, int beatsPerBar) {
    if (beatsPerBar < 1) {
        beatsPerBar = 4;
    }
    beats = std::max(0, beats);
    return QChar('-') + QString::number(beats / beatsPerBar) + QChar('.') +
            QString::number(beats % beatsPerBar);
}

// static
QString WDeckCounter::timeText(double elapsed, double remaining) {
    using mixxx::Duration;
    const auto format = [](double seconds) {
        return Duration::formatTime(std::max(0.0, seconds), Duration::Precision::SECONDS);
    };
    return format(elapsed) + QStringLiteral(" / ") + format(elapsed + remaining) +
            QStringLiteral(" / -") + format(remaining);
}

QSize WDeckCounter::minimumSizeHint() const {
    return QSize(fontMetrics().height() * 3, QLabel::minimumSizeHint().height());
}

void WDeckCounter::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const QRect box = contentsRect();
    QFont font = this->font();
    const QString shown = text();
    // Shrink until it fits (not below 7 px)
    for (int size = QFontInfo(font).pixelSize(); size > 7; --size) {
        font.setPixelSize(size);
        if (QFontMetrics(font).horizontalAdvance(shown) <= box.width()) {
            break;
        }
    }
    painter.setFont(font);
    painter.setPen(palette().color(foregroundRole()));
    painter.drawText(box, static_cast<int>(alignment()) | Qt::AlignVCenter, shown);
}

void WDeckCounter::mousePressEvent(QMouseEvent* pEvent) {
    if (pEvent->button() == Qt::LeftButton) {
        m_bars = !m_bars;
        slotUpdate();
    }
}

void WDeckCounter::slotUpdate() {
    if (!m_bars) {
        setText(timeText(m_pTimeElapsed->get(), m_pTimeRemaining->get()));
        return;
    }
    const int beatsPerBar = static_cast<int>(m_pBeatsPerBar->get());
    const int unit = static_cast<int>(m_pBeatUnit->get());
    const QString signature = QString::number(beatsPerBar > 0 ? beatsPerBar : 4) +
            QChar('/') + QString::number(unit > 0 ? unit : 4);
    if (m_pBeatInPhrase->get() < 0) {
        setText(QStringLiteral("-.-  ") + signature);
        return;
    }
    QString text = barBeat(static_cast<int>(m_pBeatNumber->get()), beatsPerBar);
    const int left = static_cast<int>(m_pSectionBeatsLeft->get());
    if (left >= 0) {
        text += (m_pSectionFound->get() > 0 ? QStringLiteral("  next ")
                                            : QStringLiteral("  end "));
        text += barsBeatsLeft(left, beatsPerBar);
    }
    setText(text + QStringLiteral("  ") + signature);
}
