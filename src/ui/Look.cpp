/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
#include "Look.h"
#include "BinaryData.h"

namespace s23
{
using namespace juce;

/* the scheme: SONNENFINSTERNIS (a night-blue panel; amber drones, red chaos, teal VCOs, violet modulation, a
 * warm grey for the filters and effects) */
const Theme &theme()
{
    static const Theme t = [] {
        Theme x;
        x.panel = Colour(0xff1b1e2b);
        x.panelEdge = Colour(0xff11131c);
        x.legend = Colour(0xfff3efe4);
        x.dim = Colour(0xffa7a9b8);
        x.knobBody = Colour(0xffefe8d6);
        x.knobCap = Colour(0xfffbf7ee);
        x.led = Colour(0xffff5a3c);
        x.ledOff = Colour(0xff3a2422);
        x.plate = Colour(0xff2a2e40);
        x.plateLit = Colour(0xfff2c14e);
        x.sec[(int)Sec::drone] = Colour(0xffe8a33d);
        x.sec[(int)Sec::chaos] = Colour(0xffd2553d);
        x.sec[(int)Sec::vco] = Colour(0xff3f8f9b);
        x.sec[(int)Sec::mod] = Colour(0xff7a6bc4);
        x.sec[(int)Sec::fx] = Colour(0xffc9c3b3);
        x.sec[(int)Sec::mix] = Colour(0xff9aa0b8);
        x.sec[(int)Sec::keys] = Colour(0xfff2c14e);
        x.secText[(int)Sec::drone] = Colour(0xff3a2405);
        x.secText[(int)Sec::chaos] = Colour(0xfffff3ee);
        x.secText[(int)Sec::vco] = Colour(0xffeafcff);
        x.secText[(int)Sec::mod] = Colour(0xfff1efff);
        x.secText[(int)Sec::fx] = Colour(0xff2c2a24);
        x.secText[(int)Sec::mix] = Colour(0xff1b1e2b);
        x.secText[(int)Sec::keys] = Colour(0xff3a2405);
        return x;
    }();
    return t;
}

Colour secColour(Sec s) { return theme().sec[(int)s]; }

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
    setColour(ResizableWindow::backgroundColourId, theme().panel);
    setColour(PopupMenu::backgroundColourId, Colour(0xff262a3a));
    setColour(PopupMenu::textColourId, theme().legend);
    setColour(PopupMenu::highlightedBackgroundColourId, secColour(Sec::drone).darker(0.3f));
    setColour(PopupMenu::highlightedTextColourId, Colours::black);
    setColour(PopupMenu::headerTextColourId, theme().dim);
    setColour(TextEditor::backgroundColourId, Colour(0xff11131c));
    setColour(TextEditor::textColourId, theme().legend);
    setColour(TextEditor::outlineColourId, theme().dim);
    setColour(AlertWindow::backgroundColourId, Colour(0xff262a3a));
    setColour(AlertWindow::textColourId, theme().legend);
    setColour(TextButton::buttonColourId, Colour(0xff3a3f55));
    setColour(TextButton::textColourOffId, theme().legend);
}

Typeface::Ptr Look::getTypefaceForFont(const Font &) { return face(Face::label); }

void Look::drawPopupMenuBackground(Graphics &g, int w, int h)
{
    g.fillAll(findColour(PopupMenu::backgroundColourId));
    g.setColour(theme().dim.withAlpha(0.5f));
    g.drawRect(0, 0, w, h, 1);
}

void drawPanel(Graphics &g, Rectangle<float> r)
{
    const Theme &t = theme();
    g.setGradientFill(ColourGradient(t.panel.brighter(0.08f), r.getX(), r.getY(), t.panel.darker(0.25f), r.getX(), r.getBottom(), false));
    g.fillRect(r);
    /* a faint field of stars */
    Random rnd(23);
    for (int i = 0; i < (int)(r.getWidth() * r.getHeight() / 2600.0f); i++) {
        float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
        g.setColour(Colours::white.withAlpha(0.03f + 0.07f * rnd.nextFloat()));
        float s = 0.6f + rnd.nextFloat() * 1.0f;
        g.fillEllipse(x, y, s, s);
    }
}

