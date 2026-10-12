# Round 16 on the phone — 12.10.2026, build 88c6160 (Android run 38207384753)

sha256 `aae98b640bee0655362c2a1bc6f7eda955ee572618ccefe58e7789c514bcb32a`, installed 07:45 CEST. New interface (`--qml`).

## BLOCKER: the new interface does not load (black screen)

`--qml --play Addis Dune`: the decks play and sections are found, but the screen stays black (`m1-mix-hole.png`):
no "Quick: interface ready", no map lines. `logcat-quick-error.txt`:

```
W DJMantra: warning [Main] QQmlApplicationEngine failed to load component
W DJMantra: warning [Main] file:///…/res/qml/djmantra/Main.qml:93:13: Type KeyBadge unavailable
W DJMantra: warning [Main] file:///…/res/qml/djmantra/KeyBadge.qml: Type Style unavailable
W DJMantra: warning [Main] file:///…/res/qml/djmantra/Style.qml:15:29: Cannot assign a value to a signal (expecting a script to be run)
E DJMantra: critical [Main] Failed to load QML file "/data/user/0/com.djmantra.app/files/res/qml/djmantra/Main.qml"
```

**Cause:** `res/qml/djmantra/Style.qml:15` `readonly property color onAccent: "#1A1A1A"`. In QML a name `on` + capital
letter is a **signal handler** (`onAccent` = "when `accent` changes"), so declaring a property with that name is
an error. Style.qml then fails, every file that uses `Style` fails, and so does Main.qml.
**Fix:** rename it (e.g. `inkOnAccent`) in Style.qml and its 4 users: `BeatPads.qml:90`, `DButton.qml:45`, `:55`,
`Main.qml:79`, `PhraseBar.qml:84`.
**Prevention:** CI built green because QML is checked only when it runs. Add `qmllint` (or a headless
`QQmlApplicationEngine` load of Main.qml) to the Linux CI job. Then a QML error fails the build, not the phone round.

Sections already work in the engine (from the same start):
`Sections: [Channel1] 3 changes, at 32.2 s, 160.2 s, 192.2 s` (Addis), `[Channel2] 5 changes, at 67.3 s, 83.8 s,
133.5 s, 150.0 s, 191.4 s` (Dune).

Nothing else tested: everything in ROUND16 is in the new interface. Round stopped here (manifest
`testing-on-a-budget.md`: stop at a blocker).
