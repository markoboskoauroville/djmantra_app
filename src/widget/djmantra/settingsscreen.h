#pragma once

#include <functional>

#include "widget/djmantra/mantraui.h"

class ControlProxy;

namespace djmantra {

/// What the settings rows open (the main window provides them)
struct SettingsActions {
    std::function<void()> library;    // the song folders (picker)
    std::function<void()> controller; // the virtual controller
    std::function<void()> sound;      // sound output
    std::function<void()> midi;       // MIDI devices
    std::function<void()> advanced;   // every setting (Mixxx's preferences)
};

/// DJ Mantra's settings in the Android style, after djay's (Marko's
/// screenshots docs/ui-reference/djay-settings-*.png): Main Volume, Split
/// Output for pre-cueing, then rows for each part of the settings, support
/// and the version.
class SettingsScreen : public ui::Screen {
    Q_OBJECT
  public:
    SettingsScreen(QWidget* pWindow, const SettingsActions& actions);

  private:
    void showAbout();

    ControlProxy* m_pMainGain;
    ControlProxy* m_pHeadSplit;
};

/// The menu at the top (djay's, docs/ui-reference/djay-menu.png): a sheet
/// over the decks with large tiles and a row of small actions.
class MainMenu : public QWidget {
    Q_OBJECT
  public:
    struct Actions {
        std::function<void()> library;
        std::function<void()> controller;
        std::function<void()> settings;
        std::function<void()> oneDeck; // switches One Deck / two decks
        bool inOneDeck = false;
    };
    MainMenu(QWidget* pWindow, const Actions& actions);

  protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;
    void mousePressEvent(QMouseEvent* pEvent) override;
    void mouseReleaseEvent(QMouseEvent* pEvent) override;
    void paintEvent(QPaintEvent* pEvent) override;

  private:
    QWidget* m_pWindow;
    QWidget* m_pSheet;
    ControlProxy* m_pRecording;
};

} // namespace djmantra
