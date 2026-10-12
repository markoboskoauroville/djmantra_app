import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's overview waveform (round 18, Marko: "sections need to be clearly
// seen ... the waveform needs to be much bigger ... make vertical line between
// each section so I can clearly see how the timeline is approaching that
// section ... clearly see the changes in arrangements"):
//   - the waveform grows from the bottom and fills the height (the loud parts
//     reach the top), the bands layered: highs blue, mids green, bass red on
//     top, so a bass going out or coming in shows at once
//   - the sections as contrasting bands (blue / black), framed by a white line
//     at each change; the NEXT change is a thick yellow line
//   - the part already played dimmed, the play position a white line
// A tap or a drag seeks.
Rectangle {
    id: root

    property string group
    property color deckColor: Style.deck1
    property var sections: []
    // the play position, 10 times a second (round 20: CPU ~75 % of a core:
    // the overviews' play line moved with every engine update, ~47 a second,
    // so the whole window was drawn again and again)
    property real shownPos: 0

    Timer {
        interval: 100
        repeat: true
        running: root.visible
        triggeredOnStart: true
        onTriggered: root.shownPos = Math.max(0, Math.min(1, position.value))
    }
    // the index of the next section change after the play position (-1: none)
    readonly property int next: {
        const p = root.shownPos;
        for (let i = 0; i < sections.length; ++i) {
            if (sections[i] > p + 0.0005)
                return i;
        }
        return -1;
    }

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
            color: "#25348f"
        }
    }

    Mixxx.WaveformOverview {
        anchors.fill: parent
        player: Mixxx.PlayerManager.getPlayer(root.group)
        channels: Mixxx.WaveformOverview.Channels.LeftChannel
        renderer: Mixxx.WaveformOverview.Renderer.Filtered
        normalize: true
        colorLow: "#ff3b30"
        colorMid: "#4cd964"
        colorHigh: "#8fc1ff"
    }

    Repeater {
        // the frames: a white line at each section change, the next one
        // thick and yellow
        model: loaded.value > 0 ? root.sections.length : 0

        Rectangle {
            required property int index
            readonly property bool upcoming: index === root.next

            x: root.sections[index] * root.width - width / 2
            width: upcoming ? 4 : 2
            height: root.height
            color: upcoming ? Style.accent : "#ffffff"
            z: 2
        }
    }

    Rectangle {
        // the part already played, dimmed
        visible: loaded.value > 0
        width: root.shownPos * root.width
        height: root.height
        color: "#99000000"
        z: 3
    }

    Rectangle {
        visible: loaded.value > 0
        x: root.shownPos * root.width - 1.5
        width: 3
        height: root.height
        color: "#ffffff"
        z: 4
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
