/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
#include "Sections.h"

namespace s23
{
using namespace juce;
using R = Rectangle<int>;

static String U(const char *s) { return String::fromUTF8(s); }

void Page::paint(Graphics &g)
{
    /* the background (the panel, its coat, the frames, the print, the wear) drawn once at the screen's own pixel
     * density (up to an 8K screen filled), then only copied */
    const float sc = jlimit(1.0f, 6.0f, (float)g.getInternalContext().getPhysicalPixelScaleFactor());
    if (!cache.isValid() || std::fabs(sc - cacheScale) > 0.02f) {
        cacheScale = sc;
        cache = Image(Image::RGB, (int)std::ceil((float)getWidth() * sc), (int)std::ceil((float)getHeight() * sc), false);
        Graphics cg(cache);
        {
            Graphics::ScopedSaveState s(cg);
            cg.addTransform(AffineTransform::scale(sc));
            drawPanel(cg, getLocalBounds().toFloat());
        }
        cg.setTiledImageFill(coatTile(sc), 0, 0, 1.0f);   /* (in screen pixels: the relief at full resolution) */
        cg.fillAll();
        cg.addTransform(AffineTransform::scale(sc));
        paintBackground(cg, 1.0f / sc);
    }
    Graphics::ScopedSaveState s(g);
    g.setImageResamplingQuality(Graphics::highResamplingQuality);
    g.drawImage(cache, getLocalBounds().toFloat());
}

void Page::paintBackground(Graphics &g, float px)
{
    for (auto &s : sections)
        drawSection(g, s.r, s.title, s.sec);
    g.setColour(ink().withAlpha(0.6f));
    for (auto &l : dotted) {
        float len = l.getLength();
        for (float t = 0.0f; t < len; t += 5.0f)
            g.fillEllipse(Rectangle<float>(1.5f, 1.5f).withCentre(l.getPointAlongLine(t)));
    }
    for (auto &t : texts) {
        if (t.vertical) {                /* a bar of the print with the letters standing up (the rows' names) */
            Graphics::ScopedSaveState s(g);
            g.addTransform(AffineTransform::rotation(-MathConstants<float>::halfPi, t.r.getCentreX(), t.r.getCentreY()));
            auto rr = Rectangle<float>(t.r.getHeight(), t.r.getWidth()).withCentre(t.r.getCentre());
            g.setColour(ink());
            g.fillRect(rr);
            g.setColour(Colour(0xff151515));
            g.setFont(font(Face::title, t.size));
            g.drawText(t.text, rr, Justification::centred, false);
        } else
            drawLegend(g, t.text, t.r, t.size);
    }
    if (extra)
        extra(g);
    drawWear(g, getLocalBounds().toFloat(), px);
    drawPanelEdge(g, getLocalBounds().toFloat());
    for (auto &p : screws)
        drawScrew(g, p, 5.5f);
}

void Builder::section(R r, const String &title, Sec s) { page.sections.push_back({r.toFloat(), title, s}); }
void Builder::text(R r, const String &t, float size, bool vertical) { page.texts.push_back({r.toFloat(), t, size, vertical}); }
void Builder::dots(float x0, float y0, float x1, float y1) { page.dotted.push_back({x0, y0, x1, y1}); }

Knob *Builder::knob(const char *id, const String &legend, KS s, R r, float size, bool ticks)
{
    auto *k = add(new Knob(apvts, id, legend, s, touch), r);
    k->knobSize = size;
    k->ticks = ticks;
    return k;
}

Switch *Builder::sw(const char *id, const String &legend, SW s, R r) { return add(new Switch(apvts, id, legend, s, touch), r); }

static String pid(const char *fmt, int a, int b = 0)
{
    char t[24];
    std::snprintf(t, sizeof t, fmt, a, b);
    return t;
}

/* DRONE 1, 2, 4, 5: five oscillators in columns (ON, TUNE, PAN), their state in a row of lamps, VOLT, ATTACK,
 * RELEASE, the level and HOLD; TONE, DRIFT, SWIRL; the key is on the drone keys */
void Builder::classic(int d, R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "DRONE " + String(d));
    text({x + 4, y + 26, 14, 30}, "ON", 8.5f, true);
    text({x + 4, y + 60, 14, 46}, "TUNE", 8.5f, true);
    text({x + 4, y + 108, 14, 34}, "PAN", 8.5f, true);
    for (int k = 1; k <= 5; k++) {
        int cx = x + 22 + (k - 1) * 40;
        text({cx, y + 21, 40, 10}, String(k), 9.5f);
        sw(pid("d%d_on%d", d, k).toRawUTF8(), "", SW::button, {cx, y + 32, 40, 24});
        knob(pid("d%d_t%d", d, k).toRawUTF8(), "", KS::black, {cx, y + 60, 40, 44}, 34.0f);
        knob(pid("d%d_pan%d", d, k).toRawUTF8(), "", KS::light, {cx + 4, y + 106, 32, 36}, 27.0f);
        if (k < 5)
            dots((float)cx + 40.0f, (float)y + 30.0f, (float)cx + 40.0f, (float)y + 140.0f);
    }
    /* the five oscillators' state, a row of lamps */
    for (int k = 1; k <= 5; k++) {
        auto *p = apvts.getRawParameterValue(pid("d%d_on%d", d, k));
        add(new Lamp(Lamp::led, [p] { return p->load() >= 0.5f ? 1.0f : 0.0f; }, RED_LED), {x + 226 + (k - 1) * 13, y + 26, 12, 12});
    }
    text({x + 222, y + 38, 70, 10}, "1  2  3  4  5", 8.0f);
    knob(pid("d%d_volt", d).toRawUTF8(), "VOLT", KS::blue, {x + 222, y + 52, 72, 84}, 66.0f, true);
    text({x + 222, y + 134, 72, 11}, "down  |  fm", 8.5f);
    dots((float)x + 8.0f, (float)y + 150.0f, (float)r.getRight() - 8.0f, (float)y + 150.0f);
    knob(pid("d%d_att", d).toRawUTF8(), "ATT", KS::slate, {x + 14, y + 162, 62, 76}, 50.0f, true);
    knob(pid("d%d_rls", d).toRawUTF8(), "RLS", KS::slate, {x + 80, y + 162, 62, 76}, 50.0f, true);
    auto vf = meters.voice;
    static const int MIXI[7] = {0, 0, 1, 0, 5, 6, 0};
    int mi = MIXI[d];
    add(new Lamp(Lamp::led, [vf, mi] { return vf(mi) * 1.6f; }, AMBER_LED), {x + 52, y + 250, 18, 18});
    text({x + 8, y + 252, 44, 14}, "level", 9.5f);
    sw(pid("d%d_hold", d).toRawUTF8(), "HOLD", SW::toggle, {x + 92, y + 238, 40, 50});
    /* the drone's character: how dark, how unsteady, how far round the stereo field its five oscillators travel */
    dots((float)x + 150.0f, (float)y + 156.0f, (float)x + 150.0f, (float)y + 284.0f);
    knob(pid("d%d_tone", d).toRawUTF8(), "TONE", KS::slate, {x + 152, y + 160, 48, 80}, 38.0f, true);
    knob(pid("d%d_drift", d).toRawUTF8(), "DRIFT", KS::slate, {x + 199, y + 160, 48, 80}, 38.0f, true);
    knob(pid("d%d_swirl", d).toRawUTF8(), "SWIRL", KS::blue, {x + 246, y + 160, 48, 80}, 38.0f, true);
    auto lf = meters.light;
    const float off = 0.13f * (float)d;
    add(new Lamp(Lamp::orbit, [lf, off] { return lf() + off; }), {x + 236, y + 240, 50, 50});
    text({x + 152, y + 252, 82, 22}, "dark  |  wander  |  orbit", 8.0f);
}

