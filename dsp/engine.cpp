/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* SOLARIS-23's engine (solaris.h): the eight voices, the modulation, the mixer, the dual filter, the distortion and
 * the dual effector, rendered in blocks of 16 samples (the control rate; the modulation and the knobs move once a
 * block, the audio runs per sample). */
#include "solaris.h"
#include "dsp.h"
#include "fx.h"
#include "maps.h"
#include <atomic>

namespace sol
{
namespace
{
constexpr int BLOCK = 16;
const int CLASSIC[4] = {D1_T1, D2_T1, D4_T1, D5_T1};   /* (each: T1..T5, ON1..ON5, MOD1..MOD5, VOLT, ATT, RLS, GATE) */
const int CHAOS[2] = {D3_PITCH, D6_PITCH};             /* (PITCH, HI, RATE, X10, MODA, DIV, NOISE, SH, FM, AM, ATT, RLS, GATE) */
const int CLASSIC_HOLD[4] = {D1_HOLD, D2_HOLD, D4_HOLD, D5_HOLD};   /* (HOLD: open without the key) */
const int CHAOS_HOLD[2] = {D3_HOLD, D6_HOLD};
const int VCO[2] = {VA_WAVE, VB_WAVE};                 /* (WAVE, PW, TUNE, OCT, SUB, A, D, S, R, HOLD, LOOP) */
/* the mixer's channels: DRONE 1, 2, 3, VCO A, VCO B, DRONE 4, 5, 6 */
enum { M_D1, M_D2, M_D3, M_VA, M_VB, M_D4, M_D5, M_D6 };
const int CLASSIC_MIX[4] = {M_D1, M_D2, M_D4, M_D5};
const int CHAOS_MIX[2] = {M_D3, M_D6};

inline float quantHz(float hz)                         /* to the nearest semitone (A = 440) */
{
    float s = std::round(12.0f * std::log2(hz / 440.0f));
    return 440.0f * fexp2(s / 12.0f);
}

/* an attack/release envelope as on the drone voices */
struct AR {
    float v = 0.0f;
    float run(bool gate, float a, float r)
    {
        v += (gate ? a * (1.02f - v) : r * (0.0f - v));
        if (v > 1.0f)
            v = 1.0f;
        if (v < 1e-6f)
            v = 0.0f;
        return v;
    }
};

struct Classic {
    float ph[5] = {}, last[5] = {}, gain[5] = {}, inc[5] = {};
    float drift[5] = {}, driftTarget[5] = {}, driftClock[5] = {};
    float fm = 0.0f, lp = 0.0f, envA = 0, envR = 0;
    bool on[5] = {};
    AR env;
};

struct Chaos {
    float mph = 0.0f, tph = 0.0f, sh = 0.0f, nlp = 0.0f, am = 1.0f, lp = 0.0f;
    float mInc = 0.0f, tHz = 100.0f, fmDepth = 0.0f, noise = 0.0f, shAmt = 0.0f, div = 0.0f, envA = 0, envR = 0;
    bool fm = true, amOn = false;
    int count = 0;
    float d2 = 1.0f, d4 = 1.0f;
    AR env;
};

struct Vco {
    float ph = 0.0f, sub = 1.0f, inc = 0.0f, out = 0.0f;
    float wave = 0.0f, pw = 0.5f, subOn = 0.0f;
    float env = 0.0f;
    int stage = 0;                       /* 0 off, 1 attack, 2 decay, 3 sustain, 4 release */
    float a = 0, d = 0, s = 0, r = 0;
    bool hold = false, loop = false;
    bool wrapped = false;
};
} // namespace

struct Impl {
    float sr = 44100.0f;
    float p[NPARAMS] = {}, sm[NPARAMS] = {};
    bool fresh = true;
    Rng rng;

    Classic cl[4];
    Chaos ch[2];
    Vco vco[2];

    /* the keys */
    int held[32] = {};
    int nHeld = 0;
    float note = 48.0f, glideNote = 48.0f, vel = 1.0f, bendV = 0.0f, wheel = 0.0f;
    bool keyGate = false, seqGate = false, gateNow = false;

