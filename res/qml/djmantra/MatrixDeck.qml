import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// One deck in the dot-matrix display: what it shows (time left, bar.beat,
// the countdown to the next phrase / section change, the beat in the phrase)
// and the beat launch on its running lights (Marko, 12.10.2026; before the
// redesign the 8 beat pads). A tap on cell N:
//   - the deck plays: it jumps to beat N of its phrase, in time
//   - the deck is stopped: it goes exactly to its beat N and starts when the
//     other (playing) deck reaches its beat N, in sync; a second tap cancels
Item {
    id: root

    property string group
    readonly property string otherGroup: group === "[Channel1]" ? "[Channel2]" : "[Channel1]"
    property color deckColor: Style.deck1
    property var player: Mixxx.PlayerManager.getPlayer(group)
    readonly property bool loaded: player ? player.isLoaded : false
    property int armedBeat: -1

    Mixxx.ControlProxy { id: beat; group: root.group; key: "beat_in_phrase" }
    Mixxx.ControlProxy { id: beatsLeft; group: root.group; key: "section_beats_left" }
    Mixxx.ControlProxy { id: found; group: root.group; key: "section_found" }
    Mixxx.ControlProxy { id: perBar; group: root.group; key: "beats_per_bar" }
    Mixxx.ControlProxy { id: number; group: root.group; key: "beat_number" }
    Mixxx.ControlProxy { id: position; group: root.group; key: "playposition" }
    Mixxx.ControlProxy { id: duration; group: root.group; key: "duration" }
    Mixxx.ControlProxy { id: length; group: root.group; key: "phrase_length" }
    Mixxx.ControlProxy { id: play; group: root.group; key: "play" }
    Mixxx.ControlProxy { id: jump; group: root.group; key: "phrase_jump" }
    Mixxx.ControlProxy { id: sync; group: root.group; key: "sync_enabled" }
    Mixxx.ControlProxy { id: quantize; group: root.group; key: "quantize" }
    Mixxx.ControlProxy { id: otherPlay; group: root.otherGroup; key: "play" }
    Mixxx.ControlProxy {
        group: root.otherGroup
        key: "beat_in_phrase"
        onValueChanged: {
            // the other deck reached the armed beat: start, in sync
            if (root.armedBeat >= 0 && value === root.armedBeat) {
                Mixxx.DJMantra.log("beat launch " + root.group + " on beat " + (root.armedBeat + 1));
                root.armedBeat = -1;
                play.value = 1;
            }
        }
    }

    function tap(index) {
        if (!loaded)
            return;
        if (play.value > 0) {
            jump.value = index + 1;
            return;
        }
        quantize.value = 1;
        jump.value = index + 1;
        if (otherPlay.value > 0) {
            sync.value = 1;
            armedBeat = armedBeat === index ? -1 : index;
        } else {
            play.value = 1;
        }
    }

    readonly property int cells: length.value >= 2 ? Math.round(length.value) : 8
    // the beat in the phrase, 0 = beat 1; -1: none
    readonly property int phraseBeat: loaded && beat.value >= 0 ? Math.floor(beat.value) % cells : -1
    // the time left in the song, counting down (round 18, Marko: "I need time
    // until the end of the song ... it needs to go backwards")
    readonly property string remaining: loaded
            ? "-" + Mixxx.DJMantra.formatTime(Math.max(0, 1 - Math.max(0, position.value)) * Math.max(0, duration.value))
            : "-:--"
    readonly property string barBeat: loaded && beat.value >= 0 ? Mixxx.DJMantra.barBeat(number.value, perBar.value) : "-.-"
    // ONE countdown: to the next phrase / section change ("|": to the end)
    readonly property string countdown: loaded && beatsLeft.value >= 0
            ? Mixxx.DJMantra.barsBeatsLeft(beatsLeft.value, perBar.value) + (found.value > 0 ? "" : "|")
            : "-.-"
    readonly property var info: ({
        remaining: remaining,
        barBeat: barBeat,
        countdown: countdown,
        beat: phraseBeat,
        cells: cells,
        armed: armedBeat,
        color: String(deckColor)
    })
}
