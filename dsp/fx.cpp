/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The dual effector's programs. Eight cartridges of three (X, Y, Z as solaris.h's fxLabel names them):
 *
 *   NEBULA   0 SHIMMER (oct up, decay, tone)  1 CLOUD (size, decay, motion)  2 HALL (pre-delay, decay, damp)
 *   ORBIT    3 ECHOVERB (feedback, time, reverb)  4 TAPE (feedback, time, wobble)  5 PITCHDLY (feedback, time, pitch)
 *   PULSAR   6 TREMOLO (depth, rate, reverb)  7 CHORUS (depth, rate, reverb)  8 PHASER (depth, rate, feedback)
 *   COMET    9 AUTOWAH (sens, freq, reso)  10 RESONATR (pitch, decay, bright)  11 FORMANT (vowel, rate, reso)
 *   GRAVITY  12 OCTAVER (oct down, oct up, direct)  13 HARMONZR (pitch 1, pitch 2, mix)  14 FREEZE (hold, length, smear)
 *   VOID     15 BLACKHOL (size, feedback, motion)  16 GRAINS (density, size, scatter)  17 SPACEDLY (feedback, time, diffuse)
 *   AURORA   18 RINGMOD (freq, rate, reverb)  19 S&H RING (freq, S&H rate, spread)  20 FOLD (fold, bias, tone)
 *   DUST     21 CRUSH (rate, bits, tone)  22 VINYL (crackle, wear, wow)  23 RADIO (band, static, drive)
 *
 * Every time is in seconds and every frequency in Hz, so a program sounds the same at any sample rate. */
#include "fx.h"
#include "solaris.h"

