/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
#include "Look.h"
#include "BinaryData.h"
#include <atomic>

namespace s23
{
using namespace juce;

/* ------------------------------------------------------------------------------------- finish --- */
namespace
{
std::atomic<int> currentFinish{F_ORANGE};
const uint32 FINISH_COL[NFINISH] = {0xfff39a1e, 0xfff2c230, 0xffe9e6dc, 0xff1f5fa8, 0xffef8fb0};
const char *const FINISH_NAME[NFINISH] = {"ORANGE", "YELLOW", "WHITE", "BLUE", "PINK"};
} // namespace

const char *finishName(int f) { return FINISH_NAME[jlimit(0, NFINISH - 1, f)]; }
void setFinish(int f) { currentFinish = jlimit(0, NFINISH - 1, f); }
int finish() { return currentFinish.load(); }
Colour panelColour() { return Colour(FINISH_COL[finish()]); }
Colour ink() { return finish() == F_BLUE ? Colour(0xfff4f4f0) : Colour(0xff121212); }
Colour lcdText() { return Colour(0xffe8eeff); }

static Typeface::Ptr face(Face f)
{
    static Typeface::Ptr t[4] = {
        Typeface::createSystemTypefaceFor(BinaryData::FiraSansSemiBold_ttf, BinaryData::FiraSansSemiBold_ttfSize),
        Typeface::createSystemTypefaceFor(BinaryData::FiraSansBold_ttf, BinaryData::FiraSansBold_ttfSize),
        Typeface::createSystemTypefaceFor(BinaryData::FiraSansExtraBold_ttf, BinaryData::FiraSansExtraBold_ttfSize),
        Typeface::createSystemTypefaceFor(BinaryData::AnonymousProBold_ttf, BinaryData::AnonymousProBold_ttfSize),
    };
    return t[(int)f];
}

Font font(Face f, float height) { return Font(FontOptions(face(f)).withHeight(height)); }

Look::Look()
{
    setColour(ResizableWindow::backgroundColourId, Colour(0xff151515));
    setColour(PopupMenu::backgroundColourId, Colour(0xff1d1d1f));
    setColour(PopupMenu::textColourId, Colour(0xffeeeeee));
    setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xfff39a1e));
    setColour(PopupMenu::highlightedTextColourId, Colours::black);
    setColour(PopupMenu::headerTextColourId, Colour(0xff9a9a9a));
    setColour(TextEditor::backgroundColourId, Colour(0xff2d3fbf));
    setColour(TextEditor::textColourId, lcdText());
    setColour(TextEditor::outlineColourId, Colour(0xff111111));
    setColour(AlertWindow::backgroundColourId, Colour(0xff1d1d1f));
    setColour(AlertWindow::textColourId, Colour(0xffeeeeee));
    setColour(TextButton::buttonColourId, Colour(0xff333336));
    setColour(TextButton::textColourOffId, Colour(0xffeeeeee));
}

Typeface::Ptr Look::getTypefaceForFont(const Font &) { return face(Face::label); }

void Look::drawPopupMenuBackground(Graphics &g, int w, int h)
{
    g.fillAll(findColour(PopupMenu::backgroundColourId));
    g.setColour(Colour(0xff444448));
    g.drawRect(0, 0, w, h, 1);
}

/* -------------------------------------------------------------------------------------- panel --- */
/* the powder coat: a fine, even grain over the colour (a tile of specks, light and dark) */
static const Image &grain()
{
    static Image tile = [] {
        Image im(Image::ARGB, 192, 192, true);
        Random r(4223);
        for (int y = 0; y < im.getHeight(); y++)
            for (int x = 0; x < im.getWidth(); x++) {
                float v = r.nextFloat();
                if (v < 0.30f)
                    im.setPixelAt(x, y, Colours::black.withAlpha(0.035f + 0.05f * r.nextFloat()));
                else if (v > 0.74f)
                    im.setPixelAt(x, y, Colours::white.withAlpha(0.03f + 0.05f * r.nextFloat()));
            }
        return im;
    }();
    return tile;
}

void drawPanel(Graphics &g, Rectangle<float> r)
{
    g.setColour(panelColour());
    g.fillRect(r);
    g.setTiledImageFill(grain(), 0, 0, 1.0f);
    g.fillRect(r);
    /* a soft sheen from above, the corners a little darker (a bent metal sheet) */
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.10f), r.getCentreX(), r.getY(), Colours::transparentWhite,
                                     r.getCentreX(), r.getY() + r.getHeight() * 0.55f, false));
    g.fillRect(r);
    g.setGradientFill(ColourGradient(Colours::transparentBlack, r.getCentreX(), r.getCentreY(), Colours::black.withAlpha(0.13f),
                                     r.getX(), r.getY(), true));
    g.fillRect(r);
    g.setColour(Colours::black.withAlpha(0.5f));
    g.drawRect(r, 1.5f);
}

