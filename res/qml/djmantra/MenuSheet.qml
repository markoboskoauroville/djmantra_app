import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The menu (the menu button, top left): the version, Settings, Library, Camelot wheel
Rectangle {
    id: root

    signal library()
    signal wheel()
    signal settings()
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

            VersionLink {
                objectName: "menuVersion"
            }
            DButton {
                objectName: "menuSettings"
                width: parent.width
                height: 52
                text: qsTr("SETTINGS: AUTO CUE, AUDIO OUTPUTS")
                onClicked: root.settings()
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
        }
    }
}
