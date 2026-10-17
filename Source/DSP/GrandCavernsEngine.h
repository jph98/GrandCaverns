#pragma once

#include "Filters.h"

#include <atomic>
#include <cmath>

// Original cavern reverb.
//
// A rising multi-tap bloom — a slow-opening cloud, with feedback around that
// section — feeds an eight-line feedback delay network. Warp spreads those
// delays from stacked echoes out into a reverb. Density crossfades a polarity
// mix into a Hadamard mix and brings diffusors into the loop. Loveless holds
// the dry note so the cloud can swell ahead of it: the realtime shape of a
// reverse-reverb print.

enum class CavernMode
{
    Cavern = 0,
    Bloom,
    Massive,
    Inertia,
    Loveless,
    Count
};

struct GrandCavernsParams
{
    CavernMode mode = CavernMode::Cavern;
    float mix = 0.40f;
    float width = 1.10f;
    float delayMs = 520.0f;
    float warp = 0.58f;
    float feedback = 0.74f;
    float density = 0.78f;
    float modRateHz = 0.32f;
    float modDepth = 0.38f;
    float bloom = 0.52f;
    float bloomFeedback = 0.42f;
    float predelayMs = 28.0f;
    float tone = 0.58f;
    float lowEndDb = 1.0f;
    float drive = 0.08f;
    float glide = 0.0f;
};

inline const char* cavernModeName (CavernMode mode)
{
    switch (mode)
    {
        case CavernMode::Cavern: return "Cavern";
        case CavernMode::Bloom: return "Bloom";
        case CavernMode::Massive: return "Massive";
        case CavernMode::Inertia: return "Inertia";
        case CavernMode::Loveless: return "Loveless";
        case CavernMode::Count: break;
    }

    return "Cavern";
}

inline const char* cavernModeBlurb (CavernMode mode)
{
    switch (mode)
    {
        case CavernMode::Cavern:
            return "Bloom cloud into a warped cavern. The blend of the two rooms.";
        case CavernMode::Bloom:
            return "The cloud opens slowly and the tank holds it. High mix still leaves the pick intact.";
        case CavernMode::Massive:
            return "Long feedback delays. Warp walks them from echoes to an enormous wash. Density smooths the repeats.";
        case CavernMode::Inertia:
            return "Slow to wake, slow to leave. Raise Feedback and the cloud arrives sooner and hangs longer.";
        case CavernMode::Loveless:
            return "The dry note waits while a reverse swell and glide build the wall in front of it.";
        case CavernMode::Count: break;
    }

    return "";
}

inline float bloomLengthSeconds (float bloom)
{
    return 0.05f + std::clamp (bloom, 0.0f, 1.0f) * 0.75f;
}

// Seconds the dry signal waits in Loveless, matched to the loudest bloom tap.
inline float lovelessLeadSeconds (float bloom, float predelayMs)
{
    return bloomLengthSeconds (bloom) + std::max (0.0f, predelayMs) * 0.001f;
}

class GrandCavernsEngine
{
public:
    void prepare (double newSampleRate);
    void reset();
    void setTargets (const GrandCavernsParams& params);
    void snapToTargets();
    void process (const float* inL, const float* inR, float* outL, float* outR, int numSamples);

    float getWetLevel() const { return wetLevel; }
    float getBloomLevel() const { return bloomLevel; }
    int getLatencySamples() const { return latencySamples.load (std::memory_order_relaxed); }

private:
    static float directAmount (CavernMode mode, float bloom);

    GrandCavernsParams targets;

    double sampleRate = 48000.0;
    bool prepared = false;

    float sMix = 0.4f, sWidth = 1.1f, sDelayMs = 520.0f, sWarp = 0.58f;
    float sFeedback = 0.74f, sDensity = 0.78f, sModRate = 0.32f, sModDepth = 0.38f;
    float sBloom = 0.52f, sBloomFb = 0.42f, sPreMs = 28.0f, sTone = 0.58f;
    float sLow = 1.0f, sDrive = 0.08f, sGlide = 0.0f;
    float sRevMix = 0.0f, sLagMix = 0.0f;
    float cFast = 0.01f, cSlow = 0.002f;

    DCBlock dcL, dcR;
    DelayLine preL, preR;
    DelayLine bloomL, bloomR;
    DelayLine erL, erR;
    DelayLine fdn[8];
    DelayLine dryLagL, dryLagR;
    DelayLine glideL, glideR;
    DelayLine revL, revR;
    Allpass bloomApL[4], bloomApR[4];
    Allpass fdnAp[8];
    OnePole bloomDampL, bloomDampR;
    OnePole fdnDamp[8];
    Biquad toneL, toneR, shelfL, shelfR;

    float bloomFbL = 0.0f, bloomFbR = 0.0f;
    double bloomPhase[8] {};
    double tankPhase[8] {};
    double tankPhase2[8] {};
    double glidePhaseL = 0.15, glidePhaseR = 0.62;
    double revPhase0 = 0.0, revPhase1 = 0.5;

    float dampCoef = 0.2f;
    int filterTick = 0;

    float wetLevel = 0.0f;
    float bloomLevel = 0.0f;
    std::atomic<int> latencySamples { 0 };
};
