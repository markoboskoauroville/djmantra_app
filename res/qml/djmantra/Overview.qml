import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's overview waveform (round 18, Marko: "sections need to be clearly
// seen ... the contrast is not good"): the whole song in frequency colours
// (bass red, mids green, highs blue), its loudest point filling the height;
// the detected sections as alternating bands with a yellow line at each
// start; the part already played dimmed, the play position white. A tap or a
// drag seeks.
Rectangle {
    id: root

    property string group
    property color deckColor: Style.deck1
    property var sections: []

    radius: 4
    color: "#05070f"
    clip: true

    Mixxx.ControlProxy { id: position; group: root.group; key: "playposition" }
    Mixxx.ControlProxy { id: loaded; group: root.group; key: "track_loaded" }
    Mixxx.ControlProxy {
        group: root.group
        key: "section_count"
        onValueChanged: root.sections = Mixxx.DJMantra.sections(root.group)
        Component.onCompleted: root.sections = Mixxx.DJMantra.sections(root.group)
    }

    Repeater {
        // the sections: every second one a lighter band
        model: loaded.value > 0 ? root.sections.length : 0

        Rectangle {
            required property int index
            readonly property real start: root.sections[index]
            readonly property real end: index + 1 < root.sections.length ? root.sections[index + 1] : 1

            visible: index % 2 === 0
            x: start * root.width
            width: Math.max(0, (end - start) * root.width)
            height: root.height
            color: "#1a2350"
        }
    }

    Mixxx.WaveformOverview {
        anchors.fill: parent
        player: Mixxx.PlayerManager.getPlayer(root.group)
        channels: Mixxx.WaveformOverview.Channels.BothChannels
        renderer: Mixxx.WaveformOverview.Renderer.RGB
        normalize: true
        colorLow: "#ff3b30"
        colorMid: "#4cd964"
        colorHigh: "#3d8bff"
    }

    Repeater {
        // a yellow line where each section starts
        model: loaded.value > 0 ? root.sections.length : 0

        Rectangle {
            required property int index

            x: root.sections[index] * root.width - 1
            width: 2
            height: root.height
            color: Style.accent
            opacity: 0.9
        }
    }

    Rectangle {
        // the part already played, dimmed
        visible: loaded.value > 0
        width: Math.max(0, Math.min(1, position.value)) * root.width
        height: root.height
        color: "#80000000"
    }

    Rectangle {
        visible: loaded.value > 0
        x: Math.max(0, Math.min(1, position.value)) * root.width - 1.5
        width: 3
        height: root.height
        color: "#ffffff"
    }

    Rectangle {
        // the deck's colour at the edge, so the two overviews read apart
        width: 3
        height: root.height
        color: root.deckColor
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
