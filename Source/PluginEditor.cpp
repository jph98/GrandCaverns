#include "PluginEditor.h"
#include "GrandCavernsPresets.h"
#include "ParameterIDs.h"

namespace
{
constexpr juce::uint32 kInk = 0xff0c1016;
constexpr juce::uint32 kPanel = 0xff141b24;
constexpr juce::uint32 kLine = 0xff2c3848;
constexpr juce::uint32 kAmber = 0xffe2a15a;
constexpr juce::uint32 kCream = 0xfff3ecdf;
constexpr juce::uint32 kDim = 0xff9aa6b4;

juce::Colour modeColour (CavernMode mode)
{
    switch (mode)
    {
        case CavernMode::Bloom: return juce::Colour (0xffe2a15a);
        case CavernMode::Massive: return juce::Colour (0xff3dceb4);
        case CavernMode::Inertia: return juce::Colour (0xffa78bfa);
        case CavernMode::Loveless: return juce::Colour (0xffe07a8a);
        case CavernMode::Cavern:
        case CavernMode::Count: break;
    }

    return juce::Colour (0xff7eb6c9);
}

struct KnobSpec
{
    const char* paramID;
    const char* label;
    const char* tip;
};

const KnobSpec kKnobs[] = {
    { id::mix, "MIX", "Dry versus the cavern. Halfway is already a lot of room." },
    { id::delay, "DELAY", "Longest delay in the cavern. Past a second is where it stops sounding like a room." },
    { id::warp, "WARP", "Zero stacks every delay into echoes. Higher values spread them into a reverb." },
    { id::feedback, "FEEDBACK", "How long the cavern hangs. On Inertia it also wakes the cloud up faster." },
    { id::density, "DENSITY", "Low is separate repeats. High mixes them into a smooth cloud." },
    { id::width, "WIDTH", "Stereo width of the wet cavern." },
    { id::bloom, "BLOOM", "How long the cloud takes to open. Also the blend toward that swell." },
    { id::bloomFeedback, "BLOOM FB", "Feedback around the blooming section. High values outlast the tank." },
    { id::modRate, "MOD RATE", "Speed of the motion inside the cloud and the cavern." },
    { id::modDepth, "MOD DEPTH", "How far those delays drift. High and slow is lush. High and fast is seasick." },
    { id::tone, "TONE", "A resonant filter on the wet path. Darker settings pick up a little voice." },
    { id::lowEnd, "LOW END", "Shelf on the cavern, from thin to heavy." },
    { id::predelay, "PRE-DELAY", "Gap before the wet signal. In Loveless this also pushes the dry note back." },
    { id::drive, "DRIVE", "Soft clipping on the way into the cavern, and a little grit left on the dry note." },
    { id::glide, "GLIDE", "Slow pitch drift through the whole room. The tremolo-arm wash." },
};
} // namespace

void CavernDisplay::setState (float newWet, float newBloom, CavernMode newMode)
{
    wet = newWet;
    bloom = newBloom;
    mode = newMode;
    phase += 0.012f;

    if (phase > 1.0f)
        phase -= 1.0f;
}

void CavernDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colour (0xff0e141c));
    g.fillRoundedRectangle (bounds, 10.0f);

    const auto accent = modeColour (mode);
    const auto centre = bounds.getCentre();
    const float energy = std::clamp (wet * 2.4f, 0.0f, 1.0f);
    const float open = 0.35f + 0.65f * std::clamp (bloom, 0.0f, 1.0f);

    for (int i = 8; i >= 1; --i)
    {
        const float t = static_cast<float> (i) / 8.0f;
        const float w = bounds.getWidth() * (0.18f + 0.78f * t) * (0.75f + 0.25f * open);
        const float h = bounds.getHeight() * (0.10f + 0.72f * t);
        const float breathe = 1.0f + 0.015f * std::sin ((phase + t) * 6.283185f) * energy;
        juce::Rectangle<float> arch (0, 0, w * breathe, h * breathe);
        arch.setCentre (centre.x, centre.y + (1.0f - t) * 8.0f);
        const float alpha = (0.10f + 0.14f * energy) * (1.0f - 0.45f * (1.0f - t));
        g.setColour (accent.withAlpha (std::clamp (alpha, 0.0f, 0.9f)));
        g.drawRoundedRectangle (arch, h * 0.5f, 1.4f);
    }

    g.setColour (accent.withAlpha (0.18f + 0.45f * energy));
    g.fillEllipse (centre.x - 18.0f, centre.y - 10.0f, 36.0f, 22.0f);

    g.setColour (juce::Colour (kCream).withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText (cavernModeName (mode), bounds.reduced (16.0f, 12.0f).removeFromTop (18.0f), juce::Justification::centredLeft);

    g.setColour (juce::Colour (kDim));
    g.drawText ("WET", bounds.reduced (16.0f, 12.0f).removeFromBottom (16.0f), juce::Justification::centredLeft);

    auto meter = bounds.reduced (16.0f, 14.0f).removeFromBottom (16.0f).withTrimmedLeft (36.0f).withHeight (4.0f).translated (0.0f, -6.0f);
    g.setColour (juce::Colour (kLine));
    g.fillRoundedRectangle (meter, 2.0f);
    meter.setWidth (meter.getWidth() * energy);
    g.setColour (accent);
    g.fillRoundedRectangle (meter, 2.0f);
}

