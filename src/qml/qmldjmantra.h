#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>

#include "preferences/usersettings.h"

class QTimer;

namespace mixxx {
namespace qml {

/// DJ Mantra's helpers for its Qt Quick interface (Marko, 12.10.2026: the
/// whole interface drawn by the GPU). The room the system bars and the
/// camera hole take, Camelot keys and their match, the song folders, and
/// the switch between this interface and the classic one.
class QmlDJMantra : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(DJMantra)
    QML_SINGLETON
    /// Device independent pixels the visible bars or the camera hole take
    Q_PROPERTY(int insetLeft READ insetLeft NOTIFY insetsChanged)
    Q_PROPERTY(int insetTop READ insetTop NOTIFY insetsChanged)
    Q_PROPERTY(int insetRight READ insetRight NOTIFY insetsChanged)
    Q_PROPERTY(int insetBottom READ insetBottom NOTIFY insetsChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
  public:
    explicit QmlDJMantra(UserSettingsPointer pConfig, QObject* pParent = nullptr);

    int insetLeft() const {
        return m_insets[0];
    }
    int insetTop() const {
        return m_insets[1];
    }
    int insetRight() const {
        return m_insets[2];
    }
    int insetBottom() const {
        return m_insets[3];
    }
    QString version() const;

    /// "8A" for the value of a key control, "" without a key
    Q_INVOKABLE QString camelot(double keyValue) const;
    /// {text: "8A > 9A", why: "+1 energy up", color: "#3ad13a"}
    Q_INVOKABLE QVariantMap keyMatch(double fromKey, double toKey) const;
    /// "1:23" (negative: "-1:23")
    Q_INVOKABLE QString formatTime(double seconds) const;
    /// "17.3" for beat number 66 in 4/4
    Q_INVOKABLE QString barBeat(double beatNumber, double beatsPerBar) const;
    /// "-6.2" for 26 beats left in 4/4
    Q_INVOKABLE QString barsBeatsLeft(double beats, double beatsPerBar) const;

    /// The folders songs are looked for in first (Music, Download, ...)
    Q_INVOKABLE QVariantList musicRoots() const;
    /// Folders and songs in `folder`: [{name, path, isFolder}], folders first
    Q_INVOKABLE QVariantList listFolder(const QString& folder) const;

    /// Hides the status and navigation bars again
    Q_INVOKABLE void hideBars();
    /// The interface at the next start: true = this one (Qt Quick, GPU)
    Q_INVOKABLE void setQuickInterface(bool on);
    Q_INVOKABLE bool quickInterface() const;
    /// Writes a line to the app's log (the phone tests read it)
    Q_INVOKABLE void log(const QString& text) const;

    static QmlDJMantra* create(QQmlEngine* pQmlEngine, QJSEngine* pJsEngine);
    static inline void registerUserSettings(UserSettingsPointer pConfig) {
        s_pUserSettings = std::move(pConfig);
    }

  signals:
    void insetsChanged();

  private:
    void followInsets();

    static inline UserSettingsPointer s_pUserSettings = nullptr;
    const UserSettingsPointer m_pConfig;
    int m_insets[4] = {0, 0, 0, 0};
};

} // namespace qml
} // namespace mixxx
