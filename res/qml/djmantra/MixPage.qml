import "."
import QtQuick 2.12

// The mixing page, without waveforms (Marko, 12.10.2026): per deck the 8 beat
// pads (the running phrase lights; a tap starts or jumps in time), the next
// section (countdown, JUMP) and the key (MATCH, shift); between the decks the
// half-automatic transition (AUTO MIX, EQ, TEMPO BLEND, length)
Column {
    id: root

    spacing: Style.gap
    padding: Style.gap

    readonly property real innerWidth: width - 2 * Style.gap
    readonly property real padsHeight: Math.max(60, (height - 2 * Style.gap - 4 * Style.gap - 2 * 52 - autoMix.height) / 2)

    BeatPads {
        width: root.innerWidth
        height: root.padsHeight
        group: "[Channel1]"
        deckColor: Style.deck1
    }

    DeckTools {
        width: root.innerWidth
        height: 52
        group: "[Channel1]"
        deckColor: Style.deck1
    }

    AutoMix {
        id: autoMix

        width: root.innerWidth
    }

    DeckTools {
        width: root.innerWidth
        height: 52
        group: "[Channel2]"
        deckColor: Style.deck2
    }

    BeatPads {
        width: root.innerWidth
        height: root.padsHeight
        group: "[Channel2]"
        deckColor: Style.deck2
    }
}