/* DRONE 3, 6: the modulator (RATE, x1/x10), FM AMT, the tone (PITCH, LOW/HIGH), the switches FM and AM, DIVIDER,
 * NOISE, S&H, ATTACK, RELEASE */
void Builder::chaos(int d, R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "DRONE " + String(d) + "  CHAOS");
    auto id = [d](const char *f) { return pid("d%d_", d) + f; };
    knob(id("rate").toRawUTF8(), "rate", KS::slate, {x + 10, y + 26, 64, 80}, 54.0f, true);
    sw(id("x10").toRawUTF8(), "x10", SW::toggle, {x + 74, y + 34, 30, 52});
    knob(id("moda").toRawUTF8(), "fm amt", KS::slate, {x + 106, y + 26, 64, 80}, 54.0f, true);
    knob(id("pitch").toRawUTF8(), "PITCH", KS::blue, {x + 172, y + 24, 82, 88}, 68.0f, true);
    sw(id("hi").toRawUTF8(), "high", SW::toggle, {x + 256, y + 34, 34, 52});
    sw(id("fm").toRawUTF8(), "fm", SW::toggle, {x + 14, y + 120, 34, 52});
    sw(id("am").toRawUTF8(), "am", SW::toggle, {x + 52, y + 120, 34, 52});
    knob(id("div").toRawUTF8(), "divider", KS::slate, {x + 92, y + 116, 64, 74}, 48.0f);
    knob(id("noise").toRawUTF8(), "NOISE", KS::light, {x + 158, y + 116, 64, 74}, 48.0f);
    knob(id("sh").toRawUTF8(), "S&H", KS::light, {x + 224, y + 116, 64, 74}, 48.0f);
    dots((float)x + 8.0f, (float)y + 198.0f, (float)r.getRight() - 8.0f, (float)y + 198.0f);
    knob(id("att").toRawUTF8(), "ATT", KS::slate, {x + 14, y + 206, 62, 76}, 50.0f, true);
    knob(id("rls").toRawUTF8(), "RLS", KS::slate, {x + 80, y + 206, 62, 76}, 50.0f, true);
    auto vf = meters.voice;
    int mi = d == 3 ? 2 : 7;
    add(new Lamp(Lamp::led, [vf, mi] { return vf(mi) * 1.6f; }, AMBER_LED), {x + 170, y + 228, 20, 20});
    text({x + 148, y + 250, 64, 14}, "level", 9.5f);
    sw(id("hold").toRawUTF8(), "HOLD", SW::toggle, {x + 236, y + 214, 44, 60});
}

