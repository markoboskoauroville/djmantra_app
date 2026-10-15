// Hercules DJControl Mix Ultra (USB and Bluetooth LE MIDI) for DJ Mantra / Mixxx 2.5
//
// Written from a capture of the real controller (testing/results/2026-10-08_mix-ultra.md):
//   channels   0x90/0xB0 global, 0x93/0xB3 global + SHIFT,
//              0x91/0xB1 deck 1, 0x92/0xB2 deck 2, 0x94/0xB4 and 0x95/0xB5 their SHIFT layers,
//              0x96 / 0x97 pads of deck 1 / deck 2
//   SHIFT      one per deck: 91 04 / 92 04
//   knobs      14-bit (MSB n, LSB n + 0x20), mapped in the XML
//   jog        touch 08, top CC 0A, outer ring CC 09, 01 = +1 tick, 7F = -1 tick, ~240 ticks a turn
//   modes      0F HOT CUE, 10 LOOP, 11 FX, 12 NEURAL MIX; with SHIFT 13 PITCH PLAY, 14 BOUNCE LOOP,
//              15 SLICER (DJ Mantra: FX PADS), 16 SAMPLER (same deck channel). The controller switches the pad mode
//              and its LEDs itself.
//   pads       note = mode base + pad (0-7), + 8 with SHIFT; bases 00 10 20 30 40 50 60 70
//
// Beat lights (DJ Mantra default, like djay's slicer): in every pad mode the pads show where the
// deck is in the 8-beat phrase, one pad per beat, so two decks can be aligned "1 with 1" by eye.
// Turn them off with BEAT_LIGHTS = false below.
//
// TODO (next local test): deck 1 SHIFT + jog top (captured as B1 0A, expected B4 0A), pad LED
// notes and colours in each mode (only notes 30-37 lit pads, in NEURAL MIX mode), tempo fader
// direction.

var MixUltra = {};

MixUltra.BEAT_LIGHTS = true;
MixUltra.PHRASE_BEATS = 8;
MixUltra.JOG_TICKS_PER_TURN = 240;
MixUltra.SCRATCH_RPM = 33 + 1 / 3;
MixUltra.SCRATCH_ALPHA = 1.0 / 8;
MixUltra.SCRATCH_BETA = MixUltra.SCRATCH_ALPHA / 32;
MixUltra.JOG_PITCH_SCALE = 0.6;   // ring / top without touch: pitch bend
MixUltra.JOG_SEEK_SCALE = 6;      // SHIFT + top: fast seek

MixUltra.MODE_BASES = {
    hotcue: 0x00, loop: 0x10, fx: 0x20, neural: 0x30,
    pitchplay: 0x40, bounce: 0x50, fxpads: 0x60, sampler: 0x70,
};
MixUltra.MODE_BUTTONS = {
    0x0F: "hotcue", 0x10: "loop", 0x11: "fx", 0x12: "neural",
    0x13: "pitchplay", 0x14: "bounce", 0x15: "fxpads", 0x16: "sampler",
};
MixUltra.LOOP_SIZES = [0.25, 0.5, 1, 2, 4, 8, 16, 32];
MixUltra.ROLL_SIZES = [0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8];
MixUltra.PITCH_STEPS = [-3, -2, -1, 0, 1, 2, 3, 4];
// FX PADS: a pad held longer than this is on only while held (a tap stays on)
MixUltra.FX_HOLD_MS = 500;

MixUltra.decks = {};

MixUltra.Deck = function(number) {
    this.number = number;
    this.group = "[Channel" + number + "]";
    this.noteStatus = 0x90 + number;      // 0x91 / 0x92
    this.padStatus = 0x95 + number;       // 0x96 / 0x97
    this.shift = false;
    this.touching = false;
    // After power-on the firmware is in HOT CUE mode
    this.mode = "hotcue";
    this.litPad = -1;
    this.fxPress = null;   // FX PADS: the pad held down {number, time}
    this.connections = [];
};

MixUltra.init = function(id, debugging) {
    MixUltra.quietUntil = Date.now() + 1500;
    MixUltra.debugging = debugging;
    for (var number = 1; number <= 2; number++) {
        var deck = new MixUltra.Deck(number);
        MixUltra.decks[number] = deck;
        MixUltra.connectDeckLeds(deck);
    }
    // The controller sends every knob and fader position
    midi.sendShortMsg(0xB0, 0x7F, 0x7F);
    // Neural Mix logo on: the controller is connected
    midi.sendShortMsg(0x90, 0x01, 0x7F);
    MixUltra.refreshLeds();
};