GrandCavernsAudioProcessorEditor::LookAndFeel::LookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (kCream));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1a212c));
    setColour (juce::ComboBox::textColourId, juce::Colour (kCream));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (kLine));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (kAmber));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141b24));
    setColour (juce::PopupMenu::textColourId, juce::Colour (kCream));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (kAmber));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff1a1208));
    setColour (juce::Label::textColourId, juce::Colour (kDim));
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff1a212c));
    setColour (juce::TooltipWindow::textColourId, juce::Colour (kCream));
    setColour (juce::TooltipWindow::outlineColourId, juce::Colour (kLine));
}

void GrandCavernsAudioProcessorEditor::LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                                                      float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                                                      juce::Slider&)
{
    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y), static_cast<float> (width), static_cast<float> (height))
                      .reduced (8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float arcRadius = radius - 3.0f;

    g.setColour (juce::Colour (0xff1a212c));
    g.fillEllipse (centre.x - radius * 0.62f, centre.y - radius * 0.62f, radius * 1.24f, radius * 1.24f);

    juce::Path background;
    background.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (kLine));
    g.strokePath (background, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (juce::Colour (kAmber));
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto tick = centre.getPointOnCircumference (arcRadius, angle);
    g.setColour (juce::Colour (kCream));
    g.fillEllipse (tick.x - 3.0f, tick.y - 3.0f, 6.0f, 6.0f);
}

void GrandCavernsAudioProcessorEditor::LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                                                          bool highlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    g.setColour (on ? juce::Colour (kAmber) : juce::Colour (0xff1a212c));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (on ? juce::Colour (0xfff0d7b0) : juce::Colour (kLine));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    if (highlighted && ! on)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.fillRoundedRectangle (bounds, 6.0f);
    }
}

void GrandCavernsAudioProcessorEditor::LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const auto colour = button.getToggleState() ? juce::Colour (0xff1a1208) : juce::Colour (kCream);
    g.setColour (button.isEnabled() ? colour : colour.withAlpha (0.35f));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

GrandCavernsAudioProcessor& GrandCavernsAudioProcessorEditor::plugin()
{
    return static_cast<GrandCavernsAudioProcessor&> (processor);
}

