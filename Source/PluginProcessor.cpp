#include "PluginProcessor.h"
#include "GrandCavernsPresets.h"
#include "ParameterIDs.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
juce::AudioParameterFloatAttributes percentAttrs (const char* suffix = " %")
{
    return juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [suffix] (float value, int)
        {
            return juce::String (juce::roundToInt (value * 100.0f)) + suffix;
        });
}

void assignParam (juce::AudioProcessorValueTreeState& state, const char* paramID, float plain)
{
    if (auto* parameter = state.getParameter (paramID))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plain));
        parameter->endChangeGesture();
    }
}
} // namespace

GrandCavernsAudioProcessor::GrandCavernsAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

void GrandCavernsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    engine.setTargets (readParams());
    engine.snapToTargets();
    floatScratch.setSize (2, samplesPerBlock, false, false, true);
    sideScratch.setSize (1, samplesPerBlock, false, false, true);
    fedInput.setSize (2, samplesPerBlock, false, false, true);
    testVoice.prepare (sampleRate);
    lastReportedLatency = -1;
}

void GrandCavernsAudioProcessor::releaseResources() {}

void GrandCavernsAudioProcessor::reset()
{
    engine.reset();
    testVoice.reset();
}

void GrandCavernsAudioProcessor::triggerTestSound()
{
    testVoice.arm();
}

bool GrandCavernsAudioProcessor::isTestSoundPlaying() const
{
    return testVoice.isPlaying();
}

void GrandCavernsAudioProcessor::TestVoice::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
}

void GrandCavernsAudioProcessor::TestVoice::reset()
{
    samplesLeft = 0;
    age = 0;
    armed.store (false, std::memory_order_relaxed);
    playing.store (false, std::memory_order_relaxed);
}

void GrandCavernsAudioProcessor::TestVoice::arm()
{
    armed.store (true, std::memory_order_release);
}

void GrandCavernsAudioProcessor::TestVoice::mix (float* left, float* right, int numSamples)
{
    if (armed.exchange (false, std::memory_order_acq_rel))
    {
        for (int voice = 0; voice < numVoices; ++voice)
        {
            phase[voice] = static_cast<double> (voice) * 0.73;
            harmonic[voice] = phase[voice] * 2.0;
            phaseRight[voice] = phase[voice] + 0.4;
            harmonicRight[voice] = harmonic[voice] + 0.2;
        }

        age = 0;
        lfo = 0.0;
        attackSamples = juce::jmax (1, static_cast<int> (sampleRate * 1.8));
        sustainSamples = juce::jmax (1, static_cast<int> (sampleRate * 5.5));
        releaseSamples = juce::jmax (1, static_cast<int> (sampleRate * 2.8));
        samplesLeft = attackSamples + sustainSamples + releaseSamples;
        playing.store (true, std::memory_order_relaxed);
    }

    if (samplesLeft <= 0)
    {
        playing.store (false, std::memory_order_relaxed);
        return;
    }

    const float freqs[] = { 73.42f, 110.00f, 146.83f, 184.99f, 220.00f, 293.66f };
    const float cents[] = { 6.0f, -5.0f, 7.0f, -6.0f, 5.0f, -7.0f };
    const float gains[] = { 0.62f, 0.48f, 0.36f, 0.28f, 0.22f, 0.16f };
    const int count = std::min (numSamples, samplesLeft);
    constexpr double twoPi = 6.283185307179586;
    const double lfoInc = twoPi * 0.08 / sampleRate;

    for (int i = 0; i < count; ++i)
    {
        float env = 0.0f;

        if (age < attackSamples)
            env = 0.5f - 0.5f * std::cos (3.14159265f * static_cast<float> (age) / static_cast<float> (attackSamples));
        else if (age < attackSamples + sustainSamples)
            env = 1.0f;
        else
        {
            const float t = static_cast<float> (age - attackSamples - sustainSamples) / static_cast<float> (releaseSamples);
            env = 0.5f + 0.5f * std::cos (3.14159265f * juce::jlimit (0.0f, 1.0f, t));
        }

        lfo += lfoInc;

        if (lfo > twoPi)
            lfo -= twoPi;

        const float breathe = 1.0f + 0.06f * static_cast<float> (std::sin (lfo));
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        for (int voice = 0; voice < numVoices; ++voice)
        {
            const float harmonicAmount = voice < 3 ? 0.14f : 0.04f;
            const float gain = gains[voice];
            sampleL += gain * (static_cast<float> (std::sin (phase[voice])) + harmonicAmount * static_cast<float> (std::sin (harmonic[voice])));
            sampleR += gain * (static_cast<float> (std::sin (phaseRight[voice])) + harmonicAmount * static_cast<float> (std::sin (harmonicRight[voice])));

            const double inc = twoPi * static_cast<double> (freqs[voice]) / sampleRate;
            const double rightInc = inc * (1.0 + static_cast<double> (cents[voice]) * 0.00057735);
            phase[voice] += inc;
            harmonic[voice] += inc * 2.0;
            phaseRight[voice] += rightInc;
            harmonicRight[voice] += rightInc * 2.0;

            if (phase[voice] > twoPi)
                phase[voice] -= twoPi;

            if (harmonic[voice] > twoPi)
                harmonic[voice] -= twoPi;

            if (phaseRight[voice] > twoPi)
                phaseRight[voice] -= twoPi;

            if (harmonicRight[voice] > twoPi)
                harmonicRight[voice] -= twoPi;
        }

        const float level = 0.09f * env * breathe;
        left[i] += sampleL * level;
        right[i] += sampleR * level;
        ++age;
    }

    samplesLeft -= count;

    if (samplesLeft <= 0)
        playing.store (false, std::memory_order_relaxed);
}

