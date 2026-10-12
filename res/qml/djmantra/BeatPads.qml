import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's 8 beat pads (Marko, 12.10.2026: "the CUE pads become the running
// lights"): one pad per beat of the 8-beat phrase, the current one lit.
// A tap on beat N:
//   - the deck plays: it jumps to beat N of its phrase, in time
//   - the deck is stopped: it goes exactly to its beat N and starts when the
//     other (playing) deck reaches its beat N, in sync ("select beat 1, it
//     starts to play from beat 1 in sync with the current song")
Grid {
    id: root

    property string group
    property string otherGroup: group === "[Channel1]" ? "[Channel2]" : "[Channel1]"
    property color deckColor: Style.deck1
    property int armedBeat: -1 // waiting for the other deck's beat

    columns: 4
    spacing: 5

    Mixxx.ControlProxy { id: beat; group: root.group; key: "beat_in_phrase" }
    Mixxx.ControlProxy { id: play; group: root.group; key: "play" }
    Mixxx.ControlProxy { id: jump; group: root.group; key: "phrase_jump" }
    Mixxx.ControlProxy { id: sync; group: root.group; key: "sync_enabled" }
    Mixxx.ControlProxy { id: quantize; group: root.group; key: "quantize" }
    Mixxx.ControlProxy { id: otherPlay; group: root.otherGroup; key: "play" }
    Mixxx.ControlProxy {
        id: otherBeat

        group: root.otherGroup
        key: "beat_in_phrase"
        onValueChanged: {
            // the other deck reached the armed beat: start, in sync
            if (root.armedBeat >= 0 && value === root.armedBeat) {
                Mixxx.DJMantra.log("beat launch " + root.group + " on beat " + (root.armedBeat + 1));
                root.armedBeat = -1;
                play.value = 1;
            }
        }
    }

    function tap(index) {
        if (play.value > 0) {
            jump.value = index + 1;
            return;
        }
        // stopped: to the exact beat, then start with the other deck's beat
        quantize.value = 1;
        jump.value = index + 1;
        if (otherPlay.value > 0) {
            sync.value = 1;
            armedBeat = armedBeat === index ? -1 : index; // a second tap cancels
        } else {
            play.value = 1;
        }
    }

    Repeater {
        model: 8

        delegate: Rectangle {
            id: pad

            required property int index
            readonly property bool lit: beat.value === index

            objectName: (root.group === "[Channel1]" ? "beat1_" : "beat2_") + (index + 1)
            width: (root.width - 3 * root.spacing) / 4
            height: (root.height - root.spacing) / 2
            radius: Style.radius
            color: area.pressed ? Style.pressed
                    : lit ? (index === 0 ? Style.accent : root.deckColor) : Style.tile
            border.color: root.armedBeat === index ? Style.accent : (index === 0 ? Style.accent : Style.line)
            border.width: root.armedBeat === index ? 3 : (index === 0 ? 1.5 : 0.5)

            SequentialAnimation on opacity {
                // the armed pad blinks until its start
                running: root.armedBeat === pad.index
                loops: Animation.Infinite
                NumberAnimation { to: 0.4; duration: 200 }
                NumberAnimation { to: 1.0; duration: 200 }
                onStopped: pad.opacity = 1
            }

            Text {
                anchors.centerIn: parent
                text: (pad.index + 1).toString()
                color: pad.lit ? Style.inkOnAccent : Style.text
                font.pixelSize: 20
                font.bold: true
            }

            MouseArea {
                id: area

                anchors.fill: parent
                onPressed: root.tap(pad.index)
            }
        }
    }
}
