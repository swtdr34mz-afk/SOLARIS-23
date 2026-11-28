/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIUM-23A) */
/* The panel: every section laid out on a page of 1560 x 1000 (src/ui/Sections.cpp), scaled to the window. On top
 * the logo, the program (its list, the arrows, SAVE) and the readout of the control in hand. */
#pragma once
#include "PluginProcessor.h"
#include "ui/Sections.h"

class SolarisEditor : public juce::AudioProcessorEditor, private juce::Timer, private s23::Touch
{
public:
    static constexpr int W = 1560, H = 1000;
    explicit SolarisEditor(SolarisProcessor &);
    ~SolarisEditor() override;
    void resized() override;
    void paint(juce::Graphics &) override;
    void settle();                       /* (a resize at rest: the panel drawn sharp again) */
    bool sized = false;                      /* (resizes during the constructor keep nothing) */

private:
    void timerCallback() override;
    void touched(juce::RangedAudioParameter &) override;
    void programMenu();
    void saveProgram();
    void finishMenu();
    void zoomTo(int section);            /* -1: back to the whole panel */
    juce::AffineTransform pageTransform(int section) const;
    void animate();
    void makeShade();

    SolarisProcessor &proc;
    s23::Look look;
    s23::Page page;
    juce::RangedAudioParameter *inHand = nullptr;
    juce::String headerKey;
    int resizeTicks = 0;
    juce::Image frozen;                  /* (the panel's picture while the window is resized) */
    /* the section zoom: a click on a section's name tab shows it as large as the window allows */
    int zoomed = -1;
    juce::AffineTransform animFrom, animTo;
    double animStart = 0.0;
    struct Shade;
    std::unique_ptr<juce::Component> shade;
    struct Anim : juce::Timer {
        SolarisEditor &ed;
        explicit Anim(SolarisEditor &e) : ed(e) {}
        void timerCallback() override { ed.animate(); }
    } anim{*this};
    struct Header;
    std::unique_ptr<juce::Component> header;
    std::vector<juce::Component *> lamps;
};