MixUltra.shutdown = function() {
    for (var number = 1; number <= 2; number++) {
        var deck = MixUltra.decks[number];
        if (!deck) {
            continue; // init() did not run
        }
        MixUltra.clearPads(deck);
        [0x05, 0x06, 0x07, 0x0C].forEach(function(note) {
            midi.sendShortMsg(deck.noteStatus, note, 0x00);
        });
    }
    midi.sendShortMsg(0x90, 0x01, 0x00);
    midi.sendShortMsg(0xB0, 0x7F, 0x00);
};

MixUltra.connectDeckLeds = function(deck) {
    var led = function(key, note) {
        deck.connections.push(engine.makeConnection(deck.group, key, function(value) {
            midi.sendShortMsg(deck.noteStatus, note, value > 0 ? 0x7F : 0x00);
        }));
    };
    led("play_indicator", 0x07);
    led("cue_indicator", 0x06);
    led("sync_enabled", 0x05);
    led("pfl", 0x0C);
    // Beat lights: DJ Mantra's phrase position (0..7, -1 without beat grid). Older engines
    // without it fall back to counting beats.
    var phrase = engine.makeConnection(deck.group, "beat_in_phrase", function(value) {
        MixUltra.showBeat(deck, value);
    });
    if (phrase) {
        deck.connections.push(phrase);
        deck.hasPhrase = true;
    } else {
        deck.beatCount = -1;
        deck.connections.push(engine.makeConnection(deck.group, "beat_active", function(value) {
            if (value > 0) {
                deck.beatCount = (deck.beatCount + 1) % MixUltra.PHRASE_BEATS;
                MixUltra.showBeat(deck, deck.beatCount);
            }
        }));
    }
    for (var i = 1; i <= 8; i++) {
        deck.connections.push(engine.makeConnection(deck.group, "hotcue_" + i + "_status",
            MixUltra.refreshPads.bind(null, deck)));
    }
    // FX PADS: the app says which effect is on (Effects.qml)
    var fxActive = engine.makeConnection("[DJMantra]", "fx_active_" + deck.number, function(value) {
        print("MixUltra: fx_active_" + deck.number + " = " + value + " (mode " + deck.mode + ")");
        // the app has answered: its value from now on
        deck.fxLitUntil = 0;
        if (deck.mode === "fxpads") {
            MixUltra.refreshPads(deck);
        }
    });
    if (fxActive) {
        deck.connections.push(fxActive);
    } else {
        print("MixUltra: no [DJMantra] fx_active_" + deck.number + " control (FX pad lights from the script only)");
    }
};

MixUltra.refreshLeds = function() {
    for (var number = 1; number <= 2; number++) {
        var deck = MixUltra.decks[number];
        deck.connections.forEach(function(connection) {
            connection.trigger();
        });
        MixUltra.refreshPads(deck);
    }
};

// --- Pad LEDs ---------------------------------------------------------------------------------

MixUltra.padNote = function(deck, pad) {
    return MixUltra.MODE_BASES[deck.mode] + pad;
};

MixUltra.clearPads = function(deck) {
    for (var pad = 0; pad < 8; pad++) {
        midi.sendShortMsg(deck.padStatus, MixUltra.padNote(deck, pad), 0x00);
    }
    deck.litPad = -1;
};

MixUltra.showBeat = function(deck, beat) {
    // FX PADS show the effect that is on instead
    if (!MixUltra.BEAT_LIGHTS || deck.mode === "fxpads") {
        return;
    }
    var pad = beat >= 0 ? Math.floor(beat) % 8 : -1;
    if (pad === deck.litPad) {
        return;
    }
    if (deck.litPad >= 0) {
        midi.sendShortMsg(deck.padStatus, MixUltra.padNote(deck, deck.litPad),
            MixUltra.padBaseValue(deck, deck.litPad));
    }
    if (pad >= 0) {
        midi.sendShortMsg(deck.padStatus, MixUltra.padNote(deck, pad), 0x7F);
    }
    deck.litPad = pad;
};

