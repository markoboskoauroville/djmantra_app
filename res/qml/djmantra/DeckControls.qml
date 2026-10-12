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

    DButton {
        objectName: "sync" + root.deck
        width: (root.width - 3 * root.spacing) * 0.24
        height: root.height
        text: "SYNC"
        fontSize: 12
        group: root.group
        key: "sync_enabled"
        litColor: root.deckColor
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
