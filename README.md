# SOLARIUM-23A

A microtonal drone and ambient machine as a plugin: a **VST3** and a **standalone app** for Windows, and **VST3, AU**
and the **app** for the Mac (Apple Silicon and Intel in one file). By **swtdr34mz**. (A version for the M-Vave FM-1 is its own project.)

![SOLARIUM-23A](docs/panel.png)

Eight voices into a stereo mixer, a dual 12 dB filter, a stereo distortion and a dual effector:

- **DRONE 1, 2, 4, 5 (CLASSIC):** five free-running saw oscillators each (1–2 low, 3 middle, 4–5 high), each with
  its own TUNE (no semitone grid: tune chords, clusters, beatings), on/off and PAN (its place in the stereo field).
  TONE darkens the drone, DRIFT makes its oscillators wander (an old negistor's unsteadiness, up to wobbly), SWIRL
  carries them round the stereo field with ORBIT (all up: the whole circle, each a fifth of a turn apart). VOLT transposes all five down; past the middle they frequency-modulate each other. ATTACK and RELEASE, opened by
  the drone KEY while it is held, or by HOLD (latched).
- **DRONE 3, 6 (CHAOS):** a square modulator (RATE, x1/x10) and a Schmitt-trigger tone (PITCH, LOW/HIGH), FM and/or
  AM, a DIVIDER, NOISE into the tone, a S&H of the noise: sirens, birds, surf, wind. KEY and HOLD as above.
- **VCO A, B:** morphing waveform (sine – triangle – saw – ramp – square – pulse), PW, TUNE (an octave), OCTAVE, SUB,
  an ADSR each (HOLD: always open; SELF-GEN: the envelope loops like an LFO), A synced to B, B's FM from A. Played by
  the touch plates and MIDI notes (both VCOs the same note), GLIDE, BEND.
- **Modulation:** LFO A and B (square to triangle, x1/x6/x10), the joystick (X, Y), a 5-step sequencer (GATE per
  step, 3/4/5 STAGES, its own PULSER or the host's tempo, VCO PLAY plays the VCOs). Each source has a DEST: either filter, both, both opposed, the VCOs'
  pitch, PW, wave or FM, the chaos voices, the drones' pitch, the effector's X, Y, Z or BLEND.
- **Filter:** two 12 dB filters (Polivoks-style: the integrators saturate, the lows stay with the resonance), LP or
  BP each, LINK (the left's modulation moves both). **Distortion:** DIST (its share) and GAIN.
- **Dual effector:** 8 cartridges of 3 programs, one program on the left, one on the right; X, Y, Z and BLEND
  shared, as on the instrument:

  | cartridge | 1 | 2 | 3 |
  |---|---|---|---|
  | NEBULA | SHIMMER | CLOUD | HALL |
  | ORBIT | ECHOVERB | TAPE | PITCHDLY |
  | PULSAR | TREMOLO | CHORUS | PHASER |
  | COMET | AUTOWAH | RESONATR | FORMANT |
  | GRAVITY | OCTAVER | HARMONZR | FREEZE |
  | VOID | BLACKHOL | GRAINS | SPACEDLY |
  | AURORA | RINGMOD | S&H RING | FOLD |
  | DUST | CRUSH | VINYL | RADIO |

- **Touch plates, tuned:** EDIT (beside the plates) on, click a plate: it is selected and keeps sounding; turn NOTE
  to tune it up to 12 semitones up or down, freely (microtonal) or in semitones (TUNE GRID SEMI). Each plate's note is
  printed under it; the tunings are kept with the project and your own presets. MIDI keys stay in equal temperament.
- **TUNE GRID:** FREE (microtonal, as the instrument) or SEMI (every drone oscillator on the nearest semitone).
- **16 programs**, and your own (SAVE: files in `Documents/SOLARIUM-23A/Presets`).

Unofficial: inspired by the analogue drone machines of the 2020s; not made, endorsed or supported by any of their
makers, and no part of their designs, names or panels.

## Download

[Actions](https://github.com/swtdr34mz-afk/SOLARIS-23/actions) → the newest green run → **Artifacts** at the bottom:
`SOLARIUM-23A-Windows` (VST3 + app) and `SOLARIUM-23A-macOS` (VST3, AU + app); a GitHub sign-in is needed for artifacts.
A release (a tag `v1.2` and so on) carries the same zips under [Releases](https://github.com/swtdr34mz-afk/SOLARIS-23/releases).

## Installieren (kurz auf Deutsch)

**Umbenannt:** Bis v1.2 hieß das Plugin **SOLARIS-23**. Beim Update bitte die alte `SOLARIS-23.vst3` (auf dem Mac auch
`SOLARIS-23.component`) löschen, sonst steht das Plugin zweimal in der Liste. Alte Projekte laden weiter (die interne
Kennung ist gleich geblieben), und eigene Presets aus `Documents/SOLARIS-23/Presets` werden beim ersten Start in den
neuen Ordner kopiert.

**Windows (Cubase):**
1. Zip entpacken.
2. Den Ordner `SOLARIUM-23A.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren.
3. Cubase starten (oder *Studio → VST-PlugIn-Manager → Aktualisieren*). SOLARIUM-23A steht unter den Instrumenten,
   Hersteller swtdr34mz.
4. `SOLARIUM-23A.exe` ist die Standalone-App. Audio- und MIDI-Gerät stellt man unter *Options* ein.

Windows SmartScreen kann beim ersten Start der App warnen (sie ist nicht signiert): *Weitere Informationen →
Trotzdem ausführen*.

**Mac (Ableton Live, Logic, Cubase …; macOS 10.13 oder neuer, Apple Silicon und Intel):**
1. `SOLARIUM-23A-macOS.zip` per Doppelklick entpacken.
2. Im Finder *Gehe zu → Gehe zum Ordner …* (⇧⌘G) und `~/Library/Audio/Plug-Ins/` eingeben.
3. `SOLARIUM-23A.vst3` in den Ordner `VST3` kopieren, `SOLARIUM-23A.component` in den Ordner `Components`
   (Logic und GarageBand nehmen nur das AU, Ableton und Cubase das VST3 oder das AU).
4. Weil das Plugin nicht bei Apple beglaubigt ist, blockiert macOS es sonst: *Terminal* öffnen und eingeben
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/SOLARIUM-23A.vst3 ~/Library/Audio/Plug-Ins/Components/SOLARIUM-23A.component
   ```
   (für die App genauso: `xattr -cr /Pfad/zu/SOLARIUM-23A.app`, oder Rechtsklick → *Öffnen*).
5. Ableton Live: *Einstellungen → Plug-Ins →* „VST3-Plug-In-Systemordner verwenden“ und „Audio Units“ an,
   dann *Neu scannen*. SOLARIUM-23A steht unter *Plug-Ins → swtdr34mz*.
   Logic: beim nächsten Start prüft Logic das AU von selbst (unter *Instrumente → AU-Instrumente → swtdr34mz*).

## In the plugin

- **Every parameter** (212) is a host parameter: automatable, saved with the project. The order and the ids are
  permanent (`dsp/params_list.inc`, made by `tools/gen_params.py`; new ones only at the end).
- **MIDI in:** notes (the VCOs), velocity, pitch bend (BEND), CC 1 (more of the joystick's Y destination),
  CC 120/123. The touch plates on the panel play the same; their arrows choose the octave.
- **ORBIT:** what turns the drones' SWIRL: its own RATE (one turn in 100 s up to 0.4 s), LFO A or B, or the
  sequencer; WIDTH sets how wide the drones spread (0: mono, up to 150%).
- **The mod wheel** moves the joystick's Y (its DEST); the wheel and pitch bend glide, so nothing crackles.
- **The host's tempo** clocks the sequencer when its CLOCK is a note value; while the host plays, it locks to its
  position.
- **The panel:** one powder-coated metal panel, black print, plastic caps, metal toggles, the drone keys, the orbit
  rings and a black touch field, after the drone machines it is inspired by. Click the name (top left) for the
  panel's finish: ORANGE, YELLOW, WHITE, BLUE, PINK (kept with the project). Drag a knob (SHIFT: fine), double-click for its default. Right-click any knob, switch, the joystick (left half X, right half Y), a cartridge or a touch plate for the host's menu of that parameter (MIDI learn, automation …); a right click never changes a value (in the standalone app a choice's right click still opens its list). Click a
  cartridge for all 24 programs, its lamps 1-2-3 for the cartridge's programs. The joystick: drag, double-click to
  centre. The window's corner sizes it (kept with the project).

## The engine

`dsp/` (C++17, no dependencies): `engine.cpp` (the voices, the modulation, the mixer, the filter, the distortion),
`fx.cpp` (the 24 effect programs), `params.cpp`, `presets.cpp`. It runs on a grid of 16 samples, every time in
seconds, so it sounds the same at any sample rate and any buffer size. `tests/engine_tests.cpp` checks every program
at three rates, every buffer size bit for bit, every parameter swept, every effect at its ends, all at once at full,
and the CPU (about 2 % of a core per instance on a desktop).

## Build

CMake 3.22 and a C++17 compiler (Visual Studio 2022, Xcode). JUCE 8.0.9 is fetched by CMake.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The engine's tests alone (no JUCE): `cmake -S tests -B build-tests && cmake --build build-tests && build-tests/engine_tests`
(`engine_tests bench` for the CPU, `engine_tests render <program> <seconds> <out.wav>` to listen).

A picture of the panel without a host: `tools/panel/` (`s23_panel docs/panel.png`).

GitHub Actions (`.github/workflows/build.yml`) runs the engine's tests, builds Windows and checks the VST3 with
pluginval, and builds the Mac (universal) and checks the AU with auval.

## Licence

The SOLARIUM-23A code is under the [Mozilla Public License 2.0](LICENSE). The plugin is built with
[JUCE](https://juce.com) 8, used under the AGPLv3 (JUCE's open-source licence), so the plugin binaries are distributed
under the AGPLv3 with this source. The fonts are Fira Sans and Anonymous Pro (SIL OFL, `fonts/`). VST is a trademark of
Steinberg Media Technologies GmbH.