// The value of a pad without the beat light: mode state (e.g. a set hot cue) or off.
// With beat lights on, only the running light is shown so that the grid stays readable.
MixUltra.padBaseValue = function(deck, pad) {
    if (deck.mode === "fxpads") {
        // the pad of the deck's effect that is on (1..16, 9..16 are SHIFT + pad 1..8): what the
        // app says, else what this script expects after the last press (r42: the pad stayed dark)
        var active = engine.getValue("[DJMantra]", "fx_active_" + deck.number);
        if (deck.fxLit !== undefined && deck.fxLitUntil > Date.now()) {
            active = deck.fxLit;
        }
        return active > 0 && (active - 1) % 8 === pad ? 0x7F : 0x00;
    }
    if (MixUltra.BEAT_LIGHTS) {
        return 0x00;
    }
    if (deck.mode === "hotcue") {
        return engine.getValue(deck.group, "hotcue_" + (pad + 1) + "_status") > 0 ? 0x7F : 0x00;
    }
    if (deck.mode === "sampler") {
        return engine.getValue("[Sampler" + (pad + 1) + "]", "track_loaded") > 0 ? 0x7F : 0x00;
    }
    return 0x00;
};

MixUltra.refreshPads = function(deck) {
    var lit = [];
    for (var pad = 0; pad < 8; pad++) {
        var value = MixUltra.padBaseValue(deck, pad);
        midi.sendShortMsg(deck.padStatus, MixUltra.padNote(deck, pad), value);
        if (value > 0) {
            lit.push(MixUltra.padNote(deck, pad).toString(16));
        }
    }
    if (deck.mode === "fxpads") {
        // r42: the FX pad stayed dark; the log shows what was sent (notes 60-67 on 96 / 97)
        print("MixUltra: FX PADS LEDs deck " + deck.number + " status " + deck.padStatus.toString(16)
            + " lit [" + lit.join(",") + "]");
    }
    var beat = deck.litPad;
    deck.litPad = -1;
    if (deck.hasPhrase) {
        beat = engine.getValue(deck.group, "beat_in_phrase");
    }
    MixUltra.showBeat(deck, beat);
};

// --- Deck buttons -----------------------------------------------------------------------------

MixUltra.deckOf = function(status) {
    var channel = status & 0x0F; // 1/2 (deck), 4/5 (deck + SHIFT), 6/7 (pads)
    if (channel === 1 || channel === 4 || channel === 6) {
        return MixUltra.decks[1];
    }
    return MixUltra.decks[2];
};

MixUltra.shiftButton = function(channel, control, value, status, group) {
    MixUltra.deckOf(status).shift = value > 0;
};

MixUltra.play = function(channel, control, value, status, group) {
    if (value > 0) {
        // DJ Mantra: a playing deck with a STOP TIME (Settings) stops as a turntable,
        // done by the app (TurntableStop.qml)
        if (engine.getValue(group, "play") > 0 && engine.getValue("[DJMantra]", "stop_time") > 0) {
            MixUltra.tellApp("stop_deck", group === "[Channel2]" ? 2 : 1);
        } else {
            script.toggleControl(group, "play");
        }
    }
};

MixUltra.shiftPlay = function(channel, control, value, status, group) {
    // Stutter: back to the cue and play
    engine.setValue(group, "play_stutter", value > 0 ? 1 : 0);
};

MixUltra.cue = function(channel, control, value, status, group) {
    engine.setValue(group, "cue_default", value > 0 ? 1 : 0);
};

MixUltra.shiftCue = function(channel, control, value, status, group) {
    if (value > 0) {
        engine.setValue(group, "start_stop", 1);
    }
};

// SYNC as the screen's SYNC button (r40 part 9: a tap did Mixxx's one-shot beatsync,
// which lit nothing, so "SYNC from the console did not register"): a press toggles the
// deck's sync lock, the same control the screen's SYNC shows
MixUltra.sync = function(channel, control, value, status, group) {
    if (value > 0) {
        script.toggleControl(group, "sync_enabled");
        print("MixUltra: SYNC " + group + " -> " + engine.getValue(group, "sync_enabled"));
    }
};

MixUltra.shiftSync = function(channel, control, value, status, group) {
    if (value > 0) {
        script.toggleControl(group, "keylock");
    }
};

MixUltra.load = function(channel, control, value, status, group) {
    if (value > 0) {
        // the highlighted song of the app's list onto this deck
        MixUltra.tellApp("load", group === "[Channel2]" ? 2 : 1);
    }
};

