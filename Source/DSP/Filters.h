#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

// Small realtime helpers. Allpass and Hadamard routines are the standard
// unitary building blocks — Schroeder allpass and a normalised Walsh–Hadamard
// mix — not code from any commercial reverb.

struct DelayLine
{
    void setSize (int samples)
    {
        size = std::max (8, samples);
        buf.assign (static_cast<size_t> (size), 0.0f);
        write = 0;
    }

    void clear()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        write = 0;
    }

    void push (float x)
    {
        if (size < 8)
            return;

        if (std::abs (x) < 1.0e-12f)
            x = 0.0f;

        buf[static_cast<size_t> (write)] = x;

        if (++write >= size)
            write = 0;
    }

    float read (float delaySamples) const
    {
        if (size < 8)
            return 0.0f;

        const float limit = static_cast<float> (size - 4);
        float d = std::clamp (delaySamples, 2.0f, limit);
        float rp = static_cast<float> (write) - d;

        while (rp < 0.0f)
            rp += static_cast<float> (size);

        while (rp >= static_cast<float> (size))
            rp -= static_cast<float> (size);

        const int i1 = static_cast<int> (rp);
        const float f = rp - static_cast<float> (i1);

        auto at = [&] (int i)
        {
            i %= size;

            if (i < 0)
                i += size;

            return buf[static_cast<size_t> (i)];
        };

        const float y0 = at (i1 - 1);
        const float y1 = at (i1);
        const float y2 = at (i1 + 1);
        const float y3 = at (i1 + 2);
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + c0;
    }

    int getSize() const { return size; }

    std::vector<float> buf;
    int write = 0;
    int size = 0;
};

struct Allpass
{
    void prepare (int samples) { line.setSize (samples); }
    void clear() { line.clear(); }

    // Unity-gain Schroeder section: y = g x + w[n-d], w = x - g y.
    float process (float x, float delaySamples, float g)
    {
        const float wDel = line.read (delaySamples);
        const float y = g * x + wDel;
        line.push (x - g * y);
        return y;
    }

    DelayLine line;
};

struct OnePole
{
    void clear() { z = 0.0f; }

    float process (float x, float coef)
    {
        z += coef * (x - z);

        if (std::abs (z) < 1.0e-20f)
            z = 0.0f;

        return z;
    }

    float z = 0.0f;
};

struct DCBlock
{
    void clear() { x1 = y1 = 0.0f; }

    float process (float x)
    {
        const float y = x - x1 + 0.995f * y1;
        x1 = x;
        y1 = y;

        if (std::abs (y1) < 1.0e-20f)
            y1 = 0.0f;

        return y;
    }

    float x1 = 0.0f;
    float y1 = 0.0f;
};

struct Biquad
{
    void clear() { s1 = s2 = 0.0f; }

    void setLowpass (double sampleRate, float freq, float q)
    {
        freq = std::clamp (freq, 20.0f, static_cast<float> (sampleRate * 0.45));
        q = std::max (0.15f, q);
        const float w0 = 6.28318530718f * freq / static_cast<float> (sampleRate);
        const float cosw = std::cos (w0);
        const float sinw = std::sin (w0);
        const float alpha = sinw / (2.0f * q);
        const float a0 = 1.0f + alpha;
        const float nb0 = (1.0f - cosw) * 0.5f;
        const float nb1 = 1.0f - cosw;
        const float nb2 = nb0;
        const float na1 = -2.0f * cosw;
        const float na2 = 1.0f - alpha;
        assign (nb0, nb1, nb2, na1, na2, a0);
    }

    void setLowShelf (double sampleRate, float freq, float dB)
    {
        const float A = std::pow (10.0f, dB / 40.0f);
        const float clamped = std::clamp (freq, 20.0f, static_cast<float> (sampleRate * 0.45));
        const float w0 = 6.28318530718f * clamped / static_cast<float> (sampleRate);
        const float cosw = std::cos (w0);
        const float sinw = std::sin (w0);
        constexpr float S = 1.0f;
        const float alpha = sinw * 0.5f * std::sqrt ((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);
        const float twoSqrtAAlpha = 2.0f * std::sqrt (A) * alpha;
        const float Ap1 = A + 1.0f;
        const float Am1 = A - 1.0f;
        const float a0 = Ap1 + Am1 * cosw + twoSqrtAAlpha;
        const float nb0 = A * (Ap1 - Am1 * cosw + twoSqrtAAlpha);
        const float nb1 = 2.0f * A * (Am1 - Ap1 * cosw);
        const float nb2 = A * (Ap1 - Am1 * cosw - twoSqrtAAlpha);
        const float na1 = -2.0f * (Am1 + Ap1 * cosw);
        const float na2 = Ap1 + Am1 * cosw - twoSqrtAAlpha;
        assign (nb0, nb1, nb2, na1, na2, a0);
    }

    float process (float x)
    {
        const float y = b0 * x + s1;
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        return y;
    }

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float s1 = 0.0f, s2 = 0.0f;

private:
    void assign (float nb0, float nb1, float nb2, float na1, float na2, float a0)
    {
        b0 = nb0 / a0;
        b1 = nb1 / a0;
        b2 = nb2 / a0;
        a1 = na1 / a0;
        a2 = na2 / a0;
    }
};

inline void grandCavernsHadamard8 (const float* in, float* out)
{
    float a[8];

    for (int i = 0; i < 8; ++i)
        a[i] = in[i];

    for (int len = 1; len < 8; len <<= 1)
    {
        for (int i = 0; i < 8; i += len * 2)
        {
            for (int j = 0; j < len; ++j)
            {
                const float u = a[i + j];
                const float v = a[i + j + len];
                a[i + j] = u + v;
                a[i + j + len] = u - v;
            }
        }
    }

    constexpr float scale = 0.3535533905932738f; // 1 / sqrt(8)

    for (int i = 0; i < 8; ++i)
        out[i] = a[i] * scale;
}

inline float saturateDrive (float x, float drive)
{
    if (drive < 0.001f)
        return x;

    const float g = 1.0f + drive * 8.0f;
    return std::tanh (x * g) / std::tanh (g);
}

// Small-signal gain is 1, so decay stays linear until the loop gets hot.
inline float loopShape (float x)
{
    constexpr float d = 1.15f;
    return std::tanh (x * d) / d;
}

inline float softLimit (float x)
{
    constexpr float t = 0.92f;

    if (x > t)
        return t + std::tanh (x - t) * (1.0f - t);

    if (x < -t)
        return -t - std::tanh (-x - t) * (1.0f - t);

    return x;
}
