/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The panel's controls, each tied to host parameters: a knob (on a choice it steps, its value in a small LCD), a
 * switch (a metal toggle, a round push button, a drone key), the joystick (X and Y), a cartridge slot (an effect
 * program), the touch plates, a lamp (an LED, the light sensor). Touching one reports it, for the readout. */
#pragma once
#include "Look.h"

namespace s23
{
using APVTS = juce::AudioProcessorValueTreeState;

struct Touch {
    virtual ~Touch() = default;
    virtual void touched(juce::RangedAudioParameter &) = 0;
};

class Knob : public juce::Slider
{
public:
    Knob(APVTS &, const juce::String &id, const juce::String &legend, KS style, Touch *);
    void paint(juce::Graphics &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    void mouseDown(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    juce::String legend;
    KS style;
    bool ticks = false;                  /* a printed scale around it */
    bool showValue = false;              /* the value in an LCD under it (a choice: always) */
    float knobSize = 0.0f;               /* 0: as large as fits */
    juce::Rectangle<float> knobArea() const;

private:
    bool bipolar = false;
    Touch *touch;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

class Switch : public juce::Component
{
public:
    Switch(APVTS &, const juce::String &id, const juce::String &legend, SW style, Touch *);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    juce::String legend;
    SW style;
    bool on() const { return param.getValue() >= 0.5f; }

private:
    Touch *touch;
    bool down = false;
    juce::ParameterAttachment att;
};

class XYPad : public juce::Component
{
public:
    XYPad(APVTS &, const juce::String &xid, const juce::String &yid, Touch *);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseDoubleClick(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &px, &py;

private:
    void moveTo(juce::Point<float>);
    Touch *touch;
    juce::ParameterAttachment ax, ay;
};

/* a cartridge slot: the program of one side (the cartridge in the slot, its program in the LCD); click for the list,
 * the 1-2-3 switch for the cartridge's next program */
class Cartridge : public juce::Component
{
public:
    Cartridge(APVTS &, const juce::String &id, const juce::String &side, Touch *);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    int fx() const;

private:
    juce::String side;
    Touch *touch;
    juce::ParameterAttachment att;
};

/* what X, Y or Z does on one side: follows that side's program */
class FxLabel : public juce::Component
{
public:
    FxLabel(APVTS &, const juce::String &id, const juce::String &side, int k);
    void paint(juce::Graphics &) override;
    juce::RangedAudioParameter &param;
    juce::String side;
    int k;

private:
    juce::ParameterAttachment att;
};

/* the touch plates: twelve notes from C, the octave chosen by the two buttons */
class Plates : public juce::Component
{
public:
    explicit Plates(juce::MidiKeyboardState &);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    int octave = 3;                      /* C3 (48) */

private:
    juce::MidiKeyboardState &ks;
    int held = -1;
    int plateAt(juce::Point<float>) const;
    juce::Rectangle<float> plate(int i) const;
    juce::Rectangle<float> arrow(int dir) const;
};

/* anything lit: a lambda's 0..1 */
class Lamp : public juce::Component
{
public:
    enum Kind { led, dome };
    Lamp(Kind k, std::function<float()> level, juce::Colour c = RED_LED) : kind(k), level(std::move(level)), col(c)
    {
        setInterceptsMouseClicks(false, false);
    }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat();
        if (kind == dome)
            drawDome(g, r, level());
        else
            drawLed(g, r.getCentre(), (r.getWidth() < r.getHeight() ? r.getWidth() : r.getHeight()) * 0.28f, level(), col);
    }
    Kind kind;
    std::function<float()> level;
    juce::Colour col;
};
} // namespace s23
