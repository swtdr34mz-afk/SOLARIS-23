// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 swtdr34mz (SOLARIS-23)
//
// SOLARIS-23 for the MPC: the same engine as the VST3 (dsp/: engine.cpp, fx.cpp, params.cpp, presets.cpp, copied
// beside this file by mpc/build.py), hosted by the MPC Plugin SDK's framework (mpc_plugin.h).
//
// Signal flow: DRONE 1, 2, 4, 5 (five free-running saws each, the light sensor, VOLT), DRONE 3, 6 (chaos: a square
// modulator and a Schmitt-trigger tone, FM, AM, divider, noise, S&H), VCO A and B (played by the pads / MIDI notes)
// -> the mixer (volume, pan) -> the dual 12 dB filter (LP / BP) -> the distortion -> the dual effector (a program each
// side; X, Y, Z, BLEND shared) -> MASTER. LFO A, B, the joystick and the 5-step sequencer (the MPC's tempo, locked to
// its bar while it plays) move what their DEST says.
//
// Parameters: exactly the engine's, in its order (dsp/params_list.inc: permanent, new ones only at the end).
//   0 is MASTER VOLUME (the MPC nudges parameter 0 on every prepare: harmless, it glides).
//
// MIDI in: notes (the VCOs, as the touch plates), pitch bend (BEND RANGE), CC 1 mod wheel (more of the joystick's Y
// destination), CC 120 all sound off, CC 121 reset controllers, CC 123 all notes off.

#include "mpc_plugin.h"
#include "solaris.h"

#include <cmath>
#include <cstring>

namespace {

class Solaris23Mpc : public mpc::Plugin
{
public:
    Solaris23Mpc() : mpc::Plugin(makeInfo())
    {
        for (int i = 0; i < sol::NPARAMS; i++) {
            const sol::ParamDef &d = sol::PARAMS[i];
            if (d.kind == sol::K_CH) {
                std::vector<std::string> ch;
                for (int k = 0; k < d.nChoices; k++)
                    ch.push_back(d.choices[k]);
                addParam(mpc::choiceParam(d.id, d.shortName, ch, (int)d.def));
            } else if (d.kind == sol::K_SW) {
                addParam(mpc::boolParam(d.id, d.shortName, d.def >= 0.5f));
            } else {
                addParam(mpc::floatParam(d.id, d.shortName, 0.0f, 1.0f, d.def, "", 2));
            }
        }
        std::vector<float> v((size_t)sol::NPARAMS);
        for (int k = 0; k < sol::numPresets(); k++) {
            sol::presetValues(k, v.data());
            addProgram(sol::presetName(k), v);
        }
        engine.prepare(44100.0);         // (the MPC's rate: made here, on the UI thread, never in process)
        rate = 44100.0;
    }

    void prepare(double sampleRate, int) override
    {
        if (sampleRate != rate) {        // (only if the rate really changes: the MPC's is fixed at 44.1 kHz)
            engine.prepare(sampleRate);
            rate = sampleRate;
        }
        fresh = true;
    }

    void reset() override               // resume, or a program / state loaded: the notes let go
    {
        engine.allNotesOff();
        fresh = true;
    }

    void process(mpc::ProcessContext &ctx) override
    {
        const int n = ctx.numFrames;
        float *L = ctx.outputs[0], *R = ctx.numOutputs > 1 ? ctx.outputs[1] : ctx.outputs[0];
        for (int i = 0; i < sol::NPARAMS; i++) {   /* the parameters, when they change (atomics: safe here) */
            float v = (float)get(i);
            if (fresh || v != sent[i]) {
                engine.set(i, v);
                sent[i] = v;
            }
        }
        fresh = false;
        double bpm = ctx.transport.valid && ctx.transport.tempoValid ? ctx.transport.bpm : 120.0;
        engine.tempo(bpm);
        bool playing = ctx.transport.valid && ctx.transport.playing && ctx.transport.ppqValid;
        engine.position(playing ? ctx.transport.ppqPos : 0.0, playing);
        int done = 0;
        for (int i = 0; i < ctx.numMidiEvents; i++) {
            const mpc::MidiEvent &e = ctx.midi[i];
            int at = mpc::clamp(e.sampleOffset, 0, n);
            if (at > done) {
                engine.render(L + done, R + done, at - done);
                done = at;
            }
            midi(e);
        }
        if (n > done)
            engine.render(L + done, R + done, n - done);
    }

    bool formatValue(int index, float plain, std::string &out) override
    {
        if (index < 0 || index >= sol::NPARAMS)
            return false;
        char t[32];
        sol::paramText(index, plain, t, sizeof t);
        t[12] = 0;                       /* (the MPC's screen: short) */
        out = t;
        return true;
    }

private:
    static mpc::PluginInfo makeInfo()
    {
        mpc::PluginInfo i;
        i.name = "SOLARIS-23";
        i.vendor = "swtdr34mz";
        i.uniqueId = MPC_FOURCC('S', 'o', '2', '3');
        i.version = 100;
        i.isInstrument = true;
        i.numInputs = 0;
        i.numOutputs = 2;
        i.tailSamples = 44100 * 20;      /* (the black hole's tail, about) */
        return i;
    }

    void midi(const mpc::MidiEvent &e)
    {
        if (e.isNoteOn())
            engine.noteOn(e.note(), (float)e.velocity() / 127.0f);
        else if (e.isNoteOff())
            engine.noteOff(e.note());
        else if (e.isPitchBend())
            engine.bend((float)e.pitchBend() / 8192.0f);
        else if (e.isController()) {
            int cc = e.controller(), v = e.controllerValue();
            if (cc == 1)
                engine.modWheel((float)v / 127.0f);
            else if (cc == 120)
                engine.panic();
            else if (cc == 123)
                engine.allNotesOff();
            else if (cc == 121) {
                engine.bend(0.0f);
                engine.modWheel(0.0f);
            }
        }
    }

    sol::Engine engine;
    double rate = 0.0;
    float sent[sol::NPARAMS] = {};
    bool fresh = true;
};

} // namespace

MPC_PLUGIN_ENTRY(Solaris23Mpc)
