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
// A tap jumps there. Holding or dragging (Marko, 12.10.2026: "when user
// scrolls through the waveform ... automatically zoom in ... so user can
// precisely put his playhead") opens a zoomed scrolling waveform above the
// overview, and the finger then moves the play position finely: the waveform
// follows the finger 1:1.
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
    color: "#141414" // djay Pro's background
    clip: true

    Mixxx.ControlProxy { id: position; group: root.group; key: "playposition"; maxRate: 10 }
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

    Mixxx.ControlProxy { id: duration; group: root.group; key: "duration" }

    // the zoom while scrubbing: 100 px show this many seconds
    readonly property real scrubZoom: 1.0

    // the deck's finger is scrubbing (AUTO PHASE waits for it)
    readonly property bool scrubbing: area.scrubbing

    Rectangle {
        // the zoomed waveform while scrubbing, above the overview. Round 27:
        // inside the overview it was clipped away (clip: true): it lives on
        // the window's top layer, placed over the overview when it opens
        id: scrubView

        readonly property point origin: visible && parent ? root.mapToItem(parent, 0, 0) : Qt.point(0, 0)

        parent: root.Window.contentItem ? root.Window.contentItem : root
        objectName: "scrub" + root.group
        visible: area.scrubbing
        width: root.width
        height: 96
        x: origin.x
        y: Math.max(0, origin.y - height - 4)
        z: 100
        radius: 4
        color: "#141414"
        border.color: Style.accent
        border.width: 2
        clip: true

        Loader {
            // made only while scrubbing: hidden, it would still follow the
            // play position 60 times a second
            anchors.fill: parent
            anchors.margins: 2
            active: area.scrubbing

            sourceComponent: Skin.WaveformRow {
                group: root.group
                levelGain: Math.max(1, Mixxx.DJMantra.waveformLevel(root.group))
                zoom: root.scrubZoom
                showRoll: false
            }
        }
    }

    MouseArea {
        id: area

        property bool scrubbing: false
        property real startX: 0
        property real lastX: 0
        property real startPos: 0

        anchors.fill: parent
        enabled: loaded.value > 0
        preventStealing: true

        function clamp(p) {
            return Math.max(0, Math.min(1, p));
        }

        function beginScrub() {
            scrubbing = true;
            startX = lastX;
            startPos = position.value;
            Mixxx.DJMantra.log("scrub " + root.group + " zoomed from " + (startPos * duration.value).toFixed(2) + " s");
        }

        Timer {
            id: hold

            interval: 250
            onTriggered: area.beginScrub()
        }

        onPressed: (mouse) => {
            startX = mouse.x;
            lastX = mouse.x;
            scrubbing = false;
            hold.restart();
        }
        onPositionChanged: (mouse) => {
            lastX = mouse.x;
            if (!scrubbing && Math.abs(mouse.x - startX) > 12 && hold.running) {
                hold.stop();
                beginScrub();
            }
            if (scrubbing && duration.value > 0)
                position.value = clamp(startPos + (mouse.x - startX) * root.scrubZoom / 100 / duration.value);
        }
        onReleased: (mouse) => {
            hold.stop();
            if (!scrubbing) {
                // a tap: straight there, no zoom
                position.value = clamp(mouse.x / width);
                Mixxx.DJMantra.log("jump " + root.group + " to " + (position.value * duration.value).toFixed(2) + " s");
            } else {
                Mixxx.DJMantra.log("scrub " + root.group + " ended at " + (position.value * duration.value).toFixed(2) + " s");
            }
            scrubbing = false;
        }
        onCanceled: {
            hold.stop();
            scrubbing = false;
        }
    }
}
