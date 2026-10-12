import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// PHRASING (round 18, Marko: "bigger LEDs for song beats in matrix style, but
// they are big so I can jump easily ... when it's 32, almost impossible" in
// the small matrix): both decks' scrolling waveforms, big, half each (deck 1
// up, deck 2 down from the play line), with a zoom; under them the beat LEDs,
// 8 per row (16: 2 rows, 32: 4 rows), bars set apart, beat 1 yellow. A tap
// launches or jumps to that beat (as a tap on the matrix's cells).
Item {
    id: root

    property var deck1   // MatrixDeck
    property var deck2

    // the waveforms' zoom, both decks the same: 100 px show `zoom` seconds
    // (round 18: waveform_zoom does not exist in this interface, + / − did
    // nothing)
    property real zoom: 3
    function setZoom(value) {
        zoom = Math.min(12, Math.max(0.5, value));
        Mixxx.DJMantra.log("waveform zoom " + zoom.toFixed(2));
    }

    // the LEDs come first (round 18: at 32 beats they were 19 dp tall under a
    // 480 px waveform): each LED row gets at least 46 px, the waveforms the
    // rest (at most half the page). Landscape: the two decks' LEDs side by side.
    readonly property bool wide: width > height * 1.2
    readonly property int ledRows: Math.max(1, Math.ceil((deck1 ? deck1.cells : 8) / 8))
    readonly property real ledNeed: wide ? ledRows * 46 + (ledRows - 1) * 5 + 12
                                         : 2 * (ledRows * 46 + (ledRows - 1) * 5 + 12) + Style.gap

    Rectangle {
        id: waves

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Style.gap
        height: Math.max(60, Math.min((root.height - 3 * Style.gap) * 0.5,
                                      root.height - 3 * Style.gap - root.ledNeed))
        radius: 6
        color: "#05070f"
        clip: true

        BigWaveform {
            objectName: "bigWave1"
            width: parent.width
            height: parent.height / 2
            group: "[Channel1]"
            upper: true
            zoom: root.zoom
        }

        BigWaveform {
            objectName: "bigWave2"
            y: parent.height / 2
            width: parent.width
            height: parent.height / 2
            group: "[Channel2]"
            upper: false
            zoom: root.zoom
        }

        Rectangle {
            // the line between the decks
            y: parent.height / 2 - 0.5
            width: parent.width
            height: 1
            color: Style.line
        }

        Column {
            // zoom
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: 4
            spacing: 4

            DButton {
                objectName: "zoomIn"
                width: 36
                height: 32
                text: "+"
                fontSize: 18
                onClicked: root.setZoom(root.zoom / 1.5)
            }

            DButton {
                objectName: "zoomOut"
                width: 36
                height: 32
                text: "−"
                fontSize: 18
                onClicked: root.setZoom(root.zoom * 1.5)
            }
        }
    }

    Grid {
        id: leds

        columns: root.wide ? 2 : 1
        anchors.top: waves.bottom
        anchors.topMargin: Style.gap
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Style.gap
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Style.gap
        anchors.rightMargin: Style.gap
        spacing: Style.gap

        Repeater {
            model: [root.deck1, root.deck2]

            delegate: Rectangle {
                id: panel

                required property var modelData
                required property int index
                readonly property var deck: modelData
                readonly property int cells: deck ? deck.cells : 8
                readonly property int rows: Math.max(1, Math.ceil(cells / 8))

                objectName: "bigLeds" + (index + 1)
                width: root.wide ? (leds.width - leds.spacing) / 2 : leds.width
                height: root.wide ? leds.height : (leds.height - leds.spacing) / 2
                radius: 6
                color: "#05070f"

                Grid {
                    id: grid

                    anchors.fill: parent
                    anchors.margins: 6
                    columns: 8
                    rowSpacing: 5
                    columnSpacing: 5

                    Repeater {
                        model: panel.cells

                        delegate: Rectangle {
                            id: led

                            required property int index
                            readonly property bool lit: panel.deck && panel.deck.phraseBeat === index
                            readonly property bool armed: panel.deck && panel.deck.armedBeat === index
                            property bool blink: false

                            objectName: "led" + (panel.index + 1) + "_" + (index + 1)
                            // a wider gap between the bars (4 beats)
                            width: (grid.width - 7 * grid.columnSpacing - 6) / 8
                            height: (grid.height - (panel.rows - 1) * grid.rowSpacing) / panel.rows
                            radius: 3
                            color: area.pressed ? Style.pressed
                                    : (lit || (armed && blink)) ? (index % 8 === 0 || armed ? Style.accent : panel.deck.deckColor)
                                    : index % 8 === 0 ? "#4a4326" : "#1d2550"
                            border.color: armed ? Style.accent : "transparent"
                            border.width: armed ? 2 : 0

                            Timer {
                                interval: 250
                                repeat: true
                                running: led.armed
                                onTriggered: led.blink = !led.blink
                                onRunningChanged: led.blink = false
                            }

                            Text {
                                anchors.centerIn: parent
                                text: led.index + 1
                                color: led.lit ? Style.inkOnAccent : Style.subText
                                font.pixelSize: Math.min(18, led.height * 0.35)
                                font.bold: true
                                opacity: 0.8
                            }

                            MouseArea {
                                id: area

                                anchors.fill: parent
                                onPressed: panel.deck.tap(led.index)
                            }
                        }
                    }
                }
            }
        }
    }
}
