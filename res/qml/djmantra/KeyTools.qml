import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's key: its Camelot code, KEY MATCH and key shift - / + (as djay).
// KEY MATCH moves the deck to the other deck's code on its own ring: the same
// key when both are minor (A) or both major (B), the relative key across the
// rings (8A against a B song: 8B). Round 16b: Mixxx's sync_key picked 7B
// against 8A, a diagonal.
Rectangle {
    id: root

    property string group
    readonly property string otherGroup: group === "[Channel1]" ? "[Channel2]" : "[Channel1]"
    property color deckColor: Style.deck1
    property bool keyMatched: false

    radius: Style.radius
    color: Style.surface

    Mixxx.ControlProxy { id: keyValue; group: root.group; key: "key" }
    Mixxx.ControlProxy { id: otherKey; group: root.otherGroup; key: "key" }
    Mixxx.ControlProxy { id: pitchAdjust; group: root.group; key: "pitch_adjust" }
    Mixxx.ControlProxy { id: pitchUp; group: root.group; key: "pitch_up" }
    Mixxx.ControlProxy { id: pitchDown; group: root.group; key: "pitch_down" }
    Mixxx.ControlProxy { id: keylock; group: root.group; key: "keylock" }
    Mixxx.ControlProxy { id: resetKey; group: root.group; key: "reset_key" }

    function pulse(control) {
        keylock.value = 1; // the key moves without the tempo
        control.value = 1;
        control.value = 0;
    }

    function match() {
        const shift = Mixxx.DJMantra.keyMatchShift(keyValue.value, otherKey.value);
        Mixxx.DJMantra.log("key match " + root.group + " " + Mixxx.DJMantra.camelot(keyValue.value) +
                " > " + Mixxx.DJMantra.camelot(otherKey.value) + ": " + shift + " semitones");
        if (shift === 0 && Mixxx.DJMantra.camelot(otherKey.value) === "")
            return;
        keylock.value = 1;
        pitchAdjust.value = pitchAdjust.value + shift;
        keyMatched = true;
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: (root.group === "[Channel2]" ? "2  " : "1  ") + (Mixxx.DJMantra.camelot(keyValue.value) || "-")
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
            objectName: "keyMatch" + (root.group === "[Channel2]" ? 2 : 1)
            width: 110
            height: root.height - 10
            text: qsTr("KEY MATCH")
            fontSize: 12
            lit: root.keyMatched
            onClicked: {
                // off: back to the song's own key
                if (root.keyMatched) {
                    root.pulse(resetKey);
                    root.keyMatched = false;
                } else {
                    root.match();
                }
            }
        }

        DButton {
            width: 44
            height: root.height - 10
            text: "−"
            fontSize: 20
            onClicked: root.pulse(pitchDown)
        }

        DButton {
            width: 44
            height: root.height - 10
            text: "+"
            fontSize: 20
            onClicked: root.pulse(pitchUp)
        }
    }
}
