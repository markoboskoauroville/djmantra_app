import "."
import ".." as Skin
import Mixxx 1.0 as Mixxx
import QtQuick 2.12
import QtQuick.Window 2.12

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
// A tap jumps there; a drag scrubs freely through the whole song (Marko
// 14.10.2026: the zoom window that opened while dragging is gone, the Phrasing
// tab shows the zoomed waveform).
Rectangle {
    id: root

    property string group
    property color deckColor: Style.deck1
    property var sections: []
    // V91: their names (rbxport + the page's names), "" where unnamed
    property var sectionNames: []
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
    color: "#141414" // djay Pro's background
    clip: true

    Mixxx.ControlProxy { id: position; group: root.group; key: "playposition"; maxRate: 10 }
    Mixxx.ControlProxy { id: loaded; group: root.group; key: "track_loaded" }
    Mixxx.ControlProxy {
        group: root.group
        key: "section_count"
        onValueChanged: {
            root.sections = Mixxx.DJMantra.sections(root.group);
            root.sectionNames = Mixxx.DJMantra.sectionNames(root.group);
        }
        Component.onCompleted: {
            root.sections = Mixxx.DJMantra.sections(root.group);
            root.sectionNames = Mixxx.DJMantra.sectionNames(root.group);
        }
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
            color: "#262626" // a lighter band, not a colour: djay's colours stay true
        }
    }

    Mixxx.WaveformOverview {
        anchors.fill: parent
        player: Mixxx.PlayerManager.getPlayer(root.group)
        channels: Mixxx.WaveformOverview.Channels.LeftChannel
        // V9 (Marko 13.10.2026: "djay algorithm waveform shape is paramount"):
        // djay Pro's overview, mirrored, its colours (src/waveform/djaycolor.h)
        renderer: Mixxx.WaveformOverview.Renderer.Djay
        normalize: true
        // Settings -> WAVEFORM: DJAY (V80, the default), 3-BAND (V78) or ICON (V77)
        threeBand: Mixxx.DJMantra.setting("wave_style", "djay") === "3band"
        djayMix: Mixxx.DJMantra.setting("wave_style", "djay") === "djay"
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

    Repeater {
        // V91: each section's name at its start, where there is room for it
        model: loaded.value > 0 ? root.sectionNames.length : 0

        Text {
            required property int index
            readonly property real start: root.sections[index] || 0
            readonly property real end: index + 1 < root.sections.length ? root.sections[index + 1] : 1

            visible: (end - start) * root.width > implicitWidth + 6
            x: start * root.width + 3
            y: 1
            z: 4
            text: root.sectionNames[index]
            color: index === root.next ? Style.accent : "#e6ffffff"
            font.pixelSize: 9
            font.bold: true
            style: Text.Outline
            styleColor: "#cc000000"
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

    Mixxx.ControlProxy { id: duration; group: root.group; key: "duration" }

    // the deck's finger is scrubbing (AUTO PHASE waits for it)
    readonly property bool scrubbing: area.scrubbing

    MouseArea {
        id: area

        property bool scrubbing: false

        anchors.fill: parent
        enabled: loaded.value > 0
        preventStealing: true

        function clamp(p) {
            return Math.max(0, Math.min(1, p));
        }

        // Marko 14.10.2026: the Phrasing tab has the zoomed waveform already, so no zoom
        // window here any more: a touch puts the play position under the finger and a
        // drag scrubs freely through the whole song
        function scrubTo(x) {
            position.value = clamp(x / width);
        }

        onPressed: (mouse) => {
            scrubbing = true;
            scrubTo(mouse.x);
        }
        onPositionChanged: (mouse) => scrubTo(mouse.x)
        onReleased: (mouse) => {
            scrubTo(mouse.x);
            Mixxx.DJMantra.log("scrub " + root.group + " to " + (position.value * duration.value).toFixed(2) + " s");
            scrubbing = false;
        }
        onCanceled: scrubbing = false
    }
}
