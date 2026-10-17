#pragma once

#include "DSP/GrandCavernsEngine.h"

#include <array>
#include <cmath>

struct FactoryPreset
{
    const char* name;
    const char* description;
    GrandCavernsParams params;
};

// Starting points, not reproductions of anyone's factory patches.
inline const std::array<FactoryPreset, 8>& factoryPresets()
{
    static const std::array<FactoryPreset, 8> presets { {
        { "Wide Blend",
          "The blend. A medium bloom opening into a wide modulated cavern.",
          { .mode = CavernMode::Cavern, .mix = 0.40f, .width = 1.10f, .delayMs = 520.0f, .warp = 0.58f,
            .feedback = 0.74f, .density = 0.78f, .modRateHz = 0.32f, .modDepth = 0.38f, .bloom = 0.52f,
            .bloomFeedback = 0.42f, .predelayMs = 28.0f, .tone = 0.58f, .lowEndDb = 1.0f, .drive = 0.08f, .glide = 0.0f } },
        { "Late Room",
          "Long tank, medium cloud. The note is dry for a moment, then the room fills in.",
          { .mode = CavernMode::Bloom, .mix = 0.46f, .width = 1.05f, .delayMs = 380.0f, .warp = 0.42f,
            .feedback = 0.68f, .density = 0.90f, .modRateHz = 0.25f, .modDepth = 0.48f, .bloom = 0.56f,
            .bloomFeedback = 0.50f, .predelayMs = 36.0f, .tone = 0.55f, .lowEndDb = 2.0f, .drive = 0.0f, .glide = 0.0f } },
        { "Long Smear",
          "Longest delays, warp past halfway, density backed off so the echoes smear instead of stacking.",
          { .mode = CavernMode::Massive, .mix = 0.38f, .width = 1.28f, .delayMs = 1400.0f, .warp = 0.72f,
            .feedback = 0.84f, .density = 0.40f, .modRateHz = 0.12f, .modDepth = 0.46f, .bloom = 0.16f,
            .bloomFeedback = 0.12f, .predelayMs = 0.0f, .tone = 0.78f, .lowEndDb = 0.0f, .drive = 0.0f, .glide = 0.0f } },
        { "Slow Wake",
          "Hold a chord. The cavern is late and it does not want to leave.",
          { .mode = CavernMode::Inertia, .mix = 0.48f, .width = 1.20f, .delayMs = 980.0f, .warp = 0.64f,
            .feedback = 0.80f, .density = 0.86f, .modRateHz = 0.10f, .modDepth = 0.30f, .bloom = 0.72f,
            .bloomFeedback = 0.48f, .predelayMs = 20.0f, .tone = 0.48f, .lowEndDb = 1.0f, .drive = 0.0f, .glide = 0.05f } },
        { "Reverse Wall",
          "Reverse swell ahead of the note, wide, with grit on the way in. The shoegaze wall.",
          { .mode = CavernMode::Loveless, .mix = 0.58f, .width = 1.36f, .delayMs = 700.0f, .warp = 0.55f,
            .feedback = 0.76f, .density = 0.88f, .modRateHz = 0.36f, .modDepth = 0.42f, .bloom = 0.62f,
            .bloomFeedback = 0.55f, .predelayMs = 12.0f, .tone = 0.40f, .lowEndDb = 2.5f, .drive = 0.42f, .glide = 0.62f } },
        { "Soft Focus",
          "Less grit, a longer cloud, the chord sitting inside the wash.",
          { .mode = CavernMode::Loveless, .mix = 0.52f, .width = 1.28f, .delayMs = 560.0f, .warp = 0.48f,
            .feedback = 0.72f, .density = 0.92f, .modRateHz = 0.28f, .modDepth = 0.36f, .bloom = 0.74f,
            .bloomFeedback = 0.58f, .predelayMs = 18.0f, .tone = 0.50f, .lowEndDb = 2.0f, .drive = 0.20f, .glide = 0.38f } },
        { "Glide Echo",
          "Warp low enough that you still hear repeats, then they dissolve. A little glide on each one.",
          { .mode = CavernMode::Loveless, .mix = 0.45f, .width = 1.15f, .delayMs = 480.0f, .warp = 0.22f,
            .feedback = 0.66f, .density = 0.32f, .modRateHz = 0.55f, .modDepth = 0.28f, .bloom = 0.34f,
            .bloomFeedback = 0.30f, .predelayMs = 8.0f, .tone = 0.62f, .lowEndDb = 1.0f, .drive = 0.28f, .glide = 0.22f } },
        { "Full Glide",
          "The whole room bends, as if the tremolo arm is moving the cavern and not just the string.",
          { .mode = CavernMode::Loveless, .mix = 0.55f, .width = 1.42f, .delayMs = 640.0f, .warp = 0.60f,
            .feedback = 0.78f, .density = 0.84f, .modRateHz = 0.18f, .modDepth = 0.55f, .bloom = 0.58f,
            .bloomFeedback = 0.46f, .predelayMs = 10.0f, .tone = 0.45f, .lowEndDb = 1.5f, .drive = 0.16f, .glide = 0.88f } },
    } };

    return presets;
}

inline bool grandCavernsParamsMatch (const GrandCavernsParams& a, const GrandCavernsParams& b)
{
    const auto near = [] (float left, float right, float tolerance)
    {
        return std::abs (left - right) <= tolerance;
    };

    return a.mode == b.mode
        && near (a.mix, b.mix, 0.002f)
        && near (a.width, b.width, 0.003f)
        && near (a.delayMs, b.delayMs, 0.5f)
        && near (a.warp, b.warp, 0.002f)
        && near (a.feedback, b.feedback, 0.002f)
        && near (a.density, b.density, 0.002f)
        && near (a.modRateHz, b.modRateHz, 0.01f)
        && near (a.modDepth, b.modDepth, 0.002f)
        && near (a.bloom, b.bloom, 0.002f)
        && near (a.bloomFeedback, b.bloomFeedback, 0.002f)
        && near (a.predelayMs, b.predelayMs, 0.5f)
        && near (a.tone, b.tone, 0.002f)
        && near (a.lowEndDb, b.lowEndDb, 0.05f)
        && near (a.drive, b.drive, 0.002f)
        && near (a.glide, b.glide, 0.002f);
}
