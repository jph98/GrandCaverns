#pragma once

#include "DSP/GrandCavernsEngine.h"

#include <juce_audio_processors/juce_audio_processors.h>

class GrandCavernsAudioProcessor : public juce::AudioProcessor
{
public:
    GrandCavernsAudioProcessor();
    ~GrandCavernsAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GrandCaverns"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 30.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void applyPreset (int index);
    void applyParams (const GrandCavernsParams& params, int programIndex);
    GrandCavernsParams getCurrentParams() const;
    void triggerTestSound();
    bool isTestSoundPlaying() const;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> wetLevel { 0.0f };
    std::atomic<float> bloomLevel { 0.0f };

private:
    struct TestVoice
    {
        static constexpr int numVoices = 6;

        void prepare (double newSampleRate);
        void reset();
        void arm();
        void mix (float* left, float* right, int numSamples);
        bool isPlaying() const { return playing.load (std::memory_order_relaxed); }

        double sampleRate = 48000.0;
        double phase[numVoices] {};
        double phaseRight[numVoices] {};
        double harmonic[numVoices] {};
        double harmonicRight[numVoices] {};
        int samplesLeft = 0;
        int age = 0;
        int attackSamples = 1;
        int sustainSamples = 1;
        int releaseSamples = 1;
        double lfo = 0.0;
        std::atomic<bool> armed { false };
        std::atomic<bool> playing { false };
    };

    GrandCavernsParams readParams() const;
    void renderFloat (juce::AudioBuffer<float>& buffer);
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    GrandCavernsEngine engine;
    TestVoice testVoice;
    juce::AudioBuffer<float> floatScratch;
    juce::AudioBuffer<float> sideScratch;
    juce::AudioBuffer<float> fedInput;
    std::atomic<int> currentProgram { 0 };
    int lastReportedLatency = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrandCavernsAudioProcessor)
};
