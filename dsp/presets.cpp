/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The factory programs: each one the parameters' defaults with its own changes. The drones are tuned in notes and
 * cents (a microtonal machine: a few cents apart they beat, which is the point). */
#include "solaris.h"
#include "maps.h"
#include <cmath>
#include <vector>

namespace sol
{
namespace
{
struct Set {
    const char *id;
    float v;
};
struct Program {
    const char *name;
    std::vector<Set> set;               /* (a vector: its own storage, on every compiler) */
};

/* a classic drone's oscillator tuned to a note (MIDI, A4 = 69) plus cents */
float tune(int osc, float midi, float cents = 0.0f)
{
    static const float base[5] = {24.0f, 24.0f, 48.0f, 96.0f, 96.0f};
    float hz = 440.0f * std::exp2((midi + cents / 100.0f - 69.0f) / 12.0f);
    float v = std::log2(hz / base[osc]) / 3.4f;
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}

#define T(d, o, m, c) Set{"d" #d "_t" #o, tune(o - 1, m, c)}

const std::vector<Program> PROGRAMS = {
    {"INIT", {}},
    {"DUSK CHOIR",
     {T(1, 1, 38, 0), T(1, 2, 45, 3), T(1, 3, 50, -4), T(1, 4, 53, 2), T(1, 5, 57, -3),
      T(2, 1, 38, 6), T(2, 2, 50, -5), T(2, 3, 57, 4), T(2, 4, 62, 0), T(2, 5, 65, -6),
      {"d1_att", 0.62f}, {"d2_att", 0.7f}, {"d1_rls", 0.7f}, {"d2_rls", 0.72f},
      {"d1_drift", 0.5f}, {"d2_drift", 0.5f}, {"d1_swirl", 0.3f}, {"d2_swirl", 0.3f}, {"orbit_rate", 0.25f},
      {"fl_freq", 0.6f}, {"fl_res", 0.35f}, {"fr_freq", 0.64f}, {"fr_res", 0.3f},
      {"la_dest", D_CUTLR}, {"la_rate", 0.25f}, {"la_wave", 1}, {"la_depth", 0.62f},
      {"fxl", 1}, {"fxr", 2}, {"fx_x", 0.7f}, {"fx_y", 0.6f}, {"fx_z", 0.45f}, {"blend", 0.55f}}},
    {"ORBIT HYMN",
     {T(1, 1, 36, 0), T(1, 2, 43, -2), T(1, 3, 48, 3), T(1, 4, 55, -2), T(1, 5, 60, 4),
      T(4, 1, 48, 0), T(4, 2, 55, 5), T(4, 3, 64, -3), T(4, 4, 67, 2), T(4, 5, 71, -4),
      {"d4_vol", 0.55f}, {"d1_att", 0.6f}, {"d4_att", 0.75f}, {"d1_swirl", 0.6f}, {"d4_swirl", 0.45f}, {"orbit_rate", 0.3f},
      {"fl_freq", 0.55f}, {"fl_res", 0.45f}, {"fr_freq", 0.68f}, {"fr_res", 0.4f}, {"fr_mode", 1},
      {"lb_dest", D_CUTR}, {"lb_rate", 0.2f}, {"lb_depth", 0.7f}, {"lb_wave", 0.8f},
      {"fxl", 0}, {"fxr", 0}, {"fx_x", 0.55f}, {"fx_y", 0.75f}, {"fx_z", 0.55f}, {"blend", 0.6f}}},
    {"SOLAR WIND",
     {{"volume", 0.9f}, {"d3_vol", 0.85f}, {"d6_vol", 0.85f}, {"d3_pitch", 0.05f}, {"d6_pitch", 0.1f}, {"d3_noise", 0.95f}, {"d6_noise", 0.9f},
      {"d3_moda", 0.2f}, {"d6_moda", 0.15f}, {"d3_rate", 0.2f}, {"d6_rate", 0.25f}, {"d3_div", 0.8f}, {"d6_div", 0.9f},
      {"d3_att", 0.65f}, {"d6_att", 0.7f}, {"fl_freq", 0.45f}, {"fl_res", 0.7f}, {"fl_mode", 1}, {"fr_freq", 0.5f},
      {"fr_res", 0.72f}, {"fr_mode", 1}, {"la_dest", D_CUTLR}, {"la_rate", 0.18f}, {"la_wave", 1}, {"la_depth", 0.85f},
      {"fxl", 15}, {"fxr", 15}, {"fx_x", 0.5f}, {"fx_y", 0.6f}, {"fx_z", 0.5f}, {"blend", 0.5f}}},
    {"DEEP FIELD",
     {{"volume", 0.55f}, T(1, 1, 26, 0), T(1, 2, 33, 7), T(1, 3, 38, -5), T(1, 4, 45, 0), T(1, 5, 50, 9),
      {"d1_volt", 0.38f}, {"d1_att", 0.7f}, {"d1_rls", 0.75f},
      {"fl_freq", 0.42f}, {"fl_res", 0.55f}, {"fr_freq", 0.46f}, {"fr_res", 0.5f}, {"f_link", 1},
      {"la_dest", D_CUT}, {"la_rate", 0.12f}, {"la_wave", 1}, {"la_depth", 0.75f},
      {"dist", 0.25f}, {"gain", 0.35f}, {"fxl", 15}, {"fxr", 1}, {"fx_x", 0.8f}, {"fx_y", 0.85f}, {"fx_z", 0.4f},
      {"blend", 0.6f}}},
    {"SIREN COAST",
     {{"d3_pitch", 0.55f}, {"d3_hi", 1}, {"d3_rate", 0.35f}, {"d3_moda", 0.5f}, {"d3_fm", 1},
      {"d6_pitch", 0.05f}, {"d6_noise", 1}, {"d6_moda", 0.1f}, {"d6_div", 1}, {"d6_rate", 0.1f},
      {"d6_am", 1}, {"d6_vol", 0.6f}, {"d3_vol", 0.45f}, {"d3_pan", 0.25f}, {"d6_pan", 0.75f},
      {"fl_freq", 0.66f}, {"fl_res", 0.4f}, {"fr_freq", 0.58f}, {"fr_res", 0.3f},
      {"fxl", 3}, {"fxr", 1}, {"fx_x", 0.5f}, {"fx_y", 0.55f}, {"fx_z", 0.6f}, {"blend", 0.45f}}},
    {"BIRD MACHINE",
     {{"d3_pitch", 0.62f}, {"d3_hi", 1}, {"d3_rate", 0.55f}, {"d3_moda", 0.35f}, {"d3_sh", 0.5f},
      {"d3_am", 1}, {"d6_pitch", 0.7f}, {"d6_hi", 1}, {"d6_rate", 0.5f}, {"d6_x10", 0},
      {"d6_moda", 0.6f}, {"d6_sh", 0.25f}, {"d6_am", 1}, {"d3_pan", 0.2f}, {"d6_pan", 0.8f},
      {"d3_vol", 0.5f}, {"d6_vol", 0.45f}, {"fl_freq", 0.8f}, {"fr_freq", 0.8f},
      {"fxl", 17}, {"fxr", 2}, {"fx_x", 0.55f}, {"fx_y", 0.4f}, {"fx_z", 0.6f}, {"blend", 0.5f}}},
    {"NEON RAIN",
     {{"va_wave", 0.4f}, {"vb_wave", 0.6f}, {"vb_tune", 0.5f + 0.07f / 12.0f * 0.0f}, {"vb_oct", 2}, {"va_oct", 1},
      {"vb_tune", 0.508f}, {"va_a", 0.45f}, {"vb_a", 0.45f}, {"va_d", 0.6f}, {"vb_d", 0.6f}, {"va_s", 0.6f},
      {"vb_s", 0.6f}, {"va_r", 0.7f}, {"vb_r", 0.7f}, {"glide", 0.25f},
      T(2, 1, 36, 0), T(2, 2, 43, 4), T(2, 3, 48, -4), T(2, 4, 63, 3), T(2, 5, 67, -2), 
      {"d2_vol", 0.45f}, {"d2_att", 0.7f}, {"fl_freq", 0.62f}, {"fl_res", 0.45f}, {"fr_freq", 0.62f},
      {"fr_res", 0.45f}, {"f_link", 1}, {"la_dest", D_CUT}, {"la_rate", 0.45f}, {"la_wave", 1}, {"la_depth", 0.65f},
      {"fxl", 4}, {"fxr", 0}, {"fx_x", 0.5f}, {"fx_y", 0.55f}, {"fx_z", 0.4f}, {"blend", 0.5f}}},
    {"GLASS MOON",
     {{"va_wave", 0.0f}, {"vb_wave", 0.2f}, {"va_oct", 3}, {"vb_oct", 4}, {"vb_tune", 0.5f + 7.0f / 12.0f * 0.0f},
      {"vb_fm", 0.25f}, {"va_a", 0.2f}, {"vb_a", 0.2f}, {"va_d", 0.7f}, {"vb_d", 0.65f}, {"va_s", 0.0f},
      {"vb_s", 0.0f}, {"va_r", 0.75f}, {"vb_r", 0.75f}, {"seq_run", 1}, {"seq_dest", 0}, {"seq_clock", 3},
      {"seq_depth", 0.85f}, {"seq_s1", 0.0f}, {"seq_s2", 0.29f}, {"seq_s3", 0.5f}, {"seq_s4", 0.21f}, {"seq_s5", 0.58f},
      {"grid", 1}, T(5, 1, 40, 0), T(5, 2, 47, 3), T(5, 3, 52, -3), T(5, 4, 59, 0), T(5, 5, 64, 5), 
      {"d5_vol", 0.4f}, {"d5_att", 0.75f}, {"fl_freq", 0.75f}, {"fr_freq", 0.7f},
      {"fxl", 0}, {"fxr", 17}, {"fx_x", 0.6f}, {"fx_y", 0.65f}, {"fx_z", 0.5f}, {"blend", 0.55f}}},
    {"IRON SKY",
     {{"volume", 0.5f}, T(1, 1, 31, 0), T(1, 2, 38, 0), T(1, 3, 43, 0), T(1, 4, 50, 0), T(1, 5, 55, 0), 
      {"d1_volt", 0.82f}, {"d1_att", 0.45f}, T(2, 1, 31, 12), T(2, 2, 37, 0), T(2, 3, 44, 0), T(2, 4, 49, 0),
      T(2, 5, 56, 0), {"d2_volt", 0.75f}, {"fl_freq", 0.5f}, {"fl_res", 0.6f}, {"fr_freq", 0.52f},
      {"fr_res", 0.6f}, {"dist", 0.6f}, {"gain", 0.6f}, {"la_dest", D_CUTLR}, {"la_rate", 0.3f}, {"la_wave", 0.2f},
      {"la_depth", 0.7f}, {"fxl", 20}, {"fxr", 4}, {"fx_x", 0.45f}, {"fx_y", 0.5f}, {"fx_z", 0.5f}, {"blend", 0.4f}}},
    {"HELIOS",
     {T(4, 1, 50, 0), T(4, 2, 57, 2), T(4, 3, 62, -2), T(4, 4, 66, 3), T(4, 5, 69, -3), 
      T(5, 1, 45, 4), T(5, 2, 52, -4), T(5, 3, 57, 0), T(5, 4, 64, 5), T(5, 5, 73, -2), 
      {"orbit_src", 1}, {"d4_swirl", 0.5f}, {"d5_swirl", 0.5f}, {"d4_drift", 0.45f}, {"d5_drift", 0.45f},
      {"la_rate", 0.15f}, {"la_wave", 1}, {"d4_att", 0.7f}, {"d5_att", 0.72f}, {"fl_freq", 0.78f},
      {"fr_freq", 0.76f}, {"fl_res", 0.2f}, {"fr_res", 0.2f}, {"fxl", 0}, {"fxr", 1}, {"fx_x", 0.75f},
      {"fx_y", 0.8f}, {"fx_z", 0.55f}, {"blend", 0.65f}}},
    {"DRIFT CELLS",
     {T(1, 1, 33, 0), T(1, 2, 40, 0), T(1, 3, 45, 0), T(1, 4, 52, 0), T(1, 5, 57, 0), 
      {"orbit_src", 3}, {"d1_swirl", 0.7f}, {"d1_drift", 0.75f}, {"d1_tone", 0.75f},
      {"seq_run", 1}, {"seq_dest", D_FXX}, {"seq_depth", 0.5f}, {"seq_rate", 0.3f},
      {"seq_s1", 0.5f}, {"seq_s2", 0.57f}, {"seq_s3", 0.43f}, {"seq_s4", 0.5f}, {"seq_s5", 0.64f},
      {"fl_freq", 0.6f}, {"fr_freq", 0.6f}, {"fl_res", 0.4f}, {"fr_res", 0.4f},
      {"fxl", 16}, {"fxr", 16}, {"fx_x", 0.5f}, {"fx_y", 0.45f}, {"fx_z", 0.5f}, {"blend", 0.5f}}},
    {"FROZEN SUN",
     {T(2, 1, 41, 0), T(2, 2, 48, 3), T(2, 3, 53, -3), T(2, 4, 60, 2), T(2, 5, 64, -2), 
      {"d2_att", 0.8f}, {"d2_rls", 0.8f}, {"va_wave", 0.6f}, {"va_oct", 3}, {"va_a", 0.6f}, {"va_r", 0.8f},
      {"vb_vol", 0.0f}, {"fl_freq", 0.66f}, {"fr_freq", 0.7f}, {"fxl", 14}, {"fxr", 0}, {"fx_x", 0.9f},
      {"fx_y", 0.6f}, {"fx_z", 0.7f}, {"blend", 0.65f}}},
    {"RUST ORGAN",
     {{"volume", 0.6f}, T(1, 1, 36, 0), T(1, 2, 48, 0), T(1, 3, 55, 0), T(1, 4, 60, 0), T(1, 5, 64, 0), 
      T(2, 1, 41, 0), T(2, 2, 53, 0), T(2, 3, 57, 0), T(2, 4, 65, 0), T(2, 5, 69, 0), 
      {"grid", 1}, {"d1_volt", 0.56f}, {"d2_volt", 0.56f}, {"fl_freq", 0.58f}, {"fr_freq", 0.6f},
      {"fl_res", 0.25f}, {"fr_res", 0.25f}, {"dist", 0.3f}, {"gain", 0.3f}, {"fxl", 7}, {"fxr", 6},
      {"fx_x", 0.55f}, {"fx_y", 0.35f}, {"fx_z", 0.5f}, {"blend", 0.55f}}},
    {"ECHO PLANET",
     {{"va_wave", 0.8f}, {"vb_wave", 0.85f}, {"va_pw", 0.3f}, {"vb_oct", 3}, {"va_a", 0.15f}, {"vb_a", 0.15f},
      {"va_d", 0.5f}, {"vb_d", 0.5f}, {"va_s", 0.2f}, {"vb_s", 0.2f}, {"va_r", 0.55f}, {"vb_r", 0.55f},
      {"seq_run", 1}, {"seq_dest", 0}, {"seq_clock", 4}, {"seq_depth", 0.7f}, {"grid", 1},
      {"seq_s1", 0.0f}, {"seq_s2", 0.5f}, {"seq_s3", 0.29f}, {"seq_s4", 0.0f}, {"seq_s5", 0.42f}, {"seq_g4", 0},
      {"fl_freq", 0.55f}, {"fl_res", 0.6f}, {"fr_freq", 0.55f}, {"fr_res", 0.6f}, {"f_link", 1},
      {"lb_dest", D_CUT}, {"lb_rate", 0.3f}, {"lb_depth", 0.7f}, {"lb_wave", 1},
      {"fxl", 3}, {"fxr", 5}, {"fx_x", 0.6f}, {"fx_y", 0.45f}, {"fx_z", 0.7f}, {"blend", 0.5f}}},
    {"BLACK SUN",
     {{"volume", 0.5f}, T(1, 1, 24, 0), T(1, 2, 31, 11), T(1, 3, 36, -7), T(1, 4, 49, 0), T(1, 5, 54, 13), 
      {"d1_volt", 0.25f}, {"d3_pitch", 0.1f}, {"d3_noise", 0.7f}, {"d3_div", 0.9f},
      {"d3_vol", 0.4f}, {"fl_freq", 0.38f}, {"fl_res", 0.75f}, {"fr_freq", 0.42f}, {"fr_res", 0.7f},
      {"la_dest", D_CUTLR}, {"la_rate", 0.1f}, {"la_wave", 1}, {"la_depth", 0.8f}, {"dist", 0.35f},
      {"gain", 0.5f}, {"fxl", 15}, {"fxr", 15}, {"fx_x", 0.9f}, {"fx_y", 0.9f}, {"fx_z", 0.6f}, {"blend", 0.7f}}},
};
#undef T
const int N = (int)PROGRAMS.size();
} // namespace

int numPresets() { return N; }
const char *presetName(int k) { return PROGRAMS[k < 0 ? 0 : k >= N ? N - 1 : k].name; }

void presetValues(int k, float *out)
{
    k = k < 0 ? 0 : k >= N ? N - 1 : k;
    for (int i = 0; i < NPARAMS; i++)
        out[i] = PARAMS[i].def;
    for (const Set &s : PROGRAMS[k].set) {
        int i = paramIndex(s.id);
        if (i >= 0)
            out[i] = s.v;
    }
}
} // namespace sol
