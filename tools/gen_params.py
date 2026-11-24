#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
# Copyright (C) 2026 swtdr34mz (SOLARIS-23)
"""Writes dsp/params_list.inc: SOLARIS-23's parameters, one line each, in their permanent order.

    python3 tools/gen_params.py

The order is the MPC plugin's parameter order and the VST's ids: never reorder, only append at the end.
Each line: P(ENUM, "id", "LONG NAME", "Short name", kind, choices, default)
  kind: K_CONT (0..1), K_BIP (0..1, centre 0.5), K_SW (0/1), K_CH (0..n-1 of the named choice list)
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
rows = []


def P(enum, pid, name, short, kind="K_CONT", ch="0", default=0.0):
    assert len(short) <= 12, short
    rows.append((enum, pid, name, short, kind, ch, repr(float(default))))


P("VOLUME", "volume", "MASTER VOLUME", "Volume", default=0.7)

# the classic drones (DRONE 1, 2, 4, 5): five free-running saws each, oscillators 1-2 low, 3 middle, 4-5 high
CLASSIC_TUNE = {1: [0.30, 0.46, 0.40, 0.42, 0.58], 2: [0.36, 0.52, 0.47, 0.38, 0.55],
                4: [0.44, 0.60, 0.55, 0.47, 0.66], 5: [0.40, 0.33, 0.62, 0.51, 0.45]}
for d in (1, 2, 4, 5):
    for o in range(1, 6):
        P("D%d_T%d" % (d, o), "d%d_t%d" % (d, o), "DRONE %d TUNE %d" % (d, o), "D%d Tune %d" % (d, o), default=CLASSIC_TUNE[d][o - 1])
    for o in range(1, 6):
        P("D%d_ON%d" % (d, o), "d%d_on%d" % (d, o), "DRONE %d OSC %d" % (d, o), "D%d Osc %d" % (d, o), "K_SW", default=1)
    for o in range(1, 6):
        P("D%d_MOD%d" % (d, o), "d%d_mod%d" % (d, o), "DRONE %d MOD %d" % (d, o), "D%d Mod %d" % (d, o), "K_SW", default=0)
    P("D%d_VOLT" % d, "d%d_volt" % d, "DRONE %d VOLT" % d, "D%d Volt" % d, "K_BIP", default=0.5)
    P("D%d_ATT" % d, "d%d_att" % d, "DRONE %d ATTACK" % d, "D%d Attack" % d, default=0.35)
    P("D%d_RLS" % d, "d%d_rls" % d, "DRONE %d RELEASE" % d, "D%d Release" % d, default=0.45)
    P("D%d_GATE" % d, "d%d_gate" % d, "DRONE %d KEY" % d, "D%d Key" % d, "K_SW", default=0)

# the chaos drones (DRONE 3, 6): two Schmitt-trigger oscillators, a square modulator and a tone
for d in (3, 6):
    P("D%d_PITCH" % d, "d%d_pitch" % d, "DRONE %d PITCH" % d, "D%d Pitch" % d, default=0.45)
    P("D%d_HI" % d, "d%d_hi" % d, "DRONE %d RANGE" % d, "D%d Range" % d, "K_CH", "CH_LOHI", default=0)
    P("D%d_RATE" % d, "d%d_rate" % d, "DRONE %d RATE" % d, "D%d Rate" % d, default=0.4)
    P("D%d_X10" % d, "d%d_x10" % d, "DRONE %d RATE RANGE" % d, "D%d Rate Rng" % d, "K_CH", "CH_X1X10", default=0)
    P("D%d_MODA" % d, "d%d_moda" % d, "DRONE %d MOD" % d, "D%d Mod" % d, default=0.3)
    P("D%d_DIV" % d, "d%d_div" % d, "DRONE %d DIVIDER" % d, "D%d Divider" % d, default=0.0)
    P("D%d_NOISE" % d, "d%d_noise" % d, "DRONE %d NOISE" % d, "D%d Noise" % d, default=0.0)
    P("D%d_SH" % d, "d%d_sh" % d, "DRONE %d S&H" % d, "D%d S&H" % d, default=0.0)
    P("D%d_FM" % d, "d%d_fm" % d, "DRONE %d FM" % d, "D%d FM" % d, "K_SW", default=1)
    P("D%d_AM" % d, "d%d_am" % d, "DRONE %d AM" % d, "D%d AM" % d, "K_SW", default=0)
    P("D%d_ATT" % d, "d%d_att" % d, "DRONE %d ATTACK" % d, "D%d Attack" % d, default=0.3)
    P("D%d_RLS" % d, "d%d_rls" % d, "DRONE %d RELEASE" % d, "D%d Release" % d, default=0.4)
    P("D%d_GATE" % d, "d%d_gate" % d, "DRONE %d KEY" % d, "D%d Key" % d, "K_SW", default=0)

# the two VCOs (the touch plates / MIDI keys play both)
for v in ("A", "B"):
    P("V%s_WAVE" % v, "v%s_wave" % v.lower(), "VCO %s WAVEFORM" % v, "VCO%s Wave" % v, default=0.4 if v == "A" else 0.55)
    P("V%s_PW" % v, "v%s_pw" % v.lower(), "VCO %s PULSE WIDTH" % v, "VCO%s PW" % v, default=0.5)
    P("V%s_TUNE" % v, "v%s_tune" % v.lower(), "VCO %s TUNE" % v, "VCO%s Tune" % v, "K_BIP", default=0.5)
    P("V%s_OCT" % v, "v%s_oct" % v.lower(), "VCO %s OCTAVE" % v, "VCO%s Octave" % v, "K_CH", "CH_OCT", default=2)
    P("V%s_SUB" % v, "v%s_sub" % v.lower(), "VCO %s SUB" % v, "VCO%s Sub" % v, "K_SW", default=0)
    P("V%s_A" % v, "v%s_a" % v.lower(), "VCO %s ATTACK" % v, "VCO%s Attack" % v, default=0.25)
    P("V%s_D" % v, "v%s_d" % v.lower(), "VCO %s DECAY" % v, "VCO%s Decay" % v, default=0.5)
    P("V%s_S" % v, "v%s_s" % v.lower(), "VCO %s SUSTAIN" % v, "VCO%s Sustn" % v, default=0.7)
    P("V%s_R" % v, "v%s_r" % v.lower(), "VCO %s RELEASE" % v, "VCO%s Releas" % v, default=0.5)
    P("V%s_HOLD" % v, "v%s_hold" % v.lower(), "VCO %s HOLD" % v, "VCO%s Hold" % v, "K_SW", default=0)
    P("V%s_LOOP" % v, "v%s_loop" % v.lower(), "VCO %s SELF-GEN" % v, "VCO%s Loop" % v, "K_SW", default=0)
P("VA_SYNC", "va_sync", "VCO A SYNC", "VCOA Sync", "K_SW", default=0)
P("VB_FM", "vb_fm", "VCO B FM", "VCOB FM", default=0.0)
P("GLIDE", "glide", "GLIDE", "Glide", default=0.0)

# the mixer: DRONE 1..6, VCO A, VCO B
MIX = [("D1", "DRONE 1"), ("D2", "DRONE 2"), ("D3", "DRONE 3"), ("VA", "VCO A"), ("VB", "VCO B"),
       ("D4", "DRONE 4"), ("D5", "DRONE 5"), ("D6", "DRONE 6")]
PANS = {"D1": 0.3, "D2": 0.62, "D3": 0.4, "VA": 0.45, "VB": 0.55, "D4": 0.7, "D5": 0.38, "D6": 0.6}
for k, n in MIX:
    P("%s_VOL" % k, "%s_vol" % k.lower(), "%s VOLUME" % n, "%s Vol" % n.replace("DRONE ", "Drn").replace("VCO ", "VCO"),
      default=0.7)
for k, n in MIX:
    P("%s_PAN" % k, "%s_pan" % k.lower(), "%s PAN" % n, "%s Pan" % n.replace("DRONE ", "Drn").replace("VCO ", "VCO"),
      "K_BIP", default=PANS[k])

# the dual Polivoks-style filter and the distortion after it
P("FL_FREQ", "fl_freq", "FILTER L FREQ", "Filt L Freq", default=0.72)
P("FL_RES", "fl_res", "FILTER L RES", "Filt L Res", default=0.25)
P("FL_MODE", "fl_mode", "FILTER L MODE", "Filt L Mode", "K_CH", "CH_LPBP", default=0)
P("FR_FREQ", "fr_freq", "FILTER R FREQ", "Filt R Freq", default=0.72)
P("FR_RES", "fr_res", "FILTER R RES", "Filt R Res", default=0.25)
P("FR_MODE", "fr_mode", "FILTER R MODE", "Filt R Mode", "K_CH", "CH_LPBP", default=0)
P("F_LINK", "f_link", "FILTER LINK", "Filt Link", "K_SW", default=0)
P("DIST", "dist", "DISTORTION", "Dist", default=0.0)
P("GAIN", "gain", "DISTORTION GAIN", "Dist Gain", default=0.4)

# modulation: two LFOs, the joystick, the 5-step sequencer, the light sensor
for l in ("A", "B"):
    P("L%s_WAVE" % l, "l%s_wave" % l.lower(), "LFO %s WAVE" % l, "LFO%s Wave" % l, default=0.5)
    P("L%s_RATE" % l, "l%s_rate" % l.lower(), "LFO %s RATE" % l, "LFO%s Rate" % l, default=0.4)
    P("L%s_RANGE" % l, "l%s_range" % l.lower(), "LFO %s RANGE" % l, "LFO%s Range" % l, "K_CH", "CH_X1X6X10", default=0)
    P("L%s_DEST" % l, "l%s_dest" % l.lower(), "LFO %s DEST" % l, "LFO%s Dest" % l, "K_CH", "CH_DEST", default=0)
    P("L%s_DEPTH" % l, "l%s_depth" % l.lower(), "LFO %s DEPTH" % l, "LFO%s Depth" % l, "K_BIP", default=0.75)
P("JOY_X", "joy_x", "JOYSTICK X", "Joy X", "K_BIP", default=0.5)
P("JOY_Y", "joy_y", "JOYSTICK Y", "Joy Y", "K_BIP", default=0.5)
P("JOY_XDEST", "joy_xdest", "JOYSTICK X DEST", "Joy X Dest", "K_CH", "CH_DEST", default=3)
P("JOY_YDEST", "joy_ydest", "JOYSTICK Y DEST", "Joy Y Dest", "K_CH", "CH_DEST", default=11)
for s in range(1, 6):
    P("SEQ_S%d" % s, "seq_s%d" % s, "SEQ STEP %d" % s, "Seq Step %d" % s, default=[0.0, 0.25, 0.5, 0.33, 0.75][s - 1])
for s in range(1, 6):
    P("SEQ_G%d" % s, "seq_g%d" % s, "SEQ GATE %d" % s, "Seq Gate %d" % s, "K_SW", default=1)
P("SEQ_STAGES", "seq_stages", "SEQ STAGES", "Seq Stages", "K_CH", "CH_STAGES", default=2)
P("SEQ_RATE", "seq_rate", "SEQ PULSER", "Seq Pulser", default=0.45)
P("SEQ_CLOCK", "seq_clock", "SEQ CLOCK", "Seq Clock", "K_CH", "CH_CLOCK", default=0)
P("SEQ_DEST", "seq_dest", "SEQ DEST", "Seq Dest", "K_CH", "CH_SEQDEST", default=0)
P("SEQ_DEPTH", "seq_depth", "SEQ DEPTH", "Seq Depth", "K_BIP", default=0.75)
P("SEQ_RUN", "seq_run", "SEQ RUN", "Seq Run", "K_SW", default=0)
P("LIGHT_SRC", "light_src", "LIGHT SOURCE", "Light Src", "K_CH", "CH_LIGHT", default=0)
P("LIGHT_RATE", "light_rate", "LIGHT DRIFT", "Light Drift", default=0.35)
P("LIGHT_DEPTH", "light_depth", "LIGHT DEPTH", "Light Depth", default=0.3)

# the dual effector: a cartridge program each side; X, Y, Z and BLEND shared (as on the instrument)
P("FXL", "fxl", "EFFECT L", "Effect L", "K_CH", "CH_FX", default=0)
P("FXR", "fxr", "EFFECT R", "Effect R", "K_CH", "CH_FX", default=1)
P("FX_X", "fx_x", "EFFECT X", "Effect X", default=0.5)
P("FX_Y", "fx_y", "EFFECT Y", "Effect Y", default=0.5)
P("FX_Z", "fx_z", "EFFECT Z", "Effect Z", default=0.5)
P("BLEND", "blend", "EFFECT BLEND", "Blend", default=0.4)

# global
P("GRID", "grid", "TUNE GRID", "Tune Grid", "K_CH", "CH_GRID", default=0)
P("BEND", "bend", "BEND RANGE", "Bend Range", "K_CH", "CH_BEND", default=2)

# (appended later: the drones' HOLD, the voice open without its key)
for d in (1, 2, 4, 5, 3, 6):
    P("D%d_HOLD" % d, "d%d_hold" % d, "DRONE %d HOLD" % d, "D%d Hold" % d, "K_SW", default=0)

ids = [r[1] for r in rows]
assert len(ids) == len(set(ids))
out = os.path.join(ROOT, "dsp", "params_list.inc")
with open(out, "w") as f:
    f.write("/* generated by tools/gen_params.py: do not edit (the order is permanent; append only) */\n")
    for enum, pid, name, short, kind, ch, default in rows:
        f.write('P(%s, "%s", "%s", "%s", %s, %s, %sf)\n' % (enum, pid, name, short, kind, ch, repr(float(default))))
print("%d parameters -> %s" % (len(rows), out))
