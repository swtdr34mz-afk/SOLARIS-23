/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The relic: a black, rough powder-coated steel panel that has been on the road for decades. Bone-coloured print,
 * worn through in places; scratches, scuffs down to the grey primer and the bare metal at the edges, grime, rust
 * blooming at the screws; black bakelite and brushed aluminium knobs, bone caps gone yellow, oxide-red caps; dulled
 * chrome toggles; amber LCDs; brass touch plates. Every wear mark comes from a fixed seed: the same panel every time. */
#include "Look.h"
#include "BinaryData.h"
#include <atomic>

namespace s23
{
using namespace juce;

/* ------------------------------------------------------------------------------------- colours --- */
namespace
{
std::atomic<int> currentFinish{0};
const char *const FINISH_NAME[NFINISH] = {"RELIC", "RELIC", "RELIC", "RELIC", "RELIC"};
const Colour BONE{0xffe2d7bc}, RUST{0xffc8561e}, ALU{0xff9a9c98};
} // namespace

const char *finishName(int f) { return FINISH_NAME[jlimit(0, NFINISH - 1, f)]; }
void setFinish(int f) { currentFinish = jlimit(0, NFINISH - 1, f); }
int finish() { return currentFinish.load(); }
Colour panelColour() { return Colour(0xff191918); }
Colour ink() { return BONE; }
Colour lcdText() { return Colour(0xffffb23e); }

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
    setColour(ResizableWindow::backgroundColourId, Colour(0xff101010));
    setColour(PopupMenu::backgroundColourId, Colour(0xff1b1a18));
    setColour(PopupMenu::textColourId, BONE);
    setColour(PopupMenu::highlightedBackgroundColourId, RUST);
    setColour(PopupMenu::highlightedTextColourId, Colours::black);
    setColour(PopupMenu::headerTextColourId, Colour(0xff8f8a7e));
    setColour(TextEditor::backgroundColourId, Colour(0xff1a1206));
    setColour(TextEditor::textColourId, lcdText());
    setColour(TextEditor::outlineColourId, Colour(0xff4a4030));
    setColour(AlertWindow::backgroundColourId, Colour(0xff1b1a18));
    setColour(AlertWindow::textColourId, BONE);
    setColour(TextButton::buttonColourId, Colour(0xff2c2a26));
    setColour(TextButton::textColourOffId, BONE);
}

Typeface::Ptr Look::getTypefaceForFont(const Font &) { return face(Face::label); }

void Look::drawPopupMenuBackground(Graphics &g, int w, int h)
{
    g.fillAll(findColour(PopupMenu::backgroundColourId));
    g.setColour(Colour(0xff4a463e));
    g.drawRect(0, 0, w, h, 1);
}

/* -------------------------------------------------------------------------------------- panel --- */
/* the rough coat: a coarse grain, light and dark specks */
static const Image &grain()
{
    static Image tile = [] {
        Image im(Image::ARGB, 256, 256, true);
        Random r(4223);
        for (int y = 0; y < im.getHeight(); y++)
            for (int x = 0; x < im.getWidth(); x++) {
                float v = r.nextFloat();
                if (v < 0.34f)
                    im.setPixelAt(x, y, Colours::black.withAlpha(0.10f + 0.16f * r.nextFloat()));
                else if (v > 0.70f)
                    im.setPixelAt(x, y, Colour(0xffd8d0bc).withAlpha(0.02f + 0.05f * r.nextFloat()));
            }
        return im;
    }();
    return tile;
}

void drawPanel(Graphics &g, Rectangle<float> r)
{
    g.setColour(panelColour());
    g.fillRect(r);
    /* mottled: the coat thinner or thicker, a little brown, a little green in places */
    Random rnd(1923);
    for (int i = 0; i < (int)(r.getWidth() * r.getHeight() / 9000.0f); i++) {
        float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
        float s = 30.0f + rnd.nextFloat() * 140.0f;
        Colour c = rnd.nextFloat() < 0.5f ? Colour(0xff2c2820) : rnd.nextFloat() < 0.5f ? Colour(0xff0c0c0b) : Colour(0xff232620);
        g.setGradientFill(ColourGradient(c.withAlpha(0.22f), x, y, c.withAlpha(0.0f), x + s, y, true));
        g.fillEllipse(x - s, y - s, s * 2.0f, s * 2.0f);
    }
    g.setTiledImageFill(grain(), 0, 0, 1.0f);
    g.fillRect(r);
}