void drawScrew(Graphics &g, Point<float> c, float rr)
{
    g.setColour(Colours::black.withAlpha(0.35f));
    g.fillEllipse(Rectangle<float>(rr * 2.3f, rr * 2.3f).withCentre(c.translated(0.4f, 0.8f)));
    g.setGradientFill(ColourGradient(Colour(0xfff2f2f2), c.x - rr, c.y - rr, Colour(0xff7d7f84), c.x + rr, c.y + rr, false));
    g.fillEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c));
    g.setColour(Colour(0xff55575c));
    g.drawEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c), 0.6f);
    g.setColour(Colour(0xff3c3d40));
    Path p;                              /* a cross head */
    p.addRectangle(-rr * 0.62f, -rr * 0.12f, rr * 1.24f, rr * 0.24f);
    p.addRectangle(-rr * 0.12f, -rr * 0.62f, rr * 0.24f, rr * 1.24f);
    g.fillPath(p, AffineTransform::rotation(0.6f).translated(c));
}

void drawSection(Graphics &g, Rectangle<float> r, const String &title, Sec s)
{
    const Colour k = ink();
    if (s == Sec::field) {               /* the touch field: a black plate set into the panel */
        g.setColour(Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(r.translated(0.0f, 1.5f), 6.0f);
        g.setGradientFill(ColourGradient(Colour(0xff262628), r.getX(), r.getY(), Colour(0xff121213), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r, 6.0f);
        g.setColour(Colours::white.withAlpha(0.07f));
        g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);
    } else {
        g.setColour(k);
        g.drawRoundedRectangle(r.reduced(0.9f), 6.0f, 1.8f);
    }
    if (title.isEmpty())
        return;
    /* the name: a tab of the print, its letters the panel's colour, in the frame's corner */
    auto f = font(Face::title, 12.0f).withExtraKerningFactor(0.06f);
    GlyphArrangement ga;
    ga.addLineOfText(f, title, 0, 0);
    float w = ga.getBoundingBox(0, -1, true).getWidth() + 20.0f;
    auto tab = Rectangle<float>(r.getX(), r.getY(), jmin(w, r.getWidth()), 19.0f);
    Path p;
    p.addRoundedRectangle(tab.getX(), tab.getY(), tab.getWidth(), tab.getHeight(), 6.0f, 6.0f, true, false, false, true);
    g.setColour(s == Sec::field ? Colours::black : k);
    g.fillPath(p);
    g.setColour(s == Sec::field ? Colour(0xffe9e9e9) : panelColour());
    g.setFont(f);
    g.drawText(title, tab.withTrimmedLeft(10.0f), Justification::centredLeft, false);
}

/* --------------------------------------------------------------------------------------- knobs --- */
namespace
{
struct Cap {
    Colour skirt, top, hi, line;
};
Cap capOf(KS s)
{
    switch (s) {
    case KS::black: return {Colour(0xff151516), Colour(0xff232325), Colour(0xff55555a), Colour(0xffeeeeee)};
    case KS::slate: return {Colour(0xff2f343c), Colour(0xff4b525d), Colour(0xff8790a0), Colour(0xffeeeeee)};
    case KS::light: return {Colour(0xff8e9198), Colour(0xffcfd2d8), Colour(0xfff8f9fb), Colour(0xff151515)};
    case KS::blue: return {Colour(0xff2a2e35), Colour(0xff1466ad), Colour(0xff62b0f0), Colour(0xfff2f2f2)};
    case KS::red: return {Colour(0xff7c120c), Colour(0xffd8291c), Colour(0xffff8670), Colour(0xfff6f6f6)};
    }
    return {};
}
} // namespace

void drawKnob(Graphics &g, Rectangle<float> r, float pos, KS style, bool bipolar, bool ticks)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    const Point<float> c = r.getCentre();
    const float a0 = -2.42f, a1 = 2.42f, ang = a0 + jlimit(0.0f, 1.0f, pos) * (a1 - a0);
    const float R = d * 0.5f * (ticks ? 0.74f : 0.96f);
    const Cap cap = capOf(style);
    const Colour k = ink();

    if (ticks) {                         /* the scale, printed */
        g.setColour(k);
        for (int i = 0; i <= 10; i++) {
            float a = a0 + (a1 - a0) * (float)i / 10.0f;
            float len = (i == 0 || i == 5 || i == 10) ? 0.24f : 0.15f;
            g.drawLine(Line<float>(c.getPointOnCircumference(d * 0.5f * (1.0f - len), a), c.getPointOnCircumference(d * 0.5f, a)),
                       jmax(1.0f, d * 0.026f));
        }
        if (bipolar)
            g.fillEllipse(Rectangle<float>(d * 0.07f, d * 0.07f).withCentre(c.getPointOnCircumference(d * 0.5f + d * 0.06f, 0.0f)));
    }
    /* the shadow on the panel */
    g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.5f), c.x + R * 0.12f, c.y + R * 0.22f, Colours::transparentBlack,
                                     c.x + R * 0.12f, c.y + R * 1.32f, true));
    g.fillEllipse(Rectangle<float>(R * 2.5f, R * 2.5f).withCentre(c.translated(R * 0.12f, R * 0.22f)));
    /* the skirt: knurled */
    auto skirt = Rectangle<float>(R * 2.0f, R * 2.0f).withCentre(c);
    g.setGradientFill(ColourGradient(cap.skirt.brighter(0.3f), skirt.getX(), skirt.getY(), cap.skirt.darker(0.4f), skirt.getRight(),
                                     skirt.getBottom(), false));
    g.fillEllipse(skirt);
    const int ribs = jlimit(18, 48, (int)(R * 1.4f));
    for (int i = 0; i < ribs; i++) {
        float a = MathConstants<float>::twoPi * (float)i / (float)ribs;
        float lit = 0.5f + 0.5f * std::cos(a + 2.3f);
        g.setColour((lit > 0.5f ? Colours::white : Colours::black).withAlpha(0.06f + 0.12f * std::fabs(lit - 0.5f) * 2.0f));
        g.drawLine(Line<float>(c.getPointOnCircumference(R * 0.80f, a), c.getPointOnCircumference(R * 0.99f, a)), jmax(0.7f, R * 0.045f));
    }
    /* the cap: a dome of plastic, lit from the upper left */
    auto top = Rectangle<float>(R * 1.56f, R * 1.56f).withCentre(c.translated(0.0f, -R * 0.03f));
    g.setGradientFill(ColourGradient(cap.hi, top.getX() + top.getWidth() * 0.3f, top.getY() + top.getHeight() * 0.22f, cap.top.darker(0.25f),
                                     top.getRight(), top.getBottom(), true));
    g.fillEllipse(top);
    g.setColour(Colours::black.withAlpha(0.25f));
    g.drawEllipse(top, jmax(0.6f, R * 0.03f));
    g.setColour(Colours::white.withAlpha(style == KS::light ? 0.35f : 0.12f));
    g.drawEllipse(top.reduced(R * 0.05f), jmax(0.5f, R * 0.025f));
    /* the line */
    g.setColour(cap.line);
    g.drawLine(Line<float>(c.getPointOnCircumference(R * 0.18f, ang), c.getPointOnCircumference(R * 0.74f, ang)), jmax(1.4f, R * 0.11f));
}

