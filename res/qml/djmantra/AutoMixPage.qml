import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// AutoMix (Marko 13.10.2026: "auto mix, which will automatically look at the folder
// sort order and after one song stops playing it is going to load the next song ...
// It mix number of beats user select ... in a similar style as djay Pro"). The songs
// of the folder open on the Songs tab, in the order shown there. One deck plays, the
// other holds the next song; `beats` beats before the playing song's end (its last
// sound, the analysis' outro end) the next one starts in sync and on the beat, the
// crossfader moves across over those beats with the basses swapped at the middle
// (one bassline at a time, as djay), then the old deck stops and gets the song after.
// Lives outside the tab (Main.qml) so the mix goes on on every page.
Item {
    id: root

    clip: true

    property var library // LibraryPage: songPaths(), folder
    property bool running: false
    property int beats: Number(Mixxx.DJMantra.setting("automix_beats", "16"))
    property var songs: []
    property int index: 0 // the song on the active deck
    // the song the next mix goes to: index + 1, or the one PREVIOUS / NEXT chose
    property int nextIndex: 1
    // PREVIOUS / NEXT (Marko 14.10.2026): the mix starts on the active deck's next phrase
    property bool skipArmed: false
    property int lastBeat: -1
    property string active: "[Channel1]"
    readonly property string other: active === "[Channel1]" ? "[Channel2]" : "[Channel1]"
    property bool mixing: false
    property real mixStart: 0
    property real mixSeconds: 8
    property bool waitingToPlay: false
    property string status: qsTr("Open a folder or a playlist, sort it, then START")

    function name(path) {
        return path ? path.substring(path.lastIndexOf("/") + 1) : "";
    }

    // true once `group` holds songs[i] (not the song it had before the load)
    function holds(group, i) {
        const player = Mixxx.PlayerManager.getPlayer(group);
        if (!player || !player.isLoaded || i < 0 || i >= songs.length)
            return false;
        const here = decodeURIComponent(String(player.trackLocationUrl).replace(/^file:\/\//, ""));
        return here === songs[i] && c(group).duration.value > 0;
    }

    function c(group) {
        return group === "[Channel1]" ? deck1 : deck2;
    }

    function load(group, i) {
        if (i < 0 || i >= songs.length)
            return false;
        Mixxx.PlayerManager.getPlayer(group).loadTrackFromLocation(songs[i], false);
        Mixxx.DJMantra.noteLoaded(group);
        Mixxx.DJMantra.log("AutoMix: " + (i + 1) + "/" + songs.length + " " + name(songs[i]) + " -> " + group);
        return true;
    }

    function start() {
        songs = root.library ? root.library.songPaths() : [];
        if (songs.length === 0) {
            status = qsTr("No songs: open a folder or a playlist first");
            return;
        }
        index = 0;
        nextIndex = 1;
        skipArmed = false;
        active = "[Channel1]";
        mixing = false;
        crossfader.value = -1;
        deck1.lowEq.parameter = 1;
        deck2.lowEq.parameter = 1;
        deck2.play.value = 0;
        load("[Channel1]", 0);
        load("[Channel2]", 1);
        waitingToPlay = true;
        running = true;
        Mixxx.DJMantra.log("AutoMix: start, " + songs.length + " songs, " + beats + " beats");
    }

    function stop() {
        running = false;
        mixing = false;
        waitingToPlay = false;
        status = qsTr("Stopped");
        Mixxx.DJMantra.log("AutoMix: stop");
    }

    function beginMix() {
        const a = c(active);
        const b = c(other);
        const bpm = a.bpm.value > 0 ? a.bpm.value : 120;
        mixSeconds = beats * 60 / bpm;
        // the incoming song: from its first sound, in sync, on the beat, bass out
        if (b.introStart.value > 0 && b.samples.value > 0)
            b.position.value = b.introStart.value / b.samples.value;
        b.quantize.value = 1;
        b.sync.value = 1;
        b.lowEq.parameter = 0;
        b.play.value = 1;
        mixStart = Date.now();
        mixing = true;
        Mixxx.DJMantra.log("AutoMix: mix " + name(songs[index]) + " -> " + name(songs[nextIndex])
                + " over " + beats + " beats (" + mixSeconds.toFixed(1) + " s)");
    }

    function finishMix() {
        const a = c(active);
        const b = c(other);
        crossfader.value = active === "[Channel1]" ? 1 : -1;
        a.play.value = 0;
        a.sync.value = 0;
        a.lowEq.parameter = 1;
        b.lowEq.parameter = 1;
        mixing = false;
        const old = active;
        active = other;
        index = nextIndex;
        nextIndex = index + 1;
        // the old deck gets the song after the next
        if (!load(old, index + 1))
            Mixxx.DJMantra.log("AutoMix: the last song is playing");
    }

    // PREVIOUS (-1) / NEXT (+1): that song onto the waiting deck, mixed in on the playing
    // deck's next phrase (Marko 14.10.2026: "skip or go back through the playlist but with
    // mixing transition")
    function skip(step) {
        if (!running || waitingToPlay) {
            Toasts.show(qsTr("START AUTOMIX FIRST"));
            return;
        }
        if (mixing) {
            Toasts.show(qsTr("MIXING ALREADY"));
            return;
        }
        const t = index + step;
        if (t < 0 || t >= songs.length) {
            Toasts.show(step < 0 ? qsTr("THE FIRST SONG IS PLAYING") : qsTr("THE LAST SONG IS PLAYING"));
            return;
        }
        nextIndex = t;
        if (!holds(other, t))
            load(other, t);
        skipArmed = true;
        lastBeat = -1;
        Toasts.show((step < 0 ? qsTr("PREVIOUS: ") : qsTr("NEXT: ")) + name(songs[t]) + qsTr(" ON THE NEXT PHRASE"));
        Mixxx.DJMantra.log("AutoMix: " + (step < 0 ? "previous" : "next") + " -> " + name(songs[t]) + " on the next phrase");
    }

    Timer {
        interval: 50
        repeat: true
        running: root.running
        onTriggered: {
            const a = root.c(root.active);
            const b = root.c(root.other);
            if (root.waitingToPlay) {
                if (root.holds(root.active, root.index)) {
                    a.play.value = 1;
                    root.waitingToPlay = false;
                }
                root.status = qsTr("Loading ") + root.name(root.songs[root.index]);
                return;
            }
            if (root.mixing) {
                const p = Math.min(1, (Date.now() - root.mixStart) / 1000 / root.mixSeconds);
                crossfader.value = root.active === "[Channel1]" ? -1 + 2 * p : 1 - 2 * p;
                if (p >= 0.5) {
                    // the basses swap at the middle: one bassline at a time
                    a.lowEq.parameter = 0;
                    b.lowEq.parameter = 1;
                }
                root.status = qsTr("Mixing into ") + root.name(root.songs[root.nextIndex])
                        + " (" + Math.round(p * 100) + " %)";
                if (p >= 1)
                    root.finishMix();
                return;
            }
            if (root.skipArmed) {
                // PREVIOUS / NEXT: the mix starts on beat 1 of the playing deck's next phrase
                if (!root.holds(root.other, root.nextIndex)) {
                    root.status = qsTr("Loading ") + root.name(root.songs[root.nextIndex]);
                    return;
                }
                const beat = Math.floor(a.beatNumber.value);
                const length = a.phraseLength.value >= 2 ? Math.round(a.phraseLength.value) : 8;
                if (beat !== root.lastBeat) {
                    const first = root.lastBeat >= 0;
                    root.lastBeat = beat;
                    if (first && beat >= 0 && beat % length === 0) {
                        root.skipArmed = false;
                        root.beginMix();
                        return;
                    }
                }
                root.status = qsTr("Mixing into ") + root.name(root.songs[root.nextIndex])
                        + qsTr(" on the next phrase");
                return;
            }
            const duration = a.duration.value;
            if (!(duration > 0))
                return;
            const end = a.outroEnd.value > 0 && a.samples.value > 0 ? a.outroEnd.value / a.samples.value : 1;
            const rate = a.rate.value > 0 ? a.rate.value : 1;
            const left = (end - a.position.value) * duration / rate;
            const bpm = a.bpm.value > 0 ? a.bpm.value : 120;
            const mixSeconds = root.beats * 60 / bpm;
            const hasNext = root.holds(root.other, root.nextIndex);
            if (hasNext && left <= mixSeconds) {
                root.beginMix();
            } else if (!hasNext && a.play.value === 0) {
                root.running = false;
                root.status = qsTr("The playlist has ended");
                Mixxx.DJMantra.log("AutoMix: the playlist has ended");
            } else {
                root.status = qsTr("Playing ") + root.name(root.songs[root.index]) + ", "
                        + (hasNext ? qsTr("mix in ") + Math.max(0, Math.round(left - mixSeconds)) + " s"
                                   : qsTr("last song"));
            }
        }
    }

    Mixxx.ControlProxy { id: crossfader; group: "[Master]"; key: "crossfader" }

    // the controls AutoMix reads and drives, per deck
    component DeckControls: QtObject {
        required property string deckGroup
        property Mixxx.ControlProxy play: Mixxx.ControlProxy { group: deckGroup; key: "play" }
        property Mixxx.ControlProxy position: Mixxx.ControlProxy { group: deckGroup; key: "playposition" }
        property Mixxx.ControlProxy duration: Mixxx.ControlProxy { group: deckGroup; key: "duration" }
        property Mixxx.ControlProxy bpm: Mixxx.ControlProxy { group: deckGroup; key: "bpm" }
        property Mixxx.ControlProxy rate: Mixxx.ControlProxy { group: deckGroup; key: "rate_ratio" }
        property Mixxx.ControlProxy sync: Mixxx.ControlProxy { group: deckGroup; key: "sync_enabled" }
        property Mixxx.ControlProxy quantize: Mixxx.ControlProxy { group: deckGroup; key: "quantize" }
        property Mixxx.ControlProxy samples: Mixxx.ControlProxy { group: deckGroup; key: "track_samples" }
        property Mixxx.ControlProxy introStart: Mixxx.ControlProxy { group: deckGroup; key: "intro_start_position" }
        property Mixxx.ControlProxy outroEnd: Mixxx.ControlProxy { group: deckGroup; key: "outro_end_position" }
        property Mixxx.ControlProxy beatNumber: Mixxx.ControlProxy { group: deckGroup; key: "beat_number" }
        property Mixxx.ControlProxy phraseLength: Mixxx.ControlProxy { group: deckGroup; key: "phrase_length" }
        property Mixxx.ControlProxy lowEq: Mixxx.ControlProxy {
            group: "[EqualizerRack1_" + deckGroup + "_Effect1]"
            key: "parameter1"
        }
    }

    DeckControls { id: deck1; deckGroup: "[Channel1]" }
    DeckControls { id: deck2; deckGroup: "[Channel2]" }

    // the tab's face (Marko 14.10.2026: "it looks same as songs with only just few additional
    // buttons ... automix will open automix settings and user adjust and collapse"): the Songs
    // list itself is shown (Main.qml); here only the AUTOMIX dropdown under its top bar and,
    // while it runs, a status line at the bottom
    property bool open: false
    readonly property string nowPath: running && index < songs.length ? songs[index] : ""
    readonly property string nextPath: running && nextIndex < songs.length ? songs[nextIndex] : ""

    // a song tapped in the list while AutoMix runs: mixed in next, on the next phrase
    function pick(path) {
        const t = songs.indexOf(path);
        if (t < 0) {
            Toasts.show(qsTr("NOT IN THE AUTOMIX LIST: STOP AND START IN THIS FOLDER"));
            return;
        }
        if (t === index) {
            Toasts.show(qsTr("PLAYING ALREADY"));
            return;
        }
        skip(t - index);
    }

    MouseArea {
        // a tap beside the open dropdown collapses it
        anchors.fill: parent
        visible: root.open
        onClicked: root.open = false
    }

    Rectangle {
        id: panel

        objectName: "automixPanel"
        visible: root.open
        anchors.top: parent.top
        anchors.topMargin: 46
        anchors.right: parent.right
        anchors.rightMargin: 8
        width: Math.min(parent.width - 16, 460)
        height: face.implicitHeight + 24
        radius: Style.radius
        color: Style.surface
        border.color: Style.accent
        border.width: 1

        MouseArea {
            anchors.fill: parent // taps inside stay inside
        }

        Column {
            id: face

            x: 12
            y: 12
            width: parent.width - 24
            spacing: 10

            Item {
                width: parent.width
                height: 32

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("AUTOMIX: THE DJ ROBOT")
                    color: Style.accent
                    font.pixelSize: 16
                    font.bold: true
                }

                DButton {
                    objectName: "automixCollapse"
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 44
                    height: 32
                    mode: ""
                    text: "\u25B4"
                    fontSize: 16
                    onClicked: root.open = false
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: root.library && root.library.folder !== ""
                        ? qsTr("Plays: ") + (root.library.inPlaylist ? root.library.folder.substring(9)
                                                                      : root.name(root.library.folder))
                          + qsTr(" (in the order shown)")
                        : qsTr("Open a folder or a playlist first")
                color: Style.subText
                font.pixelSize: 13
            }

            Text {
                text: qsTr("MIX OVER")
                color: Style.subText
                font.pixelSize: 12
                font.bold: true
            }

            Row {
                spacing: 6

                Repeater {
                    model: [4, 8, 16, 32, 64]

                    delegate: DButton {
                        required property int modelData

                        objectName: "automixBeats_" + modelData
                        width: (face.width - 4 * 6) / 5
                        height: 42
                        mode: ""
                        text: modelData + qsTr(" BEATS")
                        fontSize: 11
                        lit: root.beats === modelData
                        onClicked: {
                            root.beats = modelData;
                            Mixxx.DJMantra.setSetting("automix_beats", modelData);
                            Toasts.show(qsTr("AUTOMIX OVER ") + modelData + qsTr(" BEATS"));
                        }
                    }
                }
            }

            DButton {
                objectName: "automixStart"
                width: parent.width
                height: 56
                mode: ""
                text: root.running ? qsTr("STOP AUTOMIX") : qsTr("START AUTOMIX")
                fontSize: 17
                lit: root.running
                onClicked: {
                    if (root.running) {
                        root.stop();
                    } else {
                        root.start();
                        if (root.running)
                            root.open = false; // started: the robot mixes, the list shows
                    }
                    Toasts.show(root.running ? qsTr("AUTOMIX ON") : qsTr("AUTOMIX OFF"));
                }
            }

            Row {
                // PREVIOUS / NEXT song, mixed in on the next phrase (Marko 14.10.2026)
                spacing: 6

                DButton {
                    objectName: "automixPrevious"
                    width: (face.width - 6) / 2
                    height: 48
                    mode: ""
                    text: qsTr("\u25C0\u25C0 PREVIOUS")
                    fontSize: 14
                    enabled: root.running
                    onClicked: root.skip(-1)
                }

                DButton {
                    objectName: "automixNext"
                    width: (face.width - 6) / 2
                    height: 48
                    mode: ""
                    text: qsTr("NEXT \u25B6\u25B6")
                    fontSize: 14
                    enabled: root.running
                    onClicked: root.skip(1)
                }
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: root.status + (root.running ? qsTr("\nTap a song in the list: it is mixed in next.") : "")
                color: Style.text
                font.pixelSize: 14
            }
        }
    }

    Rectangle {
        // while it runs: what the robot does, at the bottom of the list
        objectName: "automixStatus"
        visible: root.running && !root.open
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
        color: Style.surface

        Text {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            text: qsTr("AUTOMIX  ") + root.status
            color: Style.accent
            font.pixelSize: 13
            font.bold: true
        }
    }
}