MixUltra.shiftLoad = function(channel, control, value, status, group) {
    if (value > 0) {
        engine.setValue(group, "eject", 1);
    }
};

MixUltra.pfl = function(channel, control, value, status, group) {
    if (value > 0) {
        script.toggleControl(group, "pfl");
    }
};

MixUltra.shiftPfl = function(channel, control, value, status, group) {
    if (value > 0) {
        script.toggleControl(group, "quantize");
    }
};

// DJ Mantra (Marko, 12.10.2026; 14.10.2026): the pad mode buttons also choose the app's tabs at
// the bottom, one to one in their order: HOT CUE = Songs, LOOP = Phrasing, FX = Mixer,
// NEURAL MIX = AutoMix, SHIFT + NEURAL MIX (SAMPLER) = Keys (either deck's buttons);
// V77: SHIFT + FX (the FX PADS) = Mixer too, where the deck's effect shows. The
// controller still switches its own pad mode (it does so itself), so the pads are read in that
// mode as before.
MixUltra.PAGE_BUTTONS = {0x0F: 1, 0x10: 2, 0x11: 3, 0x12: 4, 0x15: 3, 0x16: 5};

// A pulse on one of the app's [DJMantra] controls (back to 0: the same value twice still counts)
// r41: not a pulse any more (set and straight back to 0: the screen read the 0 and did
// nothing, BROWSE / LOAD / the tabs): the value stays until the app has handled it and set
// it back to 0 itself; BROWSE's turns add up until then
MixUltra.tellApp = function(key, value) {
    if (key === "browse_move") {
        value += engine.getValue("[DJMantra]", key);
    }
    engine.setValue("[DJMantra]", key, value);
};

MixUltra.modeButton = function(channel, control, value, status, group) {
    if (value === 0) {
        return;
    }
    if (MixUltra.PAGE_BUTTONS[control]) {
        // r41: not a pulse (set and back to 0 at once, the screen read 0): the app sets it
        // back to 0 itself once the tab has changed
        engine.setValue("[DJMantra]", "page", MixUltra.PAGE_BUTTONS[control]);
    }
    var deck = MixUltra.deckOf(status);
    var mode = MixUltra.MODE_BUTTONS[control];
    if (!mode || mode === deck.mode) {
        return;
    }
    MixUltra.clearPads(deck);
    deck.mode = mode;
    MixUltra.refreshPads(deck);
};

// --- Jog wheels -------------------------------------------------------------------------------

MixUltra.jogTouch = function(channel, control, value, status, group) {
    var deck = MixUltra.deckOf(status);
    deck.touching = value > 0;
    if (deck.touching) {
        engine.scratchEnable(deck.number, MixUltra.JOG_TICKS_PER_TURN, MixUltra.SCRATCH_RPM,
            MixUltra.SCRATCH_ALPHA, MixUltra.SCRATCH_BETA);
    } else {
        engine.scratchDisable(deck.number);
    }
};

MixUltra.ticks = function(value) {
    return value < 0x40 ? value : value - 0x80; // 01 = +1, 7F = -1
};

// Top of the wheel: scratch while touched, else pitch bend
MixUltra.jogTop = function(channel, control, value, status, group) {
    var deck = MixUltra.deckOf(status);
    var ticks = MixUltra.ticks(value);
    if (engine.isScratching(deck.number)) {
        engine.scratchTick(deck.number, ticks);
    } else {
        engine.setValue(deck.group, "jog", ticks * MixUltra.JOG_PITCH_SCALE);
    }
};

// Outer ring: pitch bend
MixUltra.jogRing = function(channel, control, value, status, group) {
    var deck = MixUltra.deckOf(status);
    engine.setValue(deck.group, "jog", MixUltra.ticks(value) * MixUltra.JOG_PITCH_SCALE);
};

// SHIFT + top: fast seek
MixUltra.jogSeek = function(channel, control, value, status, group) {
    var deck = MixUltra.deckOf(status);
    engine.setValue(deck.group, "jog", MixUltra.ticks(value) * MixUltra.JOG_SEEK_SCALE);
};

// --- Browser and global -----------------------------------------------------------------------

