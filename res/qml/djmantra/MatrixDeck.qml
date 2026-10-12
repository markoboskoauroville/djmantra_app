import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// One deck's data for the dot-matrix display: bar.beat, the countdown to the
// next phrase / section change, the beat in the 8-beat phrase.
Item {
    id: root

    property string group
    property color deckColor: Style.deck1
    property var player: Mixxx.PlayerManager.getPlayer(group)
    readonly property bool loaded: player ? player.isLoaded : false

    Mixxx.ControlProxy { id: beat; group: root.group; key: "beat_in_phrase" }
    Mixxx.ControlProxy { id: beatsLeft; group: root.group; key: "section_beats_left" }
    Mixxx.ControlProxy { id: found; group: root.group; key: "section_found" }
    Mixxx.ControlProxy { id: perBar; group: root.group; key: "beats_per_bar" }

    // the beat in the 8-beat phrase, 0 = beat 1 (as PhraseLeds); -1: none
    readonly property int phraseBeat: loaded && beat.value >= 0 ? Math.floor(beat.value) % 8 : -1

    Mixxx.ControlProxy { id: number; group: root.group; key: "beat_number" }

    readonly property string barBeat: loaded ? Mixxx.DJMantra.barBeat(number.value, perBar.value) : "-.-"
    // ONE countdown: to the next phrase / section change ("|": to the end)
    readonly property string countdown: loaded && beatsLeft.value >= 0
            ? Mixxx.DJMantra.barsBeatsLeft(beatsLeft.value, perBar.value) + (found.value > 0 ? "" : "|")
            : "-.-"
    readonly property var info: ({
        number: group === "[Channel2]" ? 2 : 1,
        barBeat: barBeat,
        countdown: countdown,
        beat: phraseBeat,
        color: String(deckColor)
    })
}
