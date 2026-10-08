# DJ Mantra

An Android DJ app built on the open-source **[Mixxx](https://github.com/mixxxdj/mixxx)** engine,
designed for phones and the **Hercules DJControl Mix Ultra** controller.

> Status: **early port, not usable yet.** The engine source is imported and the desktop build is
> the baseline. See [docs/ANDROID_PORT.md](docs/ANDROID_PORT.md) for the roadmap and progress.

## Goals

- Mixxx's audio engine (decks, sync, time-stretch, effects, library) running on Android
- Touch UI with the familiar mobile DJ layout: two decks, waveforms, central mixer, pads
- Hercules DJControl Mix Ultra over **Bluetooth LE MIDI** (and USB)
- **MIDI learn**: map any controller button or knob to any function
- Two audio outputs at once (for example, master on Bluetooth and cue on USB headphones),
  with per-output delay so cueing stays in sync with the speaker

## Building (desktop, current baseline)

```bash
tools/debian_buildenv.sh setup     # Ubuntu/Debian build dependencies
cmake -S . -B build -G Ninja -DQT6=ON -DQML=ON
cmake --build build
(cd build && QT_QPA_PLATFORM=offscreen ctest)
```

The original Mixxx README is kept in [README.mixxx.md](README.mixxx.md).

## License

DJ Mantra is a fork of Mixxx and is licensed under the **GNU GPL v2 or later**, the same as
Mixxx. See [LICENSE](LICENSE) and [COPYING](COPYING). Source for every build must stay public.

The upstream base is Mixxx **2.5.6** (`94ccc34f91d37dec62d0d70c16fac38dc8af677f`).
