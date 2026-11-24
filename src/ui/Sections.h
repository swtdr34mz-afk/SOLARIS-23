/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The panel's sections, each built at a place on a page: the VST's panel lays them all out (PluginEditor.cpp), the
 * MPC's skin renderer (mpc/skin/) lays a few out per page, larger. Every control is a child of the page; the page
 * draws the sections' plates and the static labels. */
#pragma once
#include "Controls.h"
#include "PluginProcessor.h"

namespace s23
{
/* the levels the panel shows (the processor's meters; on the MPC renderer: zero) */
struct Meters {
    std::function<float()> light = [] { return 0.0f; };
    std::function<float(int)> lfo = [](int) { return 0.0f; };
    std::function<float(int)> voice = [](int) { return 0.0f; };
    std::function<int()> step = [] { return -1; };
};

struct Page : juce::Component {
    struct SecRect {
        juce::Rectangle<float> r;
        juce::String title;
        Sec sec;
    };
    struct Text {
        juce::Rectangle<float> r;
        juce::String text;
        float size;
        bool dim;
    };
    std::vector<SecRect> sections;
    std::vector<Text> texts;
    bool drawFace = true;
    std::function<void(juce::Graphics &)> extra;
    void paint(juce::Graphics &g) override;
};

struct Builder {
    APVTS &apvts;
    Page &page;
    Touch *touch = nullptr;
    Meters meters;
    juce::MidiKeyboardState *keys = nullptr;
    bool mpc = false;                    /* the MPC's pages: knobs for the joystick (no pad) */

    template <class C> C *add(C *c, juce::Rectangle<int> r)
    {
        page.addAndMakeVisible(c);
        c->setBounds(r);
        return c;
    }
    void section(juce::Rectangle<int> r, const juce::String &title, Sec s);
    void text(juce::Rectangle<int> r, const juce::String &t, float size = 10.5f, bool dim = true);
    Knob *knob(const char *id, const juce::String &legend, Sec s, juce::Rectangle<int> r, float size = 0.0f);
    Key *key(const char *id, const juce::String &legend, Sec s, juce::Rectangle<int> r, bool big = false);

    /* the sections (r: where; each fits the size the VST gives it, and scales) */
    void classic(int drone, juce::Rectangle<int> r);          /* DRONE 1, 2, 4, 5: 300 x 290 */
    void chaos(int drone, juce::Rectangle<int> r);            /* DRONE 3, 6: 300 x 290 */
    void vco(int v, juce::Rectangle<int> r);                  /* 0 A, 1 B: 300 x 290 */
    void effector(juce::Rectangle<int> r);                    /* 296 x 290 */
    void mixer(juce::Rectangle<int> r);                       /* 296 x 140 */
    void filter(juce::Rectangle<int> r);                      /* 296 x 142 */
    void lfo(int l, juce::Rectangle<int> r);                  /* 190 x 150 */
    void joystick(juce::Rectangle<int> r);                    /* 270 x 150 */
    void sequencer(juce::Rectangle<int> r);                   /* 636 x 150 */
    void light(juce::Rectangle<int> r);                       /* 210 x 150 */
    void droneKeys(juce::Rectangle<int> r);                   /* 260 x 140 */
    void global(juce::Rectangle<int> r);                      /* 212 x 140 */
    void plates(juce::Rectangle<int> r);                      /* 1040 x 140 */
};
} // namespace s23
