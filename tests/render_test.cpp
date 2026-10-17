#include "DSP/Filters.h"
#include "DSP/GrandCavernsEngine.h"
#include "GrandCavernsPresets.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace
{
int failures = 0;

void expect (bool ok, const std::string& message)
{
    if (ok)
    {
        std::cout << "  ok  " << message << "\n";
        return;
    }

    std::cout << "  FAIL " << message << "\n";
    ++failures;
}

float peakRange (const std::vector<float>& interleaved, int start, int end)
{
    start = std::max (0, start);
    end = std::min (end, static_cast<int> (interleaved.size()));
    float peak = 0.0f;

    for (int i = start; i < end; ++i)
        peak = std::max (peak, std::abs (interleaved[static_cast<size_t> (i)]));

    return peak;
}

float rmsRange (const std::vector<float>& interleaved, int start, int end)
{
    start = std::max (0, start);
    end = std::min (end, static_cast<int> (interleaved.size()));
    double sum = 0.0;
    int count = 0;

    for (int i = start; i < end; ++i)
    {
        const double s = interleaved[static_cast<size_t> (i)];
        sum += s * s;
        ++count;
    }

    if (count == 0)
        return 0.0f;

    return static_cast<float> (std::sqrt (sum / count));
}

bool allFinite (const std::vector<float>& buffer)
{
    for (float sample : buffer)
        if (! std::isfinite (sample))
            return false;

    return true;
}

float peakOf (const std::vector<float>& buffer)
{
    float peak = 0.0f;

    for (float sample : buffer)
        peak = std::max (peak, std::abs (sample));

    return peak;
}

int peakIndex (const std::vector<float>& interleaved)
{
    int best = 0;
    float bestAbs = 0.0f;

    for (int i = 0; i < static_cast<int> (interleaved.size()); i += 2)
    {
        const float m = std::max (std::abs (interleaved[static_cast<size_t> (i)]),
                                   std::abs (interleaved[static_cast<size_t> (i + 1)]));

        if (m > bestAbs)
        {
            bestAbs = m;
            best = i / 2;
        }
    }

    return best;
}

void writeWav (const std::filesystem::path& path, const std::vector<float>& interleaved, int sampleRate)
{
    std::filesystem::create_directories (path.parent_path());
    const float peak = std::max (1.0e-6f, peakOf (interleaved));
    const float gain = 0.89f / peak;

    std::vector<int16_t> pcm (interleaved.size());

    for (size_t i = 0; i < interleaved.size(); ++i)
    {
        const float scaled = std::clamp (interleaved[i] * gain, -1.0f, 1.0f);
        pcm[i] = static_cast<int16_t> (std::lrintf (scaled * 32767.0f));
    }

    const uint32_t dataBytes = static_cast<uint32_t> (pcm.size() * sizeof (int16_t));
    const uint32_t rate = static_cast<uint32_t> (sampleRate);
    const uint16_t channels = 2;
    const uint16_t bits = 16;
    const uint32_t byteRate = rate * channels * bits / 8;
    const uint16_t blockAlign = channels * bits / 8;
    const uint32_t chunk = 36 + dataBytes;

    std::ofstream out (path, std::ios::binary);
    out.write ("RIFF", 4);
    out.write (reinterpret_cast<const char*> (&chunk), 4);
    out.write ("WAVE", 4);
    out.write ("fmt ", 4);
    const uint32_t fmtSize = 16;
    const uint16_t format = 1;
    out.write (reinterpret_cast<const char*> (&fmtSize), 4);
    out.write (reinterpret_cast<const char*> (&format), 2);
    out.write (reinterpret_cast<const char*> (&channels), 2);
    out.write (reinterpret_cast<const char*> (&rate), 4);
    out.write (reinterpret_cast<const char*> (&byteRate), 4);
    out.write (reinterpret_cast<const char*> (&blockAlign), 2);
    out.write (reinterpret_cast<const char*> (&bits), 2);
    out.write ("data", 4);
    out.write (reinterpret_cast<const char*> (&dataBytes), 4);
    out.write (reinterpret_cast<const char*> (pcm.data()), static_cast<std::streamsize> (dataBytes));
}

std::vector<float> render (GrandCavernsEngine& engine, const std::vector<float>& input, int sampleRate)
{
    const int frames = static_cast<int> (input.size()) / 2;
    std::vector<float> left (static_cast<size_t> (frames));
    std::vector<float> right (static_cast<size_t> (frames));
    std::vector<float> outL (static_cast<size_t> (frames));
    std::vector<float> outR (static_cast<size_t> (frames));

    for (int i = 0; i < frames; ++i)
    {
        left[static_cast<size_t> (i)] = input[static_cast<size_t> (i * 2)];
        right[static_cast<size_t> (i)] = input[static_cast<size_t> (i * 2 + 1)];
    }

    constexpr int block = 256;

    for (int offset = 0; offset < frames; offset += block)
    {
        const int count = std::min (block, frames - offset);
        engine.process (left.data() + offset, right.data() + offset, outL.data() + offset, outR.data() + offset, count);
    }

    std::vector<float> interleaved (static_cast<size_t> (frames * 2));

    for (int i = 0; i < frames; ++i)
    {
        interleaved[static_cast<size_t> (i * 2)] = outL[static_cast<size_t> (i)];
        interleaved[static_cast<size_t> (i * 2 + 1)] = outR[static_cast<size_t> (i)];
    }

    (void) sampleRate;
    return interleaved;
}

void prepare (GrandCavernsEngine& engine, const GrandCavernsParams& params, double sampleRate)
{
    engine.prepare (sampleRate);
    engine.setTargets (params);
    engine.snapToTargets();
}

void testFilters()
{
    std::cout << "filters\n";
    std::mt19937 rng (7);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);

    for (int n = 0; n < 16; ++n)
    {
        float in[8], out[8];
        double energyIn = 0.0;

        for (float& sample : in)
        {
            sample = dist (rng);
            energyIn += static_cast<double> (sample) * sample;
        }

        grandCavernsHadamard8 (in, out);
        double energyOut = 0.0;

        for (float sample : out)
            energyOut += static_cast<double> (sample) * sample;

        if (std::abs (energyOut - energyIn) > 1.0e-4)
        {
            expect (false, "Hadamard keeps the vector length");
            return;
        }
    }

    expect (true, "Hadamard keeps the vector length");

    Allpass allpass;
    allpass.prepare (8000);
    double energy = 0.0;

    for (int i = 0; i < 8000; ++i)
    {
        const float x = i == 0 ? 1.0f : 0.0f;
        const float y = allpass.process (x, 97.0f, 0.7f);
        energy += static_cast<double> (y) * y;
    }

    expect (std::abs (energy - 1.0) < 0.08, "allpass impulse energy is unity (" + std::to_string (energy) + ")");

    allpass.clear();
    float last = 0.0f;

    for (int i = 0; i < 4000; ++i)
        last = allpass.process (1.0f, 40.0f, 0.6f);

    expect (std::abs (last - 1.0f) < 0.02f, "allpass DC gain is unity");

    Biquad shelf;
    shelf.setLowShelf (48000.0, 180.0f, 12.0f);
    double inSum = 0.0;
    double outSum = 0.0;

    for (int i = 0; i < 48000; ++i)
    {
        const float x = std::sin (2.0f * 3.14159265f * 80.0f * static_cast<float> (i) / 48000.0f);
        const float y = shelf.process (x);

        if (i > 8000)
        {
            inSum += static_cast<double> (x) * x;
            outSum += static_cast<double> (y) * y;
        }
    }

    const float ratio = static_cast<float> (std::sqrt (outSum / inSum));
    expect (ratio > 3.4f && ratio < 4.6f, "low shelf +12 dB is about 4x (" + std::to_string (ratio) + ")");

    Biquad lowpass;
    lowpass.setLowpass (48000.0, 400.0f, 0.7f);
    inSum = outSum = 0.0;

    for (int i = 0; i < 48000; ++i)
    {
        const float x = std::sin (2.0f * 3.14159265f * 6000.0f * static_cast<float> (i) / 48000.0f);
        const float y = lowpass.process (x);

        if (i > 8000)
        {
            inSum += static_cast<double> (x) * x;
            outSum += static_cast<double> (y) * y;
        }
    }

    const float dark = static_cast<float> (std::sqrt (outSum / std::max (1.0e-12, inSum)));
    expect (dark < 0.05f, "lowpass kills a 6 kHz sine (" + std::to_string (dark) + ")");
}

