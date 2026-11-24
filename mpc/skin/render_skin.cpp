/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The MPC skin's pictures, drawn with the VST3's own code (src/ui/: the sections, the knobs, the keys, the cartridge
 * slots), laid out per MPC page, each page as large as fits the MPC's 1280 x 628:
 *
 *   DRONE 1/2, DRONE 4/5    two classic drones each (the function key DRONES, pressed again for the second)
 *   CHAOS                   DRONE 3 and DRONE 6
 *   VCO                     VCO A and VCO B
 *   LFO/JOY, SEQ/KEYS       LFO A, the joystick, LFO B, the light, glide and tuning; the sequencer and the drone keys
 *   MIX/FX                  the mixer, the filter and distortion, the dual effector
 *
 * Each page is cut up: its background (the panel, the sections, the legends, the lamps unlit), and every control at
 * every value it can take: a knob's film strip (with its value's display), a key on and off, a cartridge slot's 24
 * programs, X/Y/Z's labels for each program. manifest.json says what each is, for make_skin.py.
 *
 *   s23_skin_render <out folder>          the MPC's pages
 *   s23_skin_render --panel <out.png>     the whole VST panel, as a picture */
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace s23;

namespace
{
constexpr int TW = 1280, TH = 628;
constexpr int MAXF = 128, MAXH = 9800;   /* a film strip: at most 128 frames, at most 9800 pixels tall */

struct Tab {
    const char *name;
    int fn, sub;
    float scale;
    std::function<void(Builder &)> build;
};

int framesOf(int steps, int h) { return jmax(2, jmin(steps, MAXF, MAXH / jmax(1, h))); }
int indexOf(int steps, int frames, int f) { return frames == steps ? f : (int)std::lround((double)f * (steps - 1) / (frames - 1)); }

int stepsOf(RangedAudioParameter &p)
{
    if (auto *c = dynamic_cast<AudioParameterChoice *>(&p))
        return c->choices.size();
    if (dynamic_cast<AudioParameterBool *>(&p))
        return 2;
    return 128;
}

void setIndex(RangedAudioParameter &p, int steps, int i)
{
    i = jlimit(0, steps - 1, i);
    p.setValueNotifyingHost(steps > 1 ? (float)i / (float)(steps - 1) : 0.0f);
}

bool writePng(const Image &img, const File &f)
{
    f.deleteFile();
    FileOutputStream out(f);
    return out.openedOk() && PNGImageFormat().writeImageToStream(img, out);
}

var rectVar(Rectangle<int> r) { return Array<var>{r.getX(), r.getY(), r.getWidth(), r.getHeight()}; }

struct Strip {
    Rectangle<int> r;
    int frames = 0;
    Image img;
    std::vector<bool> done;
    void init(Rectangle<int> rr, int n)
    {
        r = rr;
        frames = n;
        img = Image(Image::ARGB, jmax(1, r.getWidth()), jmax(1, r.getHeight() * n), true);
        done.assign((size_t)n, false);
    }
    void grab(const Image &full, int f)
    {
        if (f < 0 || f >= frames || done[(size_t)f])
            return;
        Graphics g(img);
        g.drawImageAt(full.getClippedImage(r), 0, f * r.getHeight());
        done[(size_t)f] = true;
    }
};

struct Ctl {
    enum Kind { knob, button, label } kind;
    Component *c;
    RangedAudioParameter *p;
    int steps = 2, frames = 2;
    Rectangle<int> rect, frame;          /* the picture's place; the knob's whole slot (its legend too) */
    Strip strip, on, off;
    bool bipolar = false;
};

var cutPage(Page &page, float S, const String &tag, const File &dir)
{
    auto toMpc = [S](Rectangle<float> r) {
        int x0 = (int)std::lround(r.getX() * S), y0 = (int)std::lround(r.getY() * S);
        int x1 = (int)std::lround(r.getRight() * S), y1 = (int)std::lround(r.getBottom() * S);
        x0 = jlimit(0, TW, x0), x1 = jlimit(0, TW, x1), y0 = jlimit(0, TH, y0), y1 = jlimit(0, TH, y1);
        return Rectangle<int>{x0, y0, x1 - x0, y1 - y0};
    };
    auto snap = [&] {
        Image full(Image::ARGB, TW, TH, true);
        Graphics g(full);
        g.drawImageAt(page.createComponentSnapshot(page.getLocalBounds(), true, S), 0, 0);
        return full;
    };
    int n = 0;
    auto fileFor = [&](const String &what) {
        return tag + "_" + String(n++).paddedLeft('0', 3) + "_" + what.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_") + ".png";
    };

    std::vector<std::unique_ptr<Ctl>> ctls;
    for (auto *c : page.getChildren()) {
        auto ctl = std::make_unique<Ctl>();
        ctl->c = c;
        auto b = c->getBounds().toFloat();
        if (auto *k = dynamic_cast<Knob *>(c)) {
            ctl->kind = Ctl::knob;
            ctl->p = &k->param;
            ctl->frame = toMpc(b);
            ctl->rect = toMpc(b.withTrimmedBottom(k->legendHeight()));
            for (int i = 0; i < sol::NPARAMS; i++)
                if (k->param.paramID == sol::PARAMS[i].id)
                    ctl->bipolar = sol::PARAMS[i].kind == sol::K_BIP;
        } else if (auto *ky = dynamic_cast<Key *>(c)) {
            ctl->kind = Ctl::button;
            ctl->p = &ky->param;
            ctl->rect = ctl->frame = toMpc(b);
        } else if (auto *cr = dynamic_cast<Cartridge *>(c)) {
            ctl->kind = Ctl::knob;
            ctl->p = &cr->param;
            ctl->rect = ctl->frame = toMpc(b);
        } else if (auto *fl = dynamic_cast<FxLabel *>(c)) {
            ctl->kind = Ctl::label;
            ctl->p = &fl->param;
            ctl->rect = ctl->frame = toMpc(b);
        } else
            continue;
        ctl->steps = stepsOf(*ctl->p);
        if (ctl->kind == Ctl::knob)
            ctl->frames = framesOf(ctl->steps, ctl->rect.getHeight());
        else if (ctl->kind == Ctl::label)
            ctl->frames = ctl->steps;
        if (ctl->kind == Ctl::button) {
            ctl->on.init(ctl->rect, 1);
            ctl->off.init(ctl->rect, 1);
        } else
            ctl->strip.init(ctl->rect, ctl->frames);
        ctls.push_back(std::move(ctl));
    }

    /* the background: the controls' legends only, the labels and cartridges away */
    for (auto &c : ctls) {
        if (auto *k = dynamic_cast<Knob *>(c->c))
            k->paintMode = 1;
        else if (auto *ky = dynamic_cast<Key *>(c->c))
            ky->paintMode = 1;
        else
            c->c->setVisible(false);
    }
    Image bg = snap();
    String bgName = tag + "_panel.png";
    writePng(bg, dir.getChildFile(bgName));

    /* every control at every value */
    for (auto &c : ctls) {
        c->c->setVisible(true);
        if (auto *k = dynamic_cast<Knob *>(c->c))
            k->paintMode = 2;
        else if (auto *ky = dynamic_cast<Key *>(c->c))
            ky->paintMode = 0;
    }
    for (int i = 0; i < MAXF; i++) {
        bool any = false;
        for (auto &c : ctls) {
            int frames = c->kind == Ctl::button ? 2 : c->frames;
            if (i < frames) {
                setIndex(*c->p, c->steps, c->kind == Ctl::button ? i : indexOf(c->steps, c->frames, i));
                any = true;
            }
        }
        if (!any)
            break;
        auto full = snap();
        for (auto &c : ctls) {
            if (c->kind == Ctl::button) {
                if (i == 0)
                    c->off.grab(full, 0);
                else if (i == 1)
                    c->on.grab(full, 0);
            } else if (i < c->frames)
                c->strip.grab(full, i);
        }
    }

    Array<var> list;
    for (auto &c : ctls) {
        auto *o = new DynamicObject();
        o->setProperty("id", c->p->paramID);
        o->setProperty("steps", c->steps);
        o->setProperty("rect", rectVar(c->rect));
        o->setProperty("frame", rectVar(c->frame));
        o->setProperty("bipolar", c->bipolar);
        Array<var> names;
        if (c->steps < 128)
            for (int i = 0; i < c->steps; i++) {
                setIndex(*c->p, c->steps, i);
                names.add(c->p->getCurrentValueAsText().toUpperCase());
            }
        o->setProperty("names", names);
        if (c->kind == Ctl::button) {
            o->setProperty("kind", "button");
            String fOff = fileFor(c->p->paramID + "_off"), fOn = fileFor(c->p->paramID + "_on");
            writePng(c->off.img, dir.getChildFile(fOff));
            writePng(c->on.img, dir.getChildFile(fOn));
            o->setProperty("off", fOff);
            o->setProperty("on", fOn);
        } else {
            o->setProperty("kind", c->kind == Ctl::knob ? "knob" : "label");
            String f = fileFor(c->p->paramID);
            writePng(c->strip.img, dir.getChildFile(f));
            o->setProperty("strip", f);
            o->setProperty("frames", c->frames);
        }
        list.add(var(o));
    }
    auto *pg = new DynamicObject();
    pg->setProperty("background", bgName);
    pg->setProperty("controls", list);
    return var(pg);
}
} // namespace

