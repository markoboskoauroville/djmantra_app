import "."
import Mixxx 1.0 as Mixxx
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
                // V9 Neural Mix (as djay Pro): the EQ's STEMS button turns the knobs
                // into drums / vocals / harmonies (src/effects/backends/builtin/stemeq.h)
                readonly property bool stems: stemsControl.value > 0

                Mixxx.ControlProxy { id: stemsControl; group: deck.eq; key: "button_parameter4" }

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

                    DKnob { width: 58; height: (knobs.height - 84) / 5; group: deck.modelData; key: "pregain"; label: "GAIN"; arcColor: Style.text }
                    DKnob { width: 58; height: (knobs.height - 84) / 5; group: deck.eq; key: "parameter3"; label: deck.stems ? "HARMONY" : "HIGH"; defaultParameter: 1; arcColor: Style.water }
                    DKnob { width: 58; height: (knobs.height - 84) / 5; group: deck.eq; key: "parameter2"; label: deck.stems ? "VOCALS" : "MID"; defaultParameter: 1; arcColor: Style.earth }
                    DKnob { width: 58; height: (knobs.height - 84) / 5; group: deck.eq; key: "parameter1"; label: deck.stems ? "DRUMS" : "LOW"; defaultParameter: 1; arcColor: Style.fire }
                    DKnob {
                        width: 58
                        height: (knobs.height - 84) / 5
                        group: "[QuickEffectRack1_" + deck.modelData + "]"
                        key: "super1"
                        label: "FILTER"
                        arcColor: Style.accent
                    }
                    DButton {
                        // EQ <-> NEURAL MIX
                        objectName: "neuralMix" + deck.index
                        width: 58
                        height: 36
                        text: deck.stems ? "NEURAL" : "EQ"
                        fontSize: 12
                        group: deck.eq
                        key: "button_parameter4"
                        onClicked: Mixxx.DJMantra.log("Neural Mix " + deck.modelData + " " + (deck.stems ? "off" : "on"))
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
