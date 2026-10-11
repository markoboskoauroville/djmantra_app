#pragma once

#include "widget/wlabel.h"

class ControlProxy;

/// DJ Mantra: Ableton Live's bar.beat counter for a deck, with the time
/// signature, e.g. "17.3  4/4" (Marko, 11.10.2026). Counted on the beat grid
/// from the deck's "1" (PhraseControl: beat_number, beats_per_bar, beat_unit).
/// Without a beat grid: "-.-".
class WBarBeat : public WLabel {
    Q_OBJECT
  public:
    explicit WBarBeat(const QString& group, QWidget* pParent = nullptr);

    /// "bar.beat" for beat `number` (0-based from "1"), bars and beats
    /// from 1; before "1" the bars run 0, -1, ... as in Ableton.
    static QString barBeat(int number, int beatsPerBar);

  private slots:
    void slotUpdate();

  private:
    ControlProxy* m_pBeatInPhrase;
    ControlProxy* m_pBeatNumber;
    ControlProxy* m_pBeatsPerBar;
    ControlProxy* m_pBeatUnit;
};