/* the wear, over everything printed: scuffs to the primer, scratches, the edges down to the metal, grime, the print
 * worn away where hands rest */
void drawWear(Graphics &g, Rectangle<float> r)
{
    Random rnd(4242);
    auto P = [&](float mx, float my) { return Point<float>(r.getX() + mx * r.getWidth(), r.getY() + my * r.getHeight()); };
    /* scuffs: the black rubbed thin, the grey primer coming through */
    for (int i = 0; i < 46; i++) {
        auto c = P(rnd.nextFloat(), rnd.nextFloat());
        float w = 20.0f + rnd.nextFloat() * 110.0f, h = w * (0.25f + rnd.nextFloat() * 0.5f);
        Graphics::ScopedSaveState s(g);
        g.addTransform(AffineTransform::rotation(rnd.nextFloat() * 3.14f, c.x, c.y));
        g.setGradientFill(ColourGradient(Colour(0xff6d6a62).withAlpha(0.10f + 0.16f * rnd.nextFloat()), c.x, c.y,
                                         Colour(0xff6d6a62).withAlpha(0.0f), c.x + w * 0.5f, c.y, true));
        g.fillEllipse(c.x - w * 0.5f, c.y - h * 0.5f, w, h);
    }
    /* grime: brown, in the corners and low down */
    for (int i = 0; i < 26; i++) {
        auto c = P(rnd.nextFloat(), 0.35f + 0.65f * rnd.nextFloat());
        float s = 30.0f + rnd.nextFloat() * 120.0f;
        g.setGradientFill(ColourGradient(Colour(0xff3a2a16).withAlpha(0.18f + 0.15f * rnd.nextFloat()), c.x, c.y, Colour(0xff3a2a16).withAlpha(0.0f),
                                         c.x + s, c.y, true));
        g.fillEllipse(c.x - s, c.y - s, s * 2.0f, s * 2.0f);
    }
    /* scratches: long and short, a light edge on a dark groove, some through to the metal */
    for (int i = 0; i < 520; i++) {
        auto a = P(rnd.nextFloat(), rnd.nextFloat());
        float len = rnd.nextFloat() < 0.85f ? 4.0f + rnd.nextFloat() * 40.0f : 40.0f + rnd.nextFloat() * 180.0f;
        float ang = (rnd.nextFloat() < 0.6f ? -0.35f + rnd.nextFloat() * 0.7f : rnd.nextFloat() * 3.14f);
        auto b = a + Point<float>(std::cos(ang), std::sin(ang)) * len;
        Path p;
        p.startNewSubPath(a);
        p.quadraticTo((a + b) * 0.5f + Point<float>(rnd.nextFloat() - 0.5f, rnd.nextFloat() - 0.5f) * len * 0.15f, b);
        bool deep = len < 26.0f && rnd.nextFloat() < 0.22f;   /* (only short ones go through to the metal) */
        g.setColour(Colours::black.withAlpha(0.35f));
        g.strokePath(p, PathStrokeType(deep ? 1.6f : 1.0f), AffineTransform::translation(0.6f, 0.6f));
        g.setColour((deep ? ALU : Colour(0xffb9b2a2)).withAlpha(deep ? 0.5f : len > 60.0f ? 0.04f + 0.06f * rnd.nextFloat() : 0.06f + 0.14f * rnd.nextFloat()));
        g.strokePath(p, PathStrokeType(deep ? 0.9f : 0.6f));
    }
    /* chips along the edges: the paint gone, bare steel */
    for (int i = 0; i < 90; i++) {
        float t = rnd.nextFloat();
        t = t < 0.5f ? t * t * 2.0f : 1.0f - (1.0f - t) * (1.0f - t) * 2.0f;   /* (more towards the corners) */
        int side = rnd.nextInt(4);
        Point<float> c = side == 0 ? P(t, 0.0f) : side == 1 ? P(t, 1.0f) : side == 2 ? P(0.0f, t) : P(1.0f, t);
        c += Point<float>(side == 2 ? 1.0f : side == 3 ? -1.0f : 0.0f, side == 0 ? 1.0f : side == 1 ? -1.0f : 0.0f) * rnd.nextFloat() * rnd.nextFloat() * 8.0f;
        Path chip;
        int n = 5 + rnd.nextInt(4);
        float s = 1.0f + rnd.nextFloat() * rnd.nextFloat() * (rnd.nextFloat() < 0.2f ? 14.0f : 4.0f);
        for (int k = 0; k < n; k++) {
            float a = (float)k / (float)n * 6.283f;
            auto q = c + Point<float>(std::cos(a), std::sin(a)) * s * (0.5f + rnd.nextFloat() * 0.6f);
            if (k)
                chip.lineTo(q);
            else
                chip.startNewSubPath(q);
        }
        chip.closeSubPath();
        g.setColour(Colour(0xff4b4a46));
        g.fillPath(chip, AffineTransform::translation(0.0f, 0.0f).scaled(1.15f, 1.15f, c.x, c.y));
        g.setGradientFill(ColourGradient(Colour(0xffc3c4c0), c.x - s, c.y - s, Colour(0xff6f706c), c.x + s, c.y + s, false));
        g.fillPath(chip);
    }
    /* the print worn: specks of the coat over it */
    for (int i = 0; i < 9000; i++) {
        auto c = P(rnd.nextFloat(), rnd.nextFloat());
        g.setColour(panelColour().withAlpha(0.5f + 0.5f * rnd.nextFloat()));
        float s = 0.6f + rnd.nextFloat() * 1.6f;
        g.fillEllipse(c.x, c.y, s, s * (0.5f + rnd.nextFloat()));
    }
    g.setColour(Colours::black.withAlpha(0.6f));
    g.drawRect(r, 2.0f);
}