bool GrandCavernsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    const bool inputOk = input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo();
    const bool outputOk = output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo();
    return inputOk && outputOk;
}

GrandCavernsParams GrandCavernsAudioProcessor::readParams() const
{
    GrandCavernsParams params;
    params.mix = apvts.getRawParameterValue (id::mix)->load();
    params.delayMs = apvts.getRawParameterValue (id::delay)->load();
    params.warp = apvts.getRawParameterValue (id::warp)->load();
    params.feedback = apvts.getRawParameterValue (id::feedback)->load();
    params.density = apvts.getRawParameterValue (id::density)->load();
    params.width = apvts.getRawParameterValue (id::width)->load();
    params.bloom = apvts.getRawParameterValue (id::bloom)->load();
    params.bloomFeedback = apvts.getRawParameterValue (id::bloomFeedback)->load();
    params.modRateHz = apvts.getRawParameterValue (id::modRate)->load();
    params.modDepth = apvts.getRawParameterValue (id::modDepth)->load();
    params.tone = apvts.getRawParameterValue (id::tone)->load();
    params.lowEndDb = apvts.getRawParameterValue (id::lowEnd)->load();
    params.predelayMs = apvts.getRawParameterValue (id::predelay)->load();
    params.drive = apvts.getRawParameterValue (id::drive)->load();
    params.glide = apvts.getRawParameterValue (id::glide)->load();

    const int mode = juce::roundToInt (apvts.getRawParameterValue (id::mode)->load());
    params.mode = (mode >= 0 && mode < static_cast<int> (CavernMode::Count))
                      ? static_cast<CavernMode> (mode)
                      : CavernMode::Cavern;
    return params;
}

void GrandCavernsAudioProcessor::renderFloat (juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals denormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    if (numSamples == 0 || numOut == 0)
        return;

    engine.setTargets (readParams());

    if (fedInput.getNumSamples() < numSamples)
        fedInput.setSize (2, numSamples, false, false, true);

    auto* inputL = fedInput.getWritePointer (0);
    auto* inputR = fedInput.getWritePointer (1);
    const float* sourceL = numIn > 0 ? buffer.getReadPointer (0) : nullptr;
    const float* sourceR = numIn > 1 ? buffer.getReadPointer (1) : sourceL;

    for (int i = 0; i < numSamples; ++i)
    {
        inputL[i] = sourceL != nullptr ? sourceL[i] : 0.0f;
        inputR[i] = sourceR != nullptr ? sourceR[i] : inputL[i];
    }

    testVoice.mix (inputL, inputR, numSamples);

    if (numOut == 1)
    {
        if (sideScratch.getNumSamples() < numSamples)
            sideScratch.setSize (1, numSamples, false, false, true);

        engine.process (inputL, inputR, buffer.getWritePointer (0), sideScratch.getWritePointer (0), numSamples);

        auto* left = buffer.getWritePointer (0);
        const auto* right = sideScratch.getReadPointer (0);

        for (int i = 0; i < numSamples; ++i)
            left[i] = 0.5f * (left[i] + right[i]);
    }
    else
    {
        engine.process (inputL, inputR, buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);

        for (int channel = 2; channel < buffer.getNumChannels(); ++channel)
            buffer.clear (channel, 0, numSamples);
    }

    wetLevel.store (engine.getWetLevel(), std::memory_order_relaxed);
    bloomLevel.store (engine.getBloomLevel(), std::memory_order_relaxed);

    const int latency = engine.getLatencySamples();

    if (latency != lastReportedLatency)
    {
        lastReportedLatency = latency;
        setLatencySamples (latency);
    }
}

void GrandCavernsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    renderFloat (buffer);
}

void GrandCavernsAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi)
{
    const int channels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (floatScratch.getNumChannels() < channels || floatScratch.getNumSamples() < numSamples)
        floatScratch.setSize (channels, numSamples, false, false, true);

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* dst = floatScratch.getWritePointer (channel);
        const auto* src = buffer.getReadPointer (channel);

        for (int i = 0; i < numSamples; ++i)
            dst[i] = static_cast<float> (src[i]);
    }

    renderFloat (floatScratch);

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* dst = buffer.getWritePointer (channel);
        const auto* src = floatScratch.getReadPointer (channel);

        for (int i = 0; i < numSamples; ++i)
            dst[i] = static_cast<double> (src[i]);
    }

    juce::ignoreUnused (midi);
}

