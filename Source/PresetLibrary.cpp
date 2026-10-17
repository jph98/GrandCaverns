#include "PresetLibrary.h"
#include "ParameterIDs.h"

#include <algorithm>

namespace
{
float clampRange (float value, float start, float end)
{
    return juce::jlimit (start, end, value);
}

float readParam (const juce::XmlElement& preset, const char* paramID, float fallback, float start, float end)
{
    for (auto* child = preset.getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (child->hasTagName ("Param") && child->getStringAttribute ("id") == paramID)
            return clampRange (static_cast<float> (child->getDoubleAttribute ("value", fallback)), start, end);
    }

    return fallback;
}

NamedPreset presetFromXml (const juce::XmlElement& xml)
{
    NamedPreset preset;
    preset.name = xml.getStringAttribute ("name").trim().substring (0, 48);

    if (preset.name.isEmpty())
        preset.name = "Preset";

    const int mode = juce::roundToInt (readParam (xml, id::mode, 0.0f, 0.0f, static_cast<float> (CavernMode::Count) - 1.0f));
    preset.params.mode = static_cast<CavernMode> (mode);
    preset.params.mix = readParam (xml, id::mix, preset.params.mix, 0.0f, 1.0f);
    preset.params.delayMs = readParam (xml, id::delay, preset.params.delayMs, 20.0f, 2000.0f);
    preset.params.warp = readParam (xml, id::warp, preset.params.warp, 0.0f, 1.0f);
    preset.params.feedback = readParam (xml, id::feedback, preset.params.feedback, 0.0f, 1.0f);
    preset.params.density = readParam (xml, id::density, preset.params.density, 0.0f, 1.0f);
    preset.params.width = readParam (xml, id::width, preset.params.width, 0.0f, 1.6f);
    preset.params.bloom = readParam (xml, id::bloom, preset.params.bloom, 0.0f, 1.0f);
    preset.params.bloomFeedback = readParam (xml, id::bloomFeedback, preset.params.bloomFeedback, 0.0f, 1.0f);
    preset.params.modRateHz = readParam (xml, id::modRate, preset.params.modRateHz, 0.05f, 6.0f);
    preset.params.modDepth = readParam (xml, id::modDepth, preset.params.modDepth, 0.0f, 1.0f);
    preset.params.tone = readParam (xml, id::tone, preset.params.tone, 0.0f, 1.0f);
    preset.params.lowEndDb = readParam (xml, id::lowEnd, preset.params.lowEndDb, -12.0f, 12.0f);
    preset.params.predelayMs = readParam (xml, id::predelay, preset.params.predelayMs, 0.0f, 250.0f);
    preset.params.drive = readParam (xml, id::drive, preset.params.drive, 0.0f, 1.0f);
    preset.params.glide = readParam (xml, id::glide, preset.params.glide, 0.0f, 1.0f);
    return preset;
}

void writeParam (juce::XmlElement& preset, const char* paramID, float value)
{
    auto* node = preset.createNewChildElement ("Param");
    node->setAttribute ("id", paramID);
    node->setAttribute ("value", static_cast<double> (value));
}

std::unique_ptr<juce::XmlElement> presetsToXml (const std::vector<NamedPreset>& presets)
{
    auto root = std::make_unique<juce::XmlElement> ("GrandCavernsPresets");
    root->setAttribute ("version", 1);

    for (const auto& preset : presets)
    {
        auto* node = root->createNewChildElement ("Preset");
        node->setAttribute ("name", preset.name);
        writeParam (*node, id::mode, static_cast<float> (preset.params.mode));
        writeParam (*node, id::mix, preset.params.mix);
        writeParam (*node, id::delay, preset.params.delayMs);
        writeParam (*node, id::warp, preset.params.warp);
        writeParam (*node, id::feedback, preset.params.feedback);
        writeParam (*node, id::density, preset.params.density);
        writeParam (*node, id::width, preset.params.width);
        writeParam (*node, id::bloom, preset.params.bloom);
        writeParam (*node, id::bloomFeedback, preset.params.bloomFeedback);
        writeParam (*node, id::modRate, preset.params.modRateHz);
        writeParam (*node, id::modDepth, preset.params.modDepth);
        writeParam (*node, id::tone, preset.params.tone);
        writeParam (*node, id::lowEnd, preset.params.lowEndDb);
        writeParam (*node, id::predelay, preset.params.predelayMs);
        writeParam (*node, id::drive, preset.params.drive);
        writeParam (*node, id::glide, preset.params.glide);
    }

    return root;
}

bool isFactoryName (const juce::String& name)
{
    for (const auto& preset : factoryPresets())
        if (name == preset.name)
            return true;

    return false;
}
} // namespace

