#include "GrandCavernsEngine.h"

#include <algorithm>

namespace
{
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;

constexpr float kRatios[8] = {
    0.1618f, 0.2387f, 0.3217f, 0.4142f,
    0.5291f, 0.6413f, 0.7861f, 1.0000f
};

constexpr float kSign[8] = { 1, -1, 1, -1, 1, -1, 1, -1 };

constexpr float kInGainL[4] = { 0.92f, 0.64f, 0.40f, 0.24f };
constexpr float kInGainR[4] = { 0.92f, 0.64f, 0.40f, 0.24f };

constexpr int kBloomTaps = 16;
constexpr float kTapScale = 1.0f / 9.44f;

constexpr float kErMsL[4] = { 9.7f, 15.3f, 23.9f, 37.1f };
constexpr float kErMsR[4] = { 11.2f, 18.1f, 27.4f, 41.6f };
constexpr float kErGain[4] = { 0.020f, 0.014f, 0.009f, 0.006f };

constexpr float kBloomApMs[4] = { 13.1f, 19.7f, 29.3f, 43.0f };
constexpr float kBloomApG[4] = { 0.62f, 0.55f, 0.68f, 0.50f };
constexpr float kFdnApMs[8] = { 5.3f, 7.1f, 9.7f, 11.3f, 6.1f, 8.9f, 4.7f, 10.7f };

constexpr float kInject = 0.42f;

float sine (double phase)
{
    return std::sin (static_cast<float> (phase) * kTwoPi);
}

float coefFor (double sampleRate, float seconds)
{
    return 1.0f - std::exp (-1.0f / static_cast<float> (sampleRate * static_cast<double> (seconds)));
}

float shapedFeedback (float feedback)
{
    const float shaped = std::pow (std::clamp (feedback, 0.0f, 1.0f), 0.55f);
    return shaped * 0.975f;
}
} // namespace

float GrandCavernsEngine::directAmount (CavernMode mode, float bloom)
{
    float base = 0.40f;

    switch (mode)
    {
        case CavernMode::Cavern: base = 0.40f; break;
        case CavernMode::Bloom: base = 0.18f; break;
        case CavernMode::Massive: base = 0.88f; break;
        case CavernMode::Inertia: base = 0.22f; break;
        case CavernMode::Loveless: base = 0.14f; break;
        case CavernMode::Count: break;
    }

    return std::clamp (base * (1.0f - 0.80f * bloom), 0.03f, 0.95f);
}

void GrandCavernsEngine::prepare (double newSampleRate)
{
    sampleRate = std::max (8000.0, newSampleRate);
    const float sr = static_cast<float> (sampleRate);

    auto samples = [&] (float seconds)
    {
        return static_cast<int> (std::ceil (seconds * sampleRate)) + 8;
    };

    preL.setSize (samples (0.40f));
    preR.setSize (samples (0.40f));
    bloomL.setSize (samples (1.70f));
    bloomR.setSize (samples (1.70f));
    erL.setSize (samples (0.08f));
    erR.setSize (samples (0.08f));
    dryLagL.setSize (samples (2.00f));
    dryLagR.setSize (samples (2.00f));
    glideL.setSize (samples (0.08f));
    glideR.setSize (samples (0.08f));
    revL.setSize (samples (2.00f));
    revR.setSize (samples (2.00f));

    for (int i = 0; i < 8; ++i)
        fdn[i].setSize (samples (2.60f));

    for (int i = 0; i < 4; ++i)
    {
        bloomApL[i].prepare (samples (0.12f));
        bloomApR[i].prepare (samples (0.12f));
    }

    for (int i = 0; i < 8; ++i)
        fdnAp[i].prepare (samples (0.05f));

    cFast = coefFor (sampleRate, 0.020f);
    cSlow = coefFor (sampleRate, 0.050f);

    for (int i = 0; i < 8; ++i)
    {
        bloomPhase[i] = static_cast<double> (i) / 8.0;
        tankPhase[i] = static_cast<double> (i) / 8.0 + 0.13;
        tankPhase2[i] = static_cast<double> (i) / 8.0 + 0.51;
    }

    toneL.setLowpass (sampleRate, 8000.0f, 0.7f);
    toneR.setLowpass (sampleRate, 8000.0f, 0.7f);
    shelfL.setLowShelf (sampleRate, 180.0f, 0.0f);
    shelfR.setLowShelf (sampleRate, 180.0f, 0.0f);
    dampCoef = 1.0f - std::exp (-kTwoPi * 6000.0f / sr);

    reset();
    prepared = true;
    snapToTargets();
}