/* VCO A, B: WAVEFORM (sine .. pulse), OCTAVE, TUNE, PW, SUB; the envelope and VCA (HOLD, LOOP: self-generating);
 * A's SYNC, B's FM */
void Builder::vco(int v, R r)
{
    const int x = r.getX(), y = r.getY();
    const char *p = v ? "vb_" : "va_";
    section(r, v ? "VCO B" : "VCO A");
    auto id = [p](const char *f) { return String(p) + f; };
    knob(id("wave").toRawUTF8(), "waveform", KS::slate, {x + 10, y + 24, 100, 116}, 82.0f, true)->showValue = true;
    knob(id("oct").toRawUTF8(), "octave", KS::black, {x + 116, y + 24, 72, 84}, 50.0f, true);
    knob(id("tune").toRawUTF8(), "tune", KS::blue, {x + 196, y + 24, 92, 84}, 60.0f, true);
    knob(id("pw").toRawUTF8(), "pw", KS::light, {x + 118, y + 110, 58, 60}, 40.0f);
    sw(id("sub").toRawUTF8(), "sub", SW::toggle, {x + 182, y + 114, 34, 52});
    if (v == 0)
        sw("va_sync", "sync", SW::toggle, {x + 224, y + 114, 34, 52});
    else
        knob("vb_fm", "fm", KS::light, {x + 220, y + 110, 58, 60}, 40.0f);
    dots((float)x + 8.0f, (float)y + 176.0f, (float)r.getRight() - 8.0f, (float)y + 176.0f);
    text({x + 10, y + 184, 80, 12}, "EG / VCA", 9.5f);
    sw(id("hold").toRawUTF8(), "hold", SW::toggle, {x + 150, y + 178, 34, 44});
    sw(id("loop").toRawUTF8(), "loop", SW::toggle, {x + 192, y + 178, 34, 44});
    auto vf = meters.voice;
    int mi = v ? 4 : 3;
    add(new Lamp(Lamp::led, [vf, mi] { return vf(mi) * 1.6f; }, AMBER_LED), {x + 248, y + 188, 18, 18});
    knob(id("a").toRawUTF8(), "attack", KS::light, {x + 8, y + 220, 70, 64}, 44.0f);
    knob(id("d").toRawUTF8(), "decay", KS::light, {x + 80, y + 220, 70, 64}, 44.0f);
    knob(id("s").toRawUTF8(), "sustain", KS::light, {x + 152, y + 220, 70, 64}, 44.0f);
    knob(id("r").toRawUTF8(), "release", KS::light, {x + 224, y + 220, 70, 64}, 44.0f);
}