GrandCavernsAudioProcessorEditor::GrandCavernsAudioProcessorEditor (GrandCavernsAudioProcessor& p)
    : AudioProcessorEditor (&p),
      tooltips (this, 700)
{
    setLookAndFeel (&lookAndFeel);
    setSize (1040, 860);
    setResizable (false, false);

    addAndMakeVisible (display);

    for (int i = 0; i < 5; ++i)
    {
        modeButtons[static_cast<size_t> (i)].setButtonText (cavernModeName (static_cast<CavernMode> (i)));
        modeButtons[static_cast<size_t> (i)].setRadioGroupId (2201);
        modeButtons[static_cast<size_t> (i)].setClickingTogglesState (true);
        modeButtons[static_cast<size_t> (i)].onClick = [this, i]
        {
            if (! modeButtons[static_cast<size_t> (i)].getToggleState())
                return;

            if (auto* parameter = plugin().apvts.getParameter (id::mode))
            {
                parameter->beginChangeGesture();
                parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (i)));
                parameter->endChangeGesture();
            }
        };
        addAndMakeVisible (modeButtons[static_cast<size_t> (i)]);
    }

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        knob.label.setText (kKnobs[i].label, juce::dontSendNotification);
        knob.label.setJustificationType (juce::Justification::centred);
        knob.label.setInterceptsMouseClicks (false, false);
        knob.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 16);
        knob.slider.setTooltip (kKnobs[i].tip);
        knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            plugin().apvts, kKnobs[i].paramID, knob.slider);
        addAndMakeVisible (knob.label);
        addAndMakeVisible (knob.slider);
    }

    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setJustificationType (juce::Justification::centredLeft);
    presetLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (presetLabel);

    presets.setTextWhenNothingSelected ("Custom");
    presets.setTooltip ("A whole sound, including the algorithm. Saved presets are the ones you store.");
    presets.onChange = [this]
    {
        if (ignorePresetChange)
            return;

        loadPresetById (presets.getSelectedId());
    };
    addAndMakeVisible (presets);

    saveButton.setButtonText ("Save");
    saveButton.setTooltip ("Save the current controls on this computer.");
    saveButton.onClick = [this] { promptSavePreset(); };
    addAndMakeVisible (saveButton);

    deleteButton.setButtonText ("Delete");
    deleteButton.setTooltip ("Remove the selected saved preset from this computer.");
    deleteButton.onClick = [this] { confirmDeletePreset(); };
    addAndMakeVisible (deleteButton);

    importButton.setButtonText ("Import");
    importButton.setTooltip ("Add presets from a .grandcaverns file.");
    importButton.onClick = [this] { launchImport(); };
    addAndMakeVisible (importButton);

    exportButton.setButtonText ("Export");
    exportButton.setTooltip ("Write the current sound to a .grandcaverns file.");
    exportButton.onClick = [this] { launchExport(); };
    addAndMakeVisible (exportButton);

    presetLibrary.load();
    rebuildPresetMenu();
    syncPresetMenu();
    syncAlgorithmControls();

    testButton.setButtonText ("TEST SOUND");
    testButton.setTooltip ("Play a long pad through the cavern.");
    testButton.setClickingTogglesState (false);
    testButton.onClick = [this]
    {
        plugin().triggerTestSound();
    };
    addAndMakeVisible (testButton);

    startTimerHz (30);
}

GrandCavernsAudioProcessorEditor::~GrandCavernsAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void GrandCavernsAudioProcessorEditor::timerCallback()
{
    syncPresetMenu();
    syncAlgorithmControls();
    testButton.setToggleState (plugin().isTestSoundPlaying(), juce::dontSendNotification);

    const int modeIndex = juce::roundToInt (plugin().apvts.getRawParameterValue (id::mode)->load());
    const auto mode = static_cast<CavernMode> (juce::jlimit (0, 4, modeIndex));
    juce::String text = cavernModeBlurb (mode);

    if (mode == CavernMode::Loveless)
    {
        const float bloom = plugin().apvts.getRawParameterValue (id::bloom)->load();
        const float predelay = plugin().apvts.getRawParameterValue (id::predelay)->load();
        const int ms = juce::roundToInt (lovelessLeadSeconds (bloom, predelay) * 1000.0f);
        text << "  Dry waits about " << ms << " ms.";
    }

    if (text != blurb)
    {
        blurb = text;
        repaint (blurbArea);
    }

    display.repaint();
}

namespace
{
constexpr int kUserPresetIdBase = 1001;

bool isFactoryPresetName (const juce::String& name)
{
    for (const auto& preset : factoryPresets())
        if (name == preset.name)
            return true;

    return false;
}
} // namespace

void GrandCavernsAudioProcessorEditor::rebuildPresetMenu()
{
    ignorePresetChange = true;
    presets.clear (juce::dontSendNotification);
    presets.addSectionHeading ("Built in");

    for (int i = 0; i < static_cast<int> (factoryPresets().size()); ++i)
        presets.addItem (factoryPresets()[static_cast<size_t> (i)].name, i + 1);

    if (! presetLibrary.entries().empty())
    {
        presets.addSectionHeading ("Saved");

        for (int i = 0; i < static_cast<int> (presetLibrary.entries().size()); ++i)
            presets.addItem (presetLibrary.entries()[static_cast<size_t> (i)].name, kUserPresetIdBase + i);
    }

    presets.setSelectedId (selectedPresetId, juce::dontSendNotification);
    ignorePresetChange = false;
    deleteButton.setEnabled (selectedPresetId >= kUserPresetIdBase);
}