// DJ Mantra (Marko, 12.10.2026): BROWSE brings the app's song list in front; turning moves
// the highlighted song, a press loads it onto the free deck (a folder opens), SHIFT + press goes
// up a folder; a deck's LOAD loads it onto that deck. Loading closes the list again.
// r41 (V71, Bluetooth): right after a (re)connect the controller sends its state, among it
// "90 00 7F", read as a BROWSE press that opened the song list: the browser ignores the
// controller for 1.5 s after a connect
MixUltra.quietUntil = 0;
MixUltra.settling = function() {
    return Date.now() < MixUltra.quietUntil;
};

MixUltra.browse = function(channel, control, value, status, group) {
    if (MixUltra.settling()) {
        return;
    }
    MixUltra.tellApp("browse_move", MixUltra.ticks(value));
};

MixUltra.shiftBrowse = function(channel, control, value, status, group) {
    // SHIFT + turn: ten songs at a time
    MixUltra.tellApp("browse_move", MixUltra.ticks(value) * 10);
};

MixUltra.browsePress = function(channel, control, value, status, group) {
    if (value > 0 && !MixUltra.settling()) {
        MixUltra.tellApp("browse_press", 1);
    }
};

MixUltra.shiftBrowsePress = function(channel, control, value, status, group) {
    if (value > 0 && !MixUltra.settling()) {
        MixUltra.tellApp("browse_back", 1);
    }
};

// STEMS / centre button: Mixxx 2.5 has no stem separation; it toggles the big library
// (DJ Mantra: will switch the immersive waveform / library views).
MixUltra.stems = function(channel, control, value, status, group) {
    if (value > 0) {
        script.toggleControl("[Skin]", "show_maximized_library");
    }
};

MixUltra.shiftStems = function(channel, control, value, status, group) {
    if (value > 0) {
        MixUltra.BEAT_LIGHTS = !MixUltra.BEAT_LIGHTS;
        MixUltra.refreshLeds();
    }
};

// --- Pads -------------------------------------------------------------------------------------

MixUltra.pad = function(channel, control, value, status, group) {
    var deck = MixUltra.decks[status === 0x96 ? 1 : 2];
    var base = control & 0x70;
    var shifted = (control & 0x08) !== 0;
    var pad = control & 0x07;
    var pressed = value > 0;
    var g = deck.group;
    switch (base) {
    case 0x00: // HOT CUE: set / jump; SHIFT: delete
        engine.setValue(g, "hotcue_" + (pad + 1) + (shifted ? "_clear" : "_activate"), pressed ? 1 : 0);
        break;
    case 0x10: // LOOP: beat loops 1/4 .. 32; SHIFT: loop rolls
        if (shifted) {
            engine.setValue(g, "beatlooproll_" + MixUltra.LOOP_SIZES[pad] + "_activate", pressed ? 1 : 0);
        } else if (pressed) {
            engine.setValue(g, "beatloop_" + MixUltra.LOOP_SIZES[pad] + "_toggle", 1);
        }
        break;
    case 0x20: // FX: pads 1-3 effects of unit 1, pad 4 unit 1 on/off; 5-8 the same for unit 2
        if (pressed) {
            var unit = "[EffectRack1_EffectUnit" + (pad < 4 ? 1 : 2) + "]";
            var slot = pad % 4;
            if (shifted) {
                // SHIFT: route the unit to this deck
                script.toggleControl(unit, "group_" + g + "_enable");
            } else if (slot < 3) {
                script.toggleControl("[EffectRack1_EffectUnit" + (pad < 4 ? 1 : 2) + "_Effect" + (slot + 1) + "]", "enabled");
            } else {
                script.toggleControl(unit, "enabled");
            }
        }
        break;
    case 0x30: // NEURAL MIX: no stems in Mixxx 2.5: EQ kills low/mid/high, filter reset;
        // pads 5-8 jump to beat 5-8 of the phrase; SHIFT + pad 5: "1 is here" (V77: from the
        // SLICER, which is the FX PADS now)
        if (pressed) {
            var eq = "[EqualizerRack1_" + g + "_Effect1]";
            if (shifted && pad === 4) {
                engine.setValue(g, "phrase_set_one", 1);
                engine.setValue(g, "phrase_set_one", 0);
            } else if (pad < 3) {
                script.toggleControl(eq, "button_parameter" + (pad + 1));
            } else if (pad === 3) {
                engine.setValue("[QuickEffectRack1_" + g + "]", "super1", 0.5);
            } else {
                MixUltra.jumpToBeat(deck, pad);
            }
        }
        break;
    case 0x40: // PITCH PLAY: key -3 .. +4 semitones; SHIFT: reset
        if (pressed) {
            engine.setValue(g, "pitch", shifted ? 0 : MixUltra.PITCH_STEPS[pad]);
        }
        break;
    case 0x50: // BOUNCE LOOP: loop roll while held
        engine.setValue(g, "beatlooproll_" + MixUltra.ROLL_SIZES[pad] + "_activate", pressed ? 1 : 0);
        break;
    case 0x60: // FX PADS (SHIFT + FX; V77, Marko 15.10.2026): pad 1-8 = the app's effect 1-8 on
        // this deck, SHIFT + pad 1-8 = effect 9-16. A tap switches it on (the same pad again:
        // off, another pad: that effect instead); a pad held down is on only while held.
        var number = pad + 1 + (shifted ? 8 : 0);
        if (pressed) {
            deck.fxPress = {number: number, time: Date.now()};
            // light it at once, as the app will (the same pad again: off); the app's
            // fx_active_<deck> takes over after 2 s
            var activeNow = engine.getValue("[DJMantra]", "fx_active_" + deck.number);
            deck.fxLit = activeNow === number ? 0 : number;
            deck.fxLitUntil = Date.now() + 2000;
            MixUltra.refreshPads(deck);
            MixUltra.tellApp("fx_pad", deck.number * 100 + number);
        } else if (deck.fxPress && deck.fxPress.number === number) {
            if (Date.now() - deck.fxPress.time >= MixUltra.FX_HOLD_MS) {
                deck.fxLit = 0;
                deck.fxLitUntil = Date.now() + 2000;
                MixUltra.refreshPads(deck);
                MixUltra.tellApp("fx_pad_off", deck.number * 100 + number);
            }
            deck.fxPress = null;
        }
        break;
    case 0x70: // SAMPLER: play sampler 1-8 from its start; SHIFT: stop
        if (pressed) {
            var sampler = "[Sampler" + (pad + 1) + "]";
            engine.setValue(sampler, shifted ? "stop" : "cue_gotoandplay", 1);
        }
        break;
    }
};

