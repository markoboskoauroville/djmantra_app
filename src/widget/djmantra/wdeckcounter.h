#pragma once

#include "widget/wlabel.h"

class ControlProxy;

/// DJ Mantra's deck counter (Marko, 11.10.2026). A tap switches between:
///   TIME            elapsed / length / remaining, m:ss only: "1:23 / 5:40 / -4:16"
///   BARS & BEATS    Ableton's bar.beat, the bars and beats left to the next
///                   section (or to the end when no section is found) and
///                   the time signature: "17.3  next -6.2  4/4"
/// Counted on the beat grid from the deck's "1" (PhraseControl).
class WDeckCounter : public WLabel {
    Q_OBJECT
  public:
    explicit WDeckCounter(const QString& group, QWidget* pParent = nullptr);

    /// "bar.beat" for beat `number` (0-based from "1"), bars and beats
    /// from 1; before "1" the bars run 0, -1, ... as in Ableton.
    static QString barBeat(int number, int beatsPerBar);
    /// `beats` left as "-bars.beats", e.g. 26 beats in 4/4: "-6.2"
    static QString barsBeatsLeft(int beats, int beatsPerBar);
    /// "1:23 / 5:40 / -4:16"
    static QString timeText(double elapsed, double remaining);

    bool showsBars() const {
        return m_bars;
    }

    /// Narrow is fine: the text shrinks to fit (paintEvent)
    QSize minimumSizeHint() const override;

  protected:
    void mousePressEvent(QMouseEvent* pEvent) override;
    /// The text at the skin's size, or smaller until it fits: never cut off
    /// (round 12: "0:31 / 5:56 / -5:2" under the key, Marko: "text going out
    /// of its display size, that's not allowed")
    void paintEvent(QPaintEvent* pEvent) override;

  private slots:
    void slotUpdate();

  private:
    ControlProxy* proxy(const QString& group, const QString& item);

    bool m_bars = false;
    ControlProxy* m_pTimeElapsed;
    ControlProxy* m_pTimeRemaining;
    ControlProxy* m_pBeatInPhrase;
    ControlProxy* m_pBeatNumber;
    ControlProxy* m_pBeatsPerBar;
    ControlProxy* m_pBeatUnit;
    ControlProxy* m_pSectionBeatsLeft;
    ControlProxy* m_pSectionFound;
};
