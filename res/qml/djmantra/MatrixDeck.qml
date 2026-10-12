import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// One deck's three lines for the dot-matrix display: the title (running
// through), the Camelot key and the BPM, the section countdown and the beat
// in the phrase.
Item {
    id: root

    property string group
    property color deckColor: Style.deck1
    property var player: Mixxx.PlayerManager.getPlayer(group)
    readonly property bool loaded: player ? player.isLoaded : false

    Mixxx.ControlProxy { id: keyControl; group: root.group; key: "key" }
    Mixxx.ControlProxy { id: bpm; group: root.group; key: "bpm" }
    Mixxx.ControlProxy { id: beat; group: root.group; key: "beat_in_phrase" }
    Mixxx.ControlProxy { id: left; group: root.group; key: "section_beats_left" }
    Mixxx.ControlProxy { id: found; group: root.group; key: "section_found" }
    Mixxx.ControlProxy { id: perBar; group: root.group; key: "beats_per_bar" }

    readonly property var lines: {
        if (!loaded)
            return [[{ text: "NO TRACK", color: Style.subText }], [], []];
        const title = player.title ? player.title : "?";
        const artist = player.artist ? player.artist + " - " : "";
        const code = Mixxx.DJMantra.camelot(keyControl.value);
        const countdown = left.value >= 0
                ? Mixxx.DJMantra.barsBeatsLeft(left.value, perBar.value) + (found.value > 0 ? "" : "|")
                : "-.-";
        return [
            [{ text: artist + title, color: Style.text, scroll: true }],
            [{ text: (code ? code : "--") + " ", color: root.deckColor },
             { text: bpm.value > 0 ? bpm.value.toFixed(1) : "---", color: Style.text }],
            [{ text: countdown + " ", color: Style.accent },
             { text: beat.value >= 1 ? "B" + Math.floor(beat.value) : "B-", color: root.deckColor }]
        ];
    }
}
