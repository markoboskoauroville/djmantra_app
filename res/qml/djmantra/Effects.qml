pragma Singleton
import "."
import Mixxx 1.0 as Mixxx
import QtQuick 2.12

// The decks' effects, shared by the Mixer's FX buttons and the Mix Ultra's FX pads (V77,
// Marko 15.10.2026: "each effect to the pad, and the pad is activated by Shift FX ... we have
// 8 pads ... if we have more than 8 effects we can add more effects triggering by Shift Pad.
// Then we have 16 pads"). Deck N's effect is effect unit N, slot 1, on deck N only. Effect i
// is pad i + 1 (SHIFT + pad for 9 to 16).
QtObject {
    id: fx

    // djay's four, then the classic Pioneer DJM beat effects (Marko 13.10.2026: "take some
    // classic old Pioneer ... the most important ones"; the DJM-600's BEAT EFFECTS: DELAY
    // repeats once, ECHO repeats fading, TRANS cuts the sound on the beat, PAN moves it left
    // and right), on Mixxx's beat-synced effects with DJM-like settings (`params`, real
    // units) and the DEPTH slider on `depth`
    readonly property var list: [
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
    // the Echo Out entry: it runs while chosen and its button_parameter1 is the ON
    readonly property int echoOut: 3
    // one more each time a deck's effect is chosen (the Mixer's buttons follow it)
    property int revision: 0

    // whether each deck's effect is on (the control the FX button switches)
    readonly property bool on1: (choice1 === echoOut ? echo1.value : enabled1.value) > 0
    readonly property bool on2: (choice2 === echoOut ? echo2.value : enabled2.value) > 0
    property int choice1: index(1)
    property int choice2: index(2)

    property Mixxx.ControlProxy enabled1: Mixxx.ControlProxy { group: "[EffectRack1_EffectUnit1_Effect1]"; key: "enabled" }
    property Mixxx.ControlProxy enabled2: Mixxx.ControlProxy { group: "[EffectRack1_EffectUnit2_Effect1]"; key: "enabled" }
    property Mixxx.ControlProxy echo1: Mixxx.ControlProxy { group: "[EffectRack1_EffectUnit1_Effect1]"; key: "button_parameter1" }
    property Mixxx.ControlProxy echo2: Mixxx.ControlProxy { group: "[EffectRack1_EffectUnit2_Effect1]"; key: "button_parameter1" }
    // the controller reads which pad is lit: 1..16, 0 = none (Mix Ultra script)
    property Mixxx.ControlProxy active1: Mixxx.ControlProxy { group: "[DJMantra]"; key: "fx_active_1" }
    property Mixxx.ControlProxy active2: Mixxx.ControlProxy { group: "[DJMantra]"; key: "fx_active_2" }

    // SHIFT + FILTER on the Mix Ultra (V77): [DJMantra] fx_depth_<deck> mirrors the DEPTH of the
    // deck's effect (normalized, as the Mixer's DEPTH slider), both ways; Echo Out: 1 beat = 0.25, 2 beats = 0.75 (the knob's left / right half)
    property Mixxx.ControlProxy depth1: Mixxx.ControlProxy {
        group: "[EffectRack1_EffectUnit1_Effect1]"
        key: fx.list[fx.choice1].depth
        onParameterChanged: fx.mirrorDepth(1)
    }
    property Mixxx.ControlProxy depth2: Mixxx.ControlProxy {
        group: "[EffectRack1_EffectUnit2_Effect1]"
        key: fx.list[fx.choice2].depth
        onParameterChanged: fx.mirrorDepth(2)
    }
    property Mixxx.ControlProxy knobDepth1: Mixxx.ControlProxy {
        group: "[DJMantra]"; key: "fx_depth_1"
        onValueChanged: fx.setDepth(1, value)
    }
    property Mixxx.ControlProxy knobDepth2: Mixxx.ControlProxy {
        group: "[DJMantra]"; key: "fx_depth_2"
        onValueChanged: fx.setDepth(2, value)
    }

    function mirrorDepth(deck) {
        const d = deck === 2 ? depth2 : depth1;
        const k = deck === 2 ? knobDepth2 : knobDepth1;
        const v = (deck === 2 ? choice2 : choice1) === echoOut ? (d.value >= 1.5 ? 0.75 : 0.25) : d.parameter;
        if (Math.abs(k.value - v) > 0.0005)
            k.value = v;
    }

    function setDepth(deck, v) {
        const d = deck === 2 ? depth2 : depth1;
        if ((deck === 2 ? choice2 : choice1) === echoOut) {
            const beats = v >= 0.5 ? 2 : 1;
            if (Math.round(d.value) !== beats)
                d.value = beats;
        } else if (Math.abs(d.parameter - v) > 0.0005) {
            d.parameter = v;
        }
    }

    onOn1Changed: active1.value = on1 ? choice1 + 1 : 0
    onOn2Changed: active2.value = on2 ? choice2 + 1 : 0
    onChoice1Changed: active1.value = on1 ? choice1 + 1 : 0
    onChoice2Changed: active2.value = on2 ? choice2 + 1 : 0
    Component.onCompleted: {
        mirrorDepth(1);
        mirrorDepth(2);
    }

    function group(deck) {
        return "[EffectRack1_EffectUnit" + deck + "_Effect1]";
    }

    function index(deck) {
        const i = Number(Mixxx.DJMantra.setting("fx_choice_" + deck, "0"));
        return i >= 0 && i < list.length ? i : 0;
    }

    // the control the FX button switches: Echo Out's "engaged", the others' "enabled"
    function key(i) {
        return i === echoOut ? "button_parameter1" : "enabled";
    }

    function isOn(deck) {
        return deck === 2 ? on2 : on1;
    }

    // effect `i` into deck `deck`'s unit, with its settings
    function load(deck, i) {
        const unit = "[EffectRack1_EffectUnit" + deck + "]";
        Mixxx.DJMantra.setControl(unit, "group_[Channel1]_enable", deck === 1 ? 1 : 0);
        Mixxx.DJMantra.setControl(unit, "group_[Channel2]_enable", deck === 2 ? 1 : 0);
        Mixxx.DJMantra.setControl(unit, "mix", 1);
        Mixxx.DJMantra.setControl(unit, "enabled", 1);
        const slot = Mixxx.EffectsManager.getEffectSlot(deck, 1);
        if (slot && slot.effectId !== list[i].id) {
            slot.effectId = list[i].id;
            Mixxx.DJMantra.log("FX: deck " + deck + " " + list[i].name);
        }
        // the DJM effects' settings (also when the same Mixxx effect was there already)
        const params = list[i].params || {};
        for (const k in params)
            Mixxx.DJMantra.setControl(group(deck), k, params[k]);
        // Echo Out runs while chosen (it keeps the last beats, as djay's)
        if (i === echoOut)
            Mixxx.DJMantra.setControl(group(deck), "enabled", 1);
    }

    // choose effect `i` for the deck (remembered) and switch it on or off
    function choose(deck, i, on) {
        if (i !== index(deck)) {
            // the old one off first, so that a switch does not leave Echo Out's ON
            Mixxx.DJMantra.setControl(group(deck), key(index(deck)), 0);
            Mixxx.DJMantra.setSetting("fx_choice_" + deck, i);
        }
        load(deck, i);
        Mixxx.DJMantra.setControl(group(deck), key(i), on ? 1 : 0);
        if (deck === 2)
            choice2 = i;
        else
            choice1 = i;
        revision++;
    }

    // a Mix Ultra FX pad (1..16) pressed on `deck`: that effect on; the same pad again: off
    function pad(deck, number) {
        const i = number - 1;
        if (i < 0 || i >= list.length) {
            Toasts.show("DECK " + deck + "  FX PAD " + number + ": NO EFFECT");
            return;
        }
        const on = !(i === index(deck) && isOn(deck));
        choose(deck, i, on);
        Toasts.show("DECK " + deck + "  " + list[i].name + (on ? " ON" : " OFF"));
        Mixxx.DJMantra.log("FX pad: deck " + deck + " pad " + number + " " + list[i].name + (on ? " on" : " off"));
    }

    // a pad held down and let go: its effect off (a held pad is on only while held)
    function padReleased(deck, number) {
        const i = number - 1;
        if (i === index(deck) && isOn(deck)) {
            Mixxx.DJMantra.setControl(group(deck), key(i), 0);
            Toasts.show("DECK " + deck + "  " + list[i].name + " OFF");
            Mixxx.DJMantra.log("FX pad: deck " + deck + " pad " + number + " released");
        }
    }
}