void drawSection(Graphics &g, Rectangle<float> r, const String &title, Sec s)
{
    const Colour c = secColour(s);
    const float rad = 9.0f;
    /* the plate: the colour, darkened, so the knobs and keys stand out; a lit edge */
    g.setColour(Colours::black.withAlpha(0.35f));
    g.fillRoundedRectangle(r.translated(0.0f, 2.0f), rad);
    g.setGradientFill(ColourGradient(c.darker(0.62f).withMultipliedSaturation(0.85f), r.getX(), r.getY(),
                                     c.darker(0.85f).withMultipliedSaturation(0.8f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, rad);
    g.setColour(c.withAlpha(0.55f));
    g.drawRoundedRectangle(r.reduced(0.5f), rad, 1.2f);
    /* the name's bar */
    if (title.isNotEmpty()) {
        auto bar = r.withHeight(22.0f);
        Path p;
        p.addRoundedRectangle(bar.getX(), bar.getY(), bar.getWidth(), bar.getHeight(), rad, rad, true, true, false, false);
        g.setColour(c);
        g.fillPath(p);
        g.setColour(theme().secText[(int)s]);
        g.setFont(font(Face::title, 13.5f));
        g.drawText(title, bar.reduced(10.0f, 0.0f), Justification::centredLeft, false);
    }
}

void drawKnob(Graphics &g, Rectangle<float> r, float pos, Colour accent, bool bipolar, bool stepped)
{
    const Theme &t = theme();
    const float d = jmin(r.getWidth(), r.getHeight()), R = d * 0.5f;
    const Point<float> c = r.getCentre();
    const float a0 = -2.36f, a1 = 2.36f, ang = a0 + pos * (a1 - a0);
    /* the arc of its range, and of its value (from the middle on a bipolar knob) */
    {
        Path track;
        track.addCentredArc(c.x, c.y, R * 0.92f, R * 0.92f, 0.0f, a0, a1, true);
        g.setColour(Colours::black.withAlpha(0.4f));
        g.strokePath(track, PathStrokeType(jmax(2.0f, d * 0.07f), PathStrokeType::curved, PathStrokeType::rounded));
        Path val;
        float from = bipolar ? 0.0f : a0;
        val.addCentredArc(c.x, c.y, R * 0.92f, R * 0.92f, 0.0f, jmin(from, ang), jmax(from, ang), true);
        g.setColour(accent);
        g.strokePath(val, PathStrokeType(jmax(2.0f, d * 0.07f), PathStrokeType::curved, PathStrokeType::rounded));
    }
    (void)stepped;
    /* the body: a cream cylinder with a shadow, a skirt in the section's colour */
    const float rb = R * 0.74f;
    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillEllipse(Rectangle<float>(rb * 2.1f, rb * 2.1f).withCentre(c.translated(0.0f, rb * 0.12f)));
    g.setColour(accent.darker(0.35f));
    g.fillEllipse(Rectangle<float>(rb * 2.0f, rb * 2.0f).withCentre(c));
    auto cap = Rectangle<float>(rb * 1.62f, rb * 1.62f).withCentre(c);
    g.setGradientFill(ColourGradient(t.knobCap, cap.getX(), cap.getY(), t.knobBody.darker(0.18f), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse(cap);
    g.setColour(Colours::black.withAlpha(0.18f));
    g.drawEllipse(cap, 1.0f);
    /* the line */
    g.setColour(Colour(0xff1b1e2b));
    g.drawLine(Line<float>(c.getPointOnCircumference(rb * 0.2f, ang), c.getPointOnCircumference(rb * 0.8f, ang)), jmax(1.6f, d * 0.06f));
}

void drawKey(Graphics &g, Rectangle<float> r, bool on, bool down, Colour accent, bool big)
{
    const Theme &t = theme();
    auto b = r.reduced(1.0f);
    g.setColour(Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(b.translated(0.0f, down ? 0.5f : 1.8f), 4.0f);
    auto face = down ? b.translated(0.0f, 1.0f) : b;
    g.setGradientFill(ColourGradient(Colour(0xff3c4158), face.getX(), face.getY(), Colour(0xff222536), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle(face, 4.0f);
    g.setColour(Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(face.reduced(0.5f), 4.0f, 1.0f);
    /* its LED: in the corner of a big key, in the middle of a small one */
    float lr = big ? jmin(face.getWidth(), face.getHeight()) * 0.1f : jmin(face.getWidth(), face.getHeight()) * 0.2f;
    Point<float> lc = big ? Point<float>(face.getX() + lr * 2.4f, face.getY() + lr * 2.4f) : face.getCentre();
    drawLed(g, lc, lr, on ? 1.0f : 0.0f, accent.brighter(0.2f));
    if (big && on) {
        g.setColour(accent.withAlpha(0.18f));
        g.fillRoundedRectangle(face.reduced(2.0f), 3.0f);
    }
    (void)t;
}

void drawLed(Graphics &g, Point<float> c, float r, float level, Colour col)
{
    level = jlimit(0.0f, 1.0f, level);
    if (level > 0.02f) {
        g.setGradientFill(ColourGradient(col.withAlpha(0.45f * level), c.x, c.y, col.withAlpha(0.0f), c.x + r * 3.0f, c.y, true));
        g.fillEllipse(Rectangle<float>(r * 6.0f, r * 6.0f).withCentre(c));
    }
    g.setColour(Colour(0xff15161c));
    g.fillEllipse(Rectangle<float>(r * 2.5f, r * 2.5f).withCentre(c));
    g.setColour(col.darker(2.6f).interpolatedWith(col, level));
    g.fillEllipse(Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.15f + 0.45f * level));
    g.fillEllipse(Rectangle<float>(r * 0.7f, r * 0.7f).withCentre(c.translated(-r * 0.3f, -r * 0.3f)));
}

/* the light sensor: a milky dome, lit from inside by what it sees */
void drawDome(Graphics &g, Rectangle<float> r, float level)
{
    level = jlimit(0.0f, 1.0f, level);
    const float d = jmin(r.getWidth(), r.getHeight());
    auto c = Rectangle<float>(d, d).withCentre(r.getCentre());
    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillEllipse(c.translated(0.0f, d * 0.04f));
    Colour warm = Colour(0xfff2c14e), cool = Colour(0xffd8dbe6);
    Colour in = cool.interpolatedWith(warm, level);
    g.setGradientFill(ColourGradient(in.brighter(0.2f + level * 0.4f), c.getCentreX() - d * 0.12f, c.getCentreY() - d * 0.15f,
                                     in.darker(0.9f - level * 0.5f), c.getRight(), c.getBottom(), true));
    g.fillEllipse(c);
    if (level > 0.05f) {
        g.setGradientFill(ColourGradient(warm.withAlpha(0.35f * level), c.getCentreX(), c.getCentreY(), warm.withAlpha(0.0f),
                                         c.getCentreX() + d * 0.9f, c.getCentreY(), true));
        g.fillEllipse(c.expanded(d * 0.35f));
    }
    g.setColour(Colours::white.withAlpha(0.5f));
    g.fillEllipse(Rectangle<float>(d * 0.22f, d * 0.12f).withCentre(c.getCentre().translated(-d * 0.16f, -d * 0.24f)));
    g.setColour(Colours::black.withAlpha(0.3f));
    g.drawEllipse(c, 1.0f);
}

void drawLegend(Graphics &g, const String &s, Rectangle<float> r, float size, Colour c)
{
    g.setColour(c.isTransparent() ? theme().legend : c);
    g.setFont(font(Face::label, size));
    g.drawFittedText(s, r.toNearestInt(), Justification::centred, 2, 0.8f);
}

void drawTag(Graphics &g, Rectangle<float> r)
{
    static Image tag = [] {
        Image im = ImageCache::getFromMemory(BinaryData::swtdr34mztag_png, BinaryData::swtdr34mztag_pngSize);
        if (!im.isValid())
            return im;
        Image out(Image::ARGB, im.getWidth(), im.getHeight(), true);
        for (int y = 0; y < im.getHeight(); y++)       /* (in the legends' colour, its own alpha) */
            for (int x = 0; x < im.getWidth(); x++) {
                auto p = im.getPixelAt(x, y);
                float a = p.getFloatAlpha() * (p.getBrightness() > 0.5f ? 1.0f : p.getBrightness() * 2.0f);
                out.setPixelAt(x, y, theme().legend.withAlpha(a));
            }
        return out;
    }();
    if (!tag.isValid())
        return;
    g.setOpacity(0.85f);
    g.drawImageWithin(tag, r.getSmallestIntegerContainer().getX(), r.getSmallestIntegerContainer().getY(),
                      r.getSmallestIntegerContainer().getWidth(), r.getSmallestIntegerContainer().getHeight(),
                      RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize);
    g.setOpacity(1.0f);
}

void drawLogo(Graphics &g, Rectangle<float> r)
{
    /* SOLARIS in the legends' white, -23 in the drones' amber, a small eclipse before it */
    float h = r.getHeight();
    auto sun = r.removeFromLeft(h).reduced(h * 0.12f);
    g.setColour(secColour(Sec::drone));
    g.fillEllipse(sun);
    g.setColour(theme().panel);
    g.fillEllipse(sun.reduced(sun.getWidth() * 0.08f).translated(sun.getWidth() * 0.16f, -sun.getWidth() * 0.06f));
    r.removeFromLeft(h * 0.25f);
    auto f = font(Face::logo, h * 0.92f).withExtraKerningFactor(0.04f);
    g.setFont(f);
    g.setColour(theme().legend);
    String a = "SOLARIS", b = "-23";
    GlyphArrangement ga;
    ga.addLineOfText(f, a, 0, 0);
    float wa = ga.getBoundingBox(0, -1, true).getWidth();
    g.drawText(a, r, Justification::centredLeft, false);
    g.setColour(secColour(Sec::drone));
    g.drawText(b, r.withTrimmedLeft(wa + h * 0.08f), Justification::centredLeft, false);
}
} // namespace s23
