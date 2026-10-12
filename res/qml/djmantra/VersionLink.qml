import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The version (V1, V2 ...); a tap opens the newest release on GitHub, to
// update the app (Marko, 12.10.2026: "exactly the same like in all my apps")
Text {
    id: root

    readonly property string releaseUrl: "https://github.com/markoboskoauroville/djmantra_app/releases/tag/android-latest"

    text: Mixxx.DJMantra.version
    color: Style.accent
    font.pixelSize: 15
    font.bold: true
    font.underline: true

    MouseArea {
        // a finger-sized target around the short text
        anchors.fill: parent
        anchors.margins: -12
        onClicked: {
            Mixxx.DJMantra.log("version " + root.text + ": open " + root.releaseUrl);
            Qt.openUrlExternally(root.releaseUrl);
        }
    }
}
