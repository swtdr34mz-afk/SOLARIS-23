/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The panel's controls, each tied to host parameters: a knob (on a choice it steps and shows the choice), a key
 * (a switch, with its LED), the joystick (X and Y), a cartridge slot (an effect program), the touch plates, the light
 * sensor, an LED. Touching one reports it, for the readout. */
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
    Knob(APVTS &, const juce::String &id, const juce::String &legend, Sec sec, Touch *);
    void paint(juce::Graphics &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    void mouseDown(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    juce::String legend;
    Sec sec;
    bool showValue = false;              /* the value's text under the legend (a choice: always) */
    float knobSize = 0.0f;               /* 0: as large as fits */
    int paintMode = 0;                   /* (the MPC skin's renderer: 1 the legend only, 2 all but the legend) */
    float legendHeight() const { return legend.isNotEmpty() ? 13.0f : 0.0f; }
    juce::Rectangle<float> knobArea() const;

private:
    Touch *touch;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

class Key : public juce::Component
{
public:
    Key(APVTS &, const juce::String &id, const juce::String &legend, Sec sec, Touch *, bool big = false);
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseEnter(const juce::MouseEvent &) override;
    juce::RangedAudioParameter &param;
    juce::String legend;
    Sec sec;
    bool big;
    bool on() const { return param.getValue() >= 0.5f; }
    int paintMode = 0;                   /* (as Knob's) */

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

/* a cartridge slot: the effect program of one side, its cartridge and program number; click for the list, the
 * arrows for the next */
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

/* the touch plates: twelve notes from C, an octave chosen by the two arrows */
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
    Lamp(Kind k, std::function<float()> level, juce::Colour c = juce::Colours::orange) : kind(k), level(std::move(level)), col(c)
    {
        setInterceptsMouseClicks(false, false);
    }
    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat();
        if (kind == dome)
            drawDome(g, r, level());
        else
            drawLed(g, r.getCentre(), jmin(r.getWidth(), r.getHeight()) * 0.28f, level(), col);
    }
    Kind kind;
    std::function<float()> level;
    juce::Colour col;

private:
    static float jmin(float a, float b) { return a < b ? a : b; }
};
} // namespace s23
