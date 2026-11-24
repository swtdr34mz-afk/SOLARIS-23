/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* What each knob's 0..1 means: shared by the engine and the value texts, so the panel shows what sounds. */
#pragma once
#include <cmath>

namespace sol
{
namespace map
{
inline float ex2(float x) { return std::exp2(x); }
/* the classic drones' oscillators: 1-2 low, 3 middle, 4-5 high, each knob over a little more than three octaves */
inline float classicHz(int osc, float v)
{
    static const float base[5] = {24.0f, 24.0f, 48.0f, 96.0f, 96.0f};
    return base[osc] * ex2(3.4f * v);
}
inline float chaosPitchHz(float v, bool hi) { return hi ? 165.0f * ex2(4.0f * v) : 16.35f * ex2(4.0f * v); }
inline float chaosRateHz(float v, bool x10) { return 0.1f * ex2(6.65f * v) * (x10 ? 10.0f : 1.0f); }
inline float arTime(float v) { return 0.005f * ex2(12.0f * v); }        /* 5 ms .. 20 s */
inline float vcoAttack(float v) { return 0.001f * ex2(13.3f * v); }     /* 1 ms .. 10 s */
inline float vcoDecay(float v) { return 0.003f * ex2(12.3f * v); }      /* 3 ms .. 15 s */
inline float glide(float v) { return v * v * 2.0f; }                    /* 0 .. 2 s */
inline float filterHz(float v) { return 20.0f * ex2(9.97f * v); }       /* 20 Hz .. 20 kHz */
inline float lfoHz(float v, int range)
{
    static const float m[3] = {1.0f, 6.0f, 10.0f};
    return 0.02f * ex2(7.0f * v) * m[range < 0 ? 0 : range > 2 ? 2 : range];
}
inline float seqHz(float v) { return 0.1f * ex2(7.3f * v); }            /* 0.1 .. 16 Hz */
inline float lightHz(float v) { return 0.01f * ex2(8.6f * v); }         /* 0.01 .. 4 Hz */
inline float orbitHz(float v) { return 0.01f * ex2(8.0f * v); }         /* 0.01 .. 2.6 Hz: one turn in 100 s .. 0.4 s */
inline float orbitWidth(float v) { return 1.5f * v; }                   /* 0: mono .. 150% */
inline float droneToneHz(float v) { return 9000.0f * ex2(-5.5f * (1.0f - v)); }   /* 200 Hz .. 9 kHz (all open: as before) */
inline float vcoTune(float v) { return (v - 0.5f) * 12.0f; }            /* +-6 semitones: an octave over the knob */
inline float volume(float v) { return v * v * 2.0f; }                    /* 0.7: about 0 dB */
inline float mixGain(float v) { return v * v * 1.4f; }
inline float bip(float v) { return (v - 0.5f) * 2.0f; }
} // namespace map
} // namespace sol