// Jump to beat `pad` of the current phrase, keeping the position within the beat
MixUltra.jumpToBeat = function(deck, pad) {
    if (deck.hasPhrase) {
        // Exact, in the engine (also on a paused deck)
        engine.setValue(deck.group, "phrase_jump", pad + 1);
        engine.setValue(deck.group, "phrase_jump", 0);
        return;
    }
    var current = deck.hasPhrase
        ? engine.getValue(deck.group, "beat_in_phrase")
        : deck.beatCount;
    if (current < 0) {
        return;
    }
    var beats = pad - Math.floor(current);
    if (beats !== 0) {
        engine.setValue(deck.group, "beatjump", beats);
    }
};

// The EQ knobs (V63, Marko 14.10.2026: "shift does opposite of what the knob is for ... if I
// am in the EQ mode, if I press shift and I'm turning, let's say, mids, suddenly I am in the
// [Neural Mix] mode ... temporarily overriding the actual function"). The screen's mode (EQ /
// NEURAL MIX, button_parameter4) picks the knobs' set: LOW / MID / HIGH = parameter1..3,
// DRUMS / VOCALS / HARMONIES = parameter4..6; with SHIFT held (the deck's SHIFT layer, B4 /
// B5) the other set. 14-bit: MSB 02..04, LSB 22..24.
// PICKUP, as Ableton Live's "Pickup" takeover (Marko 14.10.2026: "no knob, no fader jumps ... I
// need to pick it up"): a knob that has just changed target (SHIFT, the mode, or the value was
// moved on the screen / reset by a new song) leaves the value alone until the knob reaches it
// (within 2 %) or turns across it; from then on it follows the knob.
MixUltra.knobMsb = {};
MixUltra.knobState = {}; // per physical knob: {hw: last position, target: last target}
MixUltra.knobSet = {};   // per target: the value this script last set (else: not picked up)

// 14-bit position of a knob (MSB `n`, LSB `n + 0x20`), 0..1
MixUltra.knobPosition = function(knobId, control, value) {
    if (control < 0x20) {
        MixUltra.knobMsb[knobId] = value;
        return value / 127;
    }
    return ((MixUltra.knobMsb[knobId] || 0) * 128 + value) / 16383;
};

