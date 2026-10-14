import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// A deck's channel fader that is also its TEMPO fader (Marko 14.10.2026: named TEMPO as on
// the Mix Ultra, whose tempo slider it follows; the code still calls the mode "pitch")
// A deck's channel fader that is also its pitch fader (Marko 13.10.2026: "Long press
// just flips the fader to pitch slider, and it changed the color of the knob ... under
// the fader, there will be a label ... the scale adapting to the song BPM, and also
// this slider is following" tempo blend). Long press (0.5 s, finger still) flips
// VOLUME <-> PITCH; double tap resets (volume to the top, pitch to the song's own
// tempo). In PITCH mode the scale shows BPM on the left and % on the right for this
// song and the deck's pitch range; the handle (blue) follows the deck's tempo, also
// when sync or tempo blend move it, and carries the BPM.
Item {
    id: root

    property string group
    property color barColor: Style.accent
    readonly property string deckName: group === "[Channel2]" ? "DECK 2  " : "DECK 1  "
    property bool pitch: Mixxx.DJMantra.setting("fader_pitch_" + group, "0") === "1"
    readonly property color pitchColor: Style.water
    objectName: "fader_" + group + (pitch ? ",rate" : ",volume")

    Mixxx.ControlProxy { id: volume; group: root.group; key: "volume" }
    Mixxx.ControlProxy { id: rate; group: root.group; key: "rate" }
    Mixxx.ControlProxy { id: rateDir; group: root.group; key: "rate_dir" }
    Mixxx.ControlProxy { id: rateRange; group: root.group; key: "rateRange" }
    // the tempo where the song is now (r40: a variable beat grid made file_bpm 114.6 while the
    // deck played at 91): the scale has to match the BPM the handle shows
    Mixxx.ControlProxy { id: fileBpm; group: root.group; key: "local_bpm" }
    Mixxx.ControlProxy { id: bpm; group: root.group; key: "bpm" }

    // r41 (Marko: "it reacts in the opposite way"): the TEMPO handle sits where the console's
    // tempo slider is (rate -1 at the bottom, +1 at the top, as its MIDI), and the scale
    // says which way is faster from the deck's rate direction (Mixxx's default, as Pioneer:
    // DOWN = faster, "+" at the bottom)
    readonly property real dir: rateDir.value < 0 ? -1 : 1
    readonly property real faster: dir * rate.value
    // the handle's place, 0 = bottom, 1 = top
    readonly property real position: pitch ? (rate.value + 1) / 2 : volume.parameter

    function setPosition(p) {
        p = Math.max(0, Math.min(1, p));
        if (pitch)
            rate.value = 2 * p - 1;
        else
            volume.parameter = p;
    }

    // resets glide (Marko 14.10.2026: no snapping): TEMPO to 0 %, VOLUME to the top
    NumberAnimation {
        id: rateRamp

        target: rate
        property: "value"
        to: 0
        duration: 600
        easing.type: Easing.InOutCubic
    }
    NumberAnimation {
        id: volumeRamp

        target: volume
        property: "parameter"
        to: 1
        duration: 450
        easing.type: Easing.InOutCubic
    }
    function resetGlide() {
        if (pitch) {
            rateRamp.stop();
            rateRamp.start();
        } else {
            volumeRamp.stop();
            volumeRamp.start();
        }
    }

    function flip() {
        pitch = !pitch;
        Mixxx.DJMantra.setSetting("fader_pitch_" + group, pitch ? "1" : "0");
        Mixxx.DJMantra.log("fader " + group + ": " + (pitch ? "TEMPO" : "VOLUME"));
        Toasts.show(deckName + (pitch ? "VOLUME \u2192 TEMPO" : "TEMPO \u2192 VOLUME"));
    }

    // TEMPO RANGE as a Pioneer CDJ's button (Marko 14.10.2026): ±6, ±10 (default), ±16 %,
    // WIDE (±100 %); a tap on the TEMPO label steps through them, the tempo stays where it
    // is when it fits the new range (as the CDJ); remembered per deck, also in Settings
    readonly property var ranges: [6, 10, 16, 100]
    function setRange(percent) {
        const change = dir * rate.value * rateRange.value; // the speed change now (+ faster)
        rateRange.value = percent / 100;
        rate.value = Math.max(-1, Math.min(1, change / (dir * rateRange.value)));
        Mixxx.DJMantra.setSetting("tempo_range_" + group, percent);
        Toasts.show(deckName + qsTr("TEMPO RANGE \u2192 ") + (percent >= 100 ? qsTr("WIDE \u00B1100 %") : "\u00B1" + percent + " %"));
        Mixxx.DJMantra.log("tempo range " + group + " " + percent + " %");
    }
    function nextRange() {
        const now = Math.round(rateRange.value * 100);
        const i = ranges.indexOf(now);
        setRange(ranges[(i + 1) % ranges.length]);
    }
    Component.onCompleted: rateRange.value = Number(Mixxx.DJMantra.setting("tempo_range_" + group, "10")) / 100

    readonly property real trackTop: 13
    readonly property real trackBottom: height - label.height - 13
    readonly property real span: trackBottom - trackTop

    Rectangle {
        // the track
        x: (root.width - width) / 2
        y: root.trackTop
        width: 4
        height: root.span
        radius: 2
        color: root.pitch ? Qt.darker(root.pitchColor, 1.6) : Style.line
    }

    Repeater {
        // the pitch scale: 9 marks across the range, BPM left, % right
        model: root.pitch ? 9 : 0

        delegate: Item {
            required property int index
            // the mark's rate (-1 bottom .. 1 top) and its speed change (+ = faster)
            readonly property real f: root.dir * (-1 + index / 4)
            readonly property real percent: f * rateRange.value * 100
            readonly property bool major: index % 2 === 0

            width: root.width
            height: 1
            y: root.trackBottom - index / 8 * root.span // physical: bottom to top

            Rectangle {
                x: root.width / 2 - width / 2
                width: major ? 22 : 12
                height: 1
                color: index === 4 ? Style.accent : Style.line
            }

            Text {
                visible: major
                anchors.right: parent.horizontalCenter
                anchors.rightMargin: handle.width / 2 + 3
                anchors.verticalCenter: parent.top
                // never wider than its side of the fader (deck 2: the VU meter is on the left)
                width: Math.max(8, (root.width - handle.width) / 2 - 4)
                horizontalAlignment: Text.AlignRight
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 7
                text: fileBpm.value > 0 ? (fileBpm.value * (1 + f * rateRange.value)).toFixed(index === 4 ? 1 : 0) : ""
                color: index === 4 ? Style.accent : Style.subText
                font.pixelSize: 10
            }

            Text {
                visible: major
                anchors.left: parent.horizontalCenter
                anchors.leftMargin: handle.width / 2 + 3
                anchors.verticalCenter: parent.top
                // never wider than its side of the fader (deck 2: the VU meter is on the left)
                width: Math.max(8, (root.width - handle.width) / 2 - 4)
                horizontalAlignment: Text.AlignLeft
                fontSizeMode: Text.HorizontalFit
                minimumPixelSize: 7
                text: (percent > 0 ? "+" : "") + percent.toFixed(Math.abs(percent % 1) > 0.05 ? 1 : 0) + "%"
                color: index === 4 ? Style.accent : Style.subText
                font.pixelSize: 10
            }
        }
    }

    Rectangle {
        id: handle

        width: root.pitch ? 36 : Math.min(root.width, 48)
        height: 26
        radius: 5
        x: (root.width - width) / 2
        y: root.trackBottom - root.position * root.span - height / 2
        color: root.pitch ? root.pitchColor : Style.text
        border.color: root.pitch ? Qt.lighter(root.pitchColor, 1.3) : root.barColor
        border.width: 2

        Text {
            // the deck's tempo on the pitch handle
            visible: root.pitch
            anchors.centerIn: parent
            text: bpm.value > 0 ? bpm.value.toFixed(1) : "--"
            color: Style.inkOnAccent
            font.pixelSize: 10
            font.bold: true
        }
    }

    Text {
        id: label

        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        height: 16
        text: root.pitch ? qsTr("TEMPO ") + (rateRange.value >= 0.995 ? qsTr("WIDE")
                                                                   : "\u00B1" + Math.round(rateRange.value * 100))
                         : qsTr("VOLUME")
        color: root.pitch ? root.pitchColor : Style.subText
        font.pixelSize: 11
        font.bold: true
    }

    Timer {
        // the long press that flips the fader
        id: hold

        interval: 1000 // every long press is 1 s
        onTriggered: {
            area.flipped = true;
            root.flip();
        }
    }

    MouseArea {
        id: area

        property real startY: 0
        property real startPosition: 0
        property bool flipped: false

        anchors.fill: parent
        preventStealing: true
        onPressed: (mouse) => {
            rateRamp.stop();
            volumeRamp.stop();
            startY = mouse.y;
            startPosition = root.position;
            flipped = false;
            hold.restart();
        }
        onPositionChanged: (mouse) => {
            if (Math.abs(mouse.y - startY) > 8)
                hold.stop();
            if (flipped)
                return;
            root.setPosition(startPosition + (startY - mouse.y) / root.span);
        }
        onReleased: (mouse) => {
            hold.stop();
            // the CDJ's TEMPO RESET: a tap (no drag) on the 0 % mark glides the tempo to 0
            const zeroY = root.trackBottom - root.span / 2;
            if (root.pitch && !flipped && Math.abs(mouse.y - startY) < 6 && Math.abs(mouse.y - zeroY) < 14) {
                root.resetGlide();
                Toasts.show(root.deckName + "TEMPO \u2192 0 %");
                Mixxx.DJMantra.log("tempo reset " + root.group);
            }
        }
        onCanceled: hold.stop()
        onDoubleClicked: {
            hold.stop();
            root.resetGlide();
            Toasts.show(root.deckName + (root.pitch ? "TEMPO \u2192 0 %" : "VOLUME \u2192 0 dB"));
        }
    }

    MouseArea {
        // the CDJ's TEMPO RANGE button: a tap on the TEMPO label
        objectName: "tempoRange_" + root.group
        visible: root.pitch
        z: 2
        anchors.bottom: parent.bottom
        width: parent.width
        height: 22
        onClicked: root.nextRange()
    }
}
