/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIUM-23A) */
/* Small DSP blocks shared by the engine and the effector: fast maths, noise, band-limited steps, delay lines. */
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace sol
{
constexpr float PI = 3.14159265358979f, TWO_PI = 6.28318530717959f;

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
/* tanh, close enough for saturation (|err| < 0.002), no libm in the inner loops */
inline float ftanh(float x)
{
    x = clampf(x, -4.5f, 4.5f);
    float x2 = x * x;
    return clampf(x * (27.0f + x2) / (27.0f + 9.0f * x2), -1.0f, 1.0f);
}
/* 2^x for |x| < 30 (rel. 2e-5) */
inline float fexp2(float x)
{
    x = clampf(x, -30.0f, 30.0f);
    float fl = std::floor(x), f = x - fl;
    float p = 1.0f + f * (0.6931472f + f * (0.2402265f + f * (0.0555041f + f * (0.0096181f + f * 0.0013334f))));
    union { float f; int32_t i; } b{p};
    b.i += (int32_t)fl << 23;
    return b.f;
}
/* sin(2 pi p) for a phase 0..1 (parabolic, |err| < 0.001) */
inline float fsin01(float p)
{
    float x = p - std::floor(p);
    x = x < 0.5f ? x : x - 1.0f;     /* -0.5..0.5 */
    float y = 8.0f * x - 16.0f * x * std::fabs(x);
    return 0.225f * (y * std::fabs(y) - y) + y;
}

struct Rng {
    uint32_t s = 0x9e3779b9u;
    void seed(uint32_t x) { s = x ? x : 0x1234567u; }
    uint32_t next()
    {
        s ^= s << 13;
        s ^= s >> 17;
        s ^= s << 5;
        return s;
    }
    float uni() { return (float)(next() >> 8) * (1.0f / 16777216.0f); }   /* 0..1 */
    float bi() { return uni() * 2.0f - 1.0f; }
};

/* the band-limited step and its corner (PolyBLEP, two samples) */
inline float blep(float t, float dt)
{
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt) {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

struct OnePole {
    float z = 0.0f;
    float lp(float x, float a) { return z += a * (x - z); }
};
inline float onePoleCoef(float hz, float sr) { return 1.0f - std::exp(-TWO_PI * hz / sr); }

/* a delay line, a power of two long, read with linear or cubic interpolation */
struct Delay {
    std::vector<float> b;
    uint32_t mask = 0, w = 0;
    void init(int minLen)
    {
        uint32_t n = 16;
        while ((int)n < minLen)
            n <<= 1;
        b.assign(n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    void clear()
    {
        if (!b.empty())
            std::memset(b.data(), 0, b.size() * sizeof(float));
    }
    void push(float x)
    {
        b[w] = x;
        w = (w + 1) & mask;
    }
    float tap(int d) const { return b[(w - 1u - (uint32_t)d) & mask]; }
    float read(float d) const          /* d samples ago (>= 0) */
    {
        float fl = std::floor(d);
        int i = (int)fl;
        float f = d - fl;
        float a = tap(i), c = tap(i + 1);
        return a + f * (c - a);
    }
    float read3(float d) const         /* cubic (Hermite) */
    {
        float fl = std::floor(d);
        int i = (int)fl;
        float f = d - fl;
        float xm = tap(i - 1 < 0 ? 0 : i - 1), x0 = tap(i), x1 = tap(i + 1), x2 = tap(i + 2);
        float c1 = 0.5f * (x1 - xm), c2 = xm - 2.5f * x0 + 2.0f * x1 - 0.5f * x2, c3 = 0.5f * (x2 - xm) + 1.5f * (x0 - x1);
        return ((c3 * f + c2) * f + c1) * f + x0;
    }
    int size() const { return (int)b.size(); }
};

/* a Schroeder allpass on a delay line */
struct Allpass {
    Delay d;
    int len = 1;
    void init(int n)
    {
        len = n;
        d.init(n + 4);
    }
    float run(float x, float g)
    {
        float y = d.tap(len - 1);
        float v = x + g * y;
        d.push(v);
        return y - g * v;
    }
};

/* a linear (TPT) state-variable filter, for the effects */
struct Svf {
    float s1 = 0.0f, s2 = 0.0f, g = 0.1f, k = 1.0f, a1 = 0, a2 = 0, a3 = 0;
    void set(float hz, float q, float sr)
    {
        g = std::tan(PI * clampf(hz, 10.0f, sr * 0.45f) / sr);
        k = 1.0f / clampf(q, 0.3f, 60.0f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void run(float x, float &lp, float &bp, float &hp)
    {
        float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }
    void clear() { s1 = s2 = 0.0f; }
};
} // namespace sol