/* the dual effector: the two cartridge slots, X Y Z (shared; each meaning what the programs say), BLEND, MASTER */
void Builder::effector(R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "DUAL EFFECTOR");
    add(new Cartridge(apvts, "fxl", "L", touch), {x + 10, y + 26, 136, 62});
    add(new Cartridge(apvts, "fxr", "R", touch), {x + 150, y + 26, 136, 62});
    const char *ids[3] = {"fx_x", "fx_y", "fx_z"};
    const char *nm[3] = {"X", "Y", "Z"};
    for (int k = 0; k < 3; k++) {
        knob(ids[k], nm[k], KS::red, {x + 10 + k * 94, y + 96, 88, 90}, 72.0f, true);
        add(new FxLabel(apvts, "fxl", "L", k), {x + 6 + k * 94, y + 186, 96, 12});
        add(new FxLabel(apvts, "fxr", "R", k), {x + 6 + k * 94, y + 198, 96, 12});
    }
    dots((float)x + 8.0f, (float)y + 214.0f, (float)r.getRight() - 8.0f, (float)y + 214.0f);
    knob("blend", "BLEND", KS::slate, {x + 40, y + 218, 70, 68}, 50.0f, true);
    knob("volume", "MASTER", KS::red, {x + 178, y + 216, 80, 70}, 56.0f, true);
}

/* the mixer: PAN and VOL for DRONE 1, 2, 3, VCO A, B, DRONE 4, 5, 6 */
void Builder::mixer(R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "VOICE MIXER");
    static const char *const CH[8] = {"d1", "d2", "d3", "va", "vb", "d4", "d5", "d6"};
    static const char *const NM[8] = {"D1", "D2", "D3", "VCO A", "VCO B", "D4", "D5", "D6"};
    for (int m = 0; m < 8; m++) {
        int cx = x + 6 + m * 35;
        knob((String(CH[m]) + "_pan").toRawUTF8(), "", KS::black, {cx, y + 26, 35, 32}, 28.0f);
        auto vf = meters.voice;
        add(new Lamp(Lamp::led, [vf, m] { return vf(m) * 1.6f; }, AMBER_LED), {cx + 12, y + 60, 11, 11});
        text({cx - 2, y + 73, 39, 12}, NM[m], 8.5f);
        knob((String(CH[m]) + "_vol").toRawUTF8(), "", KS::slate, {cx, y + 88, 35, 44}, 34.0f);
        if (m < 7)
            dots((float)cx + 35.0f, (float)y + 26.0f, (float)cx + 35.0f, (float)y + 132.0f);
    }
    text({x + 216, y + 3, 76, 14}, "pan  /  vol", 8.5f);
}

/* the dual filter (FREQ, RES, LP/BP each side, LINK), the distortion (DIST, GAIN) between */
void Builder::filter(R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "FILTER L  |  DIST  |  FILTER R");
    knob("fl_freq", "freq", KS::slate, {x + 4, y + 24, 56, 66}, 46.0f, true);
    knob("fl_res", "res", KS::slate, {x + 56, y + 24, 50, 66}, 40.0f, true);
    sw("fl_mode", "bp", SW::toggle, {x + 14, y + 92, 30, 46});
    sw("f_link", "link", SW::toggle, {x + 64, y + 92, 34, 46});
    knob("dist", "dist", KS::slate, {x + 108, y + 28, 40, 58}, 32.0f);
    knob("gain", "gain", KS::slate, {x + 148, y + 28, 40, 58}, 32.0f);
    text({x + 104, y + 98, 88, 26}, U("lp / bp  12 dB"), 8.5f);
    knob("fr_res", "res", KS::slate, {x + 190, y + 24, 50, 66}, 40.0f, true);
    knob("fr_freq", "freq", KS::slate, {x + 236, y + 24, 56, 66}, 46.0f, true);
    sw("fr_mode", "bp", SW::toggle, {x + 250, y + 92, 30, 46});
}

void Builder::lfo(int l, R r)
{
    const int x = r.getX(), y = r.getY();
    const char *p = l ? "lb_" : "la_";
    section(r, l ? "LFO B" : "LFO A");
    auto id = [p](const char *f) { return String(p) + f; };
    knob(id("wave").toRawUTF8(), U("sqr – tri"), KS::slate, {x + 6, y + 22, 60, 66}, 48.0f, true);
    knob(id("rate").toRawUTF8(), "rate", KS::black, {x + 66, y + 22, 60, 66}, 48.0f, true);
    knob(id("range").toRawUTF8(), "range", KS::black, {x + 128, y + 22, 56, 72}, 32.0f);
    knob(id("dest").toRawUTF8(), "dest", KS::black, {x + 6, y + 92, 84, 56}, 26.0f);
    knob(id("depth").toRawUTF8(), "depth", KS::light, {x + 92, y + 90, 56, 58}, 40.0f);
    auto lf = meters.lfo;
    add(new Lamp(Lamp::led, [lf, l] { return lf(l); }, BLUE_LED), {x + 152, y + 108, 22, 22});
}

