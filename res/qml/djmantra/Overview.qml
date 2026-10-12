import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's overview waveform (Marko, 12.10.2026, the redesign): the whole
// song, the play position in yellow; a tap or a drag seeks.
Rectangle {
    id: root

    property string group
    property color deckColor: Style.deck1

    radius: 4
    color: Style.surface
    clip: true

    Mixxx.ControlProxy { id: position; group: root.group; key: "playposition" }
    Mixxx.ControlProxy { id: loaded; group: root.group; key: "track_loaded" }

    Mixxx.WaveformOverview {
        anchors.fill: parent
        player: Mixxx.PlayerManager.getPlayer(root.group)
        channels: Mixxx.WaveformOverview.Channels.LeftChannel
        renderer: Mixxx.WaveformOverview.Renderer.Filtered
        colorHigh: Style.text
        colorMid: root.deckColor
        colorLow: Style.fire
    }

    Rectangle {
        // the part already played, dimmed
        visible: loaded.value > 0
        width: Math.max(0, Math.min(1, position.value)) * root.width
        height: root.height
        color: "#66000000"
    }

    Rectangle {
        visible: loaded.value > 0
        x: Math.max(0, Math.min(1, position.value)) * root.width - 1
        width: 2
        height: root.height
        color: Style.accent
    }

    MouseArea {
        anchors.fill: parent
        enabled: loaded.value > 0
        onPressed: (mouse) => position.value = Math.max(0, Math.min(1, mouse.x / width))
        onPositionChanged: (mouse) => {
            if (pressed)
                position.value = Math.max(0, Math.min(1, mouse.x / width));
        }
    }
}
