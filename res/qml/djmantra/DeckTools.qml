import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Under a deck's beat pads: the next section (countdown, JUMP) and the key
// (Camelot code, KEY MATCH, key shift - / +), as in djay Pro
Row {
    id: root

    property string group
    property color deckColor: Style.deck1
    property bool keyMatched: false

    spacing: 5

    Mixxx.ControlProxy { id: left; group: root.group; key: "section_beats_left" }
    Mixxx.ControlProxy { id: found; group: root.group; key: "section_found" }
    Mixxx.ControlProxy { id: perBar; group: root.group; key: "beats_per_bar" }
    Mixxx.ControlProxy { id: keyValue; group: root.group; key: "key" }
    Mixxx.ControlProxy { id: syncKey; group: root.group; key: "sync_key" }
    Mixxx.ControlProxy { id: resetKey; group: root.group; key: "reset_key" }
    Mixxx.ControlProxy { id: pitchUp; group: root.group; key: "pitch_up" }
    Mixxx.ControlProxy { id: pitchDown; group: root.group; key: "pitch_down" }
    Mixxx.ControlProxy { id: keylock; group: root.group; key: "keylock" }

    function pulse(control) {
        keylock.value = 1; // the key moves without the tempo
        control.value = 1;
        control.value = 0;
    }

    // the next section: countdown and JUMP
    Rectangle {
        width: (root.width - root.spacing) * 0.42
        height: root.height
        radius: Style.radius
        color: Style.surface

        Column {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter

            Text {
                text: qsTr("NEXT SECTION")
                color: Style.subText
                font.pixelSize: 10
                font.bold: true
            }

            Text {
                text: left.value >= 0
                        ? Mixxx.DJMantra.barsBeatsLeft(left.value, perBar.value) + (found.value > 0 ? "" : "|")
                        : "-.-"
                color: Style.accent
                font.pixelSize: 20
                font.bold: true
            }
        }

        DButton {
            objectName: root.group === "[Channel1]" ? "jump1" : "jump2"
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            width: 58
            height: parent.height - 10
            text: qsTr("JUMP") + "\n▸|"
            fontSize: 12
            group: root.group
            key: "section_jump"
            mode: "push"
            onColor: Style.accent
        }
    }

    // the key: code, MATCH, - / +
    Rectangle {
        width: (root.width - root.spacing) * 0.58
        height: root.height
        radius: Style.radius
        color: Style.surface

        Text {
            id: code

            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 34
            text: Mixxx.DJMantra.camelot(keyValue.value) || "-"
            color: root.deckColor
            font.pixelSize: 18
            font.bold: true
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 5
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            DButton {
                objectName: root.group === "[Channel1]" ? "keyMatch1" : "keyMatch2"
                width: 62
                height: root.height - 10
                text: qsTr("KEY\nMATCH")
                fontSize: 11
                lit: root.keyMatched
                onClicked: {
                    // on: the key of the other deck (or its Camelot neighbour);
                    // off: back to the song's own key
                    if (root.keyMatched) {
                        root.pulse(resetKey);
                        root.keyMatched = false;
                    } else {
                        root.pulse(syncKey);
                        root.keyMatched = true;
                    }
                }
            }

            DButton {
                width: 34
                height: root.height - 10
                text: "−"
                onClicked: root.pulse(pitchDown)
            }

            DButton {
                width: 34
                height: root.height - 10
                text: "+"
                onClicked: root.pulse(pitchUp)
            }
        }
    }
}
