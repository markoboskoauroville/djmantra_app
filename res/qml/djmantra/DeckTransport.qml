import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// One deck's SYNC, BPM, CUE and PLAY in a column (landscape: one at each side)
Column {
    id: root

    property string group
    property int deck: 1
    readonly property color deckColor: deck === 1 ? Style.deck1 : Style.deck2

    spacing: Style.gap

    Mixxx.ControlProxy { id: bpm; group: root.group; key: "bpm" }

    DButton {
        objectName: "sync" + root.deck
        width: root.width
        height: 40
        text: "SYNC"
        group: root.group
        key: "sync_enabled"
        onColor: root.deckColor
    }

    Text {
        width: root.width
        horizontalAlignment: Text.AlignHCenter
        text: bpm.value > 0 ? bpm.value.toFixed(1) : "-"
        color: Style.text
        font.pixelSize: 20
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 10
    }

    DButton {
        objectName: "cue" + root.deck
        width: root.width
        height: 44
        text: "CUE"
        group: root.group
        key: "cue_default"
        mode: "push"
        onColor: Style.orange
    }

    DButton {
        objectName: "play" + root.deck
        width: root.width
        height: 56
        icon: lit ? "pause" : "play"
        fontSize: 22
        group: root.group
        key: "play"
        onColor: root.deckColor
    }
}