void drawScrew(Graphics &g, Point<float> c, float rr)
{
    /* rust bloom around it */
    Random rnd((int64)(c.x * 13.0f + c.y * 7.0f));
    g.setGradientFill(ColourGradient(Colour(0xff7a3a12).withAlpha(0.55f), c.x, c.y, Colour(0xff7a3a12).withAlpha(0.0f), c.x + rr * 3.6f, c.y, true));
    g.fillEllipse(Rectangle<float>(rr * 7.2f, rr * 7.2f).withCentre(c));
    for (int i = 0; i < 26; i++) {
        float a = rnd.nextFloat() * 6.283f, d = rr * (1.0f + rnd.nextFloat() * 2.6f);
        g.setColour(Colour(0xff9a4a18).withAlpha(0.3f + 0.4f * rnd.nextFloat()));
        g.fillEllipse(Rectangle<float>(1.2f + rnd.nextFloat() * 1.8f, 1.2f + rnd.nextFloat() * 1.8f).withCentre(c + Point<float>(std::cos(a), std::sin(a)) * d));
    }
    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillEllipse(Rectangle<float>(rr * 2.3f, rr * 2.3f).withCentre(c.translated(0.4f, 0.8f)));
    g.setGradientFill(ColourGradient(Colour(0xffb9b4a8), c.x - rr, c.y - rr, Colour(0xff4d4a44), c.x + rr, c.y + rr, false));
    g.fillEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c));
    g.setColour(Colour(0xff2c2a26));
    Path p;                              /* a slot, chewed */
    p.addRectangle(-rr * 0.7f, -rr * 0.13f, rr * 1.4f, rr * 0.26f);
    g.fillPath(p, AffineTransform::rotation(0.4f + rnd.nextFloat()).translated(c));
}

