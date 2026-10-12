import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// SYNC and BPM, CUE, PLAY per deck; the crossfader between the play buttons
Column {
    id: root

    spacing: Style.gap
    padding: Style.gap

    Mixxx.ControlProxy { id: bpm1; group: "[Channel1]"; key: "bpm" }
    Mixxx.ControlProxy { id: bpm2; group: "[Channel2]"; key: "bpm" }

    Item {
        width: root.width - 2 * Style.gap
        height: 44

        DButton {
            id: sync1

            objectName: "sync1"
            width: 84
            height: parent.height
            text: "SYNC"
            group: "[Channel1]"
            key: "sync_enabled"
            onColor: Style.deck1
        }

        Text {
            anchors.left: sync1.right
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: bpm1.value > 0 ? bpm1.value.toFixed(1) : "-"
            color: Style.text
            font.pixelSize: 22
        }

        Text {
            anchors.right: sync2.left
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: bpm2.value > 0 ? bpm2.value.toFixed(1) : "-"
            color: Style.text
            font.pixelSize: 22
        }

        DButton {
            id: sync2

            objectName: "sync2"
            anchors.right: parent.right
            width: 84
            height: parent.height
            text: "SYNC"
            group: "[Channel2]"
            key: "sync_enabled"
            onColor: Style.deck2
        }
    }

    Row {
        spacing: Style.gap

        DButton {
            width: (root.width - 3 * Style.gap) / 2
            height: 44
            objectName: "cue1"
            text: "CUE"
            group: "[Channel1]"
            key: "cue_default"
            mode: "push"
            onColor: Style.orange
        }

        DButton {
            width: (root.width - 3 * Style.gap) / 2
            height: 44
            objectName: "cue2"
            text: "CUE"
            group: "[Channel2]"
            key: "cue_default"
            mode: "push"
            onColor: Style.orange
        }
    }

    Row {
        spacing: Style.gap

        DButton {
            width: 64
            height: 52
            icon: lit ? "pause" : "play"
            fontSize: 22
            objectName: "play1"
            group: "[Channel1]"
            key: "play"
            onColor: Style.deck1
        }

        DFader {
            objectName: "crossfader"
            width: root.width - 2 * Style.gap - 2 * 64 - 2 * Style.gap
            height: 52
            vertical: false
            group: "[Master]"
            key: "crossfader"
        }

        DButton {
            width: 64
            height: 52
            icon: lit ? "pause" : "play"
            fontSize: 22
            objectName: "play2"
            group: "[Channel2]"
            key: "play"
            onColor: Style.deck2
        }
    }
}