void GrandCavernsEngine::reset()
{
    preL.clear(); preR.clear();
    bloomL.clear(); bloomR.clear();
    erL.clear(); erR.clear();
    dryLagL.clear(); dryLagR.clear();
    glideL.clear(); glideR.clear();
    revL.clear(); revR.clear();

    for (auto& line : fdn)
        line.clear();

    for (int i = 0; i < 4; ++i)
    {
        bloomApL[i].clear();
        bloomApR[i].clear();
    }

    for (auto& ap : fdnAp)
        ap.clear();

    bloomDampL.clear();
    bloomDampR.clear();

    for (auto& d : fdnDamp)
        d.clear();

    toneL.clear();
    toneR.clear();
    shelfL.clear();
    shelfR.clear();
    dcL.clear();
    dcR.clear();

    bloomFbL = bloomFbR = 0.0f;
    revPhase0 = 0.0;
    revPhase1 = 0.5;
    wetLevel = 0.0f;
    bloomLevel = 0.0f;
    filterTick = 0;
}

void GrandCavernsEngine::setTargets (const GrandCavernsParams& params)
{
    targets = params;
    const int mode = static_cast<int> (targets.mode);

    if (mode < 0 || mode >= static_cast<int> (CavernMode::Count))
        targets.mode = CavernMode::Cavern;
}

void GrandCavernsEngine::snapToTargets()
{
    sMix = targets.mix;
    sWidth = targets.width;
    sDelayMs = targets.delayMs;
    sWarp = targets.warp;
    sFeedback = targets.feedback;
    sDensity = targets.density;
    sModRate = targets.modRateHz;
    sModDepth = targets.modDepth;
    sBloom = targets.bloom;
    sBloomFb = targets.bloomFeedback;
    sPreMs = targets.predelayMs;
    sTone = targets.tone;
    sLow = targets.lowEndDb;
    sDrive = targets.drive;
    sGlide = targets.glide;
    sRevMix = targets.mode == CavernMode::Loveless ? 0.42f : 0.0f;
    sLagMix = targets.mode == CavernMode::Loveless ? 1.0f : 0.0f;
}

