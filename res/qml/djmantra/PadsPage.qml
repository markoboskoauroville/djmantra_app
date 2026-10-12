import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// 8 hot cues per deck: a tap on an empty pad sets it, on a set pad jumps there
Column {
    id: root

    spacing: Style.gap
    padding: Style.gap

    Repeater {
        model: ["[Channel1]", "[Channel2]"]

        delegate: Column {
            id: deck

            required property string modelData
            required property int index

            width: root.width - 2 * Style.gap
            spacing: 4

            Text {
                text: "DECK " + (deck.index + 1) + "  HOT CUES"
                color: Style.accent
                font.pixelSize: 13
                font.bold: true
            }

            Grid {
                columns: 4
                spacing: 4

                Repeater {
                    model: 8

                    delegate: DButton {
                        required property int index

                        width: (deck.width - 3 * 4) / 4
                        height: (root.height - 2 * Style.gap - 2 * 22 - Style.gap - 4 * 4) / 4
                        text: (index + 1).toString()
                        group: deck.modelData
                        key: "hotcue_" + (index + 1) + "_activate"
                        mode: "push"
                        lit: status.value > 0
                        onColor: deck.index === 0 ? Style.deck1 : Style.deck2

                        Mixxx.ControlProxy {
                            id: status

                            group: deck.modelData
                            key: "hotcue_" + (index + 1) + "_status"
                        }
                    }
                }
            }
        }
    }
}
