/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* SOLARIS-23: a microtonal drone and ambient machine, after the idea of the analogue drone synths of the 2020s.
 *
 * Eight voices into a stereo mixer with pan, a dual 12 dB filter (low-pass or band-pass, Polivoks-style), a stereo
 * distortion after it and a dual effector (a cartridge program each side). The same engine runs in the VST3 / the
 * standalone app (JUCE, src/) and on the Akai MPC (mpc/); tests/ checks it without either.
 *
 *   DRONE 1, 2, 4, 5   five free-running saw oscillators each (1-2 low, 3 middle, 4-5 high), each with its own TUNE,
 *                      on/off and MOD (the light sensor moves its pitch); VOLT bends all five down, past the middle
 *                      they modulate each other (FM); an attack/release envelope opened by the drone key
 *   DRONE 3, 6         chaos voices: a square modulator (RATE) and a Schmitt-trigger tone (PITCH), FM and/or AM,
 *                      a divider, noise into the tone and a sample & hold of the noise: sirens, birds, surf
 *   VCO A, VCO B       morphing oscillators (sine - triangle - saw - ramp - square - pulse), sub octave, an ADSR
 *                      each (HOLD: always open; SELF-GEN: the envelope loops as an LFO), A synced to B, B's FM from
 *                      A, both played by the touch plates / MIDI notes
 *   modulation         LFO A and B, the joystick (X, Y), a 5-step sequencer (with gates, 3/4/5 stages, its own
 *                      pulser or the host's tempo), the light sensor (ambient drift, or an LFO or the sequencer)
 *
 * Every parameter is a float: K_CONT and K_BIP 0..1, K_SW 0 or 1, K_CH the choice's index. The engine runs on a grid
 * of 16 samples (control rate), so any buffer size renders the same. All calls from the audio thread except prepare. */
#pragma once
#include <cstdint>
#include <memory>

namespace sol
{
enum Kind { K_CONT, K_BIP, K_SW, K_CH };

#define P(e, id, name, shortName, kind, ch, def) e,
enum ParamId {
#include "params_list.inc"
    NPARAMS
};
#undef P

struct ParamDef {
    const char *id, *name, *shortName;
    int kind;
    const char *const *choices;          /* K_CH: the names */
    int nChoices;                        /* K_CH: how many; K_SW: 2; else 0 */
    float def;
};
extern const ParamDef PARAMS[NPARAMS];
int paramIndex(const char *id);          /* -1: none */
void paramText(int i, float v, char *out, int n);   /* the value as the panel and the host show it */
float paramMax(int i);                   /* 1 (K_CONT, K_BIP, K_SW) or nChoices - 1 */

/* the dual effector: 8 cartridges of 3 programs */
constexpr int NFX = 24, NCART = 8;
const char *fxName(int fx);              /* "SHIMMER" */
const char *cartName(int cart);          /* "NEBULA" */
const char *fxLabel(int fx, int k);      /* X, Y, Z: "DECAY" */

/* the destinations of LFO A, B, the joystick and the sequencer (CH_DEST; the sequencer's 0 is VCO PLAY) */
enum Dest { D_OFF, D_CUTL, D_CUTR, D_CUT, D_CUTLR, D_VPITCH, D_VPW, D_VWAVE, D_VFM, D_CHAOS, D_DRONES, D_FXX, D_FXY,
            D_FXZ, D_BLEND, NDEST };

/* the factory programs */
int numPresets();
const char *presetName(int k);
void presetValues(int k, float *out);   /* NPARAMS values (VOLUME too) */

struct Impl;
class Engine
{
public:
    Engine();
    ~Engine();
    void prepare(double sampleRate);     /* a fresh engine (not on the audio thread: it sizes the delay lines) */
    void set(int i, float v);            /* a parameter */
    float get(int i) const;
    void noteOn(int note, float vel, bool plate = false);   /* plate: a touch plate's, tuned by its PLATE param */
    void noteOff(int note, bool plate = false);
    void allNotesOff();
    void panic();                        /* silent at once: the voices, the effects' tails */
    void bend(float v);                  /* -1..1 */
    void modWheel(float v);              /* 0..1: more of the joystick's Y */
    void tempo(double bpm);
    void position(double ppq, bool playing);
    void render(float *left, float *right, int n);

    /* for the panel (read on another thread: approximate) */
    float light() const;                 /* 0..1 */
    int seqStep() const;                 /* 0..4 */
    float voiceLevel(int v) const;       /* 0..7: DRONE 1, 2, 3, VCO A, VCO B, DRONE 4, 5, 6 (the mixer's order) */
    float lfo(int l) const;

private:
    std::unique_ptr<Impl> d;
};
} // namespace sol
