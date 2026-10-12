import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The 8-beat phrase (Marko, 12.10.2026): 8 small squares, 4 over 4, the one
// of the current beat lit, from beat_in_phrase (the slicer's own count; "1"
// is the main cue or the first beat of the grid). Beat 1 lights yellow.
Grid {
    id: root

    property string group
    property color deckColor: Style.deck1
    property int square: 7

    columns: 4
    spacing: 2

    Mixxx.ControlProxy { id: beat; group: root.group; key: "beat_in_phrase" }
    Mixxx.ControlProxy { id: length; group: root.group; key: "phrase_length" }

    Component.onCompleted: {
        // the LEDs show a phrase of 8
        if (length.value !== 8)
            length.value = 8;
    }

    Repeater {
        model: 8

        delegate: Rectangle {
            required property int index

            readonly property bool lit: beat.value === index

            width: root.square
            height: root.square
            radius: 1.5
            color: lit ? (index === 0 ? Style.accent : root.deckColor) : Style.tile
            border.color: Style.line
            border.width: lit ? 0 : 0.5
        }
    }
}
