#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSet>
#include <QVariantList>
#include <QVariantMap>

#include "analyzer/trackanalysisscheduler.h"
#include "preferences/usersettings.h"
#include "track/track_decl.h"

namespace djmantra {
class CqLink;
} // namespace djmantra

class QTimer;
class SoundManager;

namespace mixxx {
namespace qml {

/// DJ Mantra's helpers for its Qt Quick interface (Marko, 12.10.2026: the
/// whole interface drawn by the GPU). The room the system bars and the
/// camera hole take, Camelot keys and their match, and the song folders.
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
    /// The screen's rounded corner radius (dp): the app keeps out of it
    Q_PROPERTY(int cornerRadius READ cornerRadius NOTIFY insetsChanged)
    /// Songs still waiting for their BPM / key analysis (0: none)
    Q_PROPERTY(int analysisLeft READ analysisLeft NOTIFY analysisChanged)
    // the Allen & Heath CQ mixer over the network (qml/cqlink.h)
    Q_PROPERTY(QString cqStatus READ cqStatus NOTIFY cqChanged)
    Q_PROPERTY(bool cqConnected READ cqConnected NOTIFY cqChanged)
    Q_PROPERTY(bool cqDelayOn READ cqDelayOn NOTIFY cqChanged)
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
    int analysisLeft() const {
        return m_analysisLeft;
    }
    int cornerRadius() const {
        return m_cornerRadius;
    }

    /// "8A" for the value of a key control, "" without a key
    Q_INVOKABLE QString camelot(double keyValue) const;
    /// The musical name of a Camelot code: "8A" -> "Am"
    Q_INVOKABLE QString musicalName(const QString& code) const;
    /// {text: "8A > 9A", why: "+1 energy up", color: "#3ad13a"}
    Q_INVOKABLE QVariantMap keyMatch(double fromKey, double toKey) const;
    /// KEY MATCH: the semitones that move a deck's key to the other deck's
    /// Camelot number on its own ring (camelot::matchShift)
    Q_INVOKABLE int keyMatchShift(double deckKey, double otherKey) const;
    /// "1:23" (negative: "-1:23")
    Q_INVOKABLE QString formatTime(double seconds) const;
    /// "17.3" for beat number 66 in 4/4
    Q_INVOKABLE QString barBeat(double beatNumber, double beatsPerBar) const;
    /// "-6.2" for 26 beats left in 4/4
    Q_INVOKABLE QString barsBeatsLeft(double beats, double beatsPerBar) const;

    /// V9: djay's colours: 255 / the song's loudest waveform level (0 = not ready)
    Q_INVOKABLE double waveformLevel(const QString& group) const;
    /// The waveform's display gain for the deck's track: its level fills the
    /// lane (rounds 13-14: loud songs were 5-15 % wide). 0 while the track is
    /// not analysed yet. `length` is unused (kept for the QML calls).
    Q_INVOKABLE double waveformGain(const QString& group, int length) const;
    /// Whether the deck is playing (a load onto it asks first)
    Q_INVOKABLE bool isPlaying(const QString& group) const;

    /// The deck a new song goes to (Marko, 12.10.2026, as djay's "load to free
    /// deck"): the one not playing; neither plays: 1, 2, 1, 2 ...; both play:
    /// the one quieter in the mix (the load then asks first)
    Q_INVOKABLE QString freeDeck() const;
    /// A song was loaded onto `group` (for the 1, 2, 1, 2 order)
    Q_INVOKABLE void noteLoaded(const QString& group);

    /// The folders songs are looked for in first (Music, Download, ...)
    Q_INVOKABLE QVariantList musicRoots() const;
    /// Folders and songs in `folder`: [{name, path, isFolder}], folders first
    Q_INVOKABLE QVariantList listFolder(const QString& folder) const;
    /// A song's {artist, title, bpm, key (Camelot), cover (image URL or "")}:
    /// from the library, the file added to it (tags read) when new
    Q_INVOKABLE QVariantMap songInfo(const QString& path) const;
    /// BPM and key in advance (Marko, round 19: "automating BPM scanning of the
    /// songs shown in a playlist ... in advance ... also the key"): the songs
    /// without them are analysed in the background, one at a time, at low
    /// priority, without their waveform; the results go into the library.
    Q_INVOKABLE void analyzeAhead(const QStringList& paths);
    /// The CQ (Settings -> CQ MIXER): connect to its address (remembered), the
    /// DELAY on / off (the app's input -> the delay's FX send, the FX unmuted), and
    /// the delay's tap tempo from the on-air deck's BPM
    Q_INVOKABLE void cqConnect(const QString& host);
    Q_INVOKABLE void cqDisconnect();
    Q_INVOKABLE void cqSetDelay(bool on);
    Q_INVOKABLE void cqTapNow();
    QString cqStatus() const;
    bool cqConnected() const;
    bool cqDelayOn() const {
        return m_cqDelayOn;
    }
    /// Settings -> beats: the songs on the stopped decks analysed again with the
    /// options now chosen; returns how many
    Q_INVOKABLE int analyzeDecksAgain();
    /// Stops the folder's analysis (the folder was left)
    Q_INVOKABLE void stopAnalysis();
    // the phone's battery in % (-1: unknown, e.g. on the desktop)
    Q_INVOKABLE int batteryPercent() const;
    // AUTO GAIN (Settings, on by default): every song's peak to -1 dBFS (the ReplayGain stage)
    Q_INVOKABLE void setAutoGain(bool on);
    // the controller's link, one or the other (Settings): "usb" or "bluetooth"
    Q_INVOKABLE void setControllerLink(const QString& link);
    // FIND SONG (Marko 14.10.2026): audio files anywhere on the phone (Android's media index)
    // whose title / artist / album / file name has every word of `query`: name, path,
    // isFolder false, title, artist, duration (s); at most 300
    Q_INVOKABLE QVariantList findSongs(const QString& query) const;
    // Recording the mix (Marko 14.10.2026: "djay has a recording function ... build so
    // we can compare"): lossless FLAC (or WAV) of the master output into
    // Music/DJ Mantra Recordings, by Mixxx's RecordingManager ([Recording] controls).
    // Returns the new file's path ("" if a recording runs already).
    Q_INVOKABLE QString startRecording(const QString& format, const QString& name);
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE QString recordingPath() const;
    /// The decks' FILTER knob = the Pioneer DJM filter (loads it into the quick effect slots)
    Q_INVOKABLE void useDjmFilter();

