/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* SOLARIS-23's look: a dark panel, each kind of section in its own colour (the drones, the chaos voices, the VCOs,
 * the modulation, the filters and effects), cream knobs with a coloured line, keys with an LED, a glowing light
 * sensor. The colours are one table (Theme), so a new scheme is one place to change. Fonts embedded (Fira Sans,
 * Anonymous Pro: SIL OFL, fonts/). */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

namespace s23
{
enum class Sec { drone, chaos, vco, mod, fx, mix, keys };

struct Theme {
    juce::Colour panel, panelEdge, legend, dim, knobBody, knobCap, led, ledOff, plate, plateLit;
    juce::Colour sec[7];                 /* each kind of section (Sec) */
    juce::Colour secText[7];             /* the title's text on it */
};
const Theme &theme();
juce::Colour secColour(Sec s);

enum class Face { label, title, logo, mono };
juce::Font font(Face f, float height);

class Look : public juce::LookAndFeel_V4
{
public:
    Look();
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override;
    juce::Font getPopupMenuFont() override { return font(Face::label, 15.0f); }
    void drawPopupMenuBackground(juce::Graphics &, int w, int h) override;
};

/* a section: a rounded plate in its colour, its name in a bar on top */
void drawSection(juce::Graphics &, juce::Rectangle<float> r, const juce::String &title, Sec s);
void drawPanel(juce::Graphics &, juce::Rectangle<float> r);
void drawKnob(juce::Graphics &, juce::Rectangle<float> r, float pos, juce::Colour accent, bool bipolar, bool stepped);
void drawKey(juce::Graphics &, juce::Rectangle<float> r, bool on, bool down, juce::Colour accent, bool big);
void drawLed(juce::Graphics &, juce::Point<float> c, float r, float level, juce::Colour col);
void drawDome(juce::Graphics &, juce::Rectangle<float> r, float level);
void drawLegend(juce::Graphics &, const juce::String &, juce::Rectangle<float>, float size = 11.0f,
                juce::Colour c = juce::Colours::transparentBlack);
void drawTag(juce::Graphics &, juce::Rectangle<float> r);   /* the maker's tag (res/swtdr34mz-tag.png) */
void drawLogo(juce::Graphics &, juce::Rectangle<float> r);
} // namespace s23