void drawSection(Graphics &g, Rectangle<float> r, const String &title, Sec s)
{
    if (s == Sec::field) {               /* the touch field: a plate of black-oxide steel, recessed */
        g.setColour(Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(r.translated(0.0f, -1.0f), 5.0f);
        g.setGradientFill(ColourGradient(Colour(0xff0f0f0f), r.getX(), r.getY(), Colour(0xff1c1b19), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r.reduced(1.5f), 5.0f);
    } else {
        g.setColour(ink().withAlpha(0.85f));
        g.drawRoundedRectangle(r.reduced(0.8f), 4.0f, 1.3f);
    }
    if (title.isEmpty())
        return;
    /* the name: a bone tab, black letters, in the frame's corner */
    auto f = font(Face::title, 12.0f).withExtraKerningFactor(0.10f);
    GlyphArrangement ga;
    ga.addLineOfText(f, title, 0, 0);
    float w = ga.getBoundingBox(0, -1, true).getWidth() + 20.0f;
    auto tab = Rectangle<float>(r.getX(), r.getY(), jmin(w, r.getWidth()), 18.0f);
    Path p;
    p.addRoundedRectangle(tab.getX(), tab.getY(), tab.getWidth(), tab.getHeight(), 4.0f, 4.0f, true, false, false, true);
    g.setColour(ink());
    g.fillPath(p);
    g.setColour(Colour(0xff151515));
    g.setFont(f);
    g.drawText(title, tab.withTrimmedLeft(10.0f), Justification::centredLeft, false);
}

/* --------------------------------------------------------------------------------------- knobs --- */
namespace
{
struct Cap {
    Colour skirt, top, hi, line;
    bool metal;
};
Cap capOf(KS s)
{
    switch (s) {
    case KS::black: return {Colour(0xff121212), Colour(0xff1c1c1c), Colour(0xff4a4946), BONE, false};
    case KS::slate: return {Colour(0xff151515), Colour(0xff242422), Colour(0xff545250), BONE, false};
    case KS::light: return {Colour(0xff3a342a), Colour(0xffcfc2a0), Colour(0xfff2e8cc), Colour(0xff1a1a1a), false};
    case KS::blue: return {Colour(0xff1e1e1e), Colour(0xff9c9e9a), Colour(0xffefefea), Colour(0xff151515), true};
    case KS::red: return {Colour(0xff2a100c), Colour(0xff7e2a1c), Colour(0xffbf5a3e), BONE, false};
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
    Random rnd((int64)(c.x * 97.0f + c.y * 31.0f));

    if (ticks) {                         /* the scale, printed, worn where the fingers go */
        for (int i = 0; i <= 10; i++) {
            float a = a0 + (a1 - a0) * (float)i / 10.0f;
            float len = (i == 0 || i == 5 || i == 10) ? 0.24f : 0.15f;
            g.setColour(ink().withAlpha(0.55f + 0.45f * rnd.nextFloat()));
            g.drawLine(Line<float>(c.getPointOnCircumference(d * 0.5f * (1.0f - len), a), c.getPointOnCircumference(d * 0.5f, a)),
                       jmax(1.0f, d * 0.026f));
        }
        if (bipolar) {
            g.setColour(ink());
            g.fillEllipse(Rectangle<float>(d * 0.07f, d * 0.07f).withCentre(c.getPointOnCircumference(d * 0.5f + d * 0.06f, 0.0f)));
        }
    }
    /* the ring worn shiny around it by decades of fingers */
    g.setGradientFill(ColourGradient(Colour(0xff5a5852).withAlpha(0.0f), c.x, c.y, Colour(0xff5a5852).withAlpha(0.16f), c.x + R * 1.25f, c.y, true));
    g.fillEllipse(Rectangle<float>(R * 2.7f, R * 2.7f).withCentre(c));
    /* the shadow */
    g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.6f), c.x + R * 0.12f, c.y + R * 0.22f, Colours::transparentBlack,
                                     c.x + R * 0.12f, c.y + R * 1.32f, true));
    g.fillEllipse(Rectangle<float>(R * 2.5f, R * 2.5f).withCentre(c.translated(R * 0.12f, R * 0.22f)));
    /* the skirt, knurled */
    auto skirt = Rectangle<float>(R * 2.0f, R * 2.0f).withCentre(c);
    g.setGradientFill(ColourGradient(cap.skirt.brighter(0.35f), skirt.getX(), skirt.getY(), cap.skirt.darker(0.4f), skirt.getRight(),
                                     skirt.getBottom(), false));
    g.fillEllipse(skirt);
    const int ribs = jlimit(18, 48, (int)(R * 1.4f));
    for (int i = 0; i < ribs; i++) {
        float a = MathConstants<float>::twoPi * (float)i / (float)ribs;
        float lit = 0.5f + 0.5f * std::cos(a + 2.3f);
        g.setColour((lit > 0.5f ? Colours::white : Colours::black).withAlpha(0.05f + 0.12f * std::fabs(lit - 0.5f) * 2.0f));
        g.drawLine(Line<float>(c.getPointOnCircumference(R * 0.80f, a), c.getPointOnCircumference(R * 0.99f, a)), jmax(0.7f, R * 0.045f));
    }
    /* the cap */
    auto top = Rectangle<float>(R * 1.56f, R * 1.56f).withCentre(c.translated(0.0f, -R * 0.03f));
    g.setGradientFill(ColourGradient(cap.hi, top.getX() + top.getWidth() * 0.3f, top.getY() + top.getHeight() * 0.22f, cap.top.darker(0.3f),
                                     top.getRight(), top.getBottom(), true));
    g.fillEllipse(top);
    if (cap.metal) {                     /* brushed aluminium: fine rings */
        for (float k = 0.1f; k < 0.78f; k += 0.035f) {
            g.setColour((rnd.nextBool() ? Colours::white : Colours::black).withAlpha(0.05f + 0.06f * rnd.nextFloat()));
            g.drawEllipse(Rectangle<float>(R * 2.0f * k, R * 2.0f * k).withCentre(top.getCentre()), 0.7f);
        }
    }
    /* scratches and dirt on the cap */
    for (int i = 0; i < 3 + (int)(R * 0.12f); i++) {
        float a = rnd.nextFloat() * 6.283f, rr = rnd.nextFloat() * R * 0.6f, len = R * (0.1f + 0.4f * rnd.nextFloat());
        auto p0 = top.getCentre() + Point<float>(std::cos(a), std::sin(a)) * rr;
        float b = rnd.nextFloat() * 6.283f;
        auto p1 = p0 + Point<float>(std::cos(b), std::sin(b)) * len;
        if (top.reduced(R * 0.05f).contains(p1)) {
            g.setColour((cap.metal || style == KS::light ? Colours::black : Colours::white).withAlpha(0.08f + 0.12f * rnd.nextFloat()));
            g.drawLine(Line<float>(p0, p1), 0.6f);
        }
    }
    g.setColour(Colours::black.withAlpha(0.35f));
    g.drawEllipse(top, jmax(0.6f, R * 0.03f));
    /* the line, a little worn */
    g.setColour(cap.line.withAlpha(0.88f));
    g.drawLine(Line<float>(c.getPointOnCircumference(R * 0.18f, ang), c.getPointOnCircumference(R * 0.74f, ang)), jmax(1.4f, R * 0.11f));
}

/* ------------------------------------------------------------------------------------ switches --- */
void drawToggle(Graphics &g, Rectangle<float> r, bool up)
{
    const Point<float> c = r.getCentre();
    const float s = jmin(r.getWidth() * 0.9f, r.getHeight() * 0.5f);
    g.setColour(Colours::black.withAlpha(0.45f));
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
    g.setGradientFill(ColourGradient(Colour(0xffb8b4aa), c.x - s * 0.4f, c.y - s * 0.4f, Colour(0xff4a4740), c.x + s * 0.4f, c.y + s * 0.4f, false));
    g.fillPath(hex);
    g.setColour(Colour(0xff2e2c28));
    g.strokePath(hex, PathStrokeType(0.7f));
    g.setColour(Colour(0xff1d1c1a));
    g.fillEllipse(Rectangle<float>(s * 0.42f, s * 0.42f).withCentre(c));
    const float dir = up ? -1.0f : 1.0f, len = r.getHeight() * 0.40f;
    Point<float> tip(c.x, c.y + dir * len);
    Path bat;
    bat.startNewSubPath(c.x - s * 0.13f, c.y);
    bat.lineTo(tip.x - s * 0.09f, tip.y);
    bat.lineTo(tip.x + s * 0.09f, tip.y);
    bat.lineTo(c.x + s * 0.13f, c.y);
    bat.closeSubPath();
    g.setColour(Colours::black.withAlpha(0.4f));
    g.fillPath(bat, AffineTransform::translation(1.2f, 1.6f));
    g.setGradientFill(ColourGradient(Colour(0xff5c5a54), c.x - s * 0.15f, c.y, Colour(0xffd8d4ca), c.x + s * 0.02f, c.y, false));
    g.fillPath(bat);
    auto knob = Rectangle<float>(s * 0.34f, s * 0.34f).withCentre(tip);
    g.setGradientFill(ColourGradient(Colour(0xffe2ded4), knob.getX(), knob.getY(), Colour(0xff57544e), knob.getRight(), knob.getBottom(), false));
    g.fillEllipse(knob);
    g.setColour(Colour(0xff34322e));
    g.drawEllipse(knob, 0.6f);
}

void drawButton(Graphics &g, Rectangle<float> r, bool on, bool down, bool grey)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    auto b = Rectangle<float>(d, d).withCentre(r.getCentre());
    g.setColour(Colours::black.withAlpha(0.5f));
    g.fillEllipse(b.translated(0.5f, down ? 0.6f : 1.6f));
    g.setGradientFill(ColourGradient(Colour(0xff8c8880), b.getX(), b.getY(), Colour(0xff2a2824), b.getRight(), b.getBottom(), false));
    g.fillEllipse(b);                    /* (a worn metal bezel) */
    auto cap = b.reduced(d * 0.12f).translated(0.0f, down ? 0.6f : 0.0f);
    Colour c0 = grey ? Colour(0xffc9bda0) : Colour(0xff2a2927), c1 = grey ? Colour(0xff6e6450) : Colour(0xff0d0d0c);
    g.setGradientFill(ColourGradient(c0.brighter(0.25f), cap.getX(), cap.getY(), c1, cap.getRight(), cap.getBottom(), false));
    g.fillEllipse(cap);
    if (on) {                            /* lit: a ring of amber light around the cap */
        g.setColour(AMBER_LED.withAlpha(0.35f));
        g.fillEllipse(cap.expanded(d * 0.14f));
        g.setColour(AMBER_LED);
        g.drawEllipse(cap.reduced(d * 0.05f), jmax(1.2f, d * 0.08f));
    }
}

