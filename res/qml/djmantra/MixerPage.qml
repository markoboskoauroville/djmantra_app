import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The mixer, as on the Mix Ultra: per deck HIGH, MID, LOW, FILTER (V62: no GAIN, every song is
// normalized to -1 dBFS), the FX button and the headphone button; the VU meters and the volume faders in the middle.
// V18 (Marko 13.10.2026, djay Android): ONE "EQ" dropdown at the top switches both
// decks' knobs (Equalizer / Neural Mix EQ: drums, vocals, harmonies); per deck ONE FX
// button: tap = the effect on / off, hold = a drop-up chooser (Flanger, Underwater,
// Cooldown, Echo Out). Deck N's effect is effect unit N, slot 1, on deck N only.
Item {
    id: root

    // djay's four, then the classic Pioneer DJM beat effects (Marko 13.10.2026: "take some
    // classic old Pioneer ... the most important ones"; the DJM-600's BEAT EFFECTS: DELAY
    // repeats once, ECHO repeats fading, TRANS cuts the sound on the beat, PAN moves it left
    // and right), on Mixxx's beat-synced effects with DJM-like settings (`params`, real
    // units) and the DEPTH slider on `depth`
    readonly property var effects: [
        { name: "FLANGER", id: "org.mixxx.effects.flanger", depth: "parameter2" },
        { name: "UNDERWATER", id: "org.djmantra.effects.underwater", depth: "parameter1" },
        { name: "COOLDOWN", id: "org.djmantra.effects.cooldown", depth: "parameter1" },
        { name: "ECHO OUT", id: "org.djmantra.effects.echoout", depth: "parameter1" },
        // echo: parameter1 time (beats), 2 feedback, 3 ping pong, 4 send; button 1 quantize
        { name: "DJM DELAY", id: "org.mixxx.effects.echo", depth: "parameter4",
          params: { parameter1: 1, parameter2: 0, parameter3: 0, parameter4: 0.8, button_parameter1: 1 } },
        { name: "DJM ECHO", id: "org.mixxx.effects.echo", depth: "parameter2",
          params: { parameter1: 1, parameter2: 0.55, parameter3: 0, parameter4: 0.8, button_parameter1: 1 } },
        // tremolo: parameter1 depth, 2 rate (per beat), 3 width, 4 waveform (low = square)
        { name: "DJM TRANS", id: "org.mixxx.effects.tremolo", depth: "parameter1",
          params: { parameter1: 1, parameter2: 2, parameter3: 0.5, parameter4: 0.005, button_parameter1: 1 } },
        // autopan: parameter1 period (beats), 2 smoothing, 3 width
        { name: "DJM PAN", id: "org.mixxx.effects.autopan", depth: "parameter3",
          params: { parameter1: 2, parameter2: 0.5, parameter3: 1 } }
    ]
    // both decks' EQ mode (EqualizerRack1_[ChannelN]_Effect1, button_parameter4)
    readonly property bool stems: stems1.value > 0
    property int chooserDeck: 0 // the deck whose FX chooser is open, 0 = none
    readonly property int chooserEffect: chooserDeck > 0 ? effectIndex(chooserDeck) : -1
    property real chooserX: 0
    property real chooserY: 0

    Mixxx.ControlProxy { id: stems1; group: "[EqualizerRack1_[Channel1]_Effect1]"; key: "button_parameter4" }
    Mixxx.ControlProxy { id: stems2; group: "[EqualizerRack1_[Channel2]_Effect1]"; key: "button_parameter4" }

    function setStems(on) {
        stems1.value = on ? 1 : 0;
        stems2.value = on ? 1 : 0;
        Mixxx.DJMantra.log("EQ mode: " + (on ? "Neural Mix EQ" : "Equalizer"));
        Toasts.show(on ? "EQ \u2192 NEURAL MIX" : "EQ \u2192 EQUALIZER");
    }

    function effectIndex(deck) {
        const i = Number(Mixxx.DJMantra.setting("fx_choice_" + deck, "0"));
        return i >= 0 && i < effects.length ? i : 0;
    }

    function loadEffect(deck, index) {
        const unit = "[EffectRack1_EffectUnit" + deck + "]";
        Mixxx.DJMantra.setControl(unit, "group_[Channel1]_enable", deck === 1 ? 1 : 0);
        Mixxx.DJMantra.setControl(unit, "group_[Channel2]_enable", deck === 2 ? 1 : 0);
        Mixxx.DJMantra.setControl(unit, "mix", 1);
        Mixxx.DJMantra.setControl(unit, "enabled", 1);
        const slot = Mixxx.EffectsManager.getEffectSlot(deck, 1);
        if (slot && slot.effectId !== effects[index].id) {
            slot.effectId = effects[index].id;
            Mixxx.DJMantra.log("FX: deck " + deck + " " + effects[index].name);
        }
        // the DJM effects' settings (also when the same Mixxx effect was there already)
        const params = effects[index].params || {};
        for (const key in params)
            Mixxx.DJMantra.setControl("[EffectRack1_EffectUnit" + deck + "_Effect1]", key, params[key]);
        // Echo Out runs while chosen (it keeps the last beats, as djay's); its
        // button_parameter1 is the ON that cuts the song into the echo
        if (index === 3)
            Mixxx.DJMantra.setControl("[EffectRack1_EffectUnit" + deck + "_Effect1]", "enabled", 1);
    }

    // the control the FX button switches: Echo Out's "engaged", the others' "enabled"
    function fxKey(index) {
        return index === 3 ? "button_parameter1" : "enabled";
    }

    Component.onCompleted: {
        Mixxx.DJMantra.useDjmFilter();
        loadEffect(1, effectIndex(1));
        loadEffect(2, effectIndex(2));
    }

    DButton {
        // the CQ mixer's delay on the app's sound (Settings -> CQ MIXER), in time
        // with the on-air deck: shown while the CQ is connected
        id: cqDelay

        objectName: "cqDelay"
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottomMargin: Style.gap
        visible: Mixxx.DJMantra.cqConnected
        width: visible ? 160 : 0
        height: visible ? 40 : 0
        text: qsTr("CQ DELAY")
        fontSize: 13
        lit: Mixxx.DJMantra.cqDelayOn
        onClicked: Mixxx.DJMantra.cqSetDelay(!Mixxx.DJMantra.cqDelayOn)
    }

    DButton {
        // djay's "EQ ⌄": one mode for both decks
        id: eqMode

        objectName: "eqMode"
        anchors.top: parent.top
        anchors.topMargin: Style.gap
        anchors.horizontalCenter: parent.horizontalCenter
        width: 150
        height: 36
        mode: ""
        text: root.stems ? qsTr("NEURAL MIX") : qsTr("EQ")
        dropdown: true
        fontSize: 13
        lit: eqMenu.visible
        onClicked: eqMenu.visible = !eqMenu.visible
    }

    Row {
        anchors.top: eqMode.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: cqDelay.visible ? cqDelay.top : parent.bottom
        anchors.margins: Style.gap
        spacing: Style.gap

        Repeater {
            model: ["[Channel1]", "[Channel2]"]

            delegate: Item {
                id: deck

                required property string modelData
                required property int index
                readonly property string eq: "[EqualizerRack1_" + modelData + "_Effect1]"
                readonly property int number: index + 1
                readonly property string fxGroup: "[EffectRack1_EffectUnit" + number + "_Effect1]"
                property int choice: root.effectIndex(number)
                // knobs and two buttons share the column's height
                // Marko 13.10.2026: the FILTER knob is gone, the rest grows (4 knobs, 2 buttons)
                readonly property real knobHeight: (knobs.height - 2 * 52 - 5 * knobs.spacing) / 4

                Mixxx.ControlProxy { id: fxEnabled; group: deck.fxGroup; key: root.fxKey(deck.choice) }

                width: (root.width - 3 * Style.gap) / 2
                height: parent.height

                Column {
                    id: knobs

                    width: 84
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: deck.index === 0 ? parent.left : undefined
                    anchors.right: deck.index === 1 ? parent.right : undefined
                    spacing: 2

                    DKnob { width: knobs.width; height: deck.knobHeight; group: deck.eq; key: root.stems ? "parameter6" : "parameter3"; label: root.stems ? "HARMONY" : "HIGH"; outerKey: root.stems ? "parameter3" : "parameter6"; outerLabel: root.stems ? "HIGH" : "HARMONY"; outerColor: Style.subText; arcColor: Style.water; defaultParameter: 1; dbMode: "cut"; toastPrefix: "DECK " + deck.number + "  " }
                    DKnob { width: knobs.width; height: deck.knobHeight; group: deck.eq; key: root.stems ? "parameter5" : "parameter2"; label: root.stems ? "VOCALS" : "MID"; outerKey: root.stems ? "parameter2" : "parameter5"; outerLabel: root.stems ? "MID" : "VOCALS"; outerColor: Style.subText; arcColor: Style.earth; defaultParameter: 1; dbMode: "cut"; toastPrefix: "DECK " + deck.number + "  " }
                    DKnob { width: knobs.width; height: deck.knobHeight; group: deck.eq; key: root.stems ? "parameter4" : "parameter1"; label: root.stems ? "DRUMS" : "LOW"; outerKey: root.stems ? "parameter1" : "parameter4"; outerLabel: root.stems ? "LOW" : "DRUMS"; outerColor: Style.subText; arcColor: Style.fire; defaultParameter: 1; dbMode: "cut"; toastPrefix: "DECK " + deck.number + "  " }
                    DKnob {
                        // FILTER at the bottom, as on the Mix Ultra (Marko 14.10.2026: no GAIN knob, every
                        // song is normalized to -1 dBFS and the faders set the volume): the Pioneer DJM filter,
                        // left low-pass, middle off, right high-pass; a long press resets it to off
                        objectName: "filter" + deck.number
                        width: knobs.width
                        height: deck.knobHeight
                        group: "[QuickEffectRack1_" + deck.modelData + "]"
                        key: "super1"
                        label: "FILTER"
                        arcColor: Style.water
                        defaultParameter: 0.5
                        toastPrefix: "DECK " + deck.number + "  "
                        resetText: qsTr("OFF")
                    }
                    DButton {
                        // tap = on / off, hold = choose the effect
                        id: fxButton

                        property bool held: false

                        objectName: "fx" + deck.number
                        width: knobs.width
                        height: 52
                        mode: ""
                        text: root.effects[deck.choice].name
                        fontSize: 13
                        lit: fxEnabled.value > 0
                        onPressedDown: held = false
                        onLongPressed: {
                            held = true;
                            const p = fxButton.mapToItem(root, 0, 0);
                            root.chooserX = Math.max(Style.gap, Math.min(root.width - fxChooser.width - Style.gap,
                                    deck.index === 0 ? p.x : p.x + fxButton.width - fxChooser.width));
                            root.chooserY = p.y; // the chooser opens above this
                            root.chooserDeck = deck.number;
                        }
                        onClicked: {
                            if (held)
                                return;
                            // Echo Out: its slot must run to keep the last beats (round 38:
                            // the ON did nothing when the slot was off)
                            if (deck.choice === 3)
                                Mixxx.DJMantra.setControl(deck.fxGroup, "enabled", 1);
                            fxEnabled.value = fxEnabled.value > 0 ? 0 : 1;
                            Toasts.show("DECK " + deck.number + "  " + text + (fxEnabled.value > 0 ? " ON" : " OFF"));
                            const slot = Mixxx.EffectsManager.getEffectSlot(deck.number, 1);
                            Mixxx.DJMantra.log("FX: deck " + deck.number + " " + text
                                    + (fxEnabled.value > 0 ? " on" : " off")
                                    + " (slot " + (slot ? slot.effectId : "?") + ", "
                                    + root.fxKey(deck.choice) + ")");
                        }
                    }
                    DButton { width: knobs.width; height: 52; text: "🎧"; fontSize: 20; group: deck.modelData; key: "pfl" }
                }

                Connections {
                    target: root
                    function onChooserDeckChanged() {
                        // the chooser closed: show this deck's (maybe new) effect
                        deck.choice = root.effectIndex(deck.number);
                    }
                }

                Row {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: deck.index === 0 ? knobs.right : undefined
                    anchors.right: deck.index === 1 ? knobs.left : undefined
                    anchors.leftMargin: Style.gap
                    anchors.rightMargin: Style.gap
                    spacing: Style.gap
                    layoutDirection: deck.index === 0 ? Qt.LeftToRight : Qt.RightToLeft

                    PitchVolumeFader {
                        // long press: VOLUME <-> PITCH (Marko 13.10.2026); above the VU meter,
                        // which on deck 2 is on its left and hid the BPM scale (Marko 14.10.2026)
                        z: 1
                        width: deck.width - knobs.width - 3 * Style.gap - 14
                        height: parent.height
                        group: deck.modelData
                        barColor: deck.index === 0 ? Style.deck1 : Style.deck2
                    }

                    VuBar {
                        width: 14
                        height: parent.height
                        group: deck.modelData
                    }
                }
            }
        }
    }

    MouseArea {
        // a tap outside closes an open menu
        anchors.fill: parent
        visible: eqMenu.visible || root.chooserDeck > 0
        onClicked: {
            eqMenu.visible = false;
            root.chooserDeck = 0;
        }
    }

    Rectangle {
        // the EQ mode menu, under its button
        id: eqMenu

        objectName: "eqMenu"
        visible: false
        anchors.top: eqMode.bottom
        anchors.topMargin: 4
        anchors.horizontalCenter: parent.horizontalCenter
        width: 220
        height: eqItems.height + 12
        radius: Style.radius
        color: Style.surface
        border.color: Style.line

        Column {
            id: eqItems

            anchors.centerIn: parent
            spacing: 4

            Repeater {
                model: [[false, qsTr("Equalizer")], [true, qsTr("Neural Mix EQ")]]

                delegate: DButton {
                    required property var modelData

                    width: 208
                    height: 40
                    mode: ""
                    text: modelData[1]
                    fontSize: 14
                    lit: root.stems === modelData[0]
                    onClicked: {
                        root.setStems(modelData[0]);
                        eqMenu.visible = false;
                    }
                }
            }
        }
    }

    Rectangle {
        // the FX chooser, opening upwards from the deck's FX button
        id: fxChooser

        objectName: "fxChooser"
        visible: root.chooserDeck > 0
        x: root.chooserX
        // above the FX button, whatever its height (the depth row adds to it)
        y: Math.max(Style.gap, root.chooserY - height - 4)
        width: 2 * 158 + 4 + 12
        height: fxItems.height + 12
        radius: Style.radius
        color: Style.surface
        border.color: Style.line

        Column {
            id: fxItems

            anchors.centerIn: parent
            spacing: 4

            Grid {
                columns: 2
                spacing: 4

            Repeater {
                model: root.effects

                delegate: DButton {
                    required property var modelData
                    required property int index

                    objectName: "fxChoice_" + index
                    width: 158
                    height: 40
                    mode: ""
                    text: modelData.name
                    fontSize: 13
                    lit: root.chooserDeck > 0 && root.effectIndex(root.chooserDeck) === index
                    onClicked: {
                        const n = root.chooserDeck;
                        Mixxx.DJMantra.setSetting("fx_choice_" + n, index);
                        root.loadEffect(n, index);
                        Toasts.show("DECK " + n + "  FX \u2192 " + modelData.name);
                        // choosing turns it on, as in djay
                        Mixxx.DJMantra.setControl("[EffectRack1_EffectUnit" + n + "_Effect1]", root.fxKey(index), 1);
                        root.chooserDeck = 0;
                    }
                }
            }

            }

            // Marko 13.10.2026: the depth of the deck's effect (djay's slider): Flanger
            // = its width, Underwater / Cooldown = amount (0.5 = djay's middle; Cooldown
            // fades the deck out at the right end), Echo Out = 1 or 2 beats; the DJM ones:
            // DELAY the send, ECHO the feedback, TRANS the depth, PAN the width
            Text {
                visible: root.chooserDeck > 0
                width: 2 * 158 + 4
                horizontalAlignment: Text.AlignHCenter
                text: root.chooserEffect === 3 ? qsTr("ECHO") : qsTr("DEPTH")
                color: Style.subText
                font.pixelSize: 11
                font.bold: true
            }

            DFader {
                objectName: "fxDepth"
                visible: root.chooserDeck > 0 && root.chooserEffect !== 3
                width: 2 * 158 + 4
                height: 36
                vertical: false
                group: "[EffectRack1_EffectUnit" + Math.max(1, root.chooserDeck) + "_Effect1]"
                key: root.chooserEffect >= 0 ? root.effects[root.chooserEffect].depth : "parameter1"
                barColor: Style.accent
                label: "DECK " + Math.max(1, root.chooserDeck) + "  DEPTH"
                resetText: "50 %"
            }

            Row {
                visible: root.chooserDeck > 0 && root.chooserEffect === 3
                spacing: 4

                Mixxx.ControlProxy {
                    id: echoBeats

                    group: "[EffectRack1_EffectUnit" + Math.max(1, root.chooserDeck) + "_Effect1]"
                    key: "parameter1"
                }

                Repeater {
                    model: [1, 2]

                    delegate: DButton {
                        required property int modelData

                        width: 158
                        height: 36
                        mode: ""
                        text: modelData === 1 ? qsTr("1 BEAT") : qsTr("2 BEATS")
                        fontSize: 12
                        lit: Math.round(echoBeats.value) === modelData
                        onClicked: {
                            echoBeats.value = modelData;
                            Mixxx.DJMantra.log("FX: deck " + root.chooserDeck + " ECHO OUT " + modelData + " beats");
                        }
                    }
                }
            }
        }
    }
}