/* ------------------------------------------------------------------------------------ switches --- */
void drawToggle(Graphics &g, Rectangle<float> r, bool up)
{
    const Point<float> c = r.getCentre();
    const float s = jmin(r.getWidth() * 0.9f, r.getHeight() * 0.5f);
    /* the hex nut */
    g.setColour(Colours::black.withAlpha(0.35f));
    g.fillEllipse(Rectangle<float>(s * 1.05f, s * 1.05f).withCentre(c.translated(0.5f, 1.2f)));
    Path hex;
    for (int i = 0; i < 6; i++) {
        auto p = c.getPointOnCircumference(s * 0.5f, MathConstants<float>::pi / 6.0f + (float)i * MathConstants<float>::pi / 3.0f);
        if (i)
            hex.lineTo(p);
        else
            hex.startNewSubPath(p);
    }
    hex.closeSubPath();
    g.setGradientFill(ColourGradient(Colour(0xfff0f0f0), c.x - s * 0.4f, c.y - s * 0.4f, Colour(0xff6f7176), c.x + s * 0.4f, c.y + s * 0.4f, false));
    g.fillPath(hex);
    g.setColour(Colour(0xff4a4b4f));
    g.strokePath(hex, PathStrokeType(0.7f));
    g.setColour(Colour(0xff2a2b2e));
    g.fillEllipse(Rectangle<float>(s * 0.42f, s * 0.42f).withCentre(c));
    /* the bat: chrome, tapering to a round tip, up or down */
    const float dir = up ? -1.0f : 1.0f, len = r.getHeight() * 0.40f;
    Point<float> tip(c.x, c.y + dir * len);
    Path bat;
    bat.startNewSubPath(c.x - s * 0.13f, c.y);
    bat.lineTo(tip.x - s * 0.09f, tip.y);
    bat.lineTo(tip.x + s * 0.09f, tip.y);
    bat.lineTo(c.x + s * 0.13f, c.y);
    bat.closeSubPath();
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillPath(bat, AffineTransform::translation(1.2f, 1.6f));
    g.setGradientFill(ColourGradient(Colour(0xff8a8c91), c.x - s * 0.15f, c.y, Colour(0xfffbfbfb), c.x + s * 0.02f, c.y, false));
    g.fillPath(bat);
    auto knob = Rectangle<float>(s * 0.34f, s * 0.34f).withCentre(tip);
    g.setGradientFill(ColourGradient(Colours::white, knob.getX(), knob.getY(), Colour(0xff7b7d82), knob.getRight(), knob.getBottom(), false));
    g.fillEllipse(knob);
    g.setColour(Colour(0xff55575b));
    g.drawEllipse(knob, 0.6f);
}