std::vector<float> impulse (int frames)
{
    std::vector<float> input (static_cast<size_t> (frames * 2), 0.0f);
    input[0] = 1.0f;
    input[1] = 1.0f;
    return input;
}

void testBloomShape()
{
    std::cout << "bloom shape\n";
    constexpr int rate = 48000;
    constexpr int frames = rate * 2;

    auto run = [&] (CavernMode mode, float bloom)
    {
        GrandCavernsParams params;
        params.mode = mode;
        params.mix = 1.0f;
        params.delayMs = 1500.0f;
        params.warp = 0.0f;
        params.feedback = 0.25f;
        params.density = 0.7f;
        params.modDepth = 0.0f;
        params.modRateHz = 0.2f;
        params.bloom = bloom;
        params.bloomFeedback = 0.15f;
        params.predelayMs = 0.0f;
        params.tone = 1.0f;
        params.lowEndDb = 0.0f;
        params.drive = 0.0f;
        params.glide = 0.0f;
        params.width = 1.0f;

        GrandCavernsEngine engine;
        prepare (engine, params, rate);
        return render (engine, impulse (frames), rate);
    };

    const auto bloom = run (CavernMode::Bloom, 0.85f);
    const auto massive = run (CavernMode::Massive, 0.05f);
    const float bloomEarly = peakRange (bloom, 0, static_cast<int> (0.05 * rate) * 2);
    const float bloomLate = peakRange (bloom, static_cast<int> (0.55 * rate) * 2, static_cast<int> (0.80 * rate) * 2);
    const float massEarly = peakRange (massive, 0, static_cast<int> (0.05 * rate) * 2);
    const float massLate = peakRange (massive, static_cast<int> (0.55 * rate) * 2, static_cast<int> (0.80 * rate) * 2);

    std::cout << "  bloom peak dB";

    for (int window = 0; window < 10; ++window)
    {
        const float level = peakRange (bloom, static_cast<int> (window * 0.1 * rate) * 2, static_cast<int> ((window + 1) * 0.1 * rate) * 2);
        std::cout << " " << static_cast<int> (std::round (20.0 * std::log10 (level + 1.0e-9)));
    }

    std::cout << "\n  bloom late " << bloomLate << " early " << bloomEarly
              << "   massive late " << massLate << " early " << massEarly << "\n";
    expect (allFinite (bloom) && allFinite (massive), "impulse responses are finite");
    expect (bloomLate > bloomEarly * 1.3f, "bloom peak arrives with the swell, not the first 50 ms");
    expect (bloomLate / std::max (1.0e-8f, bloomEarly) > massLate / std::max (1.0e-8f, massEarly) * 1.4f,
            "massive stays quicker than the bloom cloud");
}