juce::AudioProcessorEditor* GrandCavernsAudioProcessor::createEditor()
{
    return new GrandCavernsAudioProcessorEditor (*this);
}

int GrandCavernsAudioProcessor::getNumPrograms()
{
    return static_cast<int> (factoryPresets().size());
}

int GrandCavernsAudioProcessor::getCurrentProgram()
{
    return juce::jlimit (0, juce::jmax (0, getNumPrograms() - 1), currentProgram.load());
}

void GrandCavernsAudioProcessor::setCurrentProgram (int index)
{
    applyPreset (index);
}

const juce::String GrandCavernsAudioProcessor::getProgramName (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return {};

    return factoryPresets()[static_cast<size_t> (index)].name;
}

void GrandCavernsAudioProcessor::applyPreset (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    applyParams (factoryPresets()[static_cast<size_t> (index)].params, index);
}

void GrandCavernsAudioProcessor::applyParams (const GrandCavernsParams& preset, int programIndex)
{
    currentProgram.store (programIndex);
    assignParam (apvts, id::mix, preset.mix);
    assignParam (apvts, id::delay, preset.delayMs);
    assignParam (apvts, id::warp, preset.warp);
    assignParam (apvts, id::feedback, preset.feedback);
    assignParam (apvts, id::density, preset.density);
    assignParam (apvts, id::width, preset.width);
    assignParam (apvts, id::bloom, preset.bloom);
    assignParam (apvts, id::bloomFeedback, preset.bloomFeedback);
    assignParam (apvts, id::modRate, preset.modRateHz);
    assignParam (apvts, id::modDepth, preset.modDepth);
    assignParam (apvts, id::tone, preset.tone);
    assignParam (apvts, id::lowEnd, preset.lowEndDb);
    assignParam (apvts, id::predelay, preset.predelayMs);
    assignParam (apvts, id::drive, preset.drive);
    assignParam (apvts, id::glide, preset.glide);
    assignParam (apvts, id::mode, static_cast<float> (preset.mode));
}

GrandCavernsParams GrandCavernsAudioProcessor::getCurrentParams() const
{
    return readParams();
}

void GrandCavernsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GrandCavernsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);

        if (tree.hasType (apvts.state.getType()))
        {
            currentProgram.store (static_cast<int> (tree.getProperty ("program", 0)));
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout GrandCavernsAudioProcessor::createLayout()
{
    const auto& defaults = factoryPresets()[0].params;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto percent = [] (const char* paramID, const char* name, float start, float end, float value)
    {
        return std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { paramID, 1 }, name,
            juce::NormalisableRange<float> (start, end, 0.001f), value, percentAttrs());
    };

    layout.add (percent (id::mix, "Mix", 0.0f, 1.0f, defaults.mix));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id::delay, 1 }, "Delay",
        juce::NormalisableRange<float> (20.0f, 2000.0f, 0.01f, 0.45f), defaults.delayMs,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            if (value >= 1000.0f)
                return juce::String (value / 1000.0f, 2) + " s";

            return juce::String (juce::roundToInt (value)) + " ms";
        })));
    layout.add (percent (id::warp, "Warp", 0.0f, 1.0f, defaults.warp));
    layout.add (percent (id::feedback, "Feedback", 0.0f, 1.0f, defaults.feedback));
    layout.add (percent (id::density, "Density", 0.0f, 1.0f, defaults.density));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id::width, 1 }, "Width",
        juce::NormalisableRange<float> (0.0f, 1.6f, 0.001f), defaults.width,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (juce::roundToInt (value * 100.0f)) + " %";
        })));
    layout.add (percent (id::bloom, "Bloom", 0.0f, 1.0f, defaults.bloom));
    layout.add (percent (id::bloomFeedback, "Bloom Feedback", 0.0f, 1.0f, defaults.bloomFeedback));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id::modRate, 1 }, "Mod Rate",
        juce::NormalisableRange<float> (0.05f, 6.0f, 0.001f, 0.45f), defaults.modRateHz,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value, value < 1.0f ? 2 : 1) + " Hz";
        })));
    layout.add (percent (id::modDepth, "Mod Depth", 0.0f, 1.0f, defaults.modDepth));
    layout.add (percent (id::tone, "Tone", 0.0f, 1.0f, defaults.tone));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id::lowEnd, 1 }, "Low End",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), defaults.lowEndDb,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value, 1) + " dB";
        })));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id::predelay, 1 }, "Pre-delay",
        juce::NormalisableRange<float> (0.0f, 250.0f, 0.01f, 0.6f), defaults.predelayMs,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (juce::roundToInt (value)) + " ms";
        })));
    layout.add (percent (id::drive, "Drive", 0.0f, 1.0f, defaults.drive));
    layout.add (percent (id::glide, "Glide", 0.0f, 1.0f, defaults.glide));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { id::mode, 1 }, "Mode",
        juce::StringArray { "Cavern", "Bloom", "Massive", "Inertia", "Loveless" },
        static_cast<int> (defaults.mode)));

    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrandCavernsAudioProcessor();
}