void Builder::joystick(R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "JOYSTICK");
    add(new XYPad(apvts, "joy_x", "joy_y", touch), {x + 8, y + 24, 150, 120});
    knob("joy_xdest", "x dest", KS::light, {x + 166, y + 22, 96, 60}, 30.0f);
    knob("joy_ydest", "y dest", KS::light, {x + 166, y + 86, 96, 60}, 30.0f);
}

void Builder::sequencer(R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "5 STEP SEQ.  VOLTAGE");
    for (int s = 1; s <= 5; s++) {
        int cx = x + 6 + (s - 1) * 66;
        knob(pid("seq_s%d", s).toRawUTF8(), "step " + String(s), KS::red, {cx, y + 22, 60, 74}, 52.0f, true);
        sw(pid("seq_g%d", s).toRawUTF8(), "gate", SW::toggle, {cx + 12, y + 98, 30, 48});
        auto st = meters.step;
        add(new Lamp(Lamp::led, [st, s] { return st() == s - 1 ? 1.0f : 0.0f; }, RED_LED), {cx + 44, y + 108, 16, 16});
    }
    int bx = x + 6 + 5 * 66 + 6;
    dots((float)bx - 4.0f, (float)y + 24.0f, (float)bx - 4.0f, (float)y + 144.0f);
    sw("seq_run", "run", SW::toggle, {bx + 6, y + 28, 34, 54});
    knob("seq_rate", "pulser", KS::slate, {bx + 44, y + 22, 66, 74}, 54.0f, true);
    knob("seq_stages", "stages", KS::black, {bx + 116, y + 22, 64, 76}, 34.0f);
    knob("seq_clock", "clock", KS::black, {bx + 184, y + 22, 72, 76}, 34.0f);
    knob("seq_dest", "dest", KS::black, {bx + 4, y + 92, 92, 56}, 26.0f);
    knob("seq_depth", "depth", KS::light, {bx + 116, y + 90, 64, 58}, 40.0f);
}

void Builder::light(R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "ORBIT");
    knob("orbit_src", "source", KS::black, {x + 6, y + 22, 88, 66}, 32.0f);
    knob("orbit_rate", "rate", KS::slate, {x + 6, y + 90, 60, 58}, 40.0f);
    knob("orbit_width", "width", KS::light, {x + 66, y + 90, 60, 58}, 40.0f);
    add(new Lamp(Lamp::orbit, meters.light), {x + 112, y + 26, 92, 92});
    text({x + 106, y + 124, 102, 14}, "the drones' orbit", 9.0f);
}

void Builder::droneKeys(R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "DRONE VOICES");
    for (int i = 0; i < 6; i++) {
        int d = i + 1;
        int cx = x + 14 + (i % 3) * 80, cy = y + 24 + (i / 3) * 56;
        sw(pid("d%d_gate", d).toRawUTF8(), String(d), SW::key, {cx, cy, 72, 56})->momentary = true;   /* (HOLD keeps it open) */
    }
}

void Builder::global(R r)
{
    const int x = r.getX(), y = r.getY();
    section(r, "KEYS  /  TUNING");
    knob("glide", "glide", KS::slate, {x + 6, y + 22, 64, 78}, 50.0f, true);
    knob("bend", "bend", KS::black, {x + 74, y + 22, 64, 82}, 36.0f);
    knob("grid", "tune grid", KS::black, {x + 140, y + 22, 66, 82}, 36.0f);
    text({x + 6, y + 110, 200, 24}, U("free: microtonal  ·  semi: semitones"), 9.0f);
}

void Builder::plates(R r)
{
    section(r, "TOUCH SENSITIVE CONTROLLER", Sec::field);
    if (keys) {
        auto area = r.withTrimmedTop(22).reduced(8, 4);
        auto *p = add(new Plates(*keys, apvts, SolarisProcessor::PLATE_CHANNEL), area.withTrimmedRight(160));
        add(new PlateEditor(*p, apvts, touch), area.removeFromRight(150));
    }
}
} // namespace s23