void testLovelessLead()
{
    std::cout << "loveless lead\n";
    constexpr int rate = 48000;
    GrandCavernsParams params;
    params.mode = CavernMode::Loveless;
    params.mix = 0.0f;
    params.bloom = 0.50f;
    params.predelayMs = 0.0f;
    params.drive = 0.0f;
    params.glide = 0.0f;
    params.modDepth = 0.0f;

    GrandCavernsEngine engine;
    prepare (engine, params, rate);
    const auto output = render (engine, impulse (rate * 2), rate);
    const float lead = lovelessLeadSeconds (0.50f, 0.0f);
    const int at = peakIndex (output);
    const float atSec = static_cast<float> (at) / static_cast<float> (rate);
    std::cout << "  dry peak at " << atSec << " s, lead " << lead << " s\n";
    expect (std::abs (atSec - lead) < 0.03f, "dry note waits for the bloom");
    expect (engine.getLatencySamples() > 1000, "plugin reports the dry lead as latency");
}

void testDecayAndStability()
{
    std::cout << "stability\n";
    constexpr int rate = 48000;
    std::mt19937 rng (3);
    std::uniform_real_distribution<float> noise (-0.35f, 0.35f);

    for (int mode = 0; mode < static_cast<int> (CavernMode::Count); ++mode)
    {
        GrandCavernsParams params;
        params.mode = static_cast<CavernMode> (mode);
        params.mix = 1.0f;
        params.delayMs = 640.0f;
        params.warp = 0.7f;
        params.feedback = 0.95f;
        params.density = 0.85f;
        params.modRateHz = 0.4f;
        params.modDepth = 0.7f;
        params.bloom = 0.7f;
        params.bloomFeedback = 0.6f;
        params.predelayMs = 20.0f;
        params.tone = 0.4f;
        params.drive = 0.4f;
        params.glide = 0.5f;
        params.width = 1.3f;

        constexpr int frames = rate * 4;
        std::vector<float> input (static_cast<size_t> (frames * 2));

        for (int i = 0; i < frames; ++i)
        {
            const float x = i < rate ? noise (rng) : 0.0f;
            input[static_cast<size_t> (i * 2)] = x;
            input[static_cast<size_t> (i * 2 + 1)] = x * 0.8f;
        }

        GrandCavernsEngine engine;
        prepare (engine, params, rate);
        const auto output = render (engine, input, rate);
        const float earlyTail = rmsRange (output, static_cast<int> (1.5 * rate) * 2, static_cast<int> (2.0 * rate) * 2);
        const float later = rmsRange (output, static_cast<int> (3.2 * rate) * 2, static_cast<int> (3.8 * rate) * 2);
        const float peak = peakOf (output);
        std::cout << "  " << cavernModeName (params.mode) << " peak " << peak << " earlyTail " << earlyTail << " later " << later << "\n";
        expect (allFinite (output), std::string (cavernModeName (params.mode)) + " stays finite");
        expect (peak < 1.2f, std::string (cavernModeName (params.mode)) + " stays inside the limiter");
        expect (later < earlyTail, std::string (cavernModeName (params.mode)) + " decays after the input stops");
    }
}