void GrandCavernsEngine::process (const float* inL, const float* inR, float* outL, float* outR, int numSamples)
{
    if (! prepared || numSamples <= 0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] = inL != nullptr ? inL[i] : 0.0f;
            outR[i] = inR != nullptr ? inR[i] : 0.0f;
        }

        return;
    }

    const float sr = static_cast<float> (sampleRate);
    constexpr float kHalfPi = kPi * 0.5f;

    const float leadTarget = lovelessLeadSeconds (targets.bloom, targets.predelayMs);
    const int reported = targets.mode == CavernMode::Loveless
                             ? static_cast<int> (std::lround (leadTarget * sampleRate))
                             : 0;
    latencySamples.store (reported, std::memory_order_relaxed);

    for (int n = 0; n < numSamples; ++n)
    {
        auto smooth = [] (float& value, float target, float coef)
        {
            value += coef * (target - value);
        };

        smooth (sMix, std::clamp (targets.mix, 0.0f, 1.0f), cFast);
        smooth (sWidth, std::clamp (targets.width, 0.0f, 1.6f), cFast);
        smooth (sDelayMs, std::clamp (targets.delayMs, 20.0f, 2000.0f), cSlow);
        smooth (sWarp, std::clamp (targets.warp, 0.0f, 1.0f), cSlow);
        smooth (sFeedback, std::clamp (targets.feedback, 0.0f, 1.0f), cFast);
        smooth (sDensity, std::clamp (targets.density, 0.0f, 1.0f), cFast);
        smooth (sModRate, std::clamp (targets.modRateHz, 0.02f, 8.0f), cSlow);
        smooth (sModDepth, std::clamp (targets.modDepth, 0.0f, 1.0f), cFast);
        smooth (sBloom, std::clamp (targets.bloom, 0.0f, 1.0f), cSlow);
        smooth (sBloomFb, std::clamp (targets.bloomFeedback, 0.0f, 1.0f), cFast);
        smooth (sPreMs, std::clamp (targets.predelayMs, 0.0f, 250.0f), cSlow);
        smooth (sTone, std::clamp (targets.tone, 0.0f, 1.0f), cFast);
        smooth (sLow, std::clamp (targets.lowEndDb, -12.0f, 12.0f), cFast);
        smooth (sDrive, std::clamp (targets.drive, 0.0f, 1.0f), cFast);
        smooth (sGlide, std::clamp (targets.glide, 0.0f, 1.0f), cFast);

        const bool loveless = targets.mode == CavernMode::Loveless;
        smooth (sRevMix, loveless ? 0.42f : 0.0f, cFast);
        smooth (sLagMix, loveless ? 1.0f : 0.0f, cFast);

        if ((filterTick++ & 31) == 0)
        {
            float outHz = 1400.0f * std::pow (17000.0f / 1400.0f, sTone);

            if (targets.mode == CavernMode::Massive)
                outHz *= 1.25f;
            else if (targets.mode == CavernMode::Loveless)
                outHz *= 0.85f;

            outHz = std::clamp (outHz, 700.0f, 18000.0f);
            const float q = 0.70f + (1.0f - sTone) * 0.60f;
            toneL.setLowpass (sampleRate, outHz, q);
            toneR.setLowpass (sampleRate, outHz, q);
            shelfL.setLowShelf (sampleRate, 180.0f, sLow);
            shelfR.setLowShelf (sampleRate, 180.0f, sLow);

            float loopHz = std::min (18000.0f, outHz * 1.55f);
            dampCoef = 1.0f - std::exp (-kTwoPi * loopHz / sr);
        }

        const float xL = inL[n];
        const float xR = inR[n];

        float dryL = xL;
        float dryR = xR;

        if (sDrive > 0.001f)
        {
            const float grit = 0.45f * sDrive;
            dryL += (saturateDrive (xL, sDrive) - xL) * grit;
            dryR += (saturateDrive (xR, sDrive) - xR) * grit;
        }

        const float sendInL = saturateDrive (dcL.process (xL), sDrive);
        const float sendInR = saturateDrive (dcR.process (xR), sDrive);

        const float preSampL = std::clamp (sPreMs * 0.001f * sr, 2.0f, static_cast<float> (preL.getSize() - 6));
        const float preSampR = std::clamp (preSampL * 1.13f + 0.4f * sr * 0.001f, 2.0f, static_cast<float> (preR.getSize() - 6));
        const float sendL = preL.read (preSampL);
        const float sendR = preR.read (preSampR);
        preL.push (sendInL);
        preR.push (sendInR);

        float bloomSec = bloomLengthSeconds (sBloom);

        if (targets.mode == CavernMode::Inertia)
            bloomSec *= (1.35f - 0.9f * sFeedback);

        bloomSec = std::clamp (bloomSec, 0.02f, 1.35f);
        const float preSec = sPreMs * 0.001f;

        const float rate = static_cast<float> (sModRate / sampleRate);
        const float rate2 = rate * 1.71f;

        for (int i = 0; i < 8; ++i)
        {
            bloomPhase[i] += rate;

            if (bloomPhase[i] >= 1.0)
                bloomPhase[i] -= 1.0;

            tankPhase[i] += static_cast<double> (rate);

            if (tankPhase[i] >= 1.0)
                tankPhase[i] -= 1.0;

            tankPhase2[i] += static_cast<double> (rate2);

            if (tankPhase2[i] >= 1.0)
                tankPhase2[i] -= 1.0;
        }

        const float glideRateL = (0.12f + sGlide * 0.85f) / sr;
        const float glideRateR = (0.14f + sGlide * 0.95f) / sr;
        glidePhaseL += glideRateL;

        if (glidePhaseL >= 1.0)
            glidePhaseL -= 1.0;

        glidePhaseR += glideRateR;

        if (glidePhaseR >= 1.0)
            glidePhaseR -= 1.0;

        // Reverse grains. Hann windows a half-grain apart sum to about 1.
        float revOutL = 0.0f;
        float revOutR = 0.0f;
        const float grain = std::clamp (bloomSec * 0.65f * sr, 64.0f, static_cast<float> (revL.getSize() / 2));
        const float revStep = 1.0f / grain;

        if (sRevMix > 0.0008f)
        {
            auto grainAt = [&] (double phase, float& l, float& r)
            {
                const float p = static_cast<float> (phase);
                const float env = 0.5f * (1.0f - std::cos (kTwoPi * p));
                const float delay = 1.0f + p * grain;
                l = revL.read (delay) * env;
                r = revR.read (delay) * env;
            };

            float l0, r0, l1, r1;
            grainAt (revPhase0, l0, r0);
            grainAt (revPhase1, l1, r1);
            revOutL = l0 + l1;
            revOutR = r0 + r1;
        }

        revPhase0 += revStep;

        if (revPhase0 >= 1.0)
            revPhase0 -= 1.0;

        revPhase1 += revStep;

        if (revPhase1 >= 1.0)
            revPhase1 -= 1.0;

        revL.push (sendL);
        revR.push (sendR);

        const float srcL = sendL * (1.0f - sRevMix) + revOutL * sRevMix;
        const float srcR = sendR * (1.0f - sRevMix) + revOutR * sRevMix;

        const float bfb = std::pow (std::clamp (sBloomFb, 0.0f, 1.0f), 0.70f) * 0.70f;
        const float loopInL = srcL + bfb * bloomFbL;
        const float loopInR = srcR + bfb * bloomFbR;

        float tapsL = 0.0f;
        float tapsR = 0.0f;
        const float glideBloom = sine (glidePhaseL) * sGlide * 0.010f * sr;

        for (int i = 0; i < kBloomTaps; ++i)
        {
            const float pos = std::pow (static_cast<float> (i + 1) / static_cast<float> (kBloomTaps), 0.92f);
            const float gain = 0.18f + 0.82f * (static_cast<float> (i) / static_cast<float> (kBloomTaps - 1));
            const float wobble = sine (bloomPhase[i & 7]) * sModDepth * 0.0045f * sr;
            const float tL = bloomSec * pos * sr + wobble + glideBloom;
            const float tR = bloomSec * pos * sr * 1.06f + wobble * 0.85f - glideBloom;
            tapsL += gain * bloomL.read (std::clamp (tL, 3.0f, static_cast<float> (bloomL.getSize() - 6)));
            tapsR += gain * bloomR.read (std::clamp (tR, 3.0f, static_cast<float> (bloomR.getSize() - 6)));
        }

        tapsL *= kTapScale;
        tapsR *= kTapScale;
        bloomL.push (loopInL);
        bloomR.push (loopInR);

        float diffusedL = tapsL;
        float diffusedR = tapsR;

        for (int i = 0; i < 4; ++i)
        {
            const float d = kBloomApMs[i] * 0.001f * sr;
            diffusedL = bloomApL[i].process (diffusedL, d, kBloomApG[i]);
            diffusedR = bloomApR[i].process (diffusedR, d, kBloomApG[i]);
        }

        bloomFbL = loopShape (bloomDampL.process (diffusedL, std::min (0.35f, dampCoef + 0.05f)));
        bloomFbR = loopShape (bloomDampR.process (diffusedR, std::min (0.35f, dampCoef + 0.05f)));

        // Taps keep the rising envelope. A little of the diffusor smears them
        // without putting the attack back in front.
        const float cloudL = tapsL * 0.28f + diffusedL * 0.72f;
        const float cloudR = tapsR * 0.28f + diffusedR * 0.72f;
        bloomLevel = std::max (bloomLevel * 0.9995f, 0.5f * (std::abs (cloudL) + std::abs (cloudR)));

        const float direct = directAmount (targets.mode, sBloom);
        const float tankL = direct * srcL + (1.0f - direct) * tapsL;
        const float tankR = direct * srcR + (1.0f - direct) * tapsR;

        float delayed[8];
        const float baseDelay = sDelayMs * 0.001f * sr;
        const float modSamp = sModDepth * 0.0075f * sr;
        const float glideSamp = sGlide * 0.014f * sr;

        for (int i = 0; i < 8; ++i)
        {
            const float ratio = 1.0f + (kRatios[i] - 1.0f) * sWarp;
            const float skew = ((i < 4) ? -1.0f : 1.0f) * (4.0f + static_cast<float> (i) * 3.0f);
            const float lfo = sine (tankPhase[i]) * 0.75f + sine (tankPhase2[i]) * 0.25f;
            const float bend = sine (glidePhaseL + static_cast<double> (i) * 0.17) * glideSamp;
            const float maxD = static_cast<float> (fdn[i].getSize() - 6);
            const float center = std::clamp (baseDelay * ratio + skew, 8.0f, maxD);
            const float excursion = std::min (modSamp + std::abs (glideSamp), center * 0.45f);
            const float d = std::clamp (center + lfo * std::min (modSamp, excursion) + std::clamp (bend, -excursion, excursion), 8.0f, maxD);
            delayed[i] = fdn[i].read (d);
        }

        float dense[8];
        grandCavernsHadamard8 (delayed, dense);

        float mixed[8];

        for (int i = 0; i < 8; ++i)
            mixed[i] = delayed[i] * kSign[i] * (1.0f - sDensity) + dense[i] * sDensity;

        const float fb = shapedFeedback (sFeedback);
        float inject[8];

        for (int i = 0; i < 4; ++i)
        {
            inject[i] = tankL * kInGainL[i] * kInject;
            inject[i + 4] = tankR * kInGainR[i] * kInject;
        }

        for (int i = 0; i < 8; ++i)
        {
            const float damped = fdnDamp[i].process (mixed[i], dampCoef);
            const float apDelay = kFdnApMs[i] * 0.001f * sr * (0.75f + 0.5f * sDensity);
            const float diffused = fdnAp[i].process (damped, apDelay, 0.60f);
            const float fbSig = loopShape (damped + (diffused - damped) * sDensity);
            fdn[i].push (inject[i] + fb * fbSig);
        }

        float fdnL = 0.0f;
        float fdnR = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            const float heardL = delayed[i] * (1.0f - sDensity) + dense[i] * sDensity;
            const float heardR = delayed[i + 4] * (1.0f - sDensity) + dense[i + 4] * sDensity;
            fdnL += heardL;
            fdnR += heardR;
        }

        fdnL *= 0.50f;
        fdnR *= 0.50f;

        float erAccL = 0.0f;
        float erAccR = 0.0f;

        for (int i = 0; i < 4; ++i)
        {
            erAccL += kErGain[i] * erL.read (kErMsL[i] * 0.001f * sr);
            erAccR += kErGain[i] * erR.read (kErMsR[i] * 0.001f * sr);
        }

        erL.push (sendL);
        erR.push (sendR);

        float erAmt = 0.85f * (1.0f - 0.70f * sBloom);

        if (loveless)
            erAmt *= 0.40f;

        const float cloudGain = (1.0f - direct) * 0.48f;
        float wetL = cloudL * cloudGain + fdnL + erAccL * erAmt;
        float wetR = cloudR * cloudGain + fdnR + erAccR * erAmt;

        wetL = shelfL.process (toneL.process (wetL));
        wetR = shelfR.process (toneR.process (wetR));

        const float gBase = 0.018f * sr;
        const float gDepth = sGlide * 0.012f * sr;
        const float chorL = glideL.read (std::clamp (gBase + gDepth * sine (glidePhaseL), 3.0f, static_cast<float> (glideL.getSize() - 6)));
        const float chorR = glideR.read (std::clamp (gBase + gDepth * sine (glidePhaseR), 3.0f, static_cast<float> (glideR.getSize() - 6)));
        glideL.push (wetL);
        glideR.push (wetR);
        const float gAmt = sGlide * 0.45f;
        wetL = wetL * (1.0f - gAmt) + chorL * gAmt;
        wetR = wetR * (1.0f - gAmt) + chorR * gAmt;

        const float mid = 0.5f * (wetL + wetR);
        const float side = 0.5f * (wetL - wetR) * sWidth;
        wetL = mid + side;
        wetR = mid - side;

        const float lagSamp = std::clamp ((bloomSec + preSec) * sr, 2.0f, static_cast<float> (dryLagL.getSize() - 6));
        const float laggedL = dryLagL.read (lagSamp);
        const float laggedR = dryLagR.read (lagSamp);
        dryLagL.push (dryL);
        dryLagR.push (dryR);
        dryL = dryL * (1.0f - sLagMix) + laggedL * sLagMix;
        dryR = dryR * (1.0f - sLagMix) + laggedR * sLagMix;

        const float dryG = std::cos (sMix * kHalfPi);
        const float wetG = std::sin (sMix * kHalfPi);
        const float yL = softLimit (dryL * dryG + wetL * wetG);
        const float yR = softLimit (dryR * dryG + wetR * wetG);

        wetLevel = std::max (wetLevel * 0.9995f, 0.5f * (std::abs (wetL) + std::abs (wetR)));
        outL[n] = yL;
        outR[n] = yR;
    }
}
