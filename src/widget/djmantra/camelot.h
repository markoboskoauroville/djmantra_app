#pragma once

#include <QColor>
#include <QString>
#include <QWidget>

#include "proto/keys.pb.h"

class ControlProxy;

/// DJ Mantra's key mixing on the Camelot wheel (Marko, 11.10.2026): keys as
/// 8A / 5B (A = minor, inner ring; B = major, outer ring), how two keys
/// match, the wheel itself and a badge between the decks.
namespace djmantra::camelot {

struct Code {
    int number = 0; // 1..12, 0 = no key
    bool major = false;
    bool isValid() const {
        return number >= 1 && number <= 12;
    }
    bool operator==(const Code& other) const {
        return number == other.number && major == other.major;
    }
};

Code fromKey(mixxx::track::io::key::ChromaticKey key);
mixxx::track::io::key::ChromaticKey toKey(Code code);
/// "8A"; "-" without a key
QString text(Code code);
/// The musical name, e.g. "Am"
QString musicalName(Code code);

enum class Match {
    None,       // a deck without a key
    Same,       // the same key
    Compatible, // +1 / -1 on the same ring, or A <-> B with the same number
    Risky,      // +2 / -2, +7 (energy boost), or +1 / -1 with A <-> B
    Clash,
};

/// How `to` (the next song) goes with `from` (the playing one); `pWhy`
/// gets a short reason such as "+1 energy up" or "A/B switch".
Match match(Code from, Code to, QString* pWhy = nullptr);
QColor color(Match match);
QString matchName(Match match);

/// The 24-key wheel: B outside, A inside, 12 at the top. Keys given with
/// setDeckKeys() are ringed in the decks' colours and their compatible
/// neighbours lit; the other keys are dimmed.
class Wheel : public QWidget {
    Q_OBJECT
  public:
    explicit Wheel(QWidget* pParent = nullptr);
    void setDeckKeys(Code deck1, Code deck2);
    QSize sizeHint() const override;
    bool hasHeightForWidth() const override {
        return true;
    }
    int heightForWidth(int width) const override {
        return width;
    }

  protected:
    void paintEvent(QPaintEvent* pEvent) override;

  private:
    Code m_deck1;
    Code m_deck2;
};

/// The Camelot screen (Settings → Camelot wheel, or a tap on the badge):
/// the wheel with the decks' keys and how the matches work.
void showWheelScreen(QWidget* pWindow);

/// The badge between the decks: "8A > 9A  +1 up" in green / amber / red.
/// A tap opens the wheel screen.
class KeyMatchBadge : public QWidget {
    Q_OBJECT
  public:
    explicit KeyMatchBadge(QWidget* pParent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

  protected:
    void paintEvent(QPaintEvent* pEvent) override;
    void mousePressEvent(QMouseEvent* pEvent) override;

  private:
    ControlProxy* m_pKey1;
    ControlProxy* m_pKey2;
    ControlProxy* m_pPlay1;
    ControlProxy* m_pPlay2;
};

} // namespace djmantra::camelot
