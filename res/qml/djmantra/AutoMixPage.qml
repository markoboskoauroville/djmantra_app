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

    // r40: the NEXT UP list ran under the crossfader: the tab ends above it
    clip: true

    property var library // LibraryPage: songPaths(), folder
    property bool running: false
    property int beats: Number(Mixxx.DJMantra.setting("automix_beats", "16"))
    property var songs: []
    property int index: 0 // the song on the active deck
    property string active: "[Channel1]"
    readonly property string other: active === "[Channel1]" ? "[Channel2]" : "[Channel1]"
    property bool mixing: false
    property real mixStart: 0
    property real mixSeconds: 8
    property bool waitingToPlay: false
    property string status: qsTr("Open a folder on the Songs tab, sort it, then START")

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
            status = qsTr("No songs: open a folder on the Songs tab first");
            return;
        }
        index = 0;
        active = "[Channel1]";
        mixing = false;
        crossfader.value = -1;
        deck1.lowEq.parameter = 0.5;
        deck2.lowEq.parameter = 0.5;
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
        Mixxx.DJMantra.log("AutoMix: mix " + name(songs[index]) + " -> " + name(songs[index + 1])
                + " over " + beats + " beats (" + mixSeconds.toFixed(1) + " s)");
    }

    function finishMix() {
        const a = c(active);
        const b = c(other);
        crossfader.value = active === "[Channel1]" ? 1 : -1;
        a.play.value = 0;
        a.sync.value = 0;
        a.lowEq.parameter = 0.5;
        b.lowEq.parameter = 0.5;
        mixing = false;
        const old = active;
        active = other;
        index += 1;
        // the old deck gets the song after the next
        if (!load(old, index + 1))
            Mixxx.DJMantra.log("AutoMix: the last song is playing");
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
                    b.lowEq.parameter = 0.5;
                }
                root.status = qsTr("Mixing into ") + root.name(root.songs[root.index + 1])
                        + " (" + Math.round(p * 100) + " %)";
                if (p >= 1)
                    root.finishMix();
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
            const hasNext = root.holds(root.other, root.index + 1);
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
        property Mixxx.ControlProxy lowEq: Mixxx.ControlProxy {
            group: "[EqualizerRack1_" + deckGroup + "_Effect1]"
            key: "parameter1"
        }
    }

    DeckControls { id: deck1; deckGroup: "[Channel1]" }
    DeckControls { id: deck2; deckGroup: "[Channel2]" }

    // the tab's face
    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Text {
            width: parent.width
            text: qsTr("AUTOMIX")
            color: Style.accent
            font.pixelSize: 18
            font.bold: true
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: root.library && root.library.folder !== ""
                    ? qsTr("Folder: ") + root.name(root.library.folder) + qsTr(" (the order of the Songs tab)")
                    : qsTr("No folder open on the Songs tab")
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
                    width: (root.width - 24 - 4 * 6) / 5
                    height: 44
                    mode: ""
                    text: modelData + qsTr(" BEATS")
                    fontSize: 12
                    lit: root.beats === modelData
                    onClicked: {
                        root.beats = modelData;
                        Mixxx.DJMantra.setSetting("automix_beats", modelData);
                    }
                }
            }
        }

        DButton {
            objectName: "automixStart"
            width: parent.width
            height: 64
            mode: ""
            text: root.running ? qsTr("STOP AUTOMIX") : qsTr("START AUTOMIX")
            fontSize: 18
            lit: root.running
            onClicked: root.running ? root.stop() : root.start()
        }

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            text: root.status
            color: Style.text
            font.pixelSize: 15
        }

        Text {
            visible: root.songs.length > 0
            text: qsTr("NEXT UP")
            color: Style.subText
            font.pixelSize: 12
            font.bold: true
        }

        Repeater {
            model: root.running ? Math.min(6, root.songs.length - root.index - 1) : 0

            delegate: Text {
                required property int index

                width: root.width - 24
                elide: Text.ElideRight
                text: (root.index + index + 2) + ". " + root.name(root.songs[root.index + index + 1])
                color: index === 0 ? Style.accent : Style.text
                font.pixelSize: 14
            }
        }
    }
}
