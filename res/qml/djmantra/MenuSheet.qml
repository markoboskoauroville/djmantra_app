import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The menu (the menu button, top left): Library, Camelot wheel, back to the classic screen
Rectangle {
    id: root

    signal library()
    signal wheel()
    signal done()

    color: "#8c000000"

    MouseArea {
        anchors.fill: parent
        onClicked: root.done()
    }

    Rectangle {
        id: sheet

        width: parent.width
        height: column.height + 24
        color: Style.surface
        radius: 12

        MouseArea {
            anchors.fill: parent
        }

        Column {
            id: column

            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width - 24
            spacing: 8

            Text {
                text: qsTr("Version %1").arg(Mixxx.DJMantra.version)
                color: Style.subText
                font.pixelSize: 13
            }
            DButton {
                width: parent.width
                height: 52
                text: qsTr("LIBRARY")
                onClicked: root.library()
            }
            DButton {
                width: parent.width
                height: 52
                text: qsTr("CAMELOT WHEEL")
                onClicked: root.wheel()
            }
            DButton {
                id: classic

                property bool chosen: false

                width: parent.width
                height: 52
                text: chosen ? qsTr("Classic screen at the next start: close and open the app")
                             : qsTr("BACK TO THE CLASSIC SCREEN")
                fontSize: 13
                onClicked: {
                    Mixxx.DJMantra.setQuickInterface(false);
                    chosen = true;
                }
            }
        }
    }
}