int GrandCavernsAudioProcessorEditor::presetIdForParams (const GrandCavernsParams& params) const
{
    const auto matchesId = [&] (int presetId)
    {
        if (presetId > 0 && presetId <= static_cast<int> (factoryPresets().size()))
            return grandCavernsParamsMatch (factoryPresets()[static_cast<size_t> (presetId - 1)].params, params);

        const int index = presetId - kUserPresetIdBase;

        if (index >= 0 && index < static_cast<int> (presetLibrary.entries().size()))
            return grandCavernsParamsMatch (presetLibrary.entries()[static_cast<size_t> (index)].params, params);

        return false;
    };

    if (matchesId (selectedPresetId))
        return selectedPresetId;

    for (int i = 0; i < static_cast<int> (factoryPresets().size()); ++i)
        if (grandCavernsParamsMatch (factoryPresets()[static_cast<size_t> (i)].params, params))
            return i + 1;

    for (int i = 0; i < static_cast<int> (presetLibrary.entries().size()); ++i)
        if (grandCavernsParamsMatch (presetLibrary.entries()[static_cast<size_t> (i)].params, params))
            return kUserPresetIdBase + i;

    return 0;
}

void GrandCavernsAudioProcessorEditor::syncPresetMenu()
{
    const int id = presetIdForParams (plugin().getCurrentParams());

    if (id == selectedPresetId)
        return;

    selectedPresetId = id;
    ignorePresetChange = true;
    presets.setSelectedId (id, juce::dontSendNotification);
    ignorePresetChange = false;
    deleteButton.setEnabled (selectedPresetId >= kUserPresetIdBase);
}

void GrandCavernsAudioProcessorEditor::loadPresetById (int presetId)
{
    if (presetId <= 0)
        return;

    if (presetId < kUserPresetIdBase)
    {
        const int index = presetId - 1;

        if (index < 0 || index >= static_cast<int> (factoryPresets().size()))
            return;

        selectedPresetId = presetId;
        plugin().applyParams (factoryPresets()[static_cast<size_t> (index)].params, index);
    }
    else
    {
        const int index = presetId - kUserPresetIdBase;

        if (index < 0 || index >= static_cast<int> (presetLibrary.entries().size()))
            return;

        selectedPresetId = presetId;
        plugin().applyParams (presetLibrary.entries()[static_cast<size_t> (index)].params, -1);
    }

    deleteButton.setEnabled (selectedPresetId >= kUserPresetIdBase);
    syncAlgorithmControls();
}

void GrandCavernsAudioProcessorEditor::showPresetMessage (const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, title, message);
}

void GrandCavernsAudioProcessorEditor::promptSavePreset()
{
    auto* window = new juce::AlertWindow ("Save Preset", "Name the current sound. Saving replaces a saved preset of the same name.", juce::AlertWindow::NoIcon);
    juce::String suggested = "My Preset";

    if (selectedPresetId >= kUserPresetIdBase)
        suggested = presets.getText();

    window->addTextEditor ("name", suggested, "Name");
    window->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<GrandCavernsAudioProcessorEditor> safe (this);
    window->enterModalState (true, juce::ModalCallbackFunction::create ([safe, window] (int result)
    {
        if (safe == nullptr || result != 1)
            return;

        safe->commitSavePreset (window->getTextEditorContents ("name"));
    }), true);
}

void GrandCavernsAudioProcessorEditor::commitSavePreset (const juce::String& requestedName)
{
    const auto name = requestedName.trim().substring (0, 48);

    if (name.isEmpty())
        return;

    if (isFactoryPresetName (name))
    {
        showPresetMessage ("Save Preset", "That name is a built-in preset. Choose another name.");
        return;
    }

    const int index = presetLibrary.addOrReplace ({ name, plugin().getCurrentParams() });

    if (index < 0 || ! presetLibrary.save())
    {
        presetLibrary.load();
        rebuildPresetMenu();
        syncPresetMenu();
        showPresetMessage ("Save Preset", "Couldn't store that preset on this computer.");
        return;
    }

    selectedPresetId = kUserPresetIdBase + index;
    rebuildPresetMenu();
}

