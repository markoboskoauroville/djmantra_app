import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The mixer, as on the Mix Ultra: per deck GAIN, HIGH, MID, LOW, FILTER, the FX
// button and the headphone button; the VU meters and the volume faders in the middle.
// V18 (Marko 13.10.2026, djay Android): ONE "EQ" dropdown at the top switches both
// decks' knobs (Equalizer / Neural Mix EQ: drums, vocals, harmonies); per deck ONE FX
// button: tap = the effect on / off, hold = a drop-up chooser (Flanger, Underwater,
// Cooldown, Echo Out). Deck N's effect is effect unit N, slot 1, on deck N only.
Item {
    id: root

    readonly property var effects: [
        { name: "FLANGER", id: "org.mixxx.effects.flanger" },
        { name: "UNDERWATER", id: "org.djmantra.effects.underwater" },
        { name: "COOLDOWN", id: "org.djmantra.effects.cooldown" },
        { name: "ECHO OUT", id: "org.djmantra.effects.echoout" }
    ]
    // both decks' EQ mode (EqualizerRack1_[ChannelN]_Effect1, button_parameter4)
    readonly property bool stems: stems1.value > 0
    property int chooserDeck: 0 // the deck whose FX chooser is open, 0 = none
    property real chooserX: 0
    property real chooserY: 0

    Mixxx.ControlProxy { id: stems1; group: "[EqualizerRack1_[Channel1]_Effect1]"; key: "button_parameter4" }
    Mixxx.ControlProxy { id: stems2; group: "[EqualizerRack1_[Channel2]_Effect1]"; key: "button_parameter4" }

    function setStems(on) {
        stems1.value = on ? 1 : 0;
        stems2.value = on ? 1 : 0;
        Mixxx.DJMantra.log("EQ mode: " + (on ? "Neural Mix EQ" : "Equalizer"));
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
                readonly property real knobHeight: (knobs.height - 2 * 40 - 6 * knobs.spacing) / 5

                Mixxx.ControlProxy { id: fxEnabled; group: deck.fxGroup; key: root.fxKey(deck.choice) }

                width: (root.width - 3 * Style.gap) / 2
                height: parent.height

                Column {
                    id: knobs

                    width: 64
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: deck.index === 0 ? parent.left : undefined
                    anchors.right: deck.index === 1 ? parent.right : undefined
                    spacing: 2

                    DKnob { width: knobs.width; height: deck.knobHeight; group: deck.modelData; key: "pregain"; label: "GAIN"; arcColor: Style.text }
                    DKnob { width: knobs.width; height: deck.knobHeight; group: deck.eq; key: "parameter3"; label: root.stems ? "HARMONY" : "HIGH"; defaultParameter: 1; arcColor: Style.water }
                    DKnob { width: knobs.width; height: deck.knobHeight; group: deck.eq; key: "parameter2"; label: root.stems ? "VOCALS" : "MID"; defaultParameter: 1; arcColor: Style.earth }
                    DKnob { width: knobs.width; height: deck.knobHeight; group: deck.eq; key: "parameter1"; label: root.stems ? "DRUMS" : "LOW"; defaultParameter: 1; arcColor: Style.fire }
                    DKnob {
                        width: knobs.width
                        height: deck.knobHeight
                        group: "[QuickEffectRack1_" + deck.modelData + "]"
                        key: "super1"
                        label: "FILTER"
                        arcColor: Style.accent
                    }
                    DButton {
                        // tap = on / off, hold = choose the effect
                        id: fxButton

                        property bool held: false

                        objectName: "fx" + deck.number
                        width: knobs.width
                        height: 40
                        mode: ""
                        text: root.effects[deck.choice].name
                        fontSize: 11
                        lit: fxEnabled.value > 0
                        onPressedDown: held = false
                        onLongPressed: {
                            held = true;
                            const p = fxButton.mapToItem(root, 0, 0);
                            root.chooserX = Math.max(Style.gap, Math.min(root.width - fxChooser.width - Style.gap,
                                    deck.index === 0 ? p.x : p.x + fxButton.width - fxChooser.width));
                            root.chooserY = Math.max(Style.gap, p.y - fxChooser.height - 4);
                            root.chooserDeck = deck.number;
                        }
                        onClicked: {
                            if (held)
                                return;
                            fxEnabled.value = fxEnabled.value > 0 ? 0 : 1;
                            Mixxx.DJMantra.log("FX: deck " + deck.number + " " + text
                                    + (fxEnabled.value > 0 ? " on" : " off"));
                        }
                    }
                    DButton { width: knobs.width; height: 40; text: "🎧"; group: deck.modelData; key: "pfl" }
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

                    DFader {
                        width: deck.width - knobs.width - 3 * Style.gap - 14
                        height: parent.height
                        group: deck.modelData
                        key: "volume"
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
        y: root.chooserY
        width: 170
        height: fxItems.height + 12
        radius: Style.radius
        color: Style.surface
        border.color: Style.line

        Column {
            id: fxItems

            anchors.centerIn: parent
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
                        // choosing turns it on, as in djay
                        Mixxx.DJMantra.setControl("[EffectRack1_EffectUnit" + n + "_Effect1]", root.fxKey(index), 1);
                        root.chooserDeck = 0;
                    }
                }
            }
        }
    }
}