void drawButton(Graphics &g, Rectangle<float> r, bool on, bool down, bool grey)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    auto b = Rectangle<float>(d, d).withCentre(r.getCentre());
    g.setColour(Colours::black.withAlpha(0.4f));
    g.fillEllipse(b.translated(0.5f, down ? 0.6f : 1.6f));
    g.setColour(Colour(0xff0c0c0d));
    g.fillEllipse(b);
    auto cap = b.reduced(d * 0.1f).translated(0.0f, down ? 0.6f : 0.0f);
    Colour c0 = grey ? Colour(0xffb5b8be) : Colour(0xff2b2b2e), c1 = grey ? Colour(0xff70737a) : Colour(0xff0f0f10);
    g.setGradientFill(ColourGradient(c0.brighter(0.3f), cap.getX(), cap.getY(), c1, cap.getRight(), cap.getBottom(), false));
    g.fillEllipse(cap);
    g.setColour(Colours::white.withAlpha(grey ? 0.4f : 0.12f));
    g.drawEllipse(cap.reduced(0.6f), 0.8f);
    if (on) {                            /* lit: a red ring in the cap */
        g.setColour(RED_LED.withAlpha(0.3f));
        g.fillEllipse(cap.expanded(d * 0.12f));
        g.setColour(RED_LED);
        g.drawEllipse(cap.reduced(d * 0.06f), jmax(1.2f, d * 0.08f));
    }
}