void drawDroneKey(Graphics &g, Rectangle<float> r, bool on, bool down)
{
    auto b = r.reduced(2.0f);
    g.setColour(Colours::black.withAlpha(0.55f));
    g.fillRoundedRectangle(b.translated(0.8f, down ? 0.8f : 2.2f), 4.0f);
    auto k = down ? b.translated(0.0f, 1.0f) : b;
    g.setColour(Colour(0xff070707));
    g.fillRoundedRectangle(k, 4.0f);
    auto face = k.reduced(3.0f).withTrimmedBottom(3.0f);
    g.setGradientFill(ColourGradient(Colour(0xff34322e), face.getX(), face.getY(), Colour(0xff121110), face.getX(), face.getBottom(), false));
    g.fillRoundedRectangle(face, 3.0f);
    /* worn shiny in the middle, where it is pressed */
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.10f), face.getCentreX(), face.getCentreY() + 4.0f, Colours::transparentWhite,
                                     face.getCentreX() + face.getWidth() * 0.4f, face.getCentreY(), true));
    g.fillRoundedRectangle(face, 3.0f);
    auto win = Rectangle<float>(face.getWidth() * 0.22f, face.getHeight() * 0.16f).withCentre({face.getCentreX(), face.getY() + face.getHeight() * 0.22f});
    if (on) {
        g.setColour(AMBER_LED.withAlpha(0.4f));
        g.fillRoundedRectangle(win.expanded(4.0f), 3.0f);
    }
    g.setColour(on ? AMBER_LED.brighter(0.3f) : Colour(0xff3a2a14));
    g.fillRoundedRectangle(win, 1.5f);
}

