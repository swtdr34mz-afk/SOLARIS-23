/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* SOLARIS-23's look: a heavy-aged relic. A black, rough steel panel with decades of wear (scratches, scuffs to the
 * primer, chipped edges, grime, rust at the screws), bone print worn thin, black bakelite / brushed aluminium / bone /
 * oxide-red caps, dulled chrome toggles, push buttons, drone keys, old glass light sensors, amber LCDs, brass touch
 * plates. Fonts embedded (Fira Sans, Anonymous Pro: SIL OFL, fonts/). */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

namespace s23
{
/* the knobs' caps */
enum class KS { black, slate, light, blue, red };
/* the switches */
enum class SW { toggle, button, buttonGrey, key };
/* where a section sits (only the touch field is drawn differently: black) */
enum class Sec { panel, field };

/* the panel's finish */
enum Finish { F_ORANGE, F_YELLOW, F_WHITE, F_BLUE, F_PINK, NFINISH };
const char *finishName(int f);
void setFinish(int f);
int finish();
juce::Colour panelColour();
juce::Colour ink();                      /* the print: black, or white on the blue panel */
juce::Colour lcdText();

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

void drawPanel(juce::Graphics &, juce::Rectangle<float> r);
void drawWear(juce::Graphics &, juce::Rectangle<float> r);   /* over the print: scratches, scuffs, chips, grime */
void drawScrew(juce::Graphics &, juce::Point<float> c, float r);
void drawSection(juce::Graphics &, juce::Rectangle<float> r, const juce::String &title, Sec s);
void drawKnob(juce::Graphics &, juce::Rectangle<float> r, float pos, KS style, bool bipolar, bool ticks);
void drawToggle(juce::Graphics &, juce::Rectangle<float> r, bool up);
void drawButton(juce::Graphics &, juce::Rectangle<float> r, bool on, bool down, bool grey);
void drawDroneKey(juce::Graphics &, juce::Rectangle<float> r, bool on, bool down);
void drawLed(juce::Graphics &, juce::Point<float> c, float r, float level, juce::Colour col);
void drawDome(juce::Graphics &, juce::Rectangle<float> r, float level);
void drawLcd(juce::Graphics &, juce::Rectangle<float> r);
void drawLegend(juce::Graphics &, const juce::String &, juce::Rectangle<float>, float size = 11.0f,
                juce::Colour c = juce::Colours::transparentBlack);
void drawTag(juce::Graphics &, juce::Rectangle<float> r);   /* the maker's tag (res/swtdr34mz-tag.png), in the print */
void drawLogo(juce::Graphics &, juce::Rectangle<float> r);
const juce::Colour RED_LED{0xffff4a24}, AMBER_LED{0xffffa820}, BLUE_LED{0xffffa820};
} // namespace s23