namespace sol
{
namespace
{
const float LINE_MS[8] = {29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.7f, 73.1f};
const float MAX_SIZE = 4.0f;
/* formants of A, E, I, O, U (Hz) */
const float FORM[5][3] = {{800, 1150, 2900}, {400, 1700, 2600}, {350, 1900, 2800}, {450, 800, 2830}, {325, 700, 2530}};
} // namespace

void EffectSlot::prepare(float sampleRate, uint32_t seed)
{
    sr = sampleRate;
    rng.seed(seed);
    for (int i = 0; i < NL; i++) {
        lineLen[i] = LINE_MS[i] * 0.001f * sr;
        line[i].init((int)(lineLen[i] * MAX_SIZE * 1.05f + 64.0f));
        lfoPh[i] = (float)i / (float)NL;
    }
    static const float DIFF_MS[4] = {4.7f, 3.6f, 12.7f, 9.3f};
    for (int i = 0; i < 4; i++)
        diff[i].init((int)(DIFF_MS[i] * 0.001f * sr) + 1);
    dl.init((int)(2.6f * sr) + 8);
    mod.init((int)(0.06f * sr) + 8);
    ps.init((int)(0.25f * sr) + 8);
    grain.init((int)(3.0f * sr) + 8);
    pre.init((int)(0.35f * sr) + 8);
    clear();
}

void EffectSlot::clear()
{
    for (auto &l : line)
        l.clear();
    for (auto &a : diff)
        a.d.clear();
    for (Delay *d : {&dl, &mod, &ps, &grain, &pre})
        d->clear();
    for (float &x : damp)
        x = 0.0f;
    rvSize = 0.0f;                       /* a new effect starts at its own size */
    rvShimmer = 0.0f;
    for (auto &g : grains)
        g.on = false;
    for (float &a : ap)
        a = 0.0f;
    f1.clear();
    f2.clear();
    f3.clear();
    rvShimmer = apOut = env = hold = holdCount = crushPh = 0.0f;
    tone1 = tone2 = lpz = lpz2 = dcx = dcy = crackle = 0.0f;
    psPh[0] = psPh2[0] = 0.0f;
    psPh[1] = psPh2[1] = 0.5f;
}

void EffectSlot::select(int fx)
{
    want = fx < 0 ? 0 : fx >= NFX ? NFX - 1 : fx;
}

void EffectSlot::reverbSet(float size, float t60, float dampHz, float m)
{
    rvSizeTo = clampf(size, 0.25f, MAX_SIZE);   /* the size glides there in reverb(): no zipper while turning */
    if (rvSize <= 0.0f)
        rvSize = rvSizeTo;
    rvMod = m;
    for (int i = 0; i < NL; i++) {
        float len = lineLen[i] * rvSizeTo;      /* "infinite" stays just below 1: it holds, but never runs away */
        rvGain[i] = t60 > 90.0f ? 0.99995f : std::fmin(0.99995f, std::pow(10.0f, -3.0f * len / (t60 * sr)));
    }
    rvDampA = onePoleCoef(dampHz, sr);
}

float EffectSlot::reverb(float x, bool diffuse)
{
    if (diffuse)
        for (auto &a : diff)
            x = a.run(x, 0.6f);
    float y[NL], s = 0.0f, out = 0.0f;
    if (rvSizeTo > 0.0f)
        rvSize += (rvSizeTo - rvSize) * (20.0f / sr);   /* about 50 ms */
    const float modSamples = rvMod * 0.004f * sr;
    for (int i = 0; i < NL; i++) {
        float d = lineLen[i] * rvSize;
        if (modSamples > 0.0f && (i & 1)) {
            lfoPh[i] += (0.13f + 0.05f * (float)i) / sr;
            if (lfoPh[i] >= 1.0f)
                lfoPh[i] -= 1.0f;
            d += modSamples * (1.0f + fsin01(lfoPh[i]));
        }
        float v = line[i].read(d);
        damp[i] += rvDampA * (v - damp[i]);
        y[i] = damp[i] * rvGain[i];
        s += y[i];
        out += (i & 1) ? -y[i] : y[i];
    }
    s *= 2.0f / (float)NL;
    for (int i = 0; i < NL; i++)
        line[i].push(x * 0.35f + y[i] - s);
    return out * 0.35f;
}

float EffectSlot::pitchShift(float *ph, float ratio, float win)
{
    const float dph = (1.0f - ratio) / win;
    float out = 0.0f;
    for (int h = 0; h < 2; h++) {
        ph[h] += dph;
        ph[h] -= std::floor(ph[h]);
        float w = 1.0f - std::fabs(2.0f * ph[h] - 1.0f);
        out += ps.read(2.0f + ph[h] * win) * w;
    }
    return out;
}

float EffectSlot::pitchShift2(float *ph, float ratio, float win) { return pitchShift(ph, ratio, win); }

void EffectSlot::process(const float *in, float *wet, int n, float x, float y, float z)
{
    if (want != type) {                  /* fade out, clear, change, fade in */
        fade -= (float)n / (0.02f * sr);
        if (fade <= 0.0f) {
            fade = 0.0f;
            clear();
            type = want;
        }
    } else if (fade < 1.0f) {
        fade = std::fmin(1.0f, fade + (float)n / (0.02f * sr));
    }
    if (fade <= 0.0f) {
        for (int i = 0; i < n; i++)
            wet[i] = 0.0f;
        return;
    }
    run(in, wet, n, x, y, z);
    for (int i = 0; i < n; i++) {          /* DC out, the level while changing, nothing wild */
        float v = wet[i];
        dcy = v - dcx + 0.9995f * dcy;
        dcx = v;
        wet[i] = clampf(dcy * fade, -4.0f, 4.0f);
    }
}

void EffectSlot::run(const float *in, float *wet, int n, float x, float y, float z)
{
    const float isr = 1.0f / sr;
    switch (type) {
    case 0: {                            /* SHIMMER: an octave up in the reverb's feedback */
        reverbSet(1.5f, 1.0f + 20.0f * y * y, 1500.0f * fexp2(3.0f * z), 0.4f);
        const float win = 0.07f * sr;
        for (int i = 0; i < n; i++) {
            float r = reverb(in[i] + ftanh(rvShimmer * x * 0.9f), true);
            ps.push(r);
            rvShimmer = pitchShift(psPh, 2.0f, win);
            wet[i] = r * 1.4f;
        }
        break;
    }
    case 1:                              /* CLOUD */
        reverbSet(0.6f + 2.2f * x, 2.0f + 30.0f * y * y, 4500.0f, 0.3f + 1.7f * z);
        for (int i = 0; i < n; i++)
            wet[i] = reverb(in[i], true) * 1.3f;
        break;
    case 2: {                            /* HALL */
        reverbSet(1.2f, 0.5f + 11.5f * y * y, 12000.0f * fexp2(-3.0f * z), 0.15f);
        const float pd = 1.0f + 0.3f * x * sr;
        for (int i = 0; i < n; i++) {
            pre.push(in[i]);
            wet[i] = reverb(pre.read(pd), true) * 1.3f;
        }
        break;
    }
    case 3: {                            /* ECHOVERB */
        reverbSet(1.0f, 2.5f, 6000.0f, 0.2f);
        float t = (0.04f + 1.46f * y * y) * sr, f = 1.05f * x, a = onePoleCoef(5000.0f, sr);
        for (int i = 0; i < n; i++) {
            dlTime += 0.0005f * (t - dlTime);
            float e = dl.read(dlTime);
            tone1 += a * (e - tone1);
            dl.push(in[i] + ftanh(tone1 * f));
            float r = reverb(e, false);
            wet[i] = e * (1.0f - 0.5f * z) + r * z * 1.6f;
        }
        break;
    }
    case 4: {                            /* TAPE: wow, flutter, saturation, losses */
        float t = (0.06f + 1.14f * y * y) * sr, f = 1.02f * x, a = onePoleCoef(4200.0f, sr);
        float depth = z * 0.006f * sr;
        for (int i = 0; i < n; i++) {
            wobPh += 0.55f * isr;
            flutPh += 6.3f * isr;
            wobPh -= std::floor(wobPh);
            flutPh -= std::floor(flutPh);
            dlTime += 0.0005f * (t - dlTime);
            float d = dlTime + depth * (1.0f + fsin01(wobPh)) + depth * 0.12f * (1.0f + fsin01(flutPh));
            float e = dl.read(d);
            tone1 += a * (e - tone1);
            tone2 += 0.002f * (tone1 - tone2);   /* (the low end thins out) */
            dl.push(ftanh(in[i] + (tone1 - tone2 * 0.6f) * f));
            wet[i] = e;
        }
        break;
    }
    case 5: {                            /* PITCHDLY: a pitch shifter in the echo's loop */
        float t = (0.05f + 1.2f * y * y) * sr, f = 0.98f * x;
        float semis = std::round((z - 0.5f) * 24.0f), ratio = fexp2(semis / 12.0f);
        const float win = 0.06f * sr;
        for (int i = 0; i < n; i++) {
            dlTime += 0.0005f * (t - dlTime);
            float e = dl.read(dlTime);
            ps.push(e);
            float s = pitchShift(psPh, ratio, win);
            dl.push(in[i] + ftanh(s * f));
            wet[i] = s;
        }
        break;
    }
    case 6: {                            /* TREMOLO */
        reverbSet(1.0f, 2.2f, 7000.0f, 0.2f);
        float rate = 0.3f * fexp2(5.5f * y);
        for (int i = 0; i < n; i++) {
            lfo1 += rate * isr;
            lfo1 -= std::floor(lfo1);
            float t = in[i] * (1.0f - x * (0.5f + 0.5f * fsin01(lfo1)));
            wet[i] = t + z * 1.5f * reverb(t, false);
        }
        break;
    }
    case 7: {                            /* CHORUS: three taps */
        reverbSet(1.0f, 2.2f, 7000.0f, 0.2f);
        float rate = 0.05f * fexp2(6.6f * y), depth = (0.0005f + 0.008f * x) * sr, base = 0.012f * sr;
        for (int i = 0; i < n; i++) {
            lfo1 += rate * isr;
            lfo1 -= std::floor(lfo1);
            mod.push(in[i]);
            float c = 0.0f;
            for (int k = 0; k < 3; k++)
                c += mod.read(base + depth * (1.0f + fsin01(lfo1 + (float)k / 3.0f)));
            c *= 0.45f;
            wet[i] = c + z * 1.5f * reverb(c, false);
        }
        break;
    }
    case 8: {                            /* PHASER: six allpasses swept */
        float rate = 0.03f * fexp2(7.0f * y);
        lfo1 += rate * (float)n * isr;
        lfo1 -= std::floor(lfo1);
        float sweep = 0.5f + 0.5f * fsin01(lfo1);
        float hz = 180.0f * fexp2(4.5f * x * sweep);
        float t = std::tan(PI * clampf(hz, 20.0f, sr * 0.45f) * isr), a = (t - 1.0f) / (t + 1.0f);
        float fbk = z * 0.85f;
        for (int i = 0; i < n; i++) {
            float v = in[i] + apOut * fbk;
            for (int k = 0; k < 6; k++) {
                float o = a * v + ap[k];
                ap[k] = v - a * o;
                v = o;
            }
            apOut = ftanh(v);
            wet[i] = (in[i] + v) * 0.6f;
        }
        break;
    }
    case 9: {                            /* AUTOWAH: an envelope follower opens a band-pass */
        float att = onePoleCoef(1.0f / 0.005f, sr), rel = onePoleCoef(1.0f / 0.12f, sr);
        for (int i = 0; i < n; i++) {
            float a = std::fabs(in[i]);
            env += (a > env ? att : rel) * (a - env);
        }
        float hz = 150.0f * fexp2(3.7f * y) * fexp2(clampf(env * x * 30.0f, 0.0f, 4.5f));
        f1.set(hz, 1.0f + 12.0f * z, sr);
        for (int i = 0; i < n; i++) {
            float lp, bp, hp;
            f1.run(in[i], lp, bp, hp);
            wet[i] = bp * 1.6f + lp * 0.2f;
        }
        break;
    }
    case 10: {                           /* RESONATR: a tuned comb */
        float hz = 40.0f * fexp2(5.0f * x), d = sr / hz, f = 0.995f * std::sqrt(y);
        float a = onePoleCoef(1000.0f * fexp2(3.8f * z), sr);
        for (int i = 0; i < n; i++) {
            float e = mod.size() > d + 4 ? mod.read3(d - 1.0f) : dl.read3(d - 1.0f);
            tone1 += a * (e - tone1);
            float v = in[i] * 0.4f + tone1 * f;
            if (mod.size() > d + 4)
                mod.push(v);
            else
                dl.push(v);
            wet[i] = e;
        }
        break;
    }
    case 11: {                           /* FORMANT: vowels, A E I O U, a slow sweep */
        float rate = y < 0.02f ? 0.0f : 0.02f * fexp2(7.0f * y);
        lfo1 += rate * (float)n * isr;
        lfo1 -= std::floor(lfo1);
        float v = clampf(x + (rate > 0.0f ? 0.25f * fsin01(lfo1) : 0.0f), 0.0f, 1.0f) * 3.999f;
        int k = (int)v;
        float fr = v - (float)k;
        float q = 3.0f + 17.0f * z;
        Svf *fs[3] = {&f1, &f2, &f3};
        for (int j = 0; j < 3; j++)
            fs[j]->set(FORM[k][j] + fr * (FORM[k + 1][j] - FORM[k][j]), q, sr);
        for (int i = 0; i < n; i++) {
            float lp, bp, hp, s = 0.0f;
            f1.run(in[i], lp, bp, hp);
            s += bp;
            f2.run(in[i], lp, bp, hp);
            s += bp * 0.7f;
            f3.run(in[i], lp, bp, hp);
            s += bp * 0.45f;
            wet[i] = s * 1.5f;
        }
        break;
    }
    case 12: {                           /* OCTAVER */
        const float w1 = 0.07f * sr, w2 = 0.04f * sr;
        for (int i = 0; i < n; i++) {
            ps.push(in[i]);
            float dn = pitchShift(psPh, 0.5f, w1), up = pitchShift2(psPh2, 2.0f, w2);
            wet[i] = dn * x * 1.2f + up * y + in[i] * z;
        }
        break;
    }
    case 13: {                           /* HARMONZR: two voices, in semitones */
        float r1 = fexp2(std::round((x - 0.5f) * 24.0f) / 12.0f), r2 = fexp2(std::round((y - 0.5f) * 24.0f) / 12.0f);
        const float w = 0.05f * sr;
        for (int i = 0; i < n; i++) {
            ps.push(in[i]);
            float a = pitchShift(psPh, r1, w), b = pitchShift2(psPh2, r2, w);
            wet[i] = (a + b) * 0.6f * z + in[i] * (1.0f - z);
        }
        break;
    }
    case 14: {                           /* FREEZE: a loop that keeps what it holds */
        reverbSet(1.6f, 6.0f, 5000.0f, 0.6f);
        float hTo = x >= 0.97f ? 1.0f : x * 0.97f;
        float len = (0.1f + 2.4f * y) * sr;
        for (int i = 0; i < n; i++) {
            frzH += (hTo - frzH) * (30.0f / sr);   /* holding and letting go: over ~30 ms, no hard cut */
            const float h = frzH;
            dlTime += 0.0002f * (len - dlTime);
            float e = dl.read(dlTime);
            dl.push(in[i] * (1.0f - h) + e * h);
            wet[i] = e * (1.0f - 0.4f * z) + reverb(e, true) * z * 1.6f;
        }
        break;
    }
    case 15:                             /* BLACKHOL: a huge, near endless room */
        reverbSet(1.5f + 2.5f * x, y > 0.97f ? 100.0f : 3.0f + 60.0f * y * y, 5000.0f, 0.4f + 1.6f * z);
        for (int i = 0; i < n; i++)
            wet[i] = reverb(in[i], true) * 1.2f;
        break;
    case 16: {                           /* GRAINS */
        float rate = 2.0f * fexp2(5.0f * x), glen = (0.02f + 0.48f * y * y) * sr;
        for (int i = 0; i < n; i++) {
            grain.push(in[i]);
            grainClock += rate * isr;
            if (grainClock >= 1.0f) {
                grainClock -= 1.0f;
                for (auto &g : grains)
                    if (!g.on) {
                        static const float RATIOS[6] = {1.0f, 2.0f, 0.5f, 1.5f, 1.0f, 0.75f};
                        g.on = true;
                        g.age = 0.0f;
                        g.len = glen * (0.7f + 0.6f * rng.uni());
                        g.step = rng.uni() < z ? RATIOS[rng.next() % 6] : 1.0f;
                        float back = glen * 2.0f + z * rng.uni() * 2.0f * sr;
                        g.pos = clampf(back, glen * (g.step > 1.0f ? g.step : 1.0f) + 4.0f, (float)grain.size() - 8.0f);
                        g.amp = 0.6f + 0.4f * rng.uni();
                        break;
                    }
            }
            float s = 0.0f;
            for (auto &g : grains)
                if (g.on) {
                    float w = fsin01(0.5f * g.age / g.len);
                    s += grain.read(g.pos) * w * g.amp;
                    g.pos += 1.0f - g.step;
                    g.age += 1.0f;
                    if (g.age >= g.len || g.pos < 2.0f)
                        g.on = false;
                }
            wet[i] = s * 0.7f;
        }
        break;
    }
    case 17: {                           /* SPACEDLY: echoes smeared by allpasses in the loop */
        float t = (0.08f + 1.3f * y * y) * sr, f = 0.97f * x, g = 0.75f * z, a = onePoleCoef(6000.0f, sr);
        for (int i = 0; i < n; i++) {
            dlTime += 0.0005f * (t - dlTime);
            float e = dl.read(dlTime);
            for (auto &ad : diff)
                e = ad.run(e, g);
            tone1 += a * (e - tone1);
            dl.push(in[i] + ftanh(tone1 * f));
            wet[i] = e;
        }
        break;
    }
    case 18: {                           /* RINGMOD */
        reverbSet(1.0f, 2.5f, 6000.0f, 0.2f);
        float rate = y < 0.02f ? 0.0f : 0.05f * fexp2(7.4f * y);
        float base = 20.0f * fexp2(6.65f * x);
        for (int i = 0; i < n; i++) {
            lfo1 += rate * isr;
            lfo1 -= std::floor(lfo1);
            ringPh += base * (rate > 0.0f ? fexp2(fsin01(lfo1)) : 1.0f) * isr;
            ringPh -= std::floor(ringPh);
            float r = in[i] * fsin01(ringPh) * 1.4f;
            wet[i] = r + z * 1.4f * reverb(r, false);
        }
        break;
    }
    case 19: {                           /* S&H RING: the carrier jumps */
        float rate = 0.5f * fexp2(6.0f * y), base = 40.0f * fexp2(5.6f * x);
        for (int i = 0; i < n; i++) {
            shCount += rate * isr;
            if (shCount >= 1.0f) {
                shCount -= 1.0f;
                shFreq = base * fexp2(rng.bi() * 3.0f * z);
            }
            ringPh += shFreq * isr;
            ringPh -= std::floor(ringPh);
            wet[i] = in[i] * fsin01(ringPh) * 1.4f;
        }
        break;
    }
    case 20: {                           /* FOLD: a wavefolder */
        float g = 1.0f + 11.0f * x, b = (y - 0.5f) * 0.8f, a = onePoleCoef(500.0f * fexp2(5.0f * z), sr);
        for (int i = 0; i < n; i++) {
            float v = fsin01((g * in[i] + b) * 0.25f) * 0.8f;
            lpz += a * (v - lpz);
            wet[i] = lpz;
        }
        break;
    }
    case 21: {                           /* CRUSH: the rate and the bits */
        float hz = 200.0f * fexp2(7.8f * x), bits = 2.0f + 14.0f * y, q = fexp2(bits - 1.0f);
        float a = onePoleCoef(400.0f * fexp2(5.5f * z), sr);
        for (int i = 0; i < n; i++) {
            crushPh += hz * isr;
            if (crushPh >= 1.0f) {
                crushPh -= std::floor(crushPh);
                hold = std::round(clampf(in[i], -1.0f, 1.0f) * q) / q;
            }
            lpz += a * (hold - lpz);
            wet[i] = lpz;
        }
        break;
    }
    case 22: {                           /* VINYL: crackle, hiss, a worn top, wow */
        float a = onePoleCoef(12000.0f * fexp2(-3.0f * y), sr), depth = z * 0.004f * sr;
        for (int i = 0; i < n; i++) {
            wobPh += 0.55f * isr;
            wobPh -= std::floor(wobPh);
            mod.push(in[i]);
            float v = mod.read(2.0f + depth * (1.0f + fsin01(wobPh)));
            if (rng.uni() < x * x * 0.0015f)
                crackle = rng.bi() * (0.3f + 0.7f * rng.uni());
            crackle *= 0.6f;
            v += crackle + rng.bi() * y * 0.012f;
            lpz += a * (v - lpz);
            wet[i] = lpz;
        }
        break;
    }
    case 23: {                           /* RADIO: a narrow band, static, overdrive */
        f1.set(300.0f * fexp2(3.5f * x), 2.2f, sr);
        for (int i = 0; i < n; i++) {
            float lp, bp, hp;
            holdCount -= 1.0f;
            if (holdCount <= 0.0f) {     /* the static comes in bursts */
                holdCount = (0.02f + 0.2f * rng.uni()) * sr;
                hold = rng.uni() < 0.35f ? 1.0f : 0.15f;
            }
            float noise = rng.bi() * y * y * 0.35f * hold;
            f1.run(in[i] * 1.5f + noise, lp, bp, hp);
            wet[i] = ftanh(bp * (1.0f + 10.0f * z)) * (0.8f - 0.3f * z);
        }
        break;
    }
    default:
        for (int i = 0; i < n; i++)
            wet[i] = 0.0f;
    }
}
} // namespace sol
