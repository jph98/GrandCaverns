# Graph Report - grandcaverns  (2026-10-17)

## Corpus Check
- 12 files · ~10,924 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 306 nodes · 478 edges · 17 communities (13 shown, 4 thin omitted)
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 38 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- GrandCavernsEngine
- GrandCavernsAudioProcessor
- PluginEditor.cpp
- Filters.h
- Biquad
- render_test.cpp
- GrandCavernsParams
- GrandCavernsAudioProcessorEditor
- GrandCaverns
- PluginProcessor.cpp
- applyPreset
- createLayout
- processBlock
- .changeProgramName
- build.sh
- getStateInformation
- createEditor

## God Nodes (most connected - your core abstractions)
1. `GrandCavernsEngine` - 82 edges
2. `GrandCavernsAudioProcessor` - 45 edges
3. `GrandCavernsAudioProcessorEditor` - 33 edges
4. `GrandCavernsParams` - 23 edges
5. `Biquad` - 14 edges
6. `DelayLine` - 12 edges
7. `process` - 12 edges
8. `renderDemos()` - 11 edges
9. `CavernDisplay` - 10 edges
10. `prepare()` - 10 edges

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

## Communities (17 total, 4 thin omitted)

### Community 0 - "GrandCavernsEngine"
Cohesion: 0.03
Nodes (62): GrandCavernsEngine, bloomApL, bloomApR, bloomDampL, bloomDampR, bloomFbL, bloomFbR, bloomL (+54 more)

### Community 1 - "GrandCavernsAudioProcessor"
Cohesion: 0.09
Nodes (17): GrandCavernsAudioProcessor, apvts, bloomLevel, currentProgram, engine, fedInput, floatScratch, lastReportedLatency (+9 more)

### Community 2 - "PluginEditor.cpp"
Cohesion: 0.09
Nodes (29): Button, Colour, Component, Graphics, Slider, CavernDisplay, bloom, mode (+21 more)

### Community 3 - "Filters.h"
Cohesion: 0.10
Nodes (22): DCBlock, x1, y1, grandCavernsHadamard8(), loopShape(), OnePole, z, saturateDrive() (+14 more)

### Community 4 - "Biquad"
Cohesion: 0.09
Nodes (16): Allpass, line, Biquad, a1, a2, b0, b1, b2 (+8 more)

### Community 5 - "render_test.cpp"
Cohesion: 0.20
Nodes (24): mt19937, path, cavernModeName(), allFinite(), string, vector, expect(), impulse() (+16 more)

### Community 6 - "GrandCavernsParams"
Cohesion: 0.08
Nodes (24): array, cavernModeBlurb(), GrandCavernsParams, bloom, bloomFeedback, delayMs, density, drive (+16 more)

### Community 7 - "GrandCavernsAudioProcessorEditor"
Cohesion: 0.08
Nodes (29): ComboBox, Label, LookAndFeel, GrandCavernsAudioProcessorEditor, blurb, blurbArea, display, GrandCavernsAudioProcessorEditor::GrandCavernsAudioProcessorEditor() (+21 more)

### Community 8 - "GrandCaverns"
Cohesion: 0.40
Nodes (4): Build, GrandCaverns, Modes, Signal path

### Community 9 - "PluginProcessor.cpp"
Cohesion: 0.12
Nodes (10): BusesLayout, AudioProcessor, getCurrentProgram, isBusesLayoutSupported, isTestSoundPlaying, releaseResources, reset, setStateInformation (+2 more)

### Community 10 - "applyPreset"
Cohesion: 0.29
Nodes (7): assignParam(), AudioProcessorValueTreeState, String, applyPreset, getNumPrograms, getProgramName, setCurrentProgram

### Community 11 - "createLayout"
Cohesion: 0.67
Nodes (4): AudioParameterFloatAttributes, ParameterLayout, createLayout, percentAttrs()

### Community 12 - "processBlock"
Cohesion: 0.40
Nodes (6): MidiBuffer, AudioBuffer, prepareToPlay, processBlock, readParams, renderFloat

## Knowledge Gaps
- **130 isolated node(s):** `buf`, `write`, `size`, `line`, `z` (+125 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **4 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `GrandCavernsEngine` connect `GrandCavernsEngine` to `GrandCavernsAudioProcessor`, `Filters.h`, `Biquad`, `render_test.cpp`, `GrandCavernsParams`?**
  _High betweenness centrality (0.542) - this node is a cross-community bridge._
- **Why does `GrandCavernsAudioProcessor` connect `GrandCavernsAudioProcessor` to `GrandCavernsEngine`, `GrandCavernsAudioProcessorEditor`, `PluginProcessor.cpp`, `applyPreset`, `createLayout`, `processBlock`, `.changeProgramName`, `getStateInformation`, `createEditor`?**
  _High betweenness centrality (0.284) - this node is a cross-community bridge._
- **Why does `GrandCavernsAudioProcessorEditor` connect `GrandCavernsAudioProcessorEditor` to `PluginProcessor.cpp`, `PluginEditor.cpp`, `GrandCavernsParams`?**
  _High betweenness centrality (0.161) - this node is a cross-community bridge._
- **What connects `buf`, `write`, `size` to the rest of the system?**
  _130 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `GrandCavernsEngine` be split into smaller, more focused modules?**
  _Cohesion score 0.03076923076923077 - nodes in this community are weakly interconnected._
- **Should `GrandCavernsAudioProcessor` be split into smaller, more focused modules?**
  _Cohesion score 0.09090909090909091 - nodes in this community are weakly interconnected._
- **Should `PluginEditor.cpp` be split into smaller, more focused modules?**
  _Cohesion score 0.09462365591397849 - nodes in this community are weakly interconnected._