struct Karplus
{
    std::vector<float> buf;
    int index = 0;

    void strike (double sampleRate, float freq, std::mt19937& rng)
    {
        std::uniform_real_distribution<float> noise (-1.0f, 1.0f);
        const int n = std::max (8, static_cast<int> (sampleRate / static_cast<double> (freq)));
        buf.assign (static_cast<size_t> (n), 0.0f);

        for (int i = 0; i < n; ++i)
            buf[static_cast<size_t> (i)] = noise (rng);

        index = 0;
    }

    float next()
    {
        if (buf.empty())
            return 0.0f;

        const int j = (index + 1) % static_cast<int> (buf.size());
        const float y = 0.996f * 0.5f * (buf[static_cast<size_t> (index)] + buf[static_cast<size_t> (j)]);
        buf[static_cast<size_t> (index)] = y;
        index = j;
        return y;
    }
};

void renderDemos()
{
    std::cout << "demos\n";
    constexpr int rate = 48000;
    constexpr int frames = rate * 7;
    std::mt19937 rng (11);
    const auto outDir = std::filesystem::path ("tests/out");

    const float freqs[] = { 196.0f, 247.0f, 294.0f, 392.0f };

    for (const auto& preset : factoryPresets())
    {
        std::vector<Karplus> strings (4);
        std::vector<float> input (static_cast<size_t> (frames * 2), 0.0f);
        const int strums[] = { static_cast<int> (0.15 * rate), static_cast<int> (2.4 * rate), static_cast<int> (4.5 * rate) };
        int active = -1;

        for (int i = 0; i < frames; ++i)
        {
            for (int strum : strums)
            {
                if (i != strum)
                    continue;

                for (size_t s = 0; s < strings.size(); ++s)
                    strings[s].strike (rate, freqs[s] * (s == 0 ? 1.0f : 1.003f), rng);

                active = strum;
            }

            float sample = 0.0f;

            if (active >= 0)
            {
                for (auto& string : strings)
                    sample += string.next();

                sample *= 0.22f;
                const int age = i - active;

                if (age < 180)
                    sample += (static_cast<float> (rng() % 1000) / 500.0f - 1.0f) * 0.15f * (1.0f - static_cast<float> (age) / 180.0f);
            }

            input[static_cast<size_t> (i * 2)] = sample;
            input[static_cast<size_t> (i * 2 + 1)] = sample * 0.97f;
        }

        GrandCavernsEngine engine;
        prepare (engine, preset.params, rate);
        auto output = render (engine, input, rate);
        const float peak = peakOf (output);
        const float tail = rmsRange (output, static_cast<int> (6.2 * rate) * 2, static_cast<int> (6.9 * rate) * 2);
        std::cout << "  " << preset.name << " raw peak " << peak << " end rms " << tail << "\n";
        expect (allFinite (output) && peak > 0.02f && peak < 1.2f, std::string (preset.name) + " demo is in a usable range");

        std::string filename = preset.name;

        for (char& character : filename)
            if (character == ' ')
                character = '_';

        writeWav (outDir / (filename + ".wav"), output, rate);
    }
}
} // namespace

void testPresetNames()
{
    std::cout << "preset names\n";

    for (const auto& preset : factoryPresets())
    {
        const bool repeatsAlgorithm = std::string (preset.name) == cavernModeName (preset.params.mode);
        expect (! repeatsAlgorithm, std::string (preset.name) + " is a setting, not the algorithm name");
    }
}

int main()
{
    testPresetNames();
    testFilters();
    testBloomShape();
    testLovelessLead();
    testDecayAndStability();
    renderDemos();

    if (failures != 0)
    {
        std::cout << failures << " failed\n";
        return 1;
    }

    std::cout << "all checks passed\n";
    return 0;
}
