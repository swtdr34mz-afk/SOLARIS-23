/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIUM-23A) */
/* One side of the dual effector: a cartridge program (24, solaris.h's fxName), mono in, the wet signal out, its
 * X, Y, Z (0..1). A new program fades the old one out, clears it and fades in (no clicks, no old tails). */
#pragma once
#include "dsp.h"

namespace sol
{
class EffectSlot
{
public:
    void prepare(float sampleRate, uint32_t seed);
    void clear();
    void select(int fx);                 /* a program: switched over the next 40 ms */
    int current() const { return type; }
    /* n samples (n <= 16): in -> wet; x, y, z as they are for this block */
    void process(const float *in, float *wet, int n, float x, float y, float z);

private:
    float sr = 44100.0f;
    int type = 0, want = 0;
    float fade = 1.0f;                   /* the program's level while it changes */
    Rng rng;

    /* the reverb: an 8-line feedback delay network (Householder), modulated, damped */
    static constexpr int NL = 8;
    Delay line[NL];
    float lineLen[NL] = {}, damp[NL] = {}, lfoPh[NL] = {};
    Allpass diff[4];
    float rvGain[NL] = {}, rvDampA = 0.3f, rvSize = 1.0f, rvSizeTo = -1.0f, rvMod = 0.0f;
    float rvShimmer = 0.0f;
    void reverbSet(float size, float t60, float dampHz, float mod);
    float reverb(float x, bool diffuse);

    /* a long delay, a short one (modulation), a pitch shifter's buffer, a grain buffer */
    Delay dl, mod, ps, grain, pre;
    float dlTime = 0.3f, tone1 = 0.0f, tone2 = 0.0f, wobPh = 0.0f, flutPh = 0.0f;
    float lfo1 = 0.0f, frzH = 0.0f;
    float psPh[2] = {0.0f, 0.5f}, psPh2[2] = {0.0f, 0.5f};
    float pitchShift(float *ph, float ratio, float win);   /* reads ps (written by the caller) */
    float pitchShift2(float *ph, float ratio, float win);
    struct Grain {
        float pos = 0, step = 1, age = 0, len = 0, amp = 0;
        bool on = false;
    } grains[12];
    float grainClock = 0.0f;

    /* filters and the rest */
    Svf f1, f2, f3;
    float ap[6] = {}, apOut = 0.0f;      /* the phaser */
    float env = 0.0f, hold = 0.0f, holdCount = 0.0f, crushPh = 0.0f;
    float ringPh = 0.0f, shFreq = 200.0f, shCount = 0.0f, crackle = 0.0f, lpz = 0.0f, lpz2 = 0.0f;
    float dcx = 0.0f, dcy = 0.0f;

    void run(const float *in, float *wet, int n, float x, float y, float z);
};
} // namespace sol
