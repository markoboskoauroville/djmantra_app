import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// djay Pro's favourite FX for one deck (Marko 13.10.2026): FLANGER, UNDERWATER,
// COOLDOWN, ECHO OUT. Tap = on / off (several at once, as in djay); the slider and
// the D/W knob below edit the last one tapped (Echo Out: ◀ BEATS ▶ instead of the
// slider). Deck N's effect unit N holds Flanger / Underwater / Cooldown in slots
// 1-3, unit N+2 holds Echo Out; both only on this deck, unit mix 1 (each FX has its
// own D/W). DSP: src/effects/backends/builtin/djayfx.h
Column {
    id: root

    required property int deck // 1 or 2
    readonly property string channel: "[Channel" + deck + "]"
    property int selected: 0
    readonly property var fx: [
        { name: "FLANGER", unit: deck, slot: 1, id: "org.mixxx.effects.flanger", amount: "parameter1", mix: "parameter5" },
        { name: "UNDERWATER", unit: deck, slot: 2, id: "org.djmantra.effects.underwater", amount: "parameter1", mix: "parameter2" },
        { name: "COOLDOWN", unit: deck, slot: 3, id: "org.djmantra.effects.cooldown", amount: "parameter1", mix: "parameter2" },
        { name: "ECHO OUT", unit: deck + 2, slot: 1, id: "org.djmantra.effects.echoout", amount: "parameter1", mix: "parameter2" }
    ]
    readonly property var current: fx[selected]
    readonly property string currentGroup: slotGroup(current)
    readonly property var echoBeats: [0.5, 1, 2, 4]

    function slotGroup(f) {
        return "[EffectRack1_EffectUnit" + f.unit + "_Effect" + f.slot + "]";
    }

    function setup() {
        // Route the two units to this deck only and load the FX once.
        const units = [deck, deck + 2];
        for (let u = 0; u < units.length; ++u) {
            const unit = "[EffectRack1_EffectUnit" + units[u] + "]";
            Mixxx.DJMantra.setControl(unit, "group_[Channel1]_enable", root.deck === 1 ? 1 : 0);
            Mixxx.DJMantra.setControl(unit, "group_[Channel2]_enable", root.deck === 2 ? 1 : 0);
            Mixxx.DJMantra.setControl(unit, "mix", 1);
            Mixxx.DJMantra.setControl(unit, "enabled", 1);
        }
        for (let i = 0; i < fx.length; ++i) {
            const slot = Mixxx.EffectsManager.getEffectSlot(fx[i].unit, fx[i].slot);
            if (slot && slot.effectId !== fx[i].id) {
                slot.effectId = fx[i].id;
                Mixxx.DJMantra.log("FX: deck " + deck + " " + fx[i].name + " -> unit " + fx[i].unit + " slot " + fx[i].slot);
            }
        }
    }

    Component.onCompleted: setup()
    spacing: 4

    Grid {
        columns: 2
        spacing: 4

        Repeater {
            model: root.fx

            delegate: DButton {
                required property var modelData
                required property int index

                objectName: "fx" + root.deck + "_" + index
                width: (root.width - 4) / 2
                height: 34
                text: modelData.name
                fontSize: 11
                group: root.slotGroup(modelData)
                key: "enabled"
                border.color: root.selected === index ? Style.text : (lit ? litColor : Style.surface)
                onClicked: {
                    root.selected = index;
                    Mixxx.DJMantra.log("FX: deck " + root.deck + " " + modelData.name + (lit ? " on" : " off"));
                }
            }
        }
    }

    Row {
        spacing: 4
        height: 40

        Item {
            width: root.width - 4 - 40
            height: parent.height

            DFader {
                visible: root.selected !== 3
                anchors.fill: parent
                vertical: false
                group: root.currentGroup
                key: root.current.amount
                barColor: root.deck === 1 ? Style.deck1 : Style.deck2
            }

            Row {
                // Echo Out: ◀ 2 BEATS ▶
                visible: root.selected === 3
                anchors.fill: parent
                spacing: 2

                Mixxx.ControlProxy { id: beats; group: root.currentGroup; key: "parameter1" }

                DButton {
                    width: 30; height: parent.height; text: "<"; mode: ""
                    onClicked: beatsLabel.step(-1)
                }
                Text {
                    id: beatsLabel

                    function step(delta) {
                        let i = root.echoBeats.indexOf(beats.value);
                        i = Math.max(0, Math.min(root.echoBeats.length - 1, (i < 0 ? 2 : i) + delta));
                        beats.value = root.echoBeats[i];
                        Mixxx.DJMantra.log("FX: deck " + root.deck + " ECHO OUT " + beats.value + " beats");
                    }

                    width: parent.width - 64
                    height: parent.height
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Style.text
                    font.pixelSize: 12
                    text: (beats.value < 1 ? "1/2" : Math.round(beats.value)) + (beats.value > 1 ? " BEATS" : " BEAT")
                }
                DButton {
                    width: 30; height: parent.height; text: ">"; mode: ""
                    onClicked: beatsLabel.step(1)
                }
            }
        }

        DKnob {
            width: 40
            height: parent.height
            group: root.currentGroup
            key: root.current.mix
            label: "D/W"
            defaultParameter: 1
            arcColor: Style.accent
        }
    }
}
