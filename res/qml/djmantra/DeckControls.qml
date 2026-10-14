import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's player controls (Marko, 12.10.2026, the redesign): SYNC, CUE,
// PLAY (aligned start) and NEXT (the next unplayed song of the list onto this
// deck; held: the next section). Deck 2 mirrored.
Row {
    id: root

    property string group
    property color deckColor: Style.deck1
    property bool mirrored: false
    property var matrixDeck // its MatrixDeck (align, beat launch)
    property var turntable // its TurntableStop (Settings -> STOP TIME)
    property var playlist   // the song list (LibraryPage): the next unplayed song
    readonly property int deck: group === "[Channel2]" ? 2 : 1

    spacing: 4
    layoutDirection: mirrored ? Qt.RightToLeft : Qt.LeftToRight

    Mixxx.ControlProxy { id: playControl; group: root.group; key: "play" }
    Mixxx.ControlProxy { id: sectionJump; group: root.group; key: "section_jump" }
    Mixxx.ControlProxy { id: bpm1; group: "[Channel1]"; key: "bpm" }
    Mixxx.ControlProxy { id: bpm2; group: "[Channel2]"; key: "bpm" }

    Timer {
        // after SYNC: both decks' tempo in the log (round 18: not measurable)
        id: syncLog

        interval: 400
        onTriggered: Mixxx.DJMantra.log("sync " + root.group + ": deck 1 " + bpm1.value.toFixed(2) +
                " bpm, deck 2 " + bpm2.value.toFixed(2) + " bpm")
    }

    DButton {
        objectName: "sync" + root.deck
        width: (root.width - 3 * root.spacing) * 0.24
        height: root.height
        text: "SYNC"
        fontSize: 12
        group: root.group
        key: "sync_enabled"
        litColor: root.deckColor
        onClicked: syncLog.restart()
    }

    DButton {
        id: cueButton

        objectName: "cue" + root.deck
        width: (root.width - 3 * root.spacing) * 0.22
        height: root.height
        text: "CUE"
        fontSize: 12
        group: root.group
        key: "cue_default"
        mode: "" // round 23: pressed here, see below
        litColor: Style.orange
        // Round 23: paused after play-from-cue, Mixxx's cue mode SET a new
        // cue where the deck paused. Now: paused away from the cue, CUE goes
        // back to it (and stays); playing, or paused on the cue, CUE works
        // as on a CDJ (back to the cue / preview while held).
        property bool held: false
        // Marko 14.10.2026: CUE held for 1 s jumps to the very start of the song
        Timer {
            id: cueToStart

            interval: 1000
            onTriggered: {
                if (cueButton.held) {
                    cueButton.held = false;
                    cueDefault.value = 0; // the preview ends
                }
                cueStart.value = 1;
                cueStart.value = 0;
                Toasts.show("DECK " + root.deck + qsTr("  \u2192 START OF THE SONG"));
                Mixxx.DJMantra.log("cue held 1 s " + root.group + ": to the start");
            }
        }
        onPressedDown: {
            cueToStart.restart();
            // round 24: where the cue and the deck are, on every press
            const seconds = (v) => (v * cueDuration.value).toFixed(2);
            Mixxx.DJMantra.log("cue press " + root.group + ": deck at " + seconds(cuePosition.value) +
                    " s, cue at " + (cuePoint.value >= 0 && trackSamples.value > 0
                            ? seconds(cuePoint.value / trackSamples.value) + " s" : "none") +
                    (playControl.value > 0 ? ", playing" : ", paused"));
            const away = playControl.value <= 0 && cuePoint.value >= 0 && trackSamples.value > 0 &&
                    Math.abs(cuePosition.value * trackSamples.value - cuePoint.value) > 2048;
            if (away) {
                cueGoto.value = 1;
                cueGoto.value = 0;
                Mixxx.DJMantra.log("button " + root.group + ",cue: back to the cue point");
            } else {
                held = true;
                cueDefault.value = 1;
                Mixxx.DJMantra.log("button " + root.group + ",cue_default pressed");
            }
        }
        onReleasedUp: {
            cueToStart.stop();
            if (held) {
                held = false;
                cueDefault.value = 0;
            }
        }

        Mixxx.ControlProxy { id: cueDefault; group: root.group; key: "cue_default" }
        Mixxx.ControlProxy { id: cueGoto; group: root.group; key: "cue_gotoandstop" }
        Mixxx.ControlProxy { id: cueStart; group: root.group; key: "start" }
        Mixxx.ControlProxy { id: cuePoint; group: root.group; key: "cue_point" }
        Mixxx.ControlProxy { id: cuePosition; group: root.group; key: "playposition"; maxRate: 4 }
        Mixxx.ControlProxy { id: trackSamples; group: root.group; key: "track_samples" }
        Mixxx.ControlProxy { id: cueDuration; group: root.group; key: "duration" }
    }

    DButton {
        // PLAY: a stopped deck starts aligned with the playing one, on its
        // next beat, the same beat of the phrase (round 19); pause as usual
        objectName: "play" + root.deck
        width: (root.width - 3 * root.spacing) * 0.32
        height: root.height
        icon: lit ? "pause" : "play"
        fontSize: 20
        lit: playControl.value > 0
        litColor: root.deckColor
        onPressedDown: {
            if (playControl.value > 0) {
                if (root.turntable && root.turntable.braking) {
                    root.turntable.cancel(); // PLAY while slowing down: plays on
                } else if (root.turntable) {
                    root.turntable.stop(); // at once, or as a turntable (STOP TIME)
                } else {
                    playControl.value = 0;
                }
                Mixxx.DJMantra.log("button " + root.group + ",play 0");
            } else if (root.matrixDeck && (root.matrixDeck.startFromCue() || root.matrixDeck.align())) {
                Mixxx.DJMantra.log("button " + root.group + ",play armed (aligned start)");
            } else {
                playControl.value = 1;
                Mixxx.DJMantra.log("button " + root.group + ",play 1");
            }
        }
    }

    DButton {
        // NEXT (Marko, round 22: "next to the play button ... loads in the
        // current deck the next unplayed song" of the list shown); held
        // (0.6 s): the next section, in time. (Aligning is AUTO PHASE's job.)
        objectName: "next" + root.deck
        width: (root.width - 3 * root.spacing) * 0.22
        height: root.height
        icon: "next"
        litColor: Style.accent
        lit: holdTimer.held
        onPressedDown: holdTimer.restart()
        onClicked: {
            if (holdTimer.held) {
                holdTimer.held = false;
                return;
            }
            holdTimer.stop();
            if (playControl.value > 0) {
                // never replace the song that is playing
                Mixxx.DJMantra.log("next " + root.group + ": the deck is playing, nothing loaded");
                return;
            }
            const path = root.playlist ? root.playlist.nextUnplayed(root.group) : "";
            if (path === "") {
                Mixxx.DJMantra.log("next " + root.group + ": no unplayed song in the list");
                return;
            }
            root.playlist.loadOnto(path, root.group);
        }

        Timer {
            id: holdTimer

            property bool held: false

            interval: 600
            onTriggered: {
                held = true;
                Mixxx.DJMantra.log("next section " + root.group);
                sectionJump.value = 1;
                sectionJump.value = 0;
            }
        }
    }
}
