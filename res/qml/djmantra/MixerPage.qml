import "."
import QtQuick 2.12

// The mixer, as on the Mix Ultra: per deck GAIN, HIGH, MID, LOW, FILTER and
// the headphone button; the VU meters and the volume faders in the middle
Item {
    id: root

    Row {
        anchors.fill: parent
        anchors.margins: Style.gap
        spacing: Style.gap

        Repeater {
            model: ["[Channel1]", "[Channel2]"]

            delegate: Item {
                id: deck

                required property string modelData
                required property int index
                readonly property string eq: "[EqualizerRack1_" + modelData + "_Effect1]"

                width: (root.width - 3 * Style.gap) / 2
                height: root.height - 2 * Style.gap

                Column {
                    id: knobs

                    width: 58
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: deck.index === 0 ? parent.left : undefined
                    anchors.right: deck.index === 1 ? parent.right : undefined
                    spacing: 2

                    DKnob { width: 58; height: (knobs.height - 46) / 5; group: deck.modelData; key: "pregain"; label: "GAIN" }
                    DKnob { width: 58; height: (knobs.height - 46) / 5; group: deck.eq; key: "parameter3"; label: "HIGH"; defaultParameter: 1 }
                    DKnob { width: 58; height: (knobs.height - 46) / 5; group: deck.eq; key: "parameter2"; label: "MID"; defaultParameter: 1 }
                    DKnob { width: 58; height: (knobs.height - 46) / 5; group: deck.eq; key: "parameter1"; label: "LOW"; defaultParameter: 1 }
                    DKnob {
                        width: 58
                        height: (knobs.height - 46) / 5
                        group: "[QuickEffectRack1_" + deck.modelData + "]"
                        key: "super1"
                        label: "FILTER"
                        arcColor: Style.water
                    }
                    DButton { width: 58; height: 36; text: "\uD83C\uDFA7"; group: deck.modelData; key: "pfl" }
                }

                Row {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: deck.index === 0 ? knobs.right : undefined
                    anchors.right: deck.index === 1 ? knobs.left : undefined
                    anchors.leftMargin: Style.gap
                    anchors.rightMargin: Style.gap
                    spacing: Style.gap
                    layoutDirection: deck.index === 0 ? Qt.LeftToRight : Qt.RightToLeft

                    DFader {
                        width: deck.width - knobs.width - 3 * Style.gap - 14
                        height: parent.height
                        group: deck.modelData
                        key: "volume"
                        barColor: deck.index === 0 ? Style.deck1 : Style.deck2
                    }

                    VuBar {
                        width: 14
                        height: parent.height
                        group: deck.modelData
                    }
                }
            }
        }
    }
}
