#pragma once

#include "GrandCavernsPresets.h"

#include <juce_data_structures/juce_data_structures.h>

#include <vector>

struct NamedPreset
{
    juce::String name;
    GrandCavernsParams params;
};

class PresetLibrary
{
public:
    static juce::File userBankFile();

    void load();
    bool save() const;

    const std::vector<NamedPreset>& entries() const { return userPresets; }
    int indexOf (const juce::String& name) const;
    int addOrReplace (NamedPreset preset);
    void removeAt (int index);

    static std::vector<NamedPreset> readFile (const juce::File& file, juce::String& error);
    static bool writeFile (const juce::File& file, const std::vector<NamedPreset>& presets, juce::String& error);

private:
    std::vector<NamedPreset> userPresets;
};
