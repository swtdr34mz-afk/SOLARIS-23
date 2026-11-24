/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
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
    void paint(juce::Graphics &) override {}

private:
    void timerCallback() override;
    void touched(juce::RangedAudioParameter &) override;
    void programMenu();
    void saveProgram();
    void finishMenu();

    SolarisProcessor &proc;
    s23::Look look;
    s23::Page page;
    juce::RangedAudioParameter *inHand = nullptr;
    juce::String headerKey;
    struct Header;
    std::unique_ptr<juce::Component> header;
    std::vector<juce::Component *> lamps;
};
