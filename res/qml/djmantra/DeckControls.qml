import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's player controls (Marko, 12.10.2026, the redesign): SYNC, CUE,
// PLAY (aligned start) and ALIGN (held: the next section). Deck 2 mirrored.
Row {
    id: root

    property string group
    property color deckColor: Style.deck1
    property bool mirrored: false
    property var matrixDeck // its MatrixDeck (align, beat launch)
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
        objectName: "cue" + root.deck
        width: (root.width - 3 * root.spacing) * 0.22
        height: root.height
        text: "CUE"
        fontSize: 12
        group: root.group
        key: "cue_default"
        mode: "push"
        litColor: Style.orange
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
                playControl.value = 0;
            } else if (!(root.matrixDeck && root.matrixDeck.align())) {
                playControl.value = 1;
            }
            Mixxx.DJMantra.log("button " + root.group + ",play " + playControl.value);
        }
    }

    DButton {
        // ALIGN (round 19, Marko: "this button ... needs to change the
        // function"): a tap aligns this deck's phrase with the other deck's on
        // the next beat, seamless; held (0.6 s): the next section, in time
        objectName: "jump" + root.deck
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
            if (root.matrixDeck)
                root.matrixDeck.align();
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