void drawLed(Graphics &g, Point<float> c, float r, float level, Colour col)
{
    level = jlimit(0.0f, 1.0f, level);
    if (level > 0.02f) {
        g.setGradientFill(ColourGradient(col.withAlpha(0.55f * level), c.x, c.y, col.withAlpha(0.0f), c.x + r * 3.2f, c.y, true));
        g.fillEllipse(Rectangle<float>(r * 6.4f, r * 6.4f).withCentre(c));
    }
    g.setGradientFill(ColourGradient(Colour(0xff9a968c), c.x - r, c.y - r, Colour(0xff2a2824), c.x + r, c.y + r, false));
    g.fillEllipse(Rectangle<float>(r * 2.6f, r * 2.6f).withCentre(c));
    g.setColour(col.darker(2.6f).interpolatedWith(col, level));
    g.fillEllipse(Rectangle<float>(r * 2.0f, r * 2.0f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.15f + 0.5f * level));
    g.fillEllipse(Rectangle<float>(r * 0.7f, r * 0.7f).withCentre(c.translated(-r * 0.3f, -r * 0.3f)));
}

/* the light sensor: an old milky glass dome, gone yellow, scratched, glowing warm with what it sees */
void drawDome(Graphics &g, Rectangle<float> r, float level)
{
    level = jlimit(0.0f, 1.0f, level);
    const float d = jmin(r.getWidth(), r.getHeight());
    auto c = Rectangle<float>(d, d).withCentre(r.getCentre()).reduced(d * 0.07f);   /* (room for the bezel and its shadow) */
    /* its bezel */
    g.setColour(Colours::black.withAlpha(0.5f));
    g.fillEllipse(c.expanded(d * 0.05f).translated(0.6f, d * 0.03f));
    g.setGradientFill(ColourGradient(Colour(0xff8f8b82), c.getX(), c.getY(), Colour(0xff2a2825), c.getRight(), c.getBottom(), false));
    g.fillEllipse(c.expanded(d * 0.04f));
    Colour base = Colour(0xffb8af96).interpolatedWith(Colour(0xffffd78a), level);
    g.setGradientFill(ColourGradient(base.brighter(0.25f), c.getX() + d * 0.35f, c.getY() + d * 0.3f, base.darker(0.55f - 0.3f * level),
                                     c.getRight(), c.getBottom(), true));
    g.fillEllipse(c);
    if (level > 0.05f) {
        g.setGradientFill(ColourGradient(AMBER_LED.withAlpha(0.35f * level), c.getCentreX(), c.getCentreY(), AMBER_LED.withAlpha(0.0f),
                                         c.getCentreX() + d * 0.5f, c.getCentreY(), true));
        g.fillEllipse(c);
    }
    Random rnd((int64)(c.getX() * 11.0f + c.getY()));
    for (int i = 0; i < 14; i++) {
        auto p0 = c.getCentre() + Point<float>(rnd.nextFloat() - 0.5f, rnd.nextFloat() - 0.5f) * d * 0.7f;
        auto p1 = p0 + Point<float>(rnd.nextFloat() - 0.5f, rnd.nextFloat() - 0.5f) * d * 0.3f;
        g.setColour(Colours::white.withAlpha(0.08f + 0.12f * rnd.nextFloat()));
        g.drawLine(Line<float>(p0, p1), 0.6f);
    }
    g.setColour(Colours::white.withAlpha(0.35f));
    g.fillEllipse(Rectangle<float>(d * 0.2f, d * 0.1f).withCentre(c.getCentre().translated(-d * 0.16f, -d * 0.24f)));
}