void GrandCavernsAudioProcessorEditor::confirmDeletePreset()
{
    const int index = selectedPresetId - kUserPresetIdBase;

    if (index < 0 || index >= static_cast<int> (presetLibrary.entries().size()))
        return;

    const auto name = presetLibrary.entries()[static_cast<size_t> (index)].name;
    juce::Component::SafePointer<GrandCavernsAudioProcessorEditor> safe (this);
    juce::AlertWindow::showOkCancelBox (juce::MessageBoxIconType::NoIcon,
                                        "Delete Preset",
                                        "Remove \"" + name + "\" from this computer?",
                                        "Delete",
                                        "Cancel",
                                        this,
                                        juce::ModalCallbackFunction::create ([safe] (int result)
                                        {
                                            if (safe != nullptr && result == 1)
                                                safe->deleteSelectedPreset();
                                        }));
}

void GrandCavernsAudioProcessorEditor::deleteSelectedPreset()
{
    const int index = selectedPresetId - kUserPresetIdBase;

    if (index < 0 || index >= static_cast<int> (presetLibrary.entries().size()))
        return;

    presetLibrary.removeAt (index);

    if (! presetLibrary.save())
    {
        presetLibrary.load();
        rebuildPresetMenu();
        syncPresetMenu();
        showPresetMessage ("Delete Preset", "Couldn't update the saved presets on this computer.");
        return;
    }

    selectedPresetId = 0;
    rebuildPresetMenu();
    syncPresetMenu();
}

void GrandCavernsAudioProcessorEditor::launchImport()
{
    if (fileChooser != nullptr)
        return;

    fileChooser = std::make_shared<juce::FileChooser> ("Import presets",
                                                       juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                       "*.grandcaverns");
    auto chooser = fileChooser;
    juce::Component::SafePointer<GrandCavernsAudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe, chooser] (const juce::FileChooser& dialog)
                          {
                              if (safe == nullptr)
                                  return;

                              safe->fileChooser.reset();
                              safe->finishImport (dialog.getResult());
                          });
}

void GrandCavernsAudioProcessorEditor::finishImport (const juce::File& file)
{
    if (file == juce::File())
        return;

    juce::String error;
    const auto imported = PresetLibrary::readFile (file, error);

    if (imported.empty())
    {
        showPresetMessage ("Import Presets", error.isEmpty() ? "That file has no presets." : error);
        return;
    }

    struct FirstChoice
    {
        bool factory = false;
        int index = -1;
    };

    FirstChoice first;
    bool changed = false;

    for (const auto& preset : imported)
    {
        int factoryIndex = -1;

        for (int i = 0; i < static_cast<int> (factoryPresets().size()); ++i)
        {
            if (grandCavernsParamsMatch (factoryPresets()[static_cast<size_t> (i)].params, preset.params))
            {
                factoryIndex = i;
                break;
            }
        }

        if (factoryIndex >= 0)
        {
            if (first.index < 0)
                first = { true, factoryIndex };

            continue;
        }

        const int index = presetLibrary.addOrReplace (preset);
        changed = changed || index >= 0;

        if (index >= 0 && first.index < 0)
            first = { false, index };
    }

    if (changed && ! presetLibrary.save())
    {
        presetLibrary.load();
        rebuildPresetMenu();
        syncPresetMenu();
        showPresetMessage ("Import Presets", "Couldn't store the imported presets on this computer.");
        return;
    }

    if (first.index < 0)
    {
        rebuildPresetMenu();
        return;
    }

    selectedPresetId = first.factory ? first.index + 1 : kUserPresetIdBase + first.index;
    rebuildPresetMenu();
    loadPresetById (selectedPresetId);
}

void GrandCavernsAudioProcessorEditor::launchExport()
{
    if (fileChooser != nullptr)
        return;

    juce::String name = selectedPresetId > 0 ? presets.getText() : juce::String ("Preset");

    if (name.isEmpty())
        name = "Preset";

    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (juce::File::createLegalFileName (name) + ".grandcaverns");
    fileChooser = std::make_shared<juce::FileChooser> ("Export preset", suggested, "*.grandcaverns");
    auto chooser = fileChooser;
    juce::Component::SafePointer<GrandCavernsAudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::warnAboutOverwriting
                              | juce::FileBrowserComponent::canSelectFiles,
                          [safe, chooser] (const juce::FileChooser& dialog)
                          {
                              if (safe == nullptr)
                                  return;

                              safe->fileChooser.reset();
                              safe->finishExport (dialog.getResult());
                          });
}

