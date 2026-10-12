import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// Marko's mixing tool (12.10.2026): both decks' 8-beat phrase lights, deck 1
// over deck 2, so "1 with 1" is seen at a glance, and next to each the bars
// and beats to its next section change. Always on screen, on every page.
Rectangle {
    id: root

    implicitHeight: 2 * rowHeight + 3 * 5
    readonly property int rowHeight: 20
    color: Style.surface
    radius: Style.radius

    Column {
        anchors.fill: parent
        anchors.margins: 5
        spacing: 5

        Repeater {
            model: [
                {group: "[Channel1]", name: "1", color: Style.deck1},
                {group: "[Channel2]", name: "2", color: Style.deck2}
            ]

            delegate: Item {
                id: deckRow

                required property var modelData

                width: parent.width
                height: root.rowHeight

                Mixxx.ControlProxy { id: beat; group: deckRow.modelData.group; key: "beat_in_phrase" }
                Mixxx.ControlProxy { id: left; group: deckRow.modelData.group; key: "section_beats_left" }
                Mixxx.ControlProxy { id: found; group: deckRow.modelData.group; key: "section_found" }
                Mixxx.ControlProxy { id: perBar; group: deckRow.modelData.group; key: "beats_per_bar" }
                Mixxx.ControlProxy { id: length; group: deckRow.modelData.group; key: "phrase_length" }

                Component.onCompleted: {
                    if (length.value !== 8)
                        length.value = 8;
                }

                Text {
                    id: name

                    width: 16
                    anchors.verticalCenter: parent.verticalCenter
                    text: deckRow.modelData.name
                    color: deckRow.modelData.color
                    font.pixelSize: 15
                    font.bold: true
                }

                Row {
                    id: cells

                    anchors.left: name.right
                    anchors.right: countdown.left
                    anchors.rightMargin: 8
                    height: parent.height
                    spacing: 4

                    Repeater {
                        model: 8

                        delegate: Rectangle {
                            required property int index

                            readonly property bool lit: beat.value === index

                            width: (cells.width - 7 * cells.spacing) / 8
                            height: cells.height
                            radius: 4
                            color: lit ? (index === 0 ? Style.accent : deckRow.modelData.color) : Style.tile
                            border.color: index === 0 ? Style.accent : Style.line
                            border.width: lit ? 0 : (index === 0 ? 1.5 : 0.5)

                            Text {
                                anchors.centerIn: parent
                                text: (parent.index + 1).toString()
                                color: parent.lit ? Style.onAccent : Style.subText
                                font.pixelSize: 11
                                font.bold: parent.lit
                            }
                        }
                    }
                }

                Text {
                    // the bars and beats to the next section change ("|": to the end)
                    id: countdown

                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 62
                    horizontalAlignment: Text.AlignRight
                    text: left.value >= 0
                            ? Mixxx.DJMantra.barsBeatsLeft(left.value, perBar.value) + (found.value > 0 ? "" : "|")
                            : "-.-"
                    color: Style.accent
                    font.pixelSize: 15
                    font.bold: true
                }
            }
        }
    }
}