/* an amber LCD: dark glass, the backlight warm */
void drawLcd(Graphics &g, Rectangle<float> r)
{
    g.setGradientFill(ColourGradient(Colour(0xff6e6a60), r.getX(), r.getY(), Colour(0xff1e1d1a), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 3.0f);
    auto s = r.reduced(2.0f);
    g.setGradientFill(ColourGradient(Colour(0xff2a1a06), s.getX(), s.getY(), Colour(0xff140c03), s.getX(), s.getBottom(), false));
    g.fillRoundedRectangle(s, 2.0f);
    g.setColour(Colour(0xffff9a20).withAlpha(0.06f));
    g.fillRoundedRectangle(s, 2.0f);
    g.setColour(Colours::white.withAlpha(0.05f));
    g.fillRect(s.withHeight(s.getHeight() * 0.4f));
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
    static Image tag = [] {
        Image t(Image::ARGB, src.getWidth(), src.getHeight(), true);   /* (in the print's colour, its own alpha) */
        for (int y = 0; y < src.getHeight(); y++)
            for (int x = 0; x < src.getWidth(); x++) {
                auto p = src.getPixelAt(x, y);
                float a = p.getFloatAlpha() * (p.getBrightness() > 0.5f ? 1.0f : p.getBrightness() * 2.0f);
                t.setPixelAt(x, y, ink().withAlpha(a));
            }
        return t;
    }();
    auto ri = r.getSmallestIntegerContainer();
    g.drawImageWithin(tag, ri.getX(), ri.getY(), ri.getWidth(), ri.getHeight(), RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize);
}

/* SOLARIS in wide bone letters, -23 in rust, a thin rule and the machine's line under it */
void drawLogo(Graphics &g, Rectangle<float> r)
{
    auto f = font(Face::logo, r.getHeight() * 0.82f).withExtraKerningFactor(0.12f);
    g.setFont(f);
    String a = "SOLARIS", b = "-23";
    GlyphArrangement ga;
    ga.addLineOfText(f, a, 0, 0);
    float wa = ga.getBoundingBox(0, -1, true).getWidth();
    auto top = r.withTrimmedBottom(r.getHeight() * 0.12f);
    g.setColour(ink());
    g.drawText(a, top, Justification::centredLeft, false);
    g.setColour(RUST);
    g.drawText(b, top.withTrimmedLeft(wa + r.getHeight() * 0.05f), Justification::centredLeft, false);
}
} // namespace s23