    /* the modulation */
    float lfoPh[2] = {}, lfoOut[2] = {};
    float seqPh = 0.0f, seqVal = 0.0f, seqGlide = 0.0f, seqGatePh = 0.0f;
    int step = 0;
    float lightCommon = 0.5f, lightTarget = 0.5f, lightFrom = 0.5f, lightPh = 1.0f;
    float lightOwn[4] = {0.5f, 0.5f, 0.5f, 0.5f}, lightOwnT[4] = {0.5f, 0.5f, 0.5f, 0.5f}, lightVal[4] = {};
    float bus[NDEST] = {};
    double bpm = 120.0, ppq = 0.0;
    uint64_t rendered = 0, ppqAt = 0;   /* samples delivered; when ppq was given */
    bool playing = false;
    int lastSyncStep = -1;

    /* the filters, the distortion, the effector */
    float fs1[2] = {}, fs2[2] = {}, fg[2] = {0.1f, 0.1f}, fgTarget[2] = {0.1f, 0.1f}, fk[2] = {2.0f, 2.0f};
    EffectSlot fx[2];
    float fxX = 0.5f, fxY = 0.5f, fxZ = 0.5f, blend = 0.0f, master = 0.0f, masterTarget = 0.0f;
    float mixG[8] = {}, panL[8] = {}, panR[8] = {}, mixGT[8] = {};

    float outL[BLOCK] = {}, outR[BLOCK] = {};
    int pos = BLOCK;

    std::atomic<float> meterLight{0.0f}, meterVoice[8], meterLfo[2];
    std::atomic<int> meterStep{0};

