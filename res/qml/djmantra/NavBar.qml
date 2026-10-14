import "."
import QtQuick 2.12

// The bottom navigation of the horoscope app: icon + label, the selected one
// yellow. Pages: SONGS (the playlist), PHRASING (big waveforms and beat
// LEDs), MIXER, KEYS (Camelot wheel, key match), AUTOMIX.
Rectangle {
    id: root

    property int current: 0
    signal selected(int index)

    implicitHeight: 54
    color: Style.surface

    Row {
        anchors.fill: parent

        Repeater {
            // Marko 14.10.2026: in the order of the Mix Ultra's pad-mode buttons (HOT CUE, LOOP, FX,
            // NEURAL MIX, SHIFT+NEURAL MIX = SAMPLER), which switch these tabs
            model: [qsTr("Songs"), qsTr("Phrasing"), qsTr("Mixer"), qsTr("AutoMix"), qsTr("Keys")]

            delegate: Item {
                id: tab

                required property string modelData
                required property int index
                readonly property color tint: root.current === index ? Style.accent : Style.subText

                objectName: "tab" + index
                width: root.width / 5
                height: root.height

                Item {
                    // the icons, drawn: a list, faders, the wheel
                    id: icon

                    width: 22
                    height: 18
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 8

                    Row {
                        visible: tab.index === 2
                        anchors.centerIn: parent
                        spacing: 4
                        Repeater {
                            model: [0.5, 0.9, 0.65]
                            delegate: Rectangle {
                                required property real modelData
                                width: 3
                                height: icon.height
                                color: "transparent"
                                Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 1.5; height: parent.height; color: tab.tint }
                                Rectangle { anchors.horizontalCenter: parent.horizontalCenter; y: (1 - modelData) * (parent.height - 5); width: 7; height: 5; radius: 1; color: tab.tint }
                            }
                        }
                    }

                    Rectangle {
                        // the wheel
                        visible: tab.index === 4
                        anchors.centerIn: parent
                        width: 18
                        height: 18
                        radius: 9
                        color: "transparent"
                        border.color: tab.tint
                        border.width: 2

                        Rectangle {
                            anchors.centerIn: parent
                            width: 8
                            height: 8
                            radius: 4
                            color: "transparent"
                            border.color: tab.tint
                            border.width: 1.5
                        }
                    }

                    Grid {
                        // phrasing: 2 rows of 4 LEDs, the first yellow
                        visible: tab.index === 1
                        anchors.centerIn: parent
                        columns: 4
                        spacing: 2
                        Repeater {
                            model: 8
                            delegate: Rectangle {
                                required property int index
                                width: 4; height: 6; radius: 1
                                color: index === 0 ? Style.accent : tab.tint
                            }
                        }
                    }

                    Canvas {
                        // AutoMix: two arrows crossing over, one song into the next
                        visible: tab.index === 3
                        anchors.fill: parent
                        property color ink: tab.tint
                        onInkChanged: requestPaint()
                        onPaint: {
                            const ctx = getContext("2d");
                            ctx.reset();
                            ctx.strokeStyle = String(ink);
                            ctx.fillStyle = String(ink);
                            ctx.lineWidth = 2;
                            ctx.beginPath();
                            ctx.moveTo(1, 4);
                            ctx.lineTo(width - 6, 4);
                            ctx.moveTo(width - 1, height - 4);
                            ctx.lineTo(6, height - 4);
                            ctx.stroke();
                            ctx.beginPath();
                            ctx.moveTo(width - 1, 4); ctx.lineTo(width - 7, 0); ctx.lineTo(width - 7, 8);
                            ctx.closePath();
                            ctx.moveTo(1, height - 4); ctx.lineTo(7, height - 8); ctx.lineTo(7, height);
                            ctx.closePath();
                            ctx.fill();
                        }
                    }

                    Column {
                        // a list: dot and line, three times
                        visible: tab.index === 0
                        anchors.centerIn: parent
                        spacing: 4
                        Repeater {
                            model: 3
                            delegate: Row {
                                spacing: 3
                                Rectangle { width: 3; height: 3; radius: 1.5; color: tab.tint }
                                Rectangle { width: 15; height: 3; radius: 1.5; color: tab.tint }
                            }
                        }
                    }
                }

                Text {
                    anchors.top: icon.bottom
                    anchors.topMargin: 4
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: tab.modelData
                    color: tab.tint
                    font.pixelSize: 13
                    font.bold: root.current === tab.index
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: root.selected(tab.index)
                }
            }
        }
    }
}