// The pickup for one physical knob turned to `hw` (0..1) on `target` (any id) whose value is
// `current`: true when the knob has the value (then the caller sets it), false while it has not
// picked it up yet. GHOST (V77, Marko 15.10.2026: "like in Ableton Live like a ghost fader until
// it picks up the real fader ... half transparent one shows the current position of the knob"):
// the app shows [DJMantra] `ghost` = the knob's position + 1 while it has not picked up, 0 once
// it has ("they are all one").
MixUltra.pickup = function(knobId, target, current, hw, ghost) {
    var knob = MixUltra.knobState[knobId] || (MixUltra.knobState[knobId] = {hw: undefined, target: ""});
    var set = MixUltra.knobSet[target];
    // picked up: this knob set it last and nothing else moved it since
    var picked = knob.target === target && set !== undefined && Math.abs(current - set) < 0.01;
    if (!picked) {
        var near = Math.abs(hw - current) < 0.02;
        // the knob passed over the value since its last message (whatever it set then)
        var crossed = knob.hw !== undefined && (knob.hw - current) * (hw - current) <= 0;
        picked = near || crossed;
    }
    knob.hw = hw;
    knob.target = target;
    MixUltra.knobSet[target] = picked ? hw : undefined;
    if (ghost) {
        engine.setValue("[DJMantra]", ghost, picked ? 0 : hw + 1);
    }
    return picked;
};

// The app's ghost codes per deck: ghost_<deck>_<code>, 1..6 = the EQ's parameter1..6,
// 7 = FILTER, 8 = the deck's effect DEPTH (SHIFT + FILTER)
MixUltra.ghostName = function(deckNumber, code) {
    return "ghost_" + deckNumber + "_" + code;
};

MixUltra.eq = function(channel, control, value, status, group) {
    var band = (control & 0x1F) - 1; // 1 low, 2 mid, 3 high
    var shifted = (status & 0x0F) >= 4;
    var stems = engine.getValue(group, "button_parameter4") > 0;
    var number = stems !== shifted ? band + 3 : band;
    var key = "parameter" + number;
    var deckNumber = group.indexOf("Channel2") >= 0 ? 2 : 1;
    var hw = MixUltra.knobPosition(group + band, control, value);
    if (MixUltra.pickup(group + band, group + key, engine.getParameter(group, key), hw,
        MixUltra.ghostName(deckNumber, number))) {
        engine.setParameter(group, key, hw);
    }
};

// FILTER (V77): with pickup like the EQs (the same knob also sets the effect's depth with
// SHIFT, so it must not jump back); SHIFT + FILTER (Marko 15.10.2026: "when effect is
// triggered, I can change its parameter by pressing Shift, turning the filter button ... I need
// to see visually pop up with the effect fader"): the depth of the deck's effect, through the
// app ([DJMantra] fx_depth_<deck> mirrors the effect's DEPTH, fx_edit shows the pop-up)
MixUltra.filter = function(channel, control, value, status, group) {
    var deck = MixUltra.deckOf(status);
    var shifted = (status & 0x0F) >= 4 || deck.shift;
    var knobId = "filter" + deck.number;
    var hw = MixUltra.knobPosition(knobId, control, value);
    if (shifted) {
        var depthKey = "fx_depth_" + deck.number;
        MixUltra.tellApp("fx_edit", deck.number);
        if (MixUltra.pickup(knobId, depthKey, engine.getValue("[DJMantra]", depthKey), hw,
            MixUltra.ghostName(deck.number, 8))) {
            engine.setValue("[DJMantra]", depthKey, hw);
        }
        return;
    }
    var quick = "[QuickEffectRack1_" + deck.group + "]";
    if (MixUltra.pickup(knobId, quick + "super1", engine.getParameter(quick, "super1"), hw,
        MixUltra.ghostName(deck.number, 7))) {
        engine.setParameter(quick, "super1", hw);
    }
};

// Called by DJ Mantra on Android after every (re)connect over USB or Bluetooth:
// the controller's LEDs are dark again, the knob positions were requested already.
var djmantraControllerConnected = function() {
    MixUltra.quietUntil = Date.now() + 1500;
    midi.sendShortMsg(0x90, 0x01, 0x7F);
    MixUltra.refreshLeds();
};