    void prepare(float rate);
    void controls();
    void block();
    void startEnvelopes();
};

void Impl::prepare(float rate)
{
    sr = rate;
    for (int i = 0; i < NPARAMS; i++)
        if (!fresh)
            sm[i] = p[i];
        else
            p[i] = sm[i] = PARAMS[i].def;
    fresh = false;
    rng.seed(0x50a1a23u);
    for (int c = 0; c < 4; c++) {
        cl[c] = Classic();
        for (int k = 0; k < 5; k++) {
            cl[c].ph[k] = rng.uni();
            cl[c].drift[k] = cl[c].driftTarget[k] = rng.bi() * 0.003f;
            cl[c].driftClock[k] = rng.uni();
        }
    }
    for (auto &c : ch)
        c = Chaos();
    for (auto &v : vco)
        v = Vco();
    for (int i = 0; i < 2; i++) {
        fx[i].prepare(sr, 0x2342u + 77u * (uint32_t)i);
        fx[i].select((int)p[i ? FXR : FXL]);
        fs1[i] = fs2[i] = 0.0f;
    }
    for (int i = 0; i < 8; i++)
        meterVoice[i] = 0.0f;
    nHeld = 0;
    keyGate = seqGate = gateNow = false;
    pos = BLOCK;
    lastSyncStep = -1;
    master = -1.0f;
    for (float &g : mixG)
        g = -1.0f;
}

void Impl::startEnvelopes()
{
    for (auto &v : vco)
        if (!v.loop)
            v.stage = 1;
}

/* the knobs, the modulation, every coefficient: once a block */
void Impl::controls()
{
    const float kb = (float)BLOCK / sr;
    const float sa = 1.0f - std::exp(-kb / 0.025f);   /* knobs glide over about 25 ms */
    for (int i = 0; i < NPARAMS; i++) {
        int k = PARAMS[i].kind;
        if (k == K_CONT || k == K_BIP)
            sm[i] += sa * (p[i] - sm[i]);
        else
            sm[i] = p[i];
    }
    auto ch_ = [&](int i) { return (int)std::lround(sm[i]); };
    auto sw = [&](int i) { return sm[i] >= 0.5f; };

    /* LFO A, B: square to triangle, 0..1 (positive, as made for the light sensors) */
    for (int l = 0; l < 2; l++) {
        int b = l ? LB_WAVE : LA_WAVE;
        lfoPh[l] += map::lfoHz(sm[b + 1], ch_(b + 2)) * kb;
        lfoPh[l] -= std::floor(lfoPh[l]);
        float tri = 1.0f - std::fabs(2.0f * lfoPh[l] - 1.0f), sq = lfoPh[l] < 0.5f ? 1.0f : 0.0f;
        lfoOut[l] = sq + sm[b] * (tri - sq);
        meterLfo[l] = lfoOut[l];
    }

    /* the sequencer: its pulser, or the host's beat (locked to its position while it plays) */
    const int stages = 3 + ch_(SEQ_STAGES);
    bool run = sw(SEQ_RUN), stepped = false;
    if (run) {
        int clk = ch_(SEQ_CLOCK);
        static const double BEATS[7] = {0.0, 2.0, 1.0, 0.5, 0.25, 1.0 / 3.0, 1.0 / 6.0};
        if (clk == 0 || BEATS[clk] <= 0.0) {
            seqPh += map::seqHz(sm[SEQ_RATE]) * kb;
            seqGatePh = seqPh;
            if (seqPh >= 1.0f) {
                seqPh -= std::floor(seqPh);
                step = (step + 1) % stages;
                stepped = true;
            }
        } else {
            double len = BEATS[clk];
            if (playing) {
                /* (the host's position at its block's start, carried forward to this block's: any buffer size the same) */
                double now = ppq + (double)(rendered - ppqAt) / (double)sr * bpm / 60.0;
                double s = now / len;
                int st = (int)std::floor(s);
                seqGatePh = (float)(s - (double)st);
                if (st != lastSyncStep) {
                    lastSyncStep = st;
                    step = ((st % stages) + stages) % stages;
                    stepped = true;
                }
            } else {
                seqPh += (float)(bpm / 60.0 / len) * kb;
                seqGatePh = seqPh;
                if (seqPh >= 1.0f) {
                    seqPh -= std::floor(seqPh);
                    step = (step + 1) % stages;
                    stepped = true;
                }
            }
        }
        if (step >= stages)
            step = 0;
        seqVal = sm[SEQ_S1 + step];
    }
    meterStep = step;
    bool stepGate = run && sw(SEQ_G1 + step);
    bool play = run && ch_(SEQ_DEST) == 0;
    seqGate = play && stepGate && seqGatePh < 0.5f;
    if (play && stepped && stepGate)
        startEnvelopes();
    seqGlide += (1.0f - std::exp(-kb / 0.004f)) * (seqVal - seqGlide);

    /* the light sensors: the room's light drifting (each sensor a little its own), or an LFO or the sequencer */
    lightPh += map::lightHz(sm[LIGHT_RATE]) * kb;
    if (lightPh >= 1.0f) {
        lightPh -= std::floor(lightPh);
        lightFrom = lightTarget;
        lightTarget = clampf(lightTarget + rng.bi() * 0.6f, 0.0f, 1.0f);
        for (int s = 0; s < 4; s++)
            lightOwnT[s] = rng.uni();
    }
    {
        float t = lightPh * lightPh * (3.0f - 2.0f * lightPh);
        lightCommon = lightFrom + t * (lightTarget - lightFrom);
    }
    int src = ch_(LIGHT_SRC);
    for (int s = 0; s < 4; s++) {
        lightOwn[s] += (1.0f - std::exp(-kb * 0.4f)) * (lightOwnT[s] - lightOwn[s]);
        float amb = 0.75f * lightCommon + 0.25f * lightOwn[s];
        lightVal[s] = src == 1 ? lfoOut[0] : src == 2 ? lfoOut[1] : src == 3 ? seqGlide : amb;
    }
    meterLight = lightVal[0];

    /* the modulation buses */
    for (float &b : bus)
        b = 0.0f;
    auto send = [&](int dest, float v) {
        if (dest > 0 && dest < NDEST)
            bus[dest] += v;
    };
    send(ch_(LA_DEST), lfoOut[0] * map::bip(sm[LA_DEPTH]));
    send(ch_(LB_DEST), lfoOut[1] * map::bip(sm[LB_DEPTH]));
    send(ch_(JOY_XDEST), map::bip(sm[JOY_X]));
    send(ch_(JOY_YDEST), clampf(map::bip(sm[JOY_Y]) + wheel, -1.0f, 1.0f));
    if (run && !play)
        send(ch_(SEQ_DEST), seqGlide * map::bip(sm[SEQ_DEPTH]));
    const bool grid = ch_(GRID) == 1;

    /* DRONE 1, 2, 4, 5 */
    const float droneSemis = bus[D_DRONES] * 7.0f;
    for (int c = 0; c < 4; c++) {
        Classic &v = cl[c];
        const int b = CLASSIC[c];
        float volt = map::bip(sm[b + 15]);
        float down = volt < 0.0f ? fexp2(volt * 3.0f) : 1.0f;     /* down to three octaves below */
        v.fm = volt > 0.0f ? volt : 0.0f;
        float light = (lightVal[c] - 0.5f) * 2.0f * sm[LIGHT_DEPTH] * sm[LIGHT_DEPTH] * 7.0f;
        for (int k = 0; k < 5; k++) {
            v.driftClock[k] += kb * 0.15f;
            if (v.driftClock[k] >= 1.0f) {                         /* a negistor never quite holds its note */
                v.driftClock[k] -= 1.0f;
                v.driftTarget[k] = rng.bi() * 0.0035f;
            }
            v.drift[k] += kb * 0.3f * (v.driftTarget[k] - v.drift[k]);
            float hz = map::classicHz(k, sm[b + k]);
            if (grid)
                hz = quantHz(hz);
            float semis = droneSemis + (sw(b + 10 + k) ? light : 0.0f);
            hz *= down * fexp2(semis / 12.0f) * (1.0f + v.drift[k]);
            v.inc[k] = clampf(hz / sr, 0.0f, 0.45f);
            v.on[k] = sw(b + 5 + k);
        }
        v.envA = 1.0f - std::exp(-1.0f / (map::arTime(sm[b + 16]) * sr));
        v.envR = 1.0f - std::exp(-1.0f / (map::arTime(sm[b + 17]) * sr));
    }

    /* DRONE 3, 6 */
    for (int c = 0; c < 2; c++) {
        Chaos &v = ch[c];
        const int b = CHAOS[c];
        v.tHz = map::chaosPitchHz(sm[b], ch_(b + 1) == 1) * fexp2(bus[D_CHAOS] * 2.0f);
        v.mInc = map::chaosRateHz(sm[b + 2], ch_(b + 3) == 1) / sr;
        v.fmDepth = sm[b + 4];
        v.div = sm[b + 5];
        v.noise = sm[b + 6];
        v.shAmt = sm[b + 7];
        v.fm = sw(b + 8);
        v.amOn = sw(b + 9);
        v.envA = 1.0f - std::exp(-1.0f / (map::arTime(sm[b + 10]) * sr));
        v.envR = 1.0f - std::exp(-1.0f / (map::arTime(sm[b + 11]) * sr));
    }

    /* VCO A, B: the keys' note (glide), bend, the sequencer, each one's octave and tune */
    {
        float g = map::glide(sm[GLIDE]);
        float target = note;
        if (play)
            target += (grid ? std::round(seqGlide * 24.0f) : seqGlide * 24.0f) * map::bip(sm[SEQ_DEPTH]);
        glideNote = g < 0.001f ? target : glideNote + (1.0f - std::exp(-kb / (g * 0.35f))) * (target - glideNote);
        static const float BENDST[5] = {0.0f, 1.0f, 2.0f, 7.0f, 12.0f};
        float common = glideNote + bendV * BENDST[ch_(BEND)] + bus[D_VPITCH] * 12.0f;
        for (int i = 0; i < 2; i++) {
            Vco &v = vco[i];
            const int b = VCO[i];
            float n = common + (float)(ch_(b + 3) - 2) * 12.0f + map::vcoTune(sm[b + 2]);
            v.inc = clampf(440.0f * fexp2((n - 69.0f) / 12.0f) / sr, 0.0f, 0.45f);
            v.wave = clampf(sm[b] + bus[D_VWAVE] * 0.5f, 0.0f, 1.0f);
            v.pw = clampf(0.05f + 0.9f * sm[b + 1] + bus[D_VPW] * 0.45f, 0.03f, 0.97f);
            v.subOn = sw(b + 4) ? 1.0f : 0.0f;
            v.a = 1.0f / (map::vcoAttack(sm[b + 5]) * sr);
            v.d = 1.0f - std::exp(-1.0f / (map::vcoDecay(sm[b + 6]) * sr * 0.3f));
            v.s = sm[b + 7];
            v.r = 1.0f - std::exp(-1.0f / (map::vcoDecay(sm[b + 8]) * sr * 0.3f));
            v.hold = sw(b + 9);
            bool loop = sw(b + 10);
            if (loop && !v.loop)
                v.stage = 1;
            v.loop = loop;
        }
    }
    gateNow = keyGate || seqGate;

    /* the filters: FREQ and RES, and the CV (LINK: the left's CV moves both) */
    {
        float cvL = (bus[D_CUTL] + bus[D_CUT] + bus[D_CUTLR]) * 4.0f;
        float cvR = (bus[D_CUTR] + bus[D_CUT] - bus[D_CUTLR]) * 4.0f;
        if (sw(F_LINK))
            cvR = cvL;
        float oct[2] = {cvL, cvR};
        int fb[2] = {FL_FREQ, FR_FREQ};
        for (int s = 0; s < 2; s++) {
            float hz = clampf(map::filterHz(sm[fb[s]]) * fexp2(oct[s]), 16.0f, sr * 0.42f);
            fgTarget[s] = std::tan(PI * hz / sr);
            fk[s] = 2.0f - 1.97f * sm[fb[s] + 1];
        }
    }
    fxX = clampf(sm[FX_X] + bus[D_FXX], 0.0f, 1.0f);
    fxY = clampf(sm[FX_Y] + bus[D_FXY], 0.0f, 1.0f);
    fxZ = clampf(sm[FX_Z] + bus[D_FXZ], 0.0f, 1.0f);
    blend = clampf(sm[BLEND] + bus[D_BLEND], 0.0f, 1.0f);
    fx[0].select(ch_(FXL));
    fx[1].select(ch_(FXR));
    masterTarget = map::volume(sm[VOLUME]);
    for (int m = 0; m < 8; m++) {
        mixGT[m] = map::mixGain(sm[D1_VOL + m]);
        float pan = 0.5f * (map::bip(sm[D1_PAN + m]) + 1.0f);
        panL[m] = std::cos(pan * PI * 0.5f) * 1.41421f;
        panR[m] = std::sin(pan * PI * 0.5f) * 1.41421f;
    }
}

void Impl::block()
{
    controls();
    float L[BLOCK], R[BLOCK];
    float gStart[8], gStep[8];
    for (int m = 0; m < 8; m++) {
        if (mixG[m] < 0.0f)
            mixG[m] = mixGT[m];
        gStart[m] = mixG[m];
        gStep[m] = (mixGT[m] - mixG[m]) / (float)BLOCK;
        mixG[m] = mixGT[m];
    }
    float level[8] = {};
    const float oscRamp = 1.0f / (0.004f * sr);
    const float dlp = onePoleCoef(9000.0f, sr);

    for (int i = 0; i < BLOCK; i++) {
        float vs[8];
        /* DRONE 1, 2, 4, 5 */
        for (int c = 0; c < 4; c++) {
            Classic &v = cl[c];
            if (v.env.v <= 0.0f && !(sm[CLASSIC[c] + 18] >= 0.5f || sm[CLASSIC_HOLD[c]] >= 0.5f)) {   /* (closed and silent: nothing to run) */
                vs[CLASSIC_MIX[c]] = 0.0f;
                level[CLASSIC_MIX[c]] = 0.0f;
                continue;
            }
            float sum = 0.0f;
            for (int k = 0; k < 5; k++) {
                float want = v.on[k] ? 1.0f : 0.0f;
                if (v.gain[k] != want)
                    v.gain[k] = v.gain[k] < want ? std::fmin(want, v.gain[k] + oscRamp) : std::fmax(want, v.gain[k] - oscRamp);
                if (v.gain[k] <= 0.0f) {
                    v.last[k] = 0.0f;
                    continue;
                }
                float inc = v.inc[k];
                if (v.fm > 0.0f)                  /* VOLT past the middle: each runs on its neighbour */
                    inc = clampf(inc * (1.0f + v.fm * 1.8f * v.last[(k + 4) % 5]), 0.0f, 0.45f);
                v.ph[k] += inc;
                if (v.ph[k] >= 1.0f)
                    v.ph[k] -= 1.0f;
                float s = 2.0f * v.ph[k] - 1.0f - blep(v.ph[k], inc > 1e-6f ? inc : 1e-6f);
                v.last[k] = s;
                sum += s * v.gain[k];
            }
            v.lp += dlp * (sum * 0.13f - v.lp);
            float e = v.env.run((sm[CLASSIC[c] + 18] >= 0.5f || sm[CLASSIC_HOLD[c]] >= 0.5f), v.envA, v.envR);
            vs[CLASSIC_MIX[c]] = v.lp * e;
            level[CLASSIC_MIX[c]] = e;
        }
        /* DRONE 3, 6 */
        for (int c = 0; c < 2; c++) {
            Chaos &v = ch[c];
            if (v.env.v <= 0.0f && !(sm[CHAOS[c] + 12] >= 0.5f || sm[CHAOS_HOLD[c]] >= 0.5f)) {
                vs[CHAOS_MIX[c]] = 0.0f;
                level[CHAOS_MIX[c]] = 0.0f;
                continue;
            }
            float before = v.mph;
            v.mph += v.mInc;
            if (v.mph >= 1.0f)
                v.mph -= std::floor(v.mph);
            if (v.mph < before)                   /* the modulator's edge: the sample & hold takes the noise */
                v.sh = rng.bi();
            float sq = v.mph < 0.5f ? 1.0f : 0.0f;
            float tri = 1.0f - std::fabs(2.0f * v.mph - 1.0f);
            float n = rng.bi();
            v.nlp += 0.15f * (n - v.nlp);
            float oct = (v.fm ? v.fmDepth * 4.0f * (tri * 2.0f - 1.0f) : 0.0f) + v.sh * v.shAmt * 3.0f + v.nlp * v.noise * 5.0f;
            float inc = clampf(v.tHz * fexp2(oct) / sr, 1e-6f, 0.45f);
            v.tph += inc;
            if (v.tph >= 1.0f) {
                v.tph -= 1.0f;
                v.count++;
                v.d2 = (v.count & 1) ? -v.d2 : v.d2;
                if ((v.count & 3) == 0)
                    v.d4 = -v.d4;
            }
            float t = (v.tph < 0.5f ? 1.0f : -1.0f) + blep(v.tph, inc);
            float t2 = v.tph + 0.5f;
            t -= blep(t2 >= 1.0f ? t2 - 1.0f : t2, inc);
            float out = t * (1.0f - v.div) + v.div * (v.div < 0.5f ? v.d2 : 0.5f * (v.d2 + v.d4));
            v.am += 0.08f * ((v.amOn ? sq : 1.0f) - v.am);
            out = out * v.am + n * v.noise * v.noise * 0.6f;
            v.lp += 0.5f * (out - v.lp);
            float e = v.env.run((sm[CHAOS[c] + 12] >= 0.5f || sm[CHAOS_HOLD[c]] >= 0.5f), v.envA, v.envR);
            vs[CHAOS_MIX[c]] = v.lp * 0.3f * e;
            level[CHAOS_MIX[c]] = e;
        }
        /* VCO B, then A (synced to B), then B's FM from A */
        for (int j = 1; j >= 0; j--) {
            Vco &v = vco[j];
            float inc = v.inc;
            if (j == 1)
                inc = clampf(inc * (1.0f + sm[VB_FM] * 2.5f * vco[0].out), 0.0f, 0.45f);
            v.ph += inc;
            v.wrapped = false;
            if (v.ph >= 1.0f) {
                v.ph -= 1.0f;
                v.wrapped = true;
                v.sub = -v.sub;
            }
            if (j == 0 && sm[VA_SYNC] >= 0.5f && vco[1].wrapped) {
                v.ph = vco[1].ph * (inc / (vco[1].inc > 1e-6f ? vco[1].inc : 1e-6f));
                if (v.ph >= 1.0f)
                    v.ph -= std::floor(v.ph);
            }
            float idt = inc > 1e-6f ? inc : 1e-6f;
            auto shape = [&](int s) -> float {
                switch (s) {
                case 0: return fsin01(v.ph);
                case 1: return 1.0f - 4.0f * std::fabs(v.ph - 0.5f);
                case 2: return 2.0f * v.ph - 1.0f - blep(v.ph, idt);
                case 3: return -(2.0f * v.ph - 1.0f - blep(v.ph, idt));
                default: {
                    float w = s == 4 ? v.pw : v.pw * 0.22f;
                    float q = (v.ph < w ? 1.0f : -1.0f) + blep(v.ph, idt);
                    float t2 = v.ph + 1.0f - w;
                    q -= blep(t2 >= 1.0f ? t2 - 1.0f : t2, idt);
                    return q;
                }
                }
            };
            float wpos = v.wave * 5.0f;
            int s0 = (int)wpos;
            if (s0 > 4)
                s0 = 4;
            float fr = wpos - (float)s0;
            float o = shape(s0);
            if (fr > 0.001f)
                o += fr * (shape(s0 + 1) - o);
            v.out = o;
            /* the envelope: ADSR on the gate; SELF-GEN loops attack and release; HOLD keeps the VCA open */
            if (v.loop) {
                if (v.stage != 1 && v.stage != 4)
                    v.stage = 1;
                if (v.stage == 1) {
                    v.env += v.a;
                    if (v.env >= 1.0f) {
                        v.env = 1.0f;
                        v.stage = 4;
                    }
                } else {
                    v.env -= v.r * (v.env + 0.02f);
                    if (v.env <= 0.001f) {
                        v.env = 0.0f;
                        v.stage = 1;
                    }
                }
            } else {
                if (!gateNow && (v.stage == 1 || v.stage == 2 || v.stage == 3))
                    v.stage = 4;
                switch (v.stage) {
                case 1:
                    v.env += v.a;
                    if (v.env >= 1.0f) {
                        v.env = 1.0f;
                        v.stage = 2;
                    }
                    break;
                case 2:
                    v.env += v.d * (v.s - v.env);
                    if (std::fabs(v.env - v.s) < 0.0005f)
                        v.stage = 3;
                    break;
                case 3:
                    v.env = v.s;
                    break;
                case 4:
                    v.env -= v.r * v.env;
                    if (v.env < 1e-5f) {
                        v.env = 0.0f;
                        v.stage = 0;
                    }
                    break;
                default:
                    break;
                }
            }
            float e = v.hold ? 1.0f : v.env * (0.65f + 0.35f * vel);
            vs[j ? M_VB : M_VA] = (o * 0.42f + v.sub * v.subOn * 0.3f) * e;
            level[j ? M_VB : M_VA] = e;
        }
        /* the mixer */
        float l = 0.0f, r = 0.0f;
        for (int m = 0; m < 8; m++) {
            float g = gStart[m] + gStep[m] * (float)i;
            l += vs[m] * g * panL[m];
            r += vs[m] * g * panR[m];
        }
        L[i] = l;
        R[i] = r;
    }
    for (int m = 0; m < 8; m++)
        meterVoice[m] = level[m] * mixGT[m];

    /* the dual filter (Polivoks-style: 12 dB, its integrators saturate; the lows stay with the resonance) */
    for (int s = 0; s < 2; s++) {
        float *x = s ? R : L;
        float g0 = fg[s], dg = (fgTarget[s] - g0) / (float)BLOCK, k = fk[s];
        bool bp = (int)std::lround(sm[s ? FR_MODE : FL_MODE]) == 1;
        float res = sm[s ? FR_RES : FL_RES];
        float comp = 1.0f + 0.9f * res;
        for (int i = 0; i < BLOCK; i++) {
            float g = g0 + dg * (float)(i + 1);
            float in = ftanh(x[i] * 0.9f) * comp;
            float hp = (in - (k + g) * fs1[s] - fs2[s]) / (1.0f + g * (k + g));
            float v1 = g * hp, bpo = v1 + fs1[s];
            fs1[s] = ftanh(0.5f * (bpo + v1)) * 2.0f;
            float v2 = g * bpo, lp = v2 + fs2[s];
            fs2[s] = lp + v2;
            x[i] = bp ? bpo * (0.6f + 0.6f * k) : lp;
        }
        fg[s] = fgTarget[s];
    }
    /* the distortion after the filters: DIST its share, GAIN how hard */
    {
        float mix = sm[DIST], drive = 1.0f + 30.0f * sm[GAIN] * sm[GAIN], bias = 0.15f, off = ftanh(bias);
        float makeup = 0.8f / ftanh(drive * 0.5f + bias);
        if (mix > 0.0005f)
            for (int i = 0; i < BLOCK; i++) {
                L[i] += mix * ((ftanh(L[i] * drive + bias) - off) * makeup - L[i]);
                R[i] += mix * ((ftanh(R[i] * drive + bias) - off) * makeup - R[i]);
            }
    }
    /* the effector: the left program on the left, the right on the right; BLEND its share */
    float wl[BLOCK], wr[BLOCK];
    fx[0].process(L, wl, BLOCK, fxX, fxY, fxZ);
    fx[1].process(R, wr, BLOCK, fxX, fxY, fxZ);
    if (master < 0.0f)
        master = masterTarget;
    float m0 = master, dm = (masterTarget - master) / (float)BLOCK;
    master = masterTarget;
    for (int i = 0; i < BLOCK; i++) {
        float g = m0 + dm * (float)(i + 1);
        float l = (L[i] + blend * (wl[i] - L[i])) * g, r = (R[i] + blend * (wr[i] - R[i])) * g;
        /* nothing over the top: a soft knee above 0.8 */
        auto soft = [](float v) {
            float a = std::fabs(v);
            if (a <= 0.8f)
                return v;
            float o = 0.8f + 0.2f * ftanh((a - 0.8f) * 5.0f);
            return v < 0.0f ? -o : o;
        };
        outL[i] = soft(l);
        outR[i] = soft(r);
    }
}

/* ------------------------------------------------------------------------------------------- Engine --- */
Engine::Engine() : d(new Impl) { d->prepare(44100.0f); }
Engine::~Engine() = default;
void Engine::prepare(double rate) { d->prepare((float)rate); }
void Engine::set(int i, float v)
{
    if (i < 0 || i >= NPARAMS)
        return;
    d->p[i] = clampf(v, 0.0f, paramMax(i));
}
float Engine::get(int i) const { return i >= 0 && i < NPARAMS ? d->p[i] : 0.0f; }

void Engine::noteOn(int note, float vel)
{
    for (int i = 0; i < d->nHeld; i++)  /* (the same key again: to the top) */
        if (d->held[i] == note) {
            for (int k = i; k + 1 < d->nHeld; k++)
                d->held[k] = d->held[k + 1];
            d->nHeld--;
            break;
        }
    if (d->nHeld < 32)
        d->held[d->nHeld++] = note;
    d->note = (float)note;
    d->vel = clampf(vel, 0.0f, 1.0f);
    d->keyGate = true;
    d->gateNow = true;
    d->startEnvelopes();
}

void Engine::noteOff(int note)
{
    for (int i = 0; i < d->nHeld; i++)
        if (d->held[i] == note) {
            for (int k = i; k + 1 < d->nHeld; k++)
                d->held[k] = d->held[k + 1];
            d->nHeld--;
            break;
        }
    if (d->nHeld > 0)
        d->note = (float)d->held[d->nHeld - 1];
    else
        d->keyGate = false;
}

void Engine::allNotesOff()
{
    d->nHeld = 0;
    d->keyGate = false;
}

void Engine::panic()
{
    allNotesOff();
    for (auto &v : d->vco) {
        v.env = 0.0f;
        v.stage = 0;
    }
    for (auto &c : d->cl)
        c.env.v = 0.0f;
    for (auto &c : d->ch)
        c.env.v = 0.0f;
    for (auto &f : d->fx)
        f.clear();
    for (int s = 0; s < 2; s++)
        d->fs1[s] = d->fs2[s] = 0.0f;
}

void Engine::bend(float v) { d->bendV = clampf(v, -1.0f, 1.0f); }
void Engine::modWheel(float v) { d->wheel = clampf(v, 0.0f, 1.0f); }
void Engine::tempo(double bpm) { d->bpm = bpm > 20.0 && bpm < 400.0 ? bpm : 120.0; }
void Engine::position(double ppq, bool playing)
{
    d->ppq = ppq;
    d->ppqAt = d->rendered;
    if (playing && !d->playing)
        d->lastSyncStep = -1;
    d->playing = playing;
}

void Engine::render(float *left, float *right, int n)
{
    int done = 0;
    while (done < n) {
        if (d->pos >= BLOCK) {
            d->block();
            d->pos = 0;
        }
        int k = BLOCK - d->pos;
        if (k > n - done)
            k = n - done;
        std::memcpy(left + done, d->outL + d->pos, (size_t)k * sizeof(float));
        std::memcpy(right + done, d->outR + d->pos, (size_t)k * sizeof(float));
        d->pos += k;
        d->rendered += (uint64_t)k;
        done += k;
    }
}

float Engine::light() const { return d->meterLight.load(); }
int Engine::seqStep() const { return d->meterStep.load(); }
float Engine::voiceLevel(int v) const { return v >= 0 && v < 8 ? d->meterVoice[v].load() : 0.0f; }
float Engine::lfo(int l) const { return l >= 0 && l < 2 ? d->meterLfo[l].load() : 0.0f; }
} // namespace sol
