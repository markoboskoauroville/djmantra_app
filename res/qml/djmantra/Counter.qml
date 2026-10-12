import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The deck counter (Marko, 11.10.2026): a tap switches
//   TIME          elapsed / length / remaining   1:23/5:40/-4:16
//   BARS & BEATS  bar.beat, bars.beats to the next section ("|": to the
//                 end of the track), time signature   17.3  -6.2  4/4
// The text shrinks to fit its line; it is never cut off.
Text {
    id: root

    property string group
    property bool bars: false

    color: Style.text
    font.pixelSize: 15
    font.bold: true
    fontSizeMode: Text.HorizontalFit
    minimumPixelSize: 9
    elide: Text.ElideNone
    verticalAlignment: Text.AlignVCenter
    text: {
        if (!bars) {
            return Mixxx.DJMantra.formatTime(elapsed.value) + "/" +
                    Mixxx.DJMantra.formatTime(elapsed.value + remaining.value) + "/-" +
                    Mixxx.DJMantra.formatTime(remaining.value);
        }
        const signature = (perBar.value > 0 ? perBar.value : 4) + "/" + (unit.value > 0 ? unit.value : 4);
        if (inPhrase.value < 0)
            return "-.-  " + signature;
        let line = Mixxx.DJMantra.barBeat(number.value, perBar.value);
        if (left.value >= 0)
            line += "  " + Mixxx.DJMantra.barsBeatsLeft(left.value, perBar.value) + (found.value > 0 ? "" : "|");
        return line + "  " + signature;
    }

    Mixxx.ControlProxy { id: elapsed; group: root.group; key: "time_elapsed" }
    Mixxx.ControlProxy { id: remaining; group: root.group; key: "time_remaining" }
    Mixxx.ControlProxy { id: inPhrase; group: root.group; key: "beat_in_phrase" }
    Mixxx.ControlProxy { id: number; group: root.group; key: "beat_number" }
    Mixxx.ControlProxy { id: perBar; group: root.group; key: "beats_per_bar" }
    Mixxx.ControlProxy { id: unit; group: root.group; key: "beat_unit" }
    Mixxx.ControlProxy { id: left; group: root.group; key: "section_beats_left" }
    Mixxx.ControlProxy { id: found; group: root.group; key: "section_found" }

    MouseArea {
        anchors.fill: parent
        onClicked: root.bars = !root.bars
    }
}