int main(int argc, char **argv)
{
    ScopedJuceInitialiser_GUI gui;
    if (argc >= 3 && String(argv[1]) == "--panel") {
        SolarisProcessor proc;
        std::unique_ptr<AudioProcessorEditor> ed(proc.createEditor());
        ed->setSize(SolarisEditor::W, SolarisEditor::H);
        return writePng(ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f), File(argv[2])) ? 0 : 1;
    }
    if (argc < 2) {
        std::fprintf(stderr, "usage: s23_skin_render <out folder> | --panel <out.png>\n");
        return 2;
    }
    File dir = File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    dir.deleteRecursively();
    dir.createDirectory();

    const std::vector<Tab> tabs = {
        {"DRONE 1/2", 0, 0, 2.0f, [](Builder &b) { b.classic(1, {16, 2, 300, 290}); b.classic(2, {324, 2, 300, 290}); }},
        {"DRONE 4/5", 0, 1, 2.0f, [](Builder &b) { b.classic(4, {16, 2, 300, 290}); b.classic(5, {324, 2, 300, 290}); }},
        {"CHAOS", 1, 0, 2.0f, [](Builder &b) { b.chaos(3, {16, 2, 300, 290}); b.chaos(6, {324, 2, 300, 290}); }},
        {"VCO", 2, 0, 2.0f, [](Builder &b) { b.vco(0, {16, 2, 300, 290}); b.vco(1, {324, 2, 300, 290}); }},
        {"LFO/JOY", 3, 0, 1.9f, [](Builder &b) {
             b.lfo(0, {6, 4, 190, 150});
             b.joystick({202, 4, 270, 150});
             b.lfo(1, {478, 4, 190, 150});
             b.light({6, 160, 210, 150});
             b.global({222, 160, 212, 140});
         }},
        {"SEQ/KEYS", 3, 1, 1.95f, [](Builder &b) {
             b.sequencer({10, 4, 636, 150});
             b.droneKeys({10, 162, 260, 140});
         }},
        {"MIX/FX", 4, 0, 2.0f, [](Builder &b) {
             b.mixer({16, 2, 296, 140});
             b.filter({16, 150, 296, 142});
             b.effector({328, 2, 296, 290});
         }},
    };

    Array<var> pages;
    for (auto &t : tabs) {
        SolarisProcessor proc;           /* (a fresh one per page: every control at its default) */
        Look look;
        Page page;
        page.setLookAndFeel(&look);
        page.setSize((int)std::ceil((float)TW / t.scale), (int)std::ceil((float)TH / t.scale));
        Builder b{proc.apvts, page, nullptr};
        b.mpc = true;
        t.build(b);
        const float S = t.scale;
        page.extra = [S](Graphics &g) {   /* the maker's tag, bottom right: 200 x 24 on the MPC */
            drawTag(g, Rectangle<float>(TW - 18.0f - 200.0f, TH - 8.0f - 24.0f, 200.0f, 24.0f) / S);
        };
        String tag = String(t.name).toLowerCase().retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
        var pg = cutPage(page, t.scale, tag, dir);
        pg.getDynamicObject()->setProperty("name", t.name);
        pg.getDynamicObject()->setProperty("fn", t.fn);
        pg.getDynamicObject()->setProperty("sub", t.sub);
        pages.add(pg);
        page.deleteAllChildren();
        page.setLookAndFeel(nullptr);
        std::printf("%s: %d controls\n", t.name, (int)pg["controls"].size());
    }
    auto *m = new DynamicObject();
    m->setProperty("pages", pages);
    dir.getChildFile("manifest.json").replaceWithText(JSON::toString(var(m)));
    return 0;
}
