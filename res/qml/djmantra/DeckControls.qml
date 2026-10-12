import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's player controls (Marko, 12.10.2026, the redesign): SYNC, CUE,
// PLAY and JUMP (to the next section). Deck 2 mirrored: PLAY in the middle.
Row {
    id: root

    property string group
    property color deckColor: Style.deck1
    property bool mirrored: false
    readonly property int deck: group === "[Channel2]" ? 2 : 1

    spacing: 4
    layoutDirection: mirrored ? Qt.RightToLeft : Qt.LeftToRight

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
        objectName: "play" + root.deck
        width: (root.width - 3 * root.spacing) * 0.32
        height: root.height
        icon: lit ? "pause" : "play"
        fontSize: 20
        group: root.group
        key: "play"
        litColor: root.deckColor
    }

    DButton {
        // to the next section, in time
        objectName: "jump" + root.deck
        width: (root.width - 3 * root.spacing) * 0.22
        height: root.height
        icon: "next"
        group: root.group
        key: "section_jump"
        mode: "push"
        litColor: Style.accent
    }
}