    /// Hides the status and navigation bars again
    Q_INVOKABLE void hideBars();
    /// Shows them (a tap on the app's empty space); the next touch hides them
    Q_INVOKABLE void showBars();
    /// {cpu, gpu}: the app's CPU use in % of all the cores (since the last
    /// call) and the GPU's load in % from the driver; -1 when not known
    Q_INVOKABLE QVariantMap usage();
    /// The audio router (Marko, 12.10.2026): the phone's outputs now,
    /// [{key, label}] ("" = the phone's default route, follows plugging)
    Q_INVOKABLE QVariantList audioOutputs() const;
    /// Goes up whenever an output is plugged in or out
    Q_INVOKABLE int audioOutputsVersion() const;
    /// {master, masterPair, headphones, headphonesPair}: the outputs the two
    /// destinations go to ("none": headphones off) and their stereo pairs
    /// (0 = channels 1-2, 1 = 3-4 ...)
    Q_INVOKABLE QVariantMap audioRoute() const;
    /// MASTER and HEADPHONES to these outputs and stereo pairs (a 4-channel
    /// interface: master 1-2, headphones 3-4, or the other way); headphones
    /// "none" or the same output and pair: no separate headphones. "" on
    /// success, else the error
    Q_INVOKABLE QString setAudioRoute(const QString& master,
            int masterPair,
            const QString& headphones,
            int headphonesPair);
    /// A [DJMantra] setting (auto cue on / off ...), remembered
    Q_INVOKABLE QVariant setting(const QString& name, const QVariant& fallback) const;
    Q_INVOKABLE void setSetting(const QString& name, const QVariant& value);
    /// Whether the system bars are on screen now
    Q_INVOKABLE bool barsVisible() const;

    /// The detected section starts of the deck's track, fractions 0..1 of the
    /// track (empty until the detection ran; section_count tells when)
    Q_INVOKABLE QVariantList sections(const QString& group) const;
    /// Writes a line to the app's log (the phone tests read it)
    Q_INVOKABLE void log(const QString& text) const;
    // Sets any control (effect unit routing from FxBar.qml); false if it does not exist.
    Q_INVOKABLE bool setControl(const QString& group, const QString& key, double value) const;

    static QmlDJMantra* create(QQmlEngine* pQmlEngine, QJSEngine* pJsEngine);
    static inline void registerSoundManager(std::shared_ptr<SoundManager> pSoundManager) {
        s_pSoundManager = std::move(pSoundManager);
    }
    /// Also makes (or, with nullptr, removes) the [DJMantra] controls the
    /// controller mapping drives: browse_move, browse_press, browse_back,
    /// load (deck number), page (1-4); each a pulse back to 0
    static void registerUserSettings(UserSettingsPointer pConfig);

  signals:
    void insetsChanged();
    /// One more song analysed (the list reads its BPM / key again)
    void analysisChanged();
    void cqChanged();

  protected:
    bool eventFilter(QObject* pObject, QEvent* pEvent) override;

  private:
    void cqFollowTempo();
    double onAirBpm() const;
    void scheduleAnalysis(const QList<AnalyzerScheduledTrack>& tracks);
    void followInsets();
    /// A song without a genre: MusicBrainz is asked (Android, GenreLookup.java)
    void lookUpGenre(const QString& group, TrackPointer pTrack);

    static inline UserSettingsPointer s_pUserSettings = nullptr;
    static inline std::shared_ptr<SoundManager> s_pSoundManager;
    const UserSettingsPointer m_pConfig;
    int m_insets[4] = {0, 0, 0, 0};
    int m_cornerRadius = 0;
    QString m_lastLoaded;
    QSet<QString> m_genreAsked; // per file, once a run
    // usage(): the CPU ticks and the time at the last call
    qint64 m_lastCpuTicks = -1;
    qint64 m_lastCpuAtMs = 0;
    QString m_gpuSource; // the GPU load file that works on this phone
    bool m_gpuProbed = false;
    djmantra::CqLink* m_pCq = nullptr;
    bool m_cqDelayOn = false;
    double m_cqTappedBpm = 0;
    double m_cqPendingBpm = 0;
    int m_cqStableTicks = 0;
    long long m_lastGpuBusyUs = -1; // qml/gputime.h, at the last usage()
    qint64 m_lastGpuAtMs = 0;
    TrackAnalysisScheduler::Pointer m_pAnalysis = TrackAnalysisScheduler::NullPointer();
    int m_analysisLeft = 0;
};

} // namespace qml
} // namespace mixxx
