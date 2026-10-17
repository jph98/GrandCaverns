# GrandCaverns

A stereo reverb plugin (VST3, Audio Unit, and standalone) that mixes two ideas:

- a **bloom**: a rising cloud of diffused taps, with feedback around that section, so the room opens after the note
- a **cavern**: an eight-line feedback delay network whose **Warp** control spreads equal echoes out into a reverb, and whose **Density** control crossfades that from separate repeats into a Hadamard mix

Loveless mode holds the dry note so the cloud can swell in front of it. That is the realtime shape of a reverse-reverb print, the kind of wall associated with My Bloody Valentine, plus glide and a little grit on the way in.

This is an original algorithm. It follows published descriptions of how those effects behave — a blooming diffusion section into a tank, and a warped feedback-delay network with a density control — and it does not contain code or presets from Strymon or Valhalla DSP.

References for the target sound:

- [Valhalla Supermassive](https://valhalladsp.com/shop/reverb/valhalla-supermassive/) and [its controls](https://valhalladsp.com/2020/05/06/valhallasupermassive-the-controls/)
- [Strymon BigSky manual](https://www.strymon.net/manuals/BigSky_UserManual_RevD.pdf) (Bloom: a diffusion section that feeds a tank, with feedback around the bloom)
- Bloom, Supermassive, and Loveless-style examples: [BigSky](https://www.youtube.com/watch?v=KclJxqde8NQ), [Supermassive](https://www.youtube.com/watch?v=lEw_YILyukg), [My Bloody Valentine](https://www.youtube.com/watch?v=9WkOBREQ9cQ)

## Signal path

```
dry ──────────────────────────────────────────── mix
input → drive → pre-delay → rising bloom taps → tone → width → mix
                     ↑ feedback          ↓
                     reverse (Loveless)  8-line cavern (warp, density, mod)
```

Bloom sets both the length of that cloud and how much of it, versus the raw input, excites the cavern. In Loveless the dry signal waits for the loudest tap, and the plugin reports that wait as latency so the host can compensate.

## Modes

| Mode | What it does |
| --- | --- |
| Cavern | The blend. Cloud into a warped room. |
| Bloom | The tank is fed mostly by the cloud, so high mix still leaves the pick some room. |
| Massive | Long delays. Low warp is echoes; high warp and density become the wash. |
| Inertia | The cloud is late. Higher Feedback brings it in sooner and lets it hang. |
| Loveless | Reverse swell, delayed dry note, glide, grit. |

The Preset menu at the top loads a whole sound, including which algorithm is selected. The built-in starting points are Wide Blend, Late Room, Long Smear, Slow Wake, Reverse Wall, Soft Focus, Glide Echo, and Full Glide. Save stores the current controls on this computer. Import and Export read and write `.grandcaverns` files, and saved presets are kept in `~/Library/Application Support/GrandCaverns/`. Editing a control clears the menu to Custom until the sound is saved or matches a preset again.

Test Sound plays a long pad through the effect, so the cavern can open without an external input.

## Build

Needs CMake 3.22+, a C++20 compiler, and Xcode on macOS. The first configure clones JUCE 8.0.15.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The plugin is copied to:

- `~/Library/Audio/Plug-Ins/VST3/GrandCaverns.vst3`
- `~/Library/Audio/Plug-Ins/Components/GrandCaverns.component`

The standalone app and a copy of the plugins also stay in `build/GrandCaverns_artefacts`. Rescan plug-ins in the DAW after the first install.

VST here means VST3. The old VST2 format is not included.

`./build/GrandCavernsRenderTest` renders a guitar-like phrase through every preset to `tests/out/*.wav` and checks that the bloom rises, the Loveless dry note waits, and the tails stay finite and decay. The wavs are peak-normalized for listening; the test prints the raw peak before that.

JUCE is licensed separately from this code. Building links JUCE, and distributing a build means complying with the [JUCE licence](https://juce.com/legal/juce-licence/) you use (personal, commercial, or AGPL).
