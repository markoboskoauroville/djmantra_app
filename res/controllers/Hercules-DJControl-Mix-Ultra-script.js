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
//              15 SLICER, 16 SAMPLER (same deck channel). The controller switches the pad mode
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
    pitchplay: 0x40, bounce: 0x50, slicer: 0x60, sampler: 0x70,
};
MixUltra.MODE_BUTTONS = {
    0x0F: "hotcue", 0x10: "loop", 0x11: "fx", 0x12: "neural",
    0x13: "pitchplay", 0x14: "bounce", 0x15: "slicer", 0x16: "sampler",
};
MixUltra.LOOP_SIZES = [0.25, 0.5, 1, 2, 4, 8, 16, 32];
MixUltra.ROLL_SIZES = [0.0625, 0.125, 0.25, 0.5, 1, 2, 4, 8];
MixUltra.PITCH_STEPS = [-3, -2, -1, 0, 1, 2, 3, 4];

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
    this.connections = [];
};

MixUltra.init = function(id, debugging) {
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
    if (!MixUltra.BEAT_LIGHTS) {
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
    for (var pad = 0; pad < 8; pad++) {
        midi.sendShortMsg(deck.padStatus, MixUltra.padNote(deck, pad), MixUltra.padBaseValue(deck, pad));
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
        script.toggleControl(group, "play");
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

// SYNC: tap = beat sync once, hold = sync lock on/off
MixUltra.syncTimers = {};
MixUltra.sync = function(channel, control, value, status, group) {
    if (value > 0) {
        MixUltra.syncTimers[group] = engine.beginTimer(400, function() {
            MixUltra.syncTimers[group] = 0;
            script.toggleControl(group, "sync_enabled");
        }, true);
    } else if (MixUltra.syncTimers[group]) {
        engine.stopTimer(MixUltra.syncTimers[group]);
        MixUltra.syncTimers[group] = 0;
        if (engine.getValue(group, "sync_enabled") > 0) {
            engine.setValue(group, "sync_enabled", 0);
        } else {
            engine.setValue(group, "beatsync", 1);
        }
    }
};

MixUltra.shiftSync = function(channel, control, value, status, group) {
    if (value > 0) {
        script.toggleControl(group, "keylock");
    }
};

MixUltra.load = function(channel, control, value, status, group) {
    if (value > 0) {
        engine.setValue(group, "LoadSelectedTrack", 1);
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

MixUltra.modeButton = function(channel, control, value, status, group) {
    if (value === 0) {
        return;
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

MixUltra.browse = function(channel, control, value, status, group) {
    engine.setValue("[Library]", "MoveVertical", MixUltra.ticks(value));
};

MixUltra.shiftBrowse = function(channel, control, value, status, group) {
    // SHIFT + turn: move in the sidebar (Folders, playlists, ...)
    engine.setValue("[Library]", "MoveFocus", MixUltra.ticks(value) > 0 ? 1 : -1);
};

MixUltra.browsePress = function(channel, control, value, status, group) {
    if (value > 0) {
        engine.setValue("[Library]", "GoToItem", 1);
    }
};

MixUltra.shiftBrowsePress = function(channel, control, value, status, group) {
    if (value > 0) {
        engine.setValue("[Library]", "MoveFocusForward", 1);
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
    case 0x30: // NEURAL MIX: no stems in Mixxx 2.5: EQ kills low/mid/high, filter reset
        if (pressed) {
            var eq = "[EqualizerRack1_" + g + "_Effect1]";
            if (pad < 3) {
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
    case 0x60: // SLICER: jump to beat 1-8 of the phrase (the beat lights show where you are);
        // SHIFT + pad 1: "1 is here" (the beat closest to the play position)
        if (pressed) {
            if (shifted && pad === 0) {
                engine.setValue(g, "phrase_set_one", 1);
                engine.setValue(g, "phrase_set_one", 0);
            } else {
                MixUltra.jumpToBeat(deck, pad);
            }
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

// Called by DJ Mantra on Android after every (re)connect over USB or Bluetooth:
// the controller's LEDs are dark again, the knob positions were requested already.
var djmantraControllerConnected = function() {
    midi.sendShortMsg(0x90, 0x01, 0x7F);
    MixUltra.refreshLeds();
};