void drawDroneKey(Graphics &g, Rectangle<float> r, bool on, bool down)
{
    auto b = r.reduced(2.0f);
    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(b.translated(0.8f, down ? 0.8f : 2.2f), 4.0f);
    auto k = down ? b.translated(0.0f, 1.0f) : b;
    g.setColour(Colour(0xff080808));
    g.fillRoundedRectangle(k, 4.0f);
    auto face = k.reduced(3.0f).withTrimmedBottom(3.0f);
    g.setGradientFill(ColourGradient(Colour(0xff3a3a3d), face.getX(), face.getY(), Colour(0xff161617), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle(face, 3.0f);
    g.setColour(Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(face.reduced(0.5f), 3.0f, 0.8f);
    /* its window, lit while the voice is open */
    auto win = Rectangle<float>(face.getWidth() * 0.16f, face.getHeight() * 0.2f).withCentre({face.getCentreX(), face.getY() + face.getHeight() * 0.2f});
    if (on) {
        g.setColour(AMBER_LED.withAlpha(0.35f));
        g.fillRoundedRectangle(win.expanded(4.0f), 3.0f);
    }
    g.setColour(on ? AMBER_LED.brighter(0.3f) : Colour(0xffd8d8d8));
    g.fillRoundedRectangle(win, 1.5f);
}

void drawLed(Graphics &g, Point<float> c, float r, float level, Colour col)
{
    level = jlimit(0.0f, 1.0f, level);
    if (level > 0.02f) {
        g.setGradientFill(ColourGradient(col.withAlpha(0.5f * level), c.x, c.y, col.withAlpha(0.0f), c.x + r * 3.2f, c.y, true));
        g.fillEllipse(Rectangle<float>(r * 6.4f, r * 6.4f).withCentre(c));
    }
    g.setColour(Colour(0xff101010));
    g.fillEllipse(Rectangle<float>(r * 2.5f, r * 2.5f).withCentre(c));
    g.setColour(col.darker(2.4f).interpolatedWith(col, level));
    g.fillEllipse(Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.18f + 0.5f * level));
    g.fillEllipse(Rectangle<float>(r * 0.7f, r * 0.7f).withCentre(c.translated(-r * 0.3f, -r * 0.3f)));
}

/* the light sensor: a milky white disc, warmer with what it sees */
void drawDome(Graphics &g, Rectangle<float> r, float level)
{
    level = jlimit(0.0f, 1.0f, level);
    const float d = jmin(r.getWidth(), r.getHeight());
    auto c = Rectangle<float>(d, d).withCentre(r.getCentre());
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillEllipse(c.translated(0.6f, d * 0.025f));
    Colour base = Colour(0xffe9e9e6).interpolatedWith(Colour(0xfffff0d0), level);
    g.setGradientFill(ColourGradient(Colours::white, c.getX() + d * 0.35f, c.getY() + d * 0.3f, base.darker(0.12f - 0.08f * level),
                                     c.getRight(), c.getBottom(), true));
    g.fillEllipse(c);
    if (level > 0.05f) {
        g.setGradientFill(ColourGradient(AMBER_LED.withAlpha(0.3f * level), c.getCentreX(), c.getCentreY(), AMBER_LED.withAlpha(0.0f),
                                         c.getCentreX() + d * 0.5f, c.getCentreY(), true));
        g.fillEllipse(c);
    }
    g.setColour(Colours::black.withAlpha(0.22f));
    g.drawEllipse(c.reduced(0.5f), 1.0f);
}

void drawLcd(Graphics &g, Rectangle<float> r)
{
    g.setColour(Colour(0xff0b0b0c));
    g.fillRoundedRectangle(r, 3.0f);
    auto s = r.reduced(2.0f);
    g.setGradientFill(ColourGradient(Colour(0xff3c50dc), s.getX(), s.getY(), Colour(0xff2635a6), s.getX(), s.getBottom(), false));
    g.fillRoundedRectangle(s, 2.0f);
    g.setColour(Colours::white.withAlpha(0.07f));
    g.fillRect(s.withHeight(s.getHeight() * 0.42f));
}

void drawLegend(Graphics &g, const String &s, Rectangle<float> r, float size, Colour c)
{
    g.setColour(c.isTransparent() ? ink() : c);
    g.setFont(font(Face::label, size));
    g.drawFittedText(s, r.toNearestInt(), Justification::centred, 2, 0.8f);
}

void drawTag(Graphics &g, Rectangle<float> r)
{
    static Image src = ImageCache::getFromMemory(BinaryData::swtdr34mztag_png, BinaryData::swtdr34mztag_pngSize);
    if (!src.isValid())
        return;
    Image tag(Image::ARGB, src.getWidth(), src.getHeight(), true);   /* (in the print's colour, its own alpha) */
    const Colour k = ink();
    for (int y = 0; y < src.getHeight(); y++)
        for (int x = 0; x < src.getWidth(); x++) {
            auto p = src.getPixelAt(x, y);
            float a = p.getFloatAlpha() * (p.getBrightness() > 0.5f ? 1.0f : p.getBrightness() * 2.0f);
            tag.setPixelAt(x, y, k.withAlpha(a));
        }
    auto ri = r.getSmallestIntegerContainer();
    g.drawImageWithin(tag, ri.getX(), ri.getY(), ri.getWidth(), ri.getHeight(), RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize);
}

/* SOLARIS in heavy print, -23 in red */
void drawLogo(Graphics &g, Rectangle<float> r)
{
    auto f = font(Face::logo, r.getHeight()).withExtraKerningFactor(0.01f);
    g.setFont(f);
    String a = "SOLARIS", b = "-23";
    GlyphArrangement ga;
    ga.addLineOfText(f, a, 0, 0);
    float wa = ga.getBoundingBox(0, -1, true).getWidth();
    g.setColour(ink());
    g.drawText(a, r, Justification::centredLeft, false);
    g.setColour(finish() == F_BLUE ? Colour(0xffff5a4a) : Colour(0xffe8150c));
    g.drawText(b, r.withTrimmedLeft(wa + r.getHeight() * 0.06f), Justification::centredLeft, false);
}
} // namespace s23