void GrandCavernsAudioProcessorEditor::finishExport (const juce::File& file)
{
    if (file == juce::File())
        return;

    NamedPreset preset;
    preset.params = plugin().getCurrentParams();
    preset.name = selectedPresetId > 0 ? presets.getText() : juce::String ("Preset");

    if (preset.name.isEmpty())
        preset.name = "Preset";

    juce::String error;

    if (! PresetLibrary::writeFile (file, { preset }, error))
        showPresetMessage ("Export Preset", error.isEmpty() ? "Couldn't write that file." : error);
}

void GrandCavernsAudioProcessorEditor::syncAlgorithmControls()
{
    const int modeIndex = juce::roundToInt (plugin().apvts.getRawParameterValue (id::mode)->load());
    const auto mode = static_cast<CavernMode> (juce::jlimit (0, 4, modeIndex));

    for (int i = 0; i < 5; ++i)
        modeButtons[static_cast<size_t> (i)].setToggleState (i == modeIndex, juce::dontSendNotification);

    display.setState (plugin().wetLevel.load(), plugin().bloomLevel.load(), mode);
}

void GrandCavernsAudioProcessorEditor::layoutKnob (Knob& knob, juce::Rectangle<int> bounds)
{
    knob.label.setBounds (bounds.removeFromTop (16));
    knob.slider.setBounds (bounds.reduced (4, 0));
}

void GrandCavernsAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (kInk));

    auto panel = getLocalBounds().toFloat().reduced (12.0f);
    g.setColour (juce::Colour (kPanel));
    g.fillRoundedRectangle (panel, 14.0f);
    g.setColour (juce::Colour (kLine));
    g.drawRoundedRectangle (panel, 14.0f, 1.0f);

    g.setColour (juce::Colour (kCream));
    g.setFont (juce::Font (juce::FontOptions ("Avenir Next", 28.0f, juce::Font::bold)));
    g.drawText ("GRANDCAVERNS", 28, 12, 280, 34, juce::Justification::centredLeft);

    g.setColour (juce::Colour (kAmber));
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("BLOOM + CAVERN", 308, 22, 240, 16, juce::Justification::centredLeft);

    g.setColour (juce::Colour (kDim));
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawFittedText (blurb, blurbArea, juce::Justification::centredLeft, 3);
}

void GrandCavernsAudioProcessorEditor::resized()
{
    const int width = getWidth();
    const int buttonGap = 8;
    const int buttonW = (width - 56 - buttonGap * 4) / 5;

    for (int i = 0; i < 5; ++i)
        modeButtons[static_cast<size_t> (i)].setBounds (28 + i * (buttonW + buttonGap), 98, buttonW, 32);

    testButton.setBounds (width - 28 - 136, 14, 136, 30);

    const int rowY = 54;
    const int rowH = 30;
    const int exportW = 84;
    const int importW = 84;
    const int deleteW = 78;
    const int saveW = 70;
    const int right = width - 28;
    const int exportX = right - exportW;
    const int importX = exportX - buttonGap - importW;
    const int deleteX = importX - buttonGap - deleteW;
    const int saveX = deleteX - buttonGap - saveW;

    presetLabel.setBounds (28, rowY, 62, rowH);
    presets.setBounds (94, rowY, saveX - buttonGap - 94, rowH);
    saveButton.setBounds (saveX, rowY, saveW, rowH);
    deleteButton.setBounds (deleteX, rowY, deleteW, rowH);
    importButton.setBounds (importX, rowY, importW, rowH);
    exportButton.setBounds (exportX, rowY, exportW, rowH);

    display.setBounds (28, 142, width - 56, 132);

    const int knobW = (width - 56) / 6;
    const int knobH = 150;
    const int top = 286;

    for (int row = 0; row < 3; ++row)
    {
        const int count = row < 2 ? 6 : 3;

        for (int col = 0; col < count; ++col)
            layoutKnob (knobs[static_cast<size_t> (row * 6 + col)],
                        { 28 + col * knobW, top + row * knobH, knobW, knobH - 6 });
    }

    blurbArea = { 28, top + knobH * 3 + 8, width - 56, 72 };
}
