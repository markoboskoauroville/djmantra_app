#include "widget/djmantra/wbarbeat.h"

#include <cmath>

#include "control/controlproxy.h"
#include "moc_wbarbeat.cpp"

WBarBeat::WBarBeat(const QString& group, QWidget* pParent)
        : WLabel(pParent),
          m_pBeatInPhrase(new ControlProxy(
                  group, "beat_in_phrase", this, ControlFlag::NoAssertIfMissing)),
          m_pBeatNumber(new ControlProxy(
                  group, "beat_number", this, ControlFlag::NoAssertIfMissing)),
          m_pBeatsPerBar(new ControlProxy(
                  group, "beats_per_bar", this, ControlFlag::NoAssertIfMissing)),
          m_pBeatUnit(new ControlProxy(
                  group, "beat_unit", this, ControlFlag::NoAssertIfMissing)) {
    m_pBeatInPhrase->connectValueChanged(this, &WBarBeat::slotUpdate);
    m_pBeatNumber->connectValueChanged(this, &WBarBeat::slotUpdate);
    m_pBeatsPerBar->connectValueChanged(this, &WBarBeat::slotUpdate);
    m_pBeatUnit->connectValueChanged(this, &WBarBeat::slotUpdate);
    slotUpdate();
}

// static
QString WBarBeat::barBeat(int number, int beatsPerBar) {
    if (beatsPerBar < 1) {
        beatsPerBar = 4;
    }
    // Floor division, so that the beat before "1" is the last beat of bar 0
    const int bar = static_cast<int>(std::floor(static_cast<double>(number) / beatsPerBar));
    const int beat = number - bar * beatsPerBar;
    return QString::number(bar + 1) + QChar('.') + QString::number(beat + 1);
}

void WBarBeat::slotUpdate() {
    const int beatsPerBar = static_cast<int>(m_pBeatsPerBar->get());
    const int unit = static_cast<int>(m_pBeatUnit->get());
    const QString signature = QString::number(beatsPerBar > 0 ? beatsPerBar : 4) +
            QChar('/') + QString::number(unit > 0 ? unit : 4);
    const QString position = m_pBeatInPhrase->get() < 0
            ? QStringLiteral("-.-")
            : barBeat(static_cast<int>(m_pBeatNumber->get()), beatsPerBar);
    setText(position + QStringLiteral("  ") + signature);
}
