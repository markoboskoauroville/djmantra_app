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

        CloseButton {
            objectName: "menuClose"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 8
            z: 1
            onClicked: root.done()
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
            DButton {
                // record the mix (Marko 14.10.2026), lossless; Settings: its name and format
                objectName: "menuRecord"
                width: parent.width
                height: 52
                mode: ""
                lit: recStatus.value > 0
                litColor: Style.red
                text: recStatus.value > 0 ? qsTr("\u25A0 STOP RECORDING") : qsTr("\u25CF RECORD MIX")
                onClicked: {
                    Recorder.toggle();
                    root.done();
                }

                Mixxx.ControlProxy { id: recStatus; group: "[Recording]"; key: "status" }
            }
        }
    }
}
