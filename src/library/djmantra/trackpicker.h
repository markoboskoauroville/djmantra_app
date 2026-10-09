#pragma once

#include <QStringList>
#include <QWidget>

#include "preferences/usersettings.h"

class QLabel;
class QListWidget;
class QListWidgetItem;
class QToolButton;

namespace djmantra {

/// DJ Mantra: the phone's song picker for one deck, opened by a tap on the
/// deck's cover (the note icon) in the header.
///
/// Full screen over the app, in the style of an Android file picker: close,
/// folder source, title, add folder and more at the top; folders and songs
/// as large rows; Files | Queue | History at the bottom. A tap on a song
/// loads it into the deck and closes the picker. ⋮ on a song: load to the
/// other deck, add to the queue. Folders, queue and history are kept in the
/// settings.
class TrackPicker : public QWidget {
    Q_OBJECT
  public:
    /// pWindow: the main window it covers; group: "[Channel1]", "[Channel2]",
    /// or empty for the library (a tapped song asks for the deck)
    TrackPicker(UserSettingsPointer pConfig, QWidget* pWindow, const QString& group);

  signals:
    void loadRequested(const QString& location, const QString& group);

  protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;
    void keyPressEvent(QKeyEvent* pEvent) override;
    void paintEvent(QPaintEvent* pEvent) override;

  private:
    enum class Tab {
        Files,
        Queue,
        History
    };

    void showTab(Tab tab);
    void showFolder(const QString& path);
    void refresh();
    void placeEmptyText();
    void goUp();
    void itemTapped(QListWidgetItem* pItem);
    void itemMenu(QListWidgetItem* pItem);
    void load(const QString& location, const QString& group);
    void addFolder();
    void showSourceMenu();
    void showMoreMenu();

    QStringList roots() const;
    QStringList list(const QString& key) const;
    void setList(const QString& key, const QStringList& values);
    QString deckName(const QString& group) const;
    QString otherGroup() const;

    UserSettingsPointer m_pConfig;
    QWidget* m_pWindow;
    QString m_group;
    Tab m_tab;
    QString m_path; // empty: the list of folders
    bool m_newestFirst;
    int m_lastReleaseX;

    QToolButton* m_pClose;
    QToolButton* m_pSource;
    QLabel* m_pTitle;
    QToolButton* m_pAdd;
    QToolButton* m_pMore;
    QListWidget* m_pList;
    QLabel* m_pEmpty;
    QList<QToolButton*> m_tabs;
};

} // namespace djmantra