juce::File PresetLibrary::userBankFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("GrandCaverns")
        .getChildFile ("UserPresets.grandcaverns");
}

void PresetLibrary::load()
{
    userPresets.clear();
    const auto file = userBankFile();

    if (! file.existsAsFile())
        return;

    juce::String error;
    userPresets = readFile (file, error);
}

bool PresetLibrary::save() const
{
    juce::String error;
    return writeFile (userBankFile(), userPresets, error);
}

int PresetLibrary::indexOf (const juce::String& name) const
{
    for (int i = 0; i < static_cast<int> (userPresets.size()); ++i)
        if (userPresets[static_cast<size_t> (i)].name == name)
            return i;

    return -1;
}

int PresetLibrary::addOrReplace (NamedPreset preset)
{
    preset.name = preset.name.trim().substring (0, 48);

    if (preset.name.isEmpty() || isFactoryName (preset.name))
        return -1;

    const int existing = indexOf (preset.name);

    if (existing >= 0)
    {
        userPresets[static_cast<size_t> (existing)] = preset;
        return existing;
    }

    userPresets.push_back (preset);
    return static_cast<int> (userPresets.size()) - 1;
}

void PresetLibrary::removeAt (int index)
{
    if (index < 0 || index >= static_cast<int> (userPresets.size()))
        return;

    userPresets.erase (userPresets.begin() + index);
}

std::vector<NamedPreset> PresetLibrary::readFile (const juce::File& file, juce::String& error)
{
    error.clear();

    if (file.getSize() > 1024 * 1024)
    {
        error = "That file is too large.";
        return {};
    }

    const auto xml = juce::XmlDocument::parse (file);

    if (xml == nullptr || ! xml->hasTagName ("GrandCavernsPresets"))
    {
        error = "That file is not a GrandCaverns preset.";
        return {};
    }

    std::vector<NamedPreset> presets;

    for (auto* child = xml->getFirstChildElement(); child != nullptr; child = child->getNextElement())
    {
        if (! child->hasTagName ("Preset"))
            continue;

        auto preset = presetFromXml (*child);

        if (isFactoryName (preset.name))
        {
            juce::String candidate;

            for (int suffix = 2; suffix < 1000; ++suffix)
            {
                candidate = preset.name + " " + juce::String (suffix);
                const bool used = std::any_of (presets.begin(), presets.end(), [&] (const NamedPreset& existing)
                {
                    return existing.name == candidate;
                });

                if (! isFactoryName (candidate) && ! used)
                    break;
            }

            preset.name = candidate;
        }

        presets.push_back (preset);

        if (presets.size() >= 256)
            break;
    }

    if (presets.empty())
        error = "That file has no presets.";

    return presets;
}

bool PresetLibrary::writeFile (const juce::File& file, const std::vector<NamedPreset>& presets, juce::String& error)
{
    error.clear();
    auto destination = file;

    if (! destination.hasFileExtension ("grandcaverns"))
        destination = destination.withFileExtension ("grandcaverns");

    destination.getParentDirectory().createDirectory();

    if (! presetsToXml (presets)->writeTo (destination))
    {
        error = "Couldn't write " + destination.getFullPathName();
        return false;
    }

    return true;
}
