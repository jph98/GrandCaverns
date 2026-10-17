#pragma once

#include "PluginProcessor.h"
#include "PresetLibrary.h"

#include <array>
#include <memory>

class CavernDisplay : public juce::Component
{
public:
    void setState (float newWet, float newBloom, CavernMode newMode);
    void paint (juce::Graphics& g) override;

private:
    float wet = 0.0f;
    float bloom = 0.0f;
    float phase = 0.0f;
    CavernMode mode = CavernMode::Cavern;
};

class GrandCavernsAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit GrandCavernsAudioProcessorEditor (GrandCavernsAudioProcessor&);
    ~GrandCavernsAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    };

    void timerCallback() override;
    void layoutKnob (Knob& knob, juce::Rectangle<int> bounds);
    void rebuildPresetMenu();
    void syncPresetMenu();
    void syncAlgorithmControls();
    void loadPresetById (int presetId);
    void promptSavePreset();
    void commitSavePreset (const juce::String& name);
    void confirmDeletePreset();
    void deleteSelectedPreset();
    void launchImport();
    void finishImport (const juce::File& file);
    void launchExport();
    void finishExport (const juce::File& file);
    void showPresetMessage (const juce::String& title, const juce::String& message);
    int presetIdForParams (const GrandCavernsParams& params) const;
    GrandCavernsAudioProcessor& plugin();

    LookAndFeel lookAndFeel;
    CavernDisplay display;
    std::array<juce::TextButton, 5> modeButtons;
    juce::TextButton testButton;
    juce::TextButton saveButton;
    juce::TextButton deleteButton;
    juce::TextButton importButton;
    juce::TextButton exportButton;
    std::array<Knob, 15> knobs;
    juce::Label presetLabel;
    juce::ComboBox presets;
    PresetLibrary presetLibrary;
    std::shared_ptr<juce::FileChooser> fileChooser;
    juce::String blurb;
    juce::Rectangle<int> blurbArea;
    bool ignorePresetChange = false;
    int selectedPresetId = 0;
    juce::TooltipWindow tooltips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrandCavernsAudioProcessorEditor)
};
