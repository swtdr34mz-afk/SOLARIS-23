/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The parameters (dsp/params_list.inc, made by tools/gen_params.py), their choices and their value texts. */
#include "solaris.h"
#include "maps.h"
#include <cstdio>
#include <cstring>

namespace sol
{
namespace
{
const char *const CH_LOHI[] = {"LOW", "HIGH"};
const char *const CH_X1X10[] = {"x1", "x10"};
const char *const CH_OCT[] = {"-2", "-1", "0", "+1", "+2", "+3"};
const char *const CH_LPBP[] = {"LP", "BP"};
const char *const CH_X1X6X10[] = {"x1", "x6", "x10"};
const char *const CH_DEST[] = {"OFF",    "CUT L",  "CUT R",   "CUT L+R", "CUT L/R", "VCO PTCH", "VCO PW", "VCO WAVE",
                               "VCO FM", "CHAOS",  "DRONES",  "FX X",    "FX Y",    "FX Z",     "BLEND"};
const char *const CH_SEQDEST[] = {"VCO PLAY", "CUT L",  "CUT R",  "CUT L+R", "CUT L/R", "VCO PTCH", "VCO PW", "VCO WAVE",
                                  "VCO FM",   "CHAOS",  "DRONES", "FX X",    "FX Y",    "FX Z",     "BLEND"};
const char *const CH_STAGES[] = {"3", "4", "5"};
const char *const CH_CLOCK[] = {"PULSER", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/16T"};
const char *const CH_LIGHT[] = {"AMBIENT", "LFO A", "LFO B", "SEQ"};
const char *const CH_ORBIT[] = {"FREE", "LFO A", "LFO B", "SEQ"};
const char *const CH_GRID[] = {"FREE", "SEMI"};
const char *const CH_BEND[] = {"OFF", "1", "2", "7", "12"};

const char *const FX_NAME[NFX] = {"SHIMMER", "CLOUD",    "HALL",    "ECHOVERB", "TAPE",     "PITCHDLY",
                                  "TREMOLO", "CHORUS",   "PHASER",  "AUTOWAH",  "RESONATR", "FORMANT",
                                  "OCTAVER", "HARMONZR", "FREEZE",  "BLACKHOL", "GRAINS",   "SPACEDLY",
                                  "RINGMOD", "S&H RING", "FOLD",    "CRUSH",    "VINYL",    "RADIO"};
const char *const CH_FX[NFX] = {"SHIMMER", "CLOUD",    "HALL",    "ECHOVERB", "TAPE",     "PITCHDLY",
                                "TREMOLO", "CHORUS",   "PHASER",  "AUTOWAH",  "RESONATR", "FORMANT",
                                "OCTAVER", "HARMONZR", "FREEZE",  "BLACKHOL", "GRAINS",   "SPACEDLY",
                                "RINGMOD", "S&H RING", "FOLD",    "CRUSH",    "VINYL",    "RADIO"};
const char *const CART[NCART] = {"NEBULA", "ORBIT", "PULSAR", "COMET", "GRAVITY", "VOID", "AURORA", "DUST"};
const char *const FX_LABEL[NFX][3] = {
    {"OCT UP", "DECAY", "TONE"},     {"SIZE", "DECAY", "MOTION"},    {"PRE-DELAY", "DECAY", "DAMP"},
    {"FEEDBACK", "TIME", "REVERB"},  {"FEEDBACK", "TIME", "WOBBLE"}, {"FEEDBACK", "TIME", "PITCH"},
    {"DEPTH", "RATE", "REVERB"},     {"DEPTH", "RATE", "REVERB"},    {"DEPTH", "RATE", "FEEDBACK"},
    {"SENS", "FREQ", "RESO"},        {"PITCH", "DECAY", "BRIGHT"},   {"VOWEL", "RATE", "RESO"},
    {"OCT DOWN", "OCT UP", "DIRECT"}, {"PITCH 1", "PITCH 2", "MIX"}, {"HOLD", "LENGTH", "SMEAR"},
    {"SIZE", "FEEDBACK", "MOTION"},  {"DENSITY", "SIZE", "SCATTER"}, {"FEEDBACK", "TIME", "DIFFUSE"},
    {"FREQ", "RATE", "REVERB"},      {"FREQ", "S&H RATE", "SPREAD"}, {"FOLD", "BIAS", "TONE"},
    {"RATE", "BITS", "TONE"},        {"CRACKLE", "WEAR", "WOW"},     {"BAND", "STATIC", "DRIVE"},
};

template <size_t N> constexpr const char *const *chp(const char *const (&a)[N]) { return a; }
constexpr const char *const *chp(int) { return nullptr; }
template <size_t N> constexpr int chn(const char *const (&)[N]) { return (int)N; }
constexpr int chn(int) { return 0; }
} // namespace

#define P(e, id, name, shortName, kind, ch, def) {id, name, shortName, kind, chp(ch), kind == K_SW ? 2 : chn(ch), def},
const ParamDef PARAMS[NPARAMS] = {
#include "params_list.inc"
};
#undef P

const char *fxName(int fx) { return FX_NAME[fx < 0 ? 0 : fx >= NFX ? NFX - 1 : fx]; }
const char *cartName(int c) { return CART[c < 0 ? 0 : c >= NCART ? NCART - 1 : c]; }
const char *fxLabel(int fx, int k) { return FX_LABEL[fx < 0 ? 0 : fx >= NFX ? NFX - 1 : fx][k < 0 ? 0 : k > 2 ? 2 : k]; }

int paramIndex(const char *id)
{
    for (int i = 0; i < NPARAMS; i++)
        if (!std::strcmp(PARAMS[i].id, id))
            return i;
    return -1;
}

float paramMax(int i) { return PARAMS[i].kind == K_CH ? (float)(PARAMS[i].nChoices - 1) : 1.0f; }

static bool between(int i, int a, int b) { return i >= a && i <= b; }

void paramText(int i, float v, char *out, int n)
{
    if (i < 0 || i >= NPARAMS || n <= 0) {
        if (n > 0)
            out[0] = 0;
        return;
    }
    const ParamDef &d = PARAMS[i];
    v = v != v ? 0.0f : v < 0.0f ? 0.0f : v > paramMax(i) ? paramMax(i) : v;   /* (anything a host asks about) */
    if (d.kind == K_CH) {
        int k = (int)std::lround(v);
        k = k < 0 ? 0 : k >= d.nChoices ? d.nChoices - 1 : k;
        std::snprintf(out, (size_t)n, "%s", d.choices[k]);
        return;
    }
    if (d.kind == K_SW) {
        std::snprintf(out, (size_t)n, "%s", v >= 0.5f ? "ON" : "OFF");
        return;
    }
    auto hz = [&](float f) {
        if (f >= 1000.0f)
            std::snprintf(out, (size_t)n, "%.2f kHz", f / 1000.0f);
        else if (f >= 100.0f)
            std::snprintf(out, (size_t)n, "%.0f Hz", f);
        else if (f >= 10.0f)
            std::snprintf(out, (size_t)n, "%.1f Hz", f);
        else
            std::snprintf(out, (size_t)n, "%.2f Hz", f);
    };
    auto secs = [&](float t) {
        if (t < 1.0f)
            std::snprintf(out, (size_t)n, "%.0f ms", t * 1000.0f);
        else
            std::snprintf(out, (size_t)n, "%.1f s", t);
    };
    auto pct = [&](float x) { std::snprintf(out, (size_t)n, "%.0f%%", x * 100.0f); };
    auto bipPct = [&](float x) { std::snprintf(out, (size_t)n, "%+.0f%%", map::bip(x) * 100.0f); };

    if (i == VOLUME) {
        float g = map::volume(v);
        if (g <= 0.0001f)
            std::snprintf(out, (size_t)n, "-inf dB");
        else
            std::snprintf(out, (size_t)n, "%.1f dB", 20.0f * std::log10(g));
        return;
    }
    /* the classic drones: TUNE (Hz), VOLT, ATTACK, RELEASE */
    static const int classic[4] = {D1_T1, D2_T1, D4_T1, D5_T1};
    for (int c = 0; c < 4; c++) {
        int b = classic[c];
        if (between(i, b, b + 4)) {
            hz(map::classicHz(i - b, v));
            return;
        }
        if (i == b + 15) {               /* VOLT: down (transposed) or FM */
            float x = map::bip(v);
            if (x < -0.005f)
                std::snprintf(out, (size_t)n, "DN %.0f%%", -x * 100.0f);
            else if (x > 0.005f)
                std::snprintf(out, (size_t)n, "FM %.0f%%", x * 100.0f);
            else
                std::snprintf(out, (size_t)n, "0");
            return;
        }
        if (i == b + 16 || i == b + 17) {
            secs(map::arTime(v));
            return;
        }
    }
    static const int chaos[2] = {D3_PITCH, D6_PITCH};
    for (int c = 0; c < 2; c++) {
        int b = chaos[c];
        if (i == b + 2) {                /* RATE (x1; x10 is shown by its switch) */
            hz(map::chaosRateHz(v, false));
            return;
        }
        if (i == b + 10 || i == b + 11) {
            secs(map::arTime(v));
            return;
        }
    }
    switch (i) {
    case VA_TUNE: case VB_TUNE:
        std::snprintf(out, (size_t)n, "%+.2f st", map::vcoTune(v));
        return;
    default:
        break;
    }
    if (i >= PLATE_1 && i <= PLATE_12) {   /* a plate's offset from its note */
        std::snprintf(out, (size_t)n, "%+.2f st", map::bip(v) * 12.0f);
        return;
    }
    switch (i) {
    case VA_A: case VB_A:
        secs(map::vcoAttack(v));
        return;
    case VA_D: case VB_D: case VA_R: case VB_R:
        secs(map::vcoDecay(v));
        return;
    case GLIDE:
        secs(map::glide(v));
        return;
    case FL_FREQ: case FR_FREQ:
        hz(map::filterHz(v));
        return;
    case LA_RATE: case LB_RATE:
        hz(map::lfoHz(v, 0));
        return;
    case SEQ_RATE:
        hz(map::seqHz(v));
        return;
    case LIGHT_RATE:
        hz(map::lightHz(v));
        return;
    case ORBIT_RATE:
        hz(map::orbitHz(v));
        return;
    case ORBIT_WIDTH:
        std::snprintf(out, (size_t)n, "%.0f%%", map::orbitWidth(v) * 100.0f);
        return;
    case D1_TONE: case D2_TONE: case D4_TONE: case D5_TONE:
        hz(map::droneToneHz(v));
        return;
    case SEQ_S1: case SEQ_S2: case SEQ_S3: case SEQ_S4: case SEQ_S5:
        std::snprintf(out, (size_t)n, "%.2f V", v * 5.0f);
        return;
    case LA_WAVE: case LB_WAVE:          /* square .. triangle */
        if (v < 0.02f)
            std::snprintf(out, (size_t)n, "SQUARE");
        else if (v > 0.98f)
            std::snprintf(out, (size_t)n, "TRIANGLE");
        else
            std::snprintf(out, (size_t)n, "TRI %.0f%%", v * 100.0f);
        return;
    case VA_WAVE: case VB_WAVE: {
        static const char *const w[6] = {"SINE", "TRI", "SAW", "RAMP", "SQUARE", "PULSE"};
        float x = v * 5.0f;
        int k = (int)std::lround(x);
        if (std::fabs(x - (float)k) < 0.06f)
            std::snprintf(out, (size_t)n, "%s", w[k]);
        else
            std::snprintf(out, (size_t)n, "%s>%s", w[(int)x], w[(int)x + 1 > 5 ? 5 : (int)x + 1]);
        return;
    }
    default:
        break;
    }
    if (d.kind == K_BIP) {
        bool oscPan = false;
        static const int OSC_PAN1[4] = {D1_PAN1, D2_PAN1, D4_PAN1, D5_PAN1};
        for (int b : OSC_PAN1)
            oscPan = oscPan || between(i, b, b + 4);
        if (between(i, D1_PAN, D6_PAN) || oscPan) {
            float x = map::bip(v);
            if (std::fabs(x) < 0.01f)
                std::snprintf(out, (size_t)n, "C");
            else
                std::snprintf(out, (size_t)n, "%s%.0f", x < 0 ? "L" : "R", std::fabs(x) * 50.0f);
            return;
        }
        bipPct(v);
        return;
    }
    pct(v);
}
} // namespace sol
