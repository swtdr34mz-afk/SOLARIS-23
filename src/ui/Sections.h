/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The panel's sections, each built at its place on the page (PluginEditor.cpp lays them out). Every control is a
 * child of the page; the page draws the panel, the sections' frames and the printed labels. */
#pragma once
#include "Controls.h"
#include "PluginProcessor.h"

namespace s23
{
/* the levels the panel shows (the processor's meters) */
struct Meters {
    std::function<float()> light = [] { return 0.0f; };
    std::function<float(int)> lfo = [](int) { return 0.0f; };
    std::function<float(int)> voice = [](int) { return 0.0f; };
    std::function<int()> step = [] { return -1; };
};

struct Page : juce::Component {
    ~Page() override { deleteAllChildren(); }   /* (the page owns its controls: none outlives the editor) */
    struct SecRect {
        juce::Rectangle<float> r;
        juce::String title;
        Sec sec;
    };
    struct Text {
        juce::Rectangle<float> r;
        juce::String text;
        float size;
        bool vertical;
    };
    std::vector<SecRect> sections;
    std::vector<Text> texts;
    std::vector<juce::Line<float>> dotted;
    std::vector<juce::Point<float>> screws;
    std::function<void(juce::Graphics &)> extra;
    void paint(juce::Graphics &g) override;
    void paintBackground(juce::Graphics &g);

private:
    juce::Image cache;                  /* the panel, its print and its wear: drawn once */
    float cacheScale = 0.0f;
};

struct Builder {
    APVTS &apvts;
    Page &page;
    Touch *touch = nullptr;
    Meters meters;
    juce::MidiKeyboardState *keys = nullptr;

    template <class C> C *add(C *c, juce::Rectangle<int> r)
    {
        page.addAndMakeVisible(c);
        c->setBounds(r);
        return c;
    }
    void section(juce::Rectangle<int> r, const juce::String &title, Sec s = Sec::panel);
    void text(juce::Rectangle<int> r, const juce::String &t, float size = 10.5f, bool vertical = false);
    void dots(float x0, float y0, float x1, float y1);
    Knob *knob(const char *id, const juce::String &legend, KS s, juce::Rectangle<int> r, float size = 0.0f, bool ticks = false);
    Switch *sw(const char *id, const juce::String &legend, SW s, juce::Rectangle<int> r);

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
