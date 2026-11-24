/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
#include "Sections.h"

namespace s23
{
using namespace juce;
using R = Rectangle<int>;

void Page::paint(Graphics &g)
{
    if (drawFace)
        drawPanel(g, getLocalBounds().toFloat());
    for (auto &s : sections)
        drawSection(g, s.r, s.title, s.sec);
    for (auto &t : texts)
        drawLegend(g, t.text, t.r, t.size, t.dim ? theme().dim : theme().legend);
    if (extra)
        extra(g);
}

void Builder::section(R r, const String &title, Sec s) { page.sections.push_back({r.toFloat(), title, s}); }
void Builder::text(R r, const String &t, float size, bool dim) { page.texts.push_back({r.toFloat(), t, size, dim}); }

Knob *Builder::knob(const char *id, const String &legend, Sec s, R r, float size)
{
    auto *k = add(new Knob(apvts, id, legend, s, touch), r);
    k->knobSize = size;
    return k;
}

Key *Builder::key(const char *id, const String &legend, Sec s, R r, bool big)
{
    return add(new Key(apvts, id, legend, s, touch, big), r);
}

/* DRONE 1, 2, 4, 5: five oscillators (on, TUNE, MOD each), VOLT, the key, ATTACK, RELEASE, the light sensor */
void Builder::classic(int d, R r)
{
    const Sec S = Sec::drone;
    const int x = r.getX(), y = r.getY();
    section(r, "DRONE " + String(d) + juce::String::fromUTF8("  ·  CLASSIC"), S);
    auto id = [d](const char *f, int k = 0) {
        static char b[24];
        if (k)
            std::snprintf(b, sizeof b, "d%d_%s%d", d, f, k);
        else
            std::snprintf(b, sizeof b, "d%d_%s", d, f);
        return (const char *)b;
    };
    text({x + 4, y + 30, 20, 16}, "ON", 9.0f);
    text({x + 4, y + 70, 20, 16}, "TUNE", 8.0f);
    text({x + 4, y + 120, 20, 16}, "MOD", 9.0f);
    for (int k = 1; k <= 5; k++) {
        int cx = x + 24 + (k - 1) * 40;
        key(id("on", k), "", S, {cx, y + 28, 40, 24});
        knob(id("t", k), String(k), S, {cx, y + 54, 40, 58}, 36.0f);
        key(id("mod", k), "", S, {cx, y + 116, 40, 24});
    }
    knob(id("volt"), "VOLT", S, {x + 226, y + 34, 66, 86}, 58.0f);
    text({x + 226, y + 120, 66, 14}, juce::String::fromUTF8("DOWN  ·  FM"), 8.5f);
    key(id("gate"), String(d), S, {x + 12, y + 160, 64, 64}, true);
    text({x + 12, y + 226, 64, 14}, "KEY", 10.0f);
    knob(id("att"), "ATTACK", S, {x + 82, y + 156, 58, 76}, 46.0f);
    knob(id("rls"), "RELEASE", S, {x + 142, y + 156, 58, 76}, 46.0f);
    static const int MIXI[6] = {0, 1, 0, 5, 6, 0};
    int c = d == 1 ? 0 : d == 2 ? 1 : d == 4 ? 2 : 3;
    auto lightFn = meters.light;
    add(new Lamp(Lamp::dome, [lightFn, c] { return jlimit(0.0f, 1.0f, lightFn() * (0.9f + 0.05f * (float)c)); }), {x + 210, y + 150, 80, 80});
    text({x + 206, y + 232, 88, 14}, "LIGHT SENSOR", 9.0f);
    auto vf = meters.voice;
    int mi = MIXI[d - 1];
    add(new Lamp(Lamp::led, [vf, mi] { return vf(mi) * 1.6f; }, secColour(S)), {x + 60, y + 238, 16, 16});
}

/* DRONE 3, 6: the modulator (RATE, x10), the tone (PITCH, its range), FM (MOD), AM, DIVIDER, NOISE, S&H */
void Builder::chaos(int d, R r)
{
    const Sec S = Sec::chaos;
    const int x = r.getX(), y = r.getY();
    section(r, "DRONE " + String(d) + juce::String::fromUTF8("  ·  CHAOS"), S);
    auto id = [d](const char *f) {
        static char b[24];
        std::snprintf(b, sizeof b, "d%d_%s", d, f);
        return (const char *)b;
    };
    knob(id("rate"), "RATE", S, {x + 10, y + 30, 64, 80}, 52.0f);
    key(id("x10"), "x10", S, {x + 74, y + 40, 40, 40});
    knob(id("moda"), "FM AMT", S, {x + 116, y + 30, 64, 80}, 52.0f);
    knob(id("pitch"), "PITCH", S, {x + 184, y + 30, 64, 80}, 52.0f);
    key(id("hi"), "HIGH", S, {x + 248, y + 40, 44, 40});
    key(id("fm"), "FM", S, {x + 10, y + 120, 44, 40});
    key(id("am"), "AM", S, {x + 54, y + 120, 44, 40});
    knob(id("div"), "DIVIDER", S, {x + 104, y + 114, 62, 76}, 46.0f);
    knob(id("noise"), "NOISE", S, {x + 166, y + 114, 62, 76}, 46.0f);
    knob(id("sh"), "S&H", S, {x + 228, y + 114, 62, 76}, 46.0f);
    key(id("gate"), String(d), S, {x + 12, y + 200, 64, 64}, true);
    text({x + 12, y + 266, 64, 14}, "KEY", 10.0f);
    knob(id("att"), "ATTACK", S, {x + 90, y + 196, 62, 80}, 48.0f);
    knob(id("rls"), "RELEASE", S, {x + 156, y + 196, 62, 80}, 48.0f);
    auto vf = meters.voice;
    int mi = d == 3 ? 2 : 7;
    add(new Lamp(Lamp::led, [vf, mi] { return vf(mi) * 1.6f; }, secColour(S)), {x + 236, y + 222, 24, 24});
    text({x + 222, y + 248, 54, 14}, "LEVEL", 9.0f);
}

/* VCO A, B: WAVE (sine .. pulse), PW, TUNE, OCTAVE, SUB; the envelope (HOLD, SELF-GEN); A's SYNC, B's FM */
void Builder::vco(int v, R r)
{
    const Sec S = Sec::vco;
    const int x = r.getX(), y = r.getY();
    const char *p = v ? "vb_" : "va_";
    section(r, v ? "VCO B" : "VCO A", S);
    auto id = [p](const char *f) {
        static char b[24];
        std::snprintf(b, sizeof b, "%s%s", p, f);
        return (const char *)b;
    };
    knob(id("wave"), "WAVEFORM", S, {x + 8, y + 28, 100, 106}, 78.0f)->showValue = true;
    knob(id("pw"), "PW", S, {x + 112, y + 30, 58, 74}, 46.0f);
    knob(id("tune"), "TUNE", S, {x + 172, y + 30, 58, 74}, 46.0f);
    knob(id("oct"), "OCTAVE", S, {x + 232, y + 30, 62, 86}, 44.0f);
    key(id("sub"), "SUB -1", S, {x + 112, y + 110, 52, 40});
    if (v == 0)
        key("va_sync", "SYNC", S, {x + 168, y + 110, 52, 40});
    else
        knob("vb_fm", "FM", S, {x + 168, y + 104, 58, 56}, 36.0f);
    key(id("hold"), "HOLD", S, {x + 10, y + 150, 50, 40});
    key(id("loop"), "SELF-GEN", S, {x + 60, y + 150, 56, 40});
    text({x + 120, y + 162, 170, 14}, "ENVELOPE / VCA", 9.5f);
    knob(id("a"), "ATTACK", S, {x + 8, y + 198, 70, 80}, 50.0f);
    knob(id("d"), "DECAY", S, {x + 80, y + 198, 70, 80}, 50.0f);
    knob(id("s"), "SUSTAIN", S, {x + 152, y + 198, 70, 80}, 50.0f);
    knob(id("r"), "RELEASE", S, {x + 224, y + 198, 70, 80}, 50.0f);
    auto vf = meters.voice;
    int mi = v ? 4 : 3;
    add(new Lamp(Lamp::led, [vf, mi] { return vf(mi) * 1.6f; }, secColour(S)), {x + 268, y + 156, 22, 22});
}

/* the dual effector: the two cartridge slots, X Y Z (shared, each meaning what the programs say), BLEND, VOLUME */
void Builder::effector(R r)
{
    const Sec S = Sec::fx;
    const int x = r.getX(), y = r.getY();
    section(r, "DUAL EFFECTOR", S);
    add(new Cartridge(apvts, "fxl", "L", touch), {x + 10, y + 30, 134, 76});
    add(new Cartridge(apvts, "fxr", "R", touch), {x + 152, y + 30, 134, 76});
    const char *ids[3] = {"fx_x", "fx_y", "fx_z"};
    const char *nm[3] = {"X", "Y", "Z"};
    for (int k = 0; k < 3; k++) {
        knob(ids[k], nm[k], S, {x + 12 + k * 92, y + 112, 88, 86}, 60.0f);
        add(new FxLabel(apvts, "fxl", "L", k), {x + 8 + k * 92, y + 198, 96, 13});
        add(new FxLabel(apvts, "fxr", "R", k), {x + 8 + k * 92, y + 211, 96, 13});
    }
    knob("blend", "BLEND", S, {x + 40, y + 222, 70, 64}, 46.0f);
    knob("volume", "MASTER", S, {x + 186, y + 222, 70, 64}, 46.0f);
}

/* the mixer: PAN and VOLUME for DRONE 1, 2, 3, VCO A, B, DRONE 4, 5, 6 */
void Builder::mixer(R r)
{
    const Sec S = Sec::mix;
    const int x = r.getX(), y = r.getY();
    section(r, "MIXER", S);
    static const char *const CH[8] = {"d1", "d2", "d3", "va", "vb", "d4", "d5", "d6"};
    static const char *const NM[8] = {"D1", "D2", "D3", "VA", "VB", "D4", "D5", "D6"};
    static const Sec SC[8] = {Sec::drone, Sec::drone, Sec::chaos, Sec::vco, Sec::vco, Sec::drone, Sec::drone, Sec::chaos};
    for (int m = 0; m < 8; m++) {
        int cx = x + 6 + m * 35;
        knob((String(CH[m]) + "_pan").toRawUTF8(), "", SC[m], {cx, y + 26, 35, 34}, 26.0f);
        knob((String(CH[m]) + "_vol").toRawUTF8(), "", SC[m], {cx, y + 62, 35, 40}, 32.0f);
        auto vf = meters.voice;
        add(new Lamp(Lamp::led, [vf, m] { return vf(m) * 1.6f; }, secColour(SC[m])), {cx + 10, y + 102, 15, 15});
        text({cx, y + 118, 35, 14}, NM[m], 10.0f, false);
    }
}

/* the dual filter (FREQ, RES, BP each side, LINK), the distortion (DIST, GAIN) */
void Builder::filter(R r)
{
    const Sec S = Sec::fx;
    const int x = r.getX(), y = r.getY();
    section(r, juce::String::fromUTF8("FILTER L  ·  DISTORTION  ·  FILTER R"), S);
    knob("fl_freq", "FREQ", S, {x + 4, y + 26, 50, 64}, 40.0f);
    knob("fl_res", "RES", S, {x + 52, y + 26, 46, 64}, 36.0f);
    key("fl_mode", "BP", S, {x + 14, y + 94, 36, 40});
    key("f_link", "LINK", S, {x + 56, y + 94, 40, 40});
    knob("dist", "DIST", S, {x + 104, y + 30, 44, 60}, 36.0f);
    knob("gain", "GAIN", S, {x + 148, y + 30, 44, 60}, 36.0f);
    knob("fr_res", "RES", S, {x + 198, y + 26, 46, 64}, 36.0f);
    knob("fr_freq", "FREQ", S, {x + 242, y + 26, 50, 64}, 40.0f);
    key("fr_mode", "BP", S, {x + 246, y + 94, 36, 40});
    text({x + 104, y + 96, 88, 14}, juce::String::fromUTF8("LP / BP  ·  12 dB"), 8.5f);
}

void Builder::lfo(int l, R r)
{
    const Sec S = Sec::mod;
    const int x = r.getX(), y = r.getY();
    const char *p = l ? "lb_" : "la_";
    section(r, l ? "LFO B" : "LFO A", S);
    auto id = [p](const char *f) {
        static char b[24];
        std::snprintf(b, sizeof b, "%s%s", p, f);
        return (const char *)b;
    };
    knob(id("wave"), "SQR - TRI", S, {x + 6, y + 26, 60, 62}, 42.0f);
    knob(id("rate"), "RATE", S, {x + 66, y + 26, 60, 62}, 42.0f);
    knob(id("range"), "RANGE", S, {x + 128, y + 26, 56, 70}, 34.0f);
    knob(id("dest"), "DEST", S, {x + 6, y + 92, 80, 56}, 28.0f);
    knob(id("depth"), "DEPTH", S, {x + 88, y + 90, 56, 58}, 38.0f);
    auto lf = meters.lfo;
    add(new Lamp(Lamp::led, [lf, l] { return lf(l); }, secColour(S).brighter(0.4f)), {x + 150, y + 104, 24, 24});
}

void Builder::joystick(R r)
{
    const Sec S = Sec::mod;
    const int x = r.getX(), y = r.getY();
    section(r, "JOYSTICK", S);
    if (mpc) {                           /* (the MPC: X and Y as knobs) */
        knob("joy_x", "X", S, {x + 8, y + 28, 76, 112}, 64.0f);
        knob("joy_y", "Y", S, {x + 84, y + 28, 76, 112}, 64.0f);
    } else
        add(new XYPad(apvts, "joy_x", "joy_y", touch), {x + 8, y + 28, 150, 116});
    knob("joy_xdest", "X DEST", S, {x + 166, y + 26, 96, 58}, 30.0f);
    knob("joy_ydest", "Y DEST", S, {x + 166, y + 88, 96, 58}, 30.0f);
}

void Builder::sequencer(R r)
{
    const Sec S = Sec::mod;
    const int x = r.getX(), y = r.getY();
    section(r, "5-STEP SEQUENCER", S);
    for (int s = 1; s <= 5; s++) {
        int cx = x + 6 + (s - 1) * 66;
        knob(("seq_s" + String(s)).toRawUTF8(), "STEP " + String(s), S, {cx, y + 26, 66, 70}, 50.0f);
        key(("seq_g" + String(s)).toRawUTF8(), "GATE", S, {cx + 13, y + 98, 40, 40});
        auto st = meters.step;
        add(new Lamp(Lamp::led, [st, s] { return st() == s - 1 ? 1.0f : 0.0f; }, secColour(S).brighter(0.5f)), {cx + 50, y + 30, 14, 14});
    }
    int bx = x + 6 + 5 * 66 + 10;
    key("seq_run", "RUN", S, {bx, y + 30, 50, 44});
    knob("seq_rate", "PULSER", S, {bx + 54, y + 26, 64, 70}, 48.0f);
    knob("seq_stages", "STAGES", S, {bx + 122, y + 26, 60, 70}, 36.0f);
    knob("seq_clock", "CLOCK", S, {bx + 186, y + 26, 70, 70}, 36.0f);
    knob("seq_dest", "DEST", S, {bx, y + 90, 90, 56}, 28.0f);
    knob("seq_depth", "DEPTH", S, {bx + 122, y + 88, 64, 58}, 38.0f);
}

void Builder::light(R r)
{
    const Sec S = Sec::mod;
    const int x = r.getX(), y = r.getY();
    section(r, "LIGHT", S);
    knob("light_src", "SOURCE", S, {x + 6, y + 26, 84, 64}, 34.0f);
    knob("light_rate", "DRIFT", S, {x + 6, y + 90, 60, 58}, 38.0f);
    knob("light_depth", "DEPTH", S, {x + 70, y + 90, 60, 58}, 38.0f);
    add(new Lamp(Lamp::dome, meters.light), {x + 116, y + 32, 84, 84});
    text({x + 110, y + 120, 96, 14}, "the room's light", 9.0f);
}

void Builder::droneKeys(R r)
{
    const Sec S = Sec::keys;
    const int x = r.getX(), y = r.getY();
    section(r, "DRONE KEYS", S);
    static const int ORDER[6] = {1, 2, 3, 4, 5, 6};
    for (int i = 0; i < 6; i++) {
        int d = ORDER[i];
        int cx = x + 12 + (i % 3) * 82, cy = y + 28 + (i / 3) * 54;
        key(("d" + String(d) + "_gate").toRawUTF8(), String(d), (d == 3 || d == 6) ? Sec::chaos : Sec::drone, {cx, cy, 76, 50}, true);
    }
}

void Builder::global(R r)
{
    const Sec S = Sec::mix;
    const int x = r.getX(), y = r.getY();
    section(r, juce::String::fromUTF8("KEYS  ·  TUNING"), S);
    knob("glide", "GLIDE", S, {x + 6, y + 28, 64, 74}, 48.0f);
    knob("bend", "BEND", S, {x + 74, y + 28, 64, 82}, 40.0f);
    knob("grid", "TUNE GRID", S, {x + 140, y + 28, 66, 82}, 40.0f);
    text({x + 6, y + 112, 200, 24}, juce::String::fromUTF8("FREE: microtonal  ·  SEMI: semitones"), 9.0f);
}

void Builder::plates(R r)
{
    section(r, juce::String::fromUTF8("TOUCH PLATES  ·  VCO A + B"), Sec::keys);
    if (keys)
        add(new Plates(*keys), r.withTrimmedTop(24).reduced(8, 4));
}
} // namespace s23
