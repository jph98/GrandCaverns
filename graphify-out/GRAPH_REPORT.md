# Graph Report - grandcaverns  (2026-10-17)

## Corpus Check
- 14 files · ~13,101 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 359 nodes · 622 edges · 20 communities (16 shown, 4 thin omitted)
- Extraction: 89% EXTRACTED · 11% INFERRED · 0% AMBIGUOUS · INFERRED: 69 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- GrandCavernsEngine
- GrandCavernsAudioProcessor
- CavernDisplay
- Filters.h
- Biquad
- render_test.cpp
- GrandCavernsParams
- GrandCavernsAudioProcessorEditor
- GrandCaverns
- PluginProcessor.cpp
- applyParams
- createLayout
- readParams
- .changeProgramName
- build.sh
- getStateInformation
- createEditor
- PresetLibrary.cpp
- KnobSpec
- getNumPrograms

## God Nodes (most connected - your core abstractions)
1. `GrandCavernsEngine` - 82 edges
2. `GrandCavernsAudioProcessorEditor` - 53 edges
3. `GrandCavernsAudioProcessor` - 47 edges
4. `GrandCavernsParams` - 28 edges
5. `Biquad` - 14 edges
6. `NamedPreset` - 13 edges
7. `PresetLibrary` - 13 edges
8. `DelayLine` - 12 edges
9. `process` - 12 edges
10. `GrandCavernsAudioProcessorEditor::GrandCavernsAudioProcessorEditor()` - 12 edges

## Surprising Connections (you probably didn't know these)
- `testFilters()` --calls--> `grandCavernsHadamard8()`  [INFERRED]
  tests/render_test.cpp → Source/DSP/Filters.h
- `testLovelessLead()` --calls--> `lovelessLeadSeconds()`  [INFERRED]
  tests/render_test.cpp → Source/DSP/GrandCavernsEngine.h
- `prepare()` --calls--> `setTargets`  [INFERRED]
  tests/render_test.cpp → Source/DSP/GrandCavernsEngine.h
- `prepare()` --calls--> `snapToTargets`  [INFERRED]
  tests/render_test.cpp → Source/DSP/GrandCavernsEngine.h
- `render()` --calls--> `process`  [INFERRED]
  tests/render_test.cpp → Source/DSP/GrandCavernsEngine.h

## Import Cycles
- None detected.

## Communities (20 total, 4 thin omitted)

### Community 0 - "GrandCavernsEngine"
Cohesion: 0.03
Nodes (62): GrandCavernsEngine, bloomApL, bloomApR, bloomDampL, bloomDampR, bloomFbL, bloomFbR, bloomL (+54 more)

### Community 1 - "GrandCavernsAudioProcessor"
Cohesion: 0.09
Nodes (17): GrandCavernsAudioProcessor, apvts, bloomLevel, currentProgram, engine, fedInput, floatScratch, lastReportedLatency (+9 more)

### Community 2 - "CavernDisplay"
Cohesion: 0.12
Nodes (21): Button, Colour, Component, Graphics, Slider, CavernDisplay, bloom, mode (+13 more)

### Community 3 - "Filters.h"
Cohesion: 0.10
Nodes (22): DCBlock, x1, y1, grandCavernsHadamard8(), loopShape(), OnePole, z, saturateDrive() (+14 more)

### Community 4 - "Biquad"
Cohesion: 0.09
Nodes (14): Allpass, line, Biquad, a1, a2, b0, b1, b2 (+6 more)

### Community 5 - "render_test.cpp"
Cohesion: 0.20
Nodes (25): mt19937, path, cavernModeName(), allFinite(), string, vector, expect(), impulse() (+17 more)

### Community 6 - "GrandCavernsParams"
Cohesion: 0.09
Nodes (23): cavernModeBlurb(), GrandCavernsParams, bloom, bloomFeedback, delayMs, density, drive, feedback (+15 more)

### Community 7 - "GrandCavernsAudioProcessorEditor"
Cohesion: 0.06
Nodes (57): array, ComboBox, FileChooser, Label, LookAndFeel, shared_ptr, grandCavernsParamsMatch(), File (+49 more)

### Community 8 - "GrandCaverns"
Cohesion: 0.40
Nodes (4): Build, GrandCaverns, Modes, Signal path

### Community 9 - "PluginProcessor.cpp"
Cohesion: 0.13
Nodes (9): BusesLayout, AudioProcessor, isBusesLayoutSupported, isTestSoundPlaying, releaseResources, reset, setStateInformation, triggerTestSound (+1 more)

### Community 10 - "applyParams"
Cohesion: 0.40
Nodes (5): assignParam(), AudioProcessorValueTreeState, applyParams, applyPreset, setCurrentProgram

### Community 11 - "createLayout"
Cohesion: 0.67
Nodes (4): AudioParameterFloatAttributes, ParameterLayout, createLayout, percentAttrs()

### Community 12 - "readParams"
Cohesion: 0.33
Nodes (7): MidiBuffer, AudioBuffer, getCurrentParams, prepareToPlay, processBlock, readParams, renderFloat

### Community 17 - "PresetLibrary.cpp"
Cohesion: 0.15
Nodes (27): vector, clampRange(), File, String, vector, String, vector, isFactoryName() (+19 more)

### Community 18 - "KnobSpec"
Cohesion: 0.50
Nodes (4): KnobSpec, label, paramID, tip

### Community 19 - "getNumPrograms"
Cohesion: 0.50
Nodes (4): String, getCurrentProgram, getNumPrograms, getProgramName

## Knowledge Gaps
- **138 isolated node(s):** `buf`, `write`, `size`, `line`, `z` (+133 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **4 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `GrandCavernsEngine` connect `GrandCavernsEngine` to `GrandCavernsAudioProcessor`, `Filters.h`, `Biquad`, `render_test.cpp`, `GrandCavernsParams`?**
  _High betweenness centrality (0.467) - this node is a cross-community bridge._
- **Why does `GrandCavernsAudioProcessor` connect `GrandCavernsAudioProcessor` to `GrandCavernsEngine`, `GrandCavernsAudioProcessorEditor`, `PluginProcessor.cpp`, `applyParams`, `createLayout`, `readParams`, `.changeProgramName`, `getStateInformation`, `createEditor`, `getNumPrograms`?**
  _High betweenness centrality (0.237) - this node is a cross-community bridge._
- **Why does `GrandCavernsParams` connect `GrandCavernsParams` to `GrandCavernsEngine`, `Filters.h`, `render_test.cpp`, `GrandCavernsAudioProcessorEditor`, `applyParams`, `readParams`, `PresetLibrary.cpp`?**
  _High betweenness centrality (0.212) - this node is a cross-community bridge._
- **What connects `buf`, `write`, `size` to the rest of the system?**
  _138 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `GrandCavernsEngine` be split into smaller, more focused modules?**
  _Cohesion score 0.03076923076923077 - nodes in this community are weakly interconnected._
- **Should `GrandCavernsAudioProcessor` be split into smaller, more focused modules?**
  _Cohesion score 0.09090909090909091 - nodes in this community are weakly interconnected._
- **Should `CavernDisplay` be split into smaller, more focused modules?**
  _Cohesion score 0.12380952380952381 - nodes in this community are weakly interconnected._