#pragma once

#include <QAbstractButton>
#include <QColor>
#include <QIcon>
#include <QWidget>
#include <functional>
#include <memory>

class QLabel;
class QPainter;
class QVBoxLayout;

/// DJ Mantra's phone screens in the Android style (Marko's references:
/// djay's settings and Android's own, docs/ui-reference): colours, Material
/// icons and a full screen page with a back arrow, blue section headers and
/// rows with an icon, a switch or a slider.
namespace djmantra::ui {

inline const QColor kGround(0x11, 0x12, 0x15);
inline const QColor kSurface(0x23, 0x24, 0x27);
inline const QColor kBar(0x1E, 0x1F, 0x22);
inline const QColor kTile(0x26, 0x27, 0x2B);
inline const QColor kPressed(0x2E, 0x30, 0x34);
inline const QColor kText(0xF2, 0xF2, 0xF2);
inline const QColor kSubText(0x9A, 0x9C, 0xA1);
inline const QColor kAccent(0x2D, 0x8C, 0xFF);

/// Material icons (Apache 2.0), path data in a 24x24 view box
namespace icons {
extern const char* const kClose;
extern const char* const kBack;
extern const char* const kFolder;
extern const char* const kAdd;
extern const char* const kMore;
extern const char* const kDropDown;
extern const char* const kNote;
extern const char* const kQueue;
extern const char* const kHistory;
extern const char* const kSettings;
extern const char* const kSound;
extern const char* const kController;
extern const char* const kAdvanced;
extern const char* const kInfo;
extern const char* const kHelp;
extern const char* const kVolumeDown;
extern const char* const kVolumeUp;
extern const char* const kRecord;
extern const char* const kChevron;
} // namespace icons

void paintIcon(QPainter* pPainter, const char* path, const QRectF& rect, const QColor& color);
QIcon icon(const char* path, const QColor& color, int size);

/// An Android switch: a grey track, blue when on, with a round thumb
class Switch : public QAbstractButton {
    Q_OBJECT
  public:
    explicit Switch(QWidget* pParent = nullptr);
    QSize sizeHint() const override;

  protected:
    void paintEvent(QPaintEvent* pEvent) override;
};

/// Android's bottom sheet of actions (instead of QMenu: Qt's popup menus
/// lose the touch on Android, round 8: "Load to Deck 2" did nothing).
/// Fill it like a QMenu, then show() it over the window; a tap on an item
/// closes the sheet and runs the item's action.
class Sheet {
  public:
    struct Item {
        QString text;
        std::function<void()> action;
        bool checkable = false;
        bool checked = false;
        void setCheckable(bool on) {
            checkable = on;
        }
        void setChecked(bool on) {
            checked = on;
        }
    };
    Item* addAction(const QString& text, QObject* pContext, std::function<void()> action);
    void addSeparator() {
    }
    bool isEmpty() const {
        return m_items.isEmpty();
    }
    void show(QWidget* pWindow, const QString& title = QString());

  private:
    QList<std::shared_ptr<Item>> m_items;
};

/// A full screen page over the main window: ← and the title at the top,
/// then a scrolled column of sections and rows. Android's back closes it.
class Screen : public QWidget {
    Q_OBJECT
  public:
    Screen(QWidget* pWindow, const QString& title);

    /// A blue section header
    void addSection(const QString& title);
    /// A row with an icon, a title, an optional grey subtitle; a tap calls onTap
    QWidget* addRow(const char* iconPath,
            const QString& title,
            const QString& subtitle,
            std::function<void()> onTap);
    /// A row with a title, a switch at the right and a grey description below
    Switch* addSwitchRow(const QString& title, const QString& description);
    /// Any widget as a row (padded like the others)
    void addWidget(QWidget* pWidget);
    /// Space that pushes the rest to the bottom, then a centred grey line
    void addFooter(const QString& text);

  protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;
    void paintEvent(QPaintEvent* pEvent) override;

    QWidget* m_pWindow;
    QVBoxLayout* m_pColumn;
};

} // namespace djmantra::ui
