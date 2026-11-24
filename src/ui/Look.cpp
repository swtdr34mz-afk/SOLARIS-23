/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* The relic, built as hardware: a black steel panel under a rough powder coat (its relief computed at the screen's
 * own pixel density and lit from the upper left), decades of wear on it; knobs with knurled skirts (each rib a lit
 * facet), chamfered edges, bakelite, bone and oxide caps or spun aluminium; engraved, paint-filled pointers; push
 * buttons in machined bezels with translucent lenses lit from inside; toggles with threaded bushings and faceted nuts;
 * old glass light sensors; LEDs behind lenses; amber LCDs under glass. Everything but the coat's relief is vector, so
 * it stays sharp at any size; the coat is made for the pixel density it is shown at. One light, from the upper left,
 * for everything. Every wear mark comes from a fixed seed: the same panel every time. */
#include "Look.h"
#include "BinaryData.h"
#include <atomic>
#include <map>
#include <mutex>

namespace s23
{
using namespace juce;

/* ------------------------------------------------------------------------------------- colours --- */
namespace
{
std::atomic<int> currentFinish{0};
const Colour BONE{0xffe2d7bc}, RUST{0xffc8561e};
/* the light: from the upper left, a little in front */
const float LX = -0.55f, LY = -0.68f, LZ = 0.48f;
const float LANG = std::atan2(LY, LX);   /* its direction on the panel */
} // namespace

const char *finishName(int) { return "RELIC"; }
void setFinish(int f) { currentFinish = jlimit(0, NFINISH - 1, f); }
int finish() { return currentFinish.load(); }
Colour panelColour() { return Colour(0xff0f0f0f); }
Colour ink() { return BONE; }
Colour lcdText() { return Colour(0xffffb23e); }

/* The fonts and the pictures made once (the coat's relief, the maker's tag): held while an editor is open and let
 * go when the last one closes (releaseResources). Nothing of them is left for the moment the host unloads the
 * plugin: on Windows that runs under the loader's lock, and freeing embedded fonts there can deadlock the host
 * (FL Studio hung on closing). */
namespace
{
struct Res {
    Typeface::Ptr face[4];
    Image tag;
    std::map<int, Image> coat;
};
std::mutex resMutex;
std::unique_ptr<Res> res;
int users = 0;
Res &resources()
{
    if (!res)
        res = std::make_unique<Res>();
    return *res;
}
} // namespace

void acquireResources()
{
    std::lock_guard<std::mutex> l(resMutex);
    users++;
}

void releaseResources()
{
    std::unique_ptr<Res> gone;
    {
        std::lock_guard<std::mutex> l(resMutex);
        if (--users > 0)
            return;
        users = 0;
        gone = std::move(res);
    }
    gone.reset();                        /* (the fonts, the coat, the tag) */
    Typeface::clearTypefaceCache();
    ImageCache::releaseUnusedImages();
}

static Typeface::Ptr face(Face f)
{
    std::lock_guard<std::mutex> l(resMutex);
    auto &t = resources().face;
    if (!t[0]) {
        t[0] = Typeface::createSystemTypefaceFor(BinaryData::FiraSansSemiBold_ttf, BinaryData::FiraSansSemiBold_ttfSize);
        t[1] = Typeface::createSystemTypefaceFor(BinaryData::FiraSansBold_ttf, BinaryData::FiraSansBold_ttfSize);
        t[2] = Typeface::createSystemTypefaceFor(BinaryData::FiraSansExtraBold_ttf, BinaryData::FiraSansExtraBold_ttfSize);
        t[3] = Typeface::createSystemTypefaceFor(BinaryData::AnonymousProBold_ttf, BinaryData::AnonymousProBold_ttfSize);
    }
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

/* ---------------------------------------------------------------------------------- the coat --- */
/* The panel's finish: a black textured (stippled) powder coat on steel, as on the better synthesizers of today: a
 * dense, even field of tiny round bumps, each about a panel unit across, every one lit from the upper left (a
 * bright crown, a dark foot), and the pigment's own fine grain between them. No hills, no valleys: at arm's length a
 * soft matte black, close up a fine, regular pebbling. Made at the pixel density it is shown at (so on an 8K screen
 * every bump is still drawn with real pixels), kept per density, tiling seamlessly. */
static Image makeCoat(float sc)
{
    const int T = 512;
    std::vector<float> h((size_t)(T * T), 0.0f);
    Random rnd(7719);
    /* the bumps: jittered on a grid (even, never clumped), a little varied in size and height */
    const float spacing = jmax(1.6f, 1.05f * sc);
    const int n = (int)((float)T / spacing);
    const float cell = (float)T / (float)n;
    for (int gy = 0; gy < n; gy++)
        for (int gx = 0; gx < n; gx++) {
            float cx = ((float)gx + 0.2f + 0.6f * rnd.nextFloat()) * cell, cy = ((float)gy + 0.2f + 0.6f * rnd.nextFloat()) * cell;
            float rad = cell * (0.42f + 0.18f * rnd.nextFloat()), amp = 0.7f + 0.5f * rnd.nextFloat();
            int x0 = (int)std::floor(cx - rad * 1.8f), x1 = (int)std::ceil(cx + rad * 1.8f);
            int y0 = (int)std::floor(cy - rad * 1.8f), y1 = (int)std::ceil(cy + rad * 1.8f);
            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++) {
                    float dx = ((float)x - cx) / rad, dy = ((float)y - cy) / rad;
                    float e = dx * dx + dy * dy;
                    if (e < 3.2f)
                        h[(size_t)(((y % T + T) % T) * T + ((x % T + T) % T))] += amp * std::exp(-e);
                }
        }
    Image im(Image::ARGB, T, T, true);
    Image::BitmapData bd(im, Image::BitmapData::writeOnly);
    const float k = 0.9f;                /* (a bump's slope: its size is in pixels already) */
    const float ln = std::sqrt(LX * LX + LY * LY + LZ * LZ), lx = LX / ln, ly = LY / ln, lz = LZ / ln;
    for (int y = 0; y < T; y++)
        for (int x = 0; x < T; x++) {
            float dx = h[(size_t)(y * T + (x + 1) % T)] - h[(size_t)(y * T + (x + T - 1) % T)];
            float dy = h[(size_t)(((y + 1) % T) * T + x)] - h[(size_t)(((y + T - 1) % T) * T + x)];
            float nx = -dx * k, ny = -dy * k, nl = std::sqrt(nx * nx + ny * ny + 1.0f);
            float shade = (nx * lx + ny * ly + lz) / nl - lz;
            float v = shade + (rnd.nextFloat() - 0.5f) * 0.05f;
            Colour c = v > 0.0f ? Colour(0xffe4e0d6).withAlpha(jmin(0.10f, v * 0.28f)) : Colours::black.withAlpha(jmin(0.55f, -v * 0.9f));
            bd.setPixelColour(x, y, c);
        }
    return im;
}

const Image &coatTile(float sc)
{
    std::lock_guard<std::mutex> l(resMutex);
    auto &cache = resources().coat;
    int key = jlimit(4, 24, (int)std::lround(sc * 4.0f));   /* (in quarters of a pixel per unit) */
    auto it = cache.find(key);
    if (it == cache.end())
        it = cache.emplace(key, makeCoat((float)key / 4.0f)).first;
    return it->second;
}

void drawPanel(Graphics &g, Rectangle<float> r)
{
    g.setColour(panelColour());
    g.fillRect(r);
    /* the coat thinner or thicker, a little brown, a little green: large, soft */
    Random rnd(1923);
    for (int i = 0; i < (int)(r.getWidth() * r.getHeight() / 26000.0f); i++) {
        float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
        float s = 30.0f + rnd.nextFloat() * 160.0f;
        Colour c = rnd.nextFloat() < 0.45f ? Colour(0xff2e2a22) : rnd.nextFloat() < 0.5f ? Colour(0xff0b0b0a) : Colour(0xff222520);
        ColourGradient cg(c.withAlpha(0.07f), x, y, c.withAlpha(0.0f), x + s, y, true);
        cg.addColour(0.5, c.withAlpha(0.03f));
        g.setGradientFill(cg);
        g.fillEllipse(x - s, y - s, s * 2.0f, s * 2.0f);
    }
    /* the light falling across the sheet */
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.02f), r.getX(), r.getY(), Colours::black.withAlpha(0.14f), r.getRight(),
                                     r.getBottom(), false));
    g.fillRect(r);
}

void drawPanelEdge(Graphics &g, Rectangle<float> r)
{
    /* the sheet's folded edge: lit along the top and left, in shadow along the bottom and right */
    for (int i = 0; i < 4; i++) {
        float a = 0.20f - 0.045f * (float)i;
        auto e = r.reduced((float)i + 0.5f);
        g.setColour(Colour(0xffd8d2c4).withAlpha(a));
        g.drawLine(e.getX(), e.getY(), e.getRight(), e.getY(), 1.0f);
        g.drawLine(e.getX(), e.getY(), e.getX(), e.getBottom(), 1.0f);
        g.setColour(Colours::black.withAlpha(a * 2.4f));
        g.drawLine(e.getX(), e.getBottom(), e.getRight(), e.getBottom(), 1.0f);
        g.drawLine(e.getRight(), e.getY(), e.getRight(), e.getBottom(), 1.0f);
    }
}

/* the wear, over everything printed; px: one screen pixel in panel units (so the finest marks are a pixel wide at
 * any size) */
void drawWear(Graphics &g, Rectangle<float> r, float px)
{
    Random rnd(4242);
    auto P = [&](float mx, float my) { return Point<float>(r.getX() + mx * r.getWidth(), r.getY() + my * r.getHeight()); };
    /* scuffs: the black rubbed thin, the grey primer coming through, in streaks along the rubbing */
    for (int i = 0; i < 18; i++) {
        auto c = P(rnd.nextFloat(), rnd.nextFloat());
        float w = 20.0f + rnd.nextFloat() * 110.0f, h = w * (0.2f + rnd.nextFloat() * 0.4f);
        float ang = rnd.nextFloat() * 3.14f;
        Graphics::ScopedSaveState s(g);
        g.addTransform(AffineTransform::rotation(ang, c.x, c.y));
        float a = 0.04f + 0.07f * rnd.nextFloat();
        ColourGradient cg(Colour(0xff6d6a62).withAlpha(a), c.x, c.y, Colour(0xff6d6a62).withAlpha(0.0f), c.x + w * 0.5f, c.y, true);
        g.setGradientFill(cg);
        g.fillEllipse(c.x - w * 0.5f, c.y - h * 0.5f, w, h);
        for (int k = 0; k < 14; k++) {   /* the streaks */
            float yy = c.y + (rnd.nextFloat() - 0.5f) * h * 0.8f, xx = c.x + (rnd.nextFloat() - 0.5f) * w * 0.6f;
            g.setColour(Colour(0xff8a867c).withAlpha(a * 0.9f));
            g.drawLine(xx - w * 0.2f, yy, xx + w * 0.2f, yy, px);
        }
    }
    /* grime: brown, collected low and in the corners */
    for (int i = 0; i < 10; i++) {
        auto c = P(rnd.nextFloat(), 0.35f + 0.65f * rnd.nextFloat());
        float s = 30.0f + rnd.nextFloat() * 120.0f;
        ColourGradient cg(Colour(0xff3a2a16).withAlpha(0.16f + 0.14f * rnd.nextFloat()), c.x, c.y, Colour(0xff3a2a16).withAlpha(0.0f), c.x + s, c.y, true);
        cg.addColour(0.6, Colour(0xff3a2a16).withAlpha(0.05f));
        g.setGradientFill(cg);
        g.fillEllipse(c.x - s, c.y - s, s * 2.0f, s * 2.0f);
    }
    /* scratches: a groove (its shadow side) with a lit lip; the deep, short ones down to the steel */
    for (int i = 0; i < 560; i++) {
        auto a = P(rnd.nextFloat(), rnd.nextFloat());
        float len = rnd.nextFloat() < 0.85f ? 4.0f + rnd.nextFloat() * 40.0f : 40.0f + rnd.nextFloat() * 180.0f;
        float ang = (rnd.nextFloat() < 0.6f ? -0.35f + rnd.nextFloat() * 0.7f : rnd.nextFloat() * 3.14f);
        auto b = a + Point<float>(std::cos(ang), std::sin(ang)) * len;
        Path p;
        p.startNewSubPath(a);
        p.quadraticTo((a + b) * 0.5f + Point<float>(rnd.nextFloat() - 0.5f, rnd.nextFloat() - 0.5f) * len * 0.15f, b);
        bool deep = len < 26.0f && rnd.nextFloat() < 0.22f;
        float w = deep ? jmax(px * 1.5f, 0.8f) : px * (1.0f + rnd.nextFloat());
        PathStrokeType st(w, PathStrokeType::curved, PathStrokeType::rounded);
        g.setColour(Colours::black.withAlpha(deep ? 0.6f : 0.3f));
        g.strokePath(p, st, AffineTransform::translation(w * 0.7f, w * 0.7f));
        if (deep) {
            g.setGradientFill(ColourGradient(Colour(0xffd0d1cc), a.x, a.y, Colour(0xff6d6e6a), b.x, b.y, false));
            g.strokePath(p, st);
        } else {
            g.setColour(Colour(0xffbdb6a6).withAlpha(len > 60.0f ? 0.05f + 0.06f * rnd.nextFloat() : 0.07f + 0.15f * rnd.nextFloat()));
            g.strokePath(p, st);
        }
    }
    /* chips along the edges: paint gone, bare steel with a dark lip, more towards the corners */
    for (int i = 0; i < 90; i++) {
        float t = rnd.nextFloat();
        t = t < 0.5f ? t * t * 2.0f : 1.0f - (1.0f - t) * (1.0f - t) * 2.0f;
        int side = rnd.nextInt(4);
        Point<float> c = side == 0 ? P(t, 0.0f) : side == 1 ? P(t, 1.0f) : side == 2 ? P(0.0f, t) : P(1.0f, t);
        c += Point<float>(side == 2 ? 1.0f : side == 3 ? -1.0f : 0.0f, side == 0 ? 1.0f : side == 1 ? -1.0f : 0.0f) * rnd.nextFloat() * rnd.nextFloat() * 8.0f;
        Path chip;
        int n = 7 + rnd.nextInt(5);
        float s = 1.0f + rnd.nextFloat() * rnd.nextFloat() * (rnd.nextFloat() < 0.2f ? 14.0f : 4.0f);
        for (int k = 0; k < n; k++) {
            float a = (float)k / (float)n * 6.283f;
            auto q = c + Point<float>(std::cos(a), std::sin(a)) * s * (0.45f + rnd.nextFloat() * 0.7f);
            if (k)
                chip.lineTo(q);
            else
                chip.startNewSubPath(q);
        }
        chip.closeSubPath();
        g.setColour(Colours::black.withAlpha(0.55f));
        g.strokePath(chip, PathStrokeType(px * 1.5f));
        g.setGradientFill(ColourGradient(Colour(0xffc8c9c4), c.x - s, c.y - s, Colour(0xff626360), c.x + s, c.y + s, false));
        g.fillPath(chip);
    }
    /* the print worn: pin-holes of the coat over it, a pixel or two each, and a few bigger flakes */
    const int n = (int)(r.getWidth() * r.getHeight() * 0.006f / jmax(0.25f, px));
    for (int i = 0; i < n; i++) {
        auto c = P(rnd.nextFloat(), rnd.nextFloat());
        float s = px * (0.8f + rnd.nextFloat() * 1.6f) + (rnd.nextFloat() < 0.03f ? rnd.nextFloat() * 1.6f : 0.0f);
        g.setColour(panelColour().withAlpha(0.55f + 0.45f * rnd.nextFloat()));
        g.fillEllipse(c.x, c.y, s, s * (0.6f + rnd.nextFloat() * 0.8f));
    }
}

void drawScrew(Graphics &g, Point<float> c, float rr)
{
    Random rnd((int64)(c.x * 13.0f + c.y * 7.0f));
    /* rust blooming out from under it, specks */
    ColourGradient rg(Colour(0xff7a3a12).withAlpha(0.55f), c.x, c.y, Colour(0xff7a3a12).withAlpha(0.0f), c.x + rr * 3.6f, c.y, true);
    rg.addColour(0.45, Colour(0xff6a3010).withAlpha(0.25f));
    g.setGradientFill(rg);
    g.fillEllipse(Rectangle<float>(rr * 7.2f, rr * 7.2f).withCentre(c));
    for (int i = 0; i < 40; i++) {
        float a = rnd.nextFloat() * 6.283f, d = rr * (1.0f + rnd.nextFloat() * 2.6f);
        g.setColour((rnd.nextBool() ? Colour(0xff9a4a18) : Colour(0xff5a2a0c)).withAlpha(0.3f + 0.4f * rnd.nextFloat()));
        float s = 0.6f + rnd.nextFloat() * 1.6f;
        g.fillEllipse(Rectangle<float>(s, s * (0.6f + rnd.nextFloat())).withCentre(c + Point<float>(std::cos(a), std::sin(a)) * d));
    }
    /* the head: a dome of dull steel, its slot chewed by screwdrivers */
    g.setColour(Colours::black.withAlpha(0.6f));
    g.fillEllipse(Rectangle<float>(rr * 2.25f, rr * 2.25f).withCentre(c.translated(0.3f, 0.6f)));
    ColourGradient hg(Colour(0xffcac5b8), c.x - rr * 0.5f, c.y - rr * 0.6f, Colour(0xff3f3c36), c.x + rr, c.y + rr, true);
    hg.addColour(0.35, Colour(0xff8e897e));
    g.setGradientFill(hg);
    g.fillEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c));
    float a = 0.4f + rnd.nextFloat();
    Path p;
    p.addRoundedRectangle(-rr * 0.75f, -rr * 0.14f, rr * 1.5f, rr * 0.28f, rr * 0.05f);
    g.setColour(Colour(0xff151412));
    g.fillPath(p, AffineTransform::rotation(a).translated(c));
    g.setColour(Colours::white.withAlpha(0.25f));
    g.strokePath(p, PathStrokeType(rr * 0.05f), AffineTransform::rotation(a).translated(c.translated(rr * 0.04f, rr * 0.06f)));
}

void drawSection(Graphics &g, Rectangle<float> r, const String &title, Sec s)
{
    if (s == Sec::field) {               /* the touch field: a black-oxide plate set into the panel, its walls in shadow */
        g.setGradientFill(ColourGradient(Colour(0xff0e0e0d), r.getX(), r.getY(), Colour(0xff1b1a18), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle(r, 5.0f);
        for (int i = 0; i < 6; i++) {    /* the recess's inner shadow (top, left) and lit lip (bottom, right) */
            auto e = r.reduced((float)i);
            g.setColour(Colours::black.withAlpha(0.32f - 0.05f * (float)i));
            g.drawLine(e.getX() + 4.0f, e.getY(), e.getRight() - 4.0f, e.getY(), 1.0f);
            g.drawLine(e.getX(), e.getY() + 4.0f, e.getX(), e.getBottom() - 4.0f, 1.0f);
        }
        g.setColour(Colour(0xffd8d2c4).withAlpha(0.12f));
        g.drawLine(r.getX() + 5.0f, r.getBottom() - 0.5f, r.getRight() - 5.0f, r.getBottom() - 0.5f, 1.0f);
        g.drawLine(r.getRight() - 0.5f, r.getY() + 5.0f, r.getRight() - 0.5f, r.getBottom() - 5.0f, 1.0f);
    } else {
        g.setColour(ink().withAlpha(0.85f));
        g.drawRoundedRectangle(r.reduced(0.8f), 4.0f, 1.3f);
    }
    if (title.isEmpty())
        return;
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

/* ------------------------------------------------------------------------------------ helpers --- */
namespace
{
/* how much light a surface facing direction a (on the panel) and tilted by t (0 flat .. 1 vertical) catches */
float lit(float a, float t)
{
    float nx = std::cos(a) * t, ny = std::sin(a) * t, nz = std::sqrt(jmax(0.0f, 1.0f - t * t));
    float l = std::sqrt(LX * LX + LY * LY + LZ * LZ);
    return jmax(0.0f, (nx * LX + ny * LY + nz * LZ) / l);
}

/* a soft shadow: rings of falling darkness (an occlusion under anything standing on the panel) */
void softShadow(Graphics &g, Point<float> c, float r0, float r1, float alpha, Point<float> offset)
{
    ColourGradient cg(Colours::black.withAlpha(alpha), c.x + offset.x, c.y + offset.y, Colours::transparentBlack, c.x + offset.x + r1, c.y + offset.y, true);
    cg.addColour(jlimit(0.0, 0.99, (double)(r0 / r1)), Colours::black.withAlpha(alpha * 0.85f));
    cg.addColour(jlimit(0.0, 0.99, (double)((r0 + (r1 - r0) * 0.45f) / r1)), Colours::black.withAlpha(alpha * 0.3f));
    g.setGradientFill(cg);
    g.fillEllipse(Rectangle<float>(r1 * 2.0f, r1 * 2.0f).withCentre(c + offset));
}

/* a ring (between r0 and r1) shaded as a chamfer facing outwards and up: each of n wedges lit by its direction */
void chamfer(Graphics &g, Point<float> c, float r0, float r1, Colour base, float tilt, float gloss, int n = 96)
{
    for (int i = 0; i < n; i++) {
        float a0 = MathConstants<float>::twoPi * (float)i / (float)n, a1 = MathConstants<float>::twoPi * (float)(i + 1) / (float)n + 0.004f;
        float am = (a0 + a1) * 0.5f;
        float l = lit(am, tilt);
        float spec = std::pow(jmax(0.0f, std::cos(am - LANG)), 18.0f) * gloss;
        Colour col = base.withMultipliedBrightness(0.35f + 1.25f * l).interpolatedWith(Colours::white, jmin(1.0f, spec));
        Path w;
        w.addPieSegment(c.x - r1, c.y - r1, r1 * 2.0f, r1 * 2.0f, a0 + MathConstants<float>::halfPi, a1 + MathConstants<float>::halfPi, r0 / r1);
        g.setColour(col);
        g.fillPath(w);
    }
}
} // namespace

/* --------------------------------------------------------------------------------------- knobs --- */
namespace
{
struct Cap {
    Colour skirt, top;
    float gloss;
    bool metal;
    Colour line;
};
Cap capOf(KS s)
{
    switch (s) {
    case KS::black: return {Colour(0xff151515), Colour(0xff1c1c1b), 0.55f, false, BONE};
    case KS::slate: return {Colour(0xff171717), Colour(0xff232321), 0.55f, false, BONE};
    case KS::light: return {Colour(0xff2e2a22), Colour(0xffcbbd98), 0.35f, false, Colour(0xff161513)};
    case KS::blue: return {Colour(0xff1a1a1a), Colour(0xffa2a4a0), 0.9f, true, Colour(0xff131313)};
    case KS::red: return {Colour(0xff1e0d0a), Colour(0xff7a281a), 0.5f, false, BONE};
    }
    return {};
}
} // namespace

void drawKnob(Graphics &g, Rectangle<float> r, float pos, KS style, bool bipolar, bool ticks)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    const Point<float> c = r.getCentre();
    const float a0 = -2.42f, a1 = 2.42f, ang = a0 + jlimit(0.0f, 1.0f, pos) * (a1 - a0);
    const float R = d * 0.5f * (ticks ? 0.72f : 0.84f);   /* the skirt */
    const float Rc = R * 0.78f;                             /* the cap */
    const Cap cap = capOf(style);
    Random rnd((int64)(c.x * 97.0f + c.y * 31.0f));

    if (ticks) {                         /* the scale: printed, worn where the fingers go */
        for (int i = 0; i <= 10; i++) {
            float a = a0 + (a1 - a0) * (float)i / 10.0f;
            float len = (i == 0 || i == 5 || i == 10) ? 0.22f : 0.14f;
            g.setColour(ink().withAlpha(0.55f + 0.45f * rnd.nextFloat()));
            Path t;
            t.startNewSubPath(c.getPointOnCircumference(d * 0.5f * (1.0f - len), a));
            t.lineTo(c.getPointOnCircumference(d * 0.5f, a));
            g.strokePath(t, PathStrokeType(jmax(0.9f, d * 0.024f), PathStrokeType::curved, PathStrokeType::butt));
        }
        if (bipolar) {
            g.setColour(ink());
            g.fillEllipse(Rectangle<float>(d * 0.06f, d * 0.06f).withCentre(c.getPointOnCircumference(d * 0.5f + d * 0.05f, 0.0f)));
        }
    }
    /* the panel under it: rubbed to a dull shine in a ring by decades of fingers */
    {
        ColourGradient rg(Colour(0xff5a5852).withAlpha(0.0f), c.x, c.y, Colour(0xff5a5852).withAlpha(0.0f), c.x + R * 1.45f, c.y, true);
        rg.addColour(0.72, Colour(0xff5a5852).withAlpha(0.14f));
        g.setGradientFill(rg);
        g.fillEllipse(Rectangle<float>(R * 2.9f, R * 2.9f).withCentre(c));
    }
    /* its shadows: wide and soft away from the light, tight and dark where it touches */
    softShadow(g, c, R * 0.9f, R * 1.35f, 0.55f, {R * 0.10f, R * 0.16f});
    softShadow(g, c, R * 0.98f, R * 1.08f, 0.85f, {R * 0.02f, R * 0.04f});

    /* the skirt: its side a cylinder of ribs, each rib two facets, lit by its own direction */
    const int ribs = jlimit(24, 72, (int)(R * 2.0f));
    g.setColour(cap.skirt.darker(0.6f));
    g.fillEllipse(Rectangle<float>(R * 2.0f, R * 2.0f).withCentre(c));
    for (int i = 0; i < ribs; i++) {
        float aa = MathConstants<float>::twoPi * (float)i / (float)ribs, ab = MathConstants<float>::twoPi * (float)(i + 1) / (float)ribs;
        float am = (aa + ab) * 0.5f;
        for (int f = 0; f < 2; f++) {    /* (each facet faces a little to its side) */
            float fa = f ? aa : am, fb = f ? am : ab + 0.003f;
            float facing = (fa + fb) * 0.5f + (f ? -0.5f : 0.5f);
            float l = lit(facing, 0.92f);
            float spec = std::pow(jmax(0.0f, std::cos(facing - LANG)), 30.0f) * cap.gloss;
            Colour col = cap.skirt.withMultipliedBrightness(0.5f + 3.2f * l).interpolatedWith(Colour(0xffe8e4da), jmin(0.8f, spec));
            Path w;
            w.addPieSegment(c.x - R, c.y - R, R * 2.0f, R * 2.0f, fa + MathConstants<float>::halfPi, fb + MathConstants<float>::halfPi, 0.86f);
            g.setColour(col);
            g.fillPath(w);
        }
    }
    /* the skirt's top: a flat ring, then the chamfer up to the cap */
    chamfer(g, c, Rc * 1.04f, R * 0.87f, cap.skirt.brighter(0.4f), 0.15f, cap.gloss * 0.4f, 72);
    chamfer(g, c, Rc * 0.96f, Rc * 1.05f, cap.metal ? cap.top : cap.top.brighter(0.15f), 0.75f, cap.gloss, 96);

    /* the cap */
    auto top = Rectangle<float>(Rc * 2.0f * 0.96f, Rc * 2.0f * 0.96f).withCentre(c);
    const float rt = top.getWidth() * 0.5f;
    if (cap.metal) {                     /* spun aluminium: light runs across the grooves in a bow-tie, rings of the lathe */
        const int n = 120;
        for (int i = 0; i < n; i++) {
            float aa = MathConstants<float>::twoPi * (float)i / (float)n, ab = MathConstants<float>::twoPi * (float)(i + 1) / (float)n + 0.004f;
            float am = (aa + ab) * 0.5f;
            float aniso = 0.5f + 0.5f * std::cos(2.0f * (am - LANG - MathConstants<float>::halfPi));
            Colour col = cap.top.withMultipliedBrightness(0.62f + 0.62f * aniso * aniso);
            Path w;
            w.addPieSegment(top, aa + MathConstants<float>::halfPi, ab + MathConstants<float>::halfPi, 0.0f);
            g.setColour(col);
            g.fillPath(w);
        }
        for (float k = 0.06f; k < 0.99f; k += 0.022f + 0.012f * rnd.nextFloat()) {
            g.setColour((rnd.nextBool() ? Colours::white : Colours::black).withAlpha(0.04f + 0.08f * rnd.nextFloat()));
            g.drawEllipse(Rectangle<float>(rt * 2.0f * k, rt * 2.0f * k).withCentre(c), jmax(0.35f, rt * 0.008f));
        }
        g.setColour(Colours::black.withAlpha(0.35f));   /* the centre where the tool was */
        g.fillEllipse(Rectangle<float>(rt * 0.08f, rt * 0.08f).withCentre(c));
    } else {                             /* moulded: a satin dome, the light a soft bloom up and left, a darker rim */
        ColourGradient cg(cap.top.brighter(0.5f), c.x + LX * rt * 0.45f, c.y + LY * rt * 0.45f, cap.top.darker(0.45f), c.x - LX * rt, c.y - LY * rt, true);
        cg.addColour(0.45, cap.top);
        g.setGradientFill(cg);
        g.fillEllipse(top);
        ColourGradient sp(Colours::white.withAlpha(0.22f * cap.gloss), c.x + LX * rt * 0.5f, c.y + LY * rt * 0.5f, Colours::white.withAlpha(0.0f),
                          c.x + LX * rt * 0.5f + rt * 0.45f, c.y + LY * rt * 0.5f, true);
        g.setGradientFill(sp);
        g.fillEllipse(top);
    }
    /* use: fine scratches, a little dirt in the edge */
    for (int i = 0; i < 4 + (int)(rt * 0.25f); i++) {
        float a = rnd.nextFloat() * 6.283f, rr = rnd.nextFloat() * rt * 0.8f, len = rt * (0.08f + 0.3f * rnd.nextFloat());
        auto p0 = c + Point<float>(std::cos(a), std::sin(a)) * rr;
        float b = rnd.nextFloat() * 6.283f;
        auto p1 = p0 + Point<float>(std::cos(b), std::sin(b)) * len;
        if (p1.getDistanceFrom(c) < rt * 0.92f) {
            g.setColour((cap.metal || style == KS::light ? Colours::black : Colours::white).withAlpha(0.06f + 0.1f * rnd.nextFloat()));
            g.drawLine(Line<float>(p0, p1), jmax(0.35f, rt * 0.012f));
        }
    }
    {
        ColourGradient dg(Colours::transparentBlack, c.x, c.y, Colour(0xff201a10).withAlpha(0.35f), c.x + rt, c.y, true);
        dg.addColour(0.85, Colours::transparentBlack);
        g.setGradientFill(dg);
        g.fillEllipse(top);
    }
    /* the pointer: an engraved groove filled with paint, worn at the tip */
    {
        auto p0 = c.getPointOnCircumference(rt * 0.22f, ang), p1 = c.getPointOnCircumference(rt * 0.9f, ang);
        float w = jmax(1.2f, rt * 0.13f);
        Path line;
        line.startNewSubPath(p0);
        line.lineTo(p1);
        PathStrokeType st(w, PathStrokeType::curved, PathStrokeType::rounded);
        g.setColour(Colours::black.withAlpha(0.75f));   /* the groove's wall in shadow */
        g.strokePath(line, PathStrokeType(w * 1.35f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(cap.line.withAlpha(0.92f));
        g.strokePath(line, st, AffineTransform::translation(w * 0.08f, w * 0.12f));
        g.setColour(Colours::white.withAlpha(cap.metal || style == KS::light ? 0.15f : 0.25f));   /* its lit edge */
        g.strokePath(line, PathStrokeType(jmax(0.4f, w * 0.18f)), AffineTransform::translation(-w * 0.32f, -w * 0.3f));
    }
}

/* ------------------------------------------------------------------------------------ switches --- */
void drawToggle(Graphics &g, Rectangle<float> r, bool up)
{
    const Point<float> c = r.getCentre();
    const float s = jmin(r.getWidth() * 0.9f, r.getHeight() * 0.5f);
    softShadow(g, c, s * 0.45f, s * 0.75f, 0.6f, {s * 0.06f, s * 0.1f});
    /* the nut: six facets, each lit by its own direction, a chamfered top */
    Point<float> v[6];
    for (int i = 0; i < 6; i++)
        v[i] = c.getPointOnCircumference(s * 0.5f, MathConstants<float>::pi / 6.0f + (float)i * MathConstants<float>::pi / 3.0f);
    for (int i = 0; i < 6; i++) {
        Path f;
        auto inner0 = c + (v[i] - c) * 0.72f, inner1 = c + (v[(i + 1) % 6] - c) * 0.72f;
        f.startNewSubPath(v[i]);
        f.lineTo(v[(i + 1) % 6]);
        f.lineTo(inner1);
        f.lineTo(inner0);
        f.closeSubPath();
        auto mid = (v[i] + v[(i + 1) % 6]) * 0.5f - c;
        float l = lit(std::atan2(mid.y, mid.x) - MathConstants<float>::halfPi + MathConstants<float>::halfPi, 0.7f);
        g.setColour(Colour(0xff8c887e).withMultipliedBrightness(0.45f + 1.3f * l));
        g.fillPath(f);
    }
    Path hex;
    for (int i = 0; i < 6; i++) {
        auto p = c + (v[i] - c) * 0.72f;
        if (i)
            hex.lineTo(p);
        else
            hex.startNewSubPath(p);
    }
    hex.closeSubPath();
    g.setGradientFill(ColourGradient(Colour(0xffa9a498), c.x - s * 0.3f, c.y - s * 0.3f, Colour(0xff4d4a43), c.x + s * 0.3f, c.y + s * 0.3f, false));
    g.fillPath(hex);
    /* the threaded bushing */
    for (int i = 0; i < 4; i++) {
        float rr = s * (0.27f - 0.035f * (float)i);
        chamfer(g, c, rr * 0.86f, rr, Colour(0xff7d796f), 0.5f, 0.6f, 36);
    }
    g.setColour(Colour(0xff0f0f0e));
    g.fillEllipse(Rectangle<float>(s * 0.2f, s * 0.2f).withCentre(c));
    /* the bat: a chrome cylinder, tapering, its shadow on the panel; the ball at its tip */
    const float dir = up ? -1.0f : 1.0f, len = r.getHeight() * 0.40f;
    Point<float> tip(c.x, c.y + dir * len);
    Path bat;
    bat.startNewSubPath(c.x - s * 0.12f, c.y);
    bat.lineTo(tip.x - s * 0.085f, tip.y);
    bat.lineTo(tip.x + s * 0.085f, tip.y);
    bat.lineTo(c.x + s * 0.12f, c.y);
    bat.closeSubPath();
    g.setColour(Colours::black.withAlpha(0.45f));
    g.fillPath(bat, AffineTransform::translation(s * 0.1f, s * 0.14f));
    ColourGradient cyl(Colour(0xff3b3934), c.x - s * 0.13f, c.y, Colour(0xff4a4740), c.x + s * 0.13f, c.y, false);
    cyl.addColour(0.22, Colour(0xffe9e5da));
    cyl.addColour(0.34, Colour(0xffb3aea2));
    cyl.addColour(0.7, Colour(0xff6c685f));
    g.setGradientFill(cyl);
    g.fillPath(bat);
    auto ball = Rectangle<float>(s * 0.34f, s * 0.34f).withCentre(tip);
    softShadow(g, tip, s * 0.15f, s * 0.24f, 0.5f, {s * 0.08f, s * 0.12f});
    ColourGradient bg(Colour(0xfff4f1ea), tip.x - s * 0.06f, tip.y - s * 0.07f, Colour(0xff3c3a35), tip.x + s * 0.17f, tip.y + s * 0.17f, true);
    bg.addColour(0.3, Colour(0xffa09b8f));
    g.setGradientFill(bg);
    g.fillEllipse(ball);
}

void drawButton(Graphics &g, Rectangle<float> r, bool on, bool down, bool lens)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    const Point<float> c = r.getCentre();
    const float R = d * 0.5f;
    softShadow(g, c, R * 0.85f, R * 1.25f, 0.55f, {R * 0.08f, R * 0.12f});
    /* the bezel: a machined steel ring, its outer chamfer lit, its inner wall in shadow */
    chamfer(g, c, R * 0.80f, R, Colour(0xff8e897e), 0.55f, 0.8f, 72);
    g.setColour(Colour(0xff0a0a09));
    g.fillEllipse(Rectangle<float>(R * 1.62f, R * 1.62f).withCentre(c));
    const float push = down ? R * 0.05f : 0.0f;
    auto cap = Rectangle<float>(R * 1.42f, R * 1.42f).withCentre(c.translated(push * 0.5f, push));
    const float rc = cap.getWidth() * 0.5f;
    if (lens) {                          /* a translucent lens, lit from inside (amber) or dark */
        Colour body = on ? Colour(0xffffa23a) : Colour(0xff5a4a2c);
        ColourGradient lg(on ? Colour(0xfffff1c8) : Colour(0xff8c7b58), c.x, c.y + push, body.darker(on ? 0.5f : 0.9f), c.x + rc, c.y + push, true);
        lg.addColour(0.55, body);
        g.setGradientFill(lg);
        g.fillEllipse(cap);
        if (on) {                        /* its light spilling over the bezel */
            ColourGradient bloom(Colour(0xffffa23a).withAlpha(0.45f), c.x, c.y, Colour(0xffffa23a).withAlpha(0.0f), c.x + R * 2.0f, c.y, true);
            bloom.addColour(0.4, Colour(0xffffa23a).withAlpha(0.14f));
            g.setGradientFill(bloom);
            g.fillEllipse(Rectangle<float>(R * 4.0f, R * 4.0f).withCentre(c));
        }
        for (float k = 0.3f; k < 0.95f; k += 0.16f) {   /* the lens's moulded rings */
            g.setColour(Colours::white.withAlpha(on ? 0.12f : 0.06f));
            g.drawEllipse(Rectangle<float>(rc * 2.0f * k, rc * 2.0f * k).withCentre(cap.getCentre()), jmax(0.4f, rc * 0.03f));
        }
    } else {                             /* a dished black cap: darker at its upper rim, lit at its lower */
        ColourGradient bg(Colour(0xff0d0d0c), cap.getCentreX() + LX * rc, cap.getCentreY() + LY * rc, Colour(0xff3d3b37),
                          cap.getCentreX() - LX * rc, cap.getCentreY() - LY * rc, false);
        g.setGradientFill(bg);
        g.fillEllipse(cap);
        g.setColour(Colour(0xff1b1a18));
        g.fillEllipse(cap.reduced(rc * 0.14f));
        if (on) {
            g.setColour(Colour(0xffffa23a).withAlpha(0.55f));
            g.drawEllipse(cap.reduced(rc * 0.06f), jmax(1.0f, rc * 0.1f));
        }
    }
    /* the glass of it: a crescent of the light, a hot spot */
    {
        Path cres;
        cres.addCentredArc(cap.getCentreX(), cap.getCentreY(), rc * 0.78f, rc * 0.78f, 0.0f, LANG + MathConstants<float>::halfPi - 0.9f,
                           LANG + MathConstants<float>::halfPi + 0.9f, true);
        g.setColour(Colours::white.withAlpha(lens ? 0.35f : 0.16f));
        g.strokePath(cres, PathStrokeType(jmax(0.6f, rc * 0.1f), PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour(Colours::white.withAlpha(lens ? 0.55f : 0.25f));
        g.fillEllipse(Rectangle<float>(rc * 0.22f, rc * 0.16f).withCentre(cap.getCentre() + Point<float>(LX, LY) * rc * 0.55f));
    }
}

void drawDroneKey(Graphics &g, Rectangle<float> r, bool on, bool down)
{
    auto b = r.reduced(3.0f);
    const float push = down ? 1.2f : 0.0f;
    /* the well in the panel, the key's shadow in it */
    g.setColour(Colour(0xff050505));
    g.fillRoundedRectangle(b.expanded(1.5f), 5.0f);
    g.setColour(Colours::black.withAlpha(0.7f));
    g.fillRoundedRectangle(b.translated(1.2f, 2.4f - push), 4.0f);
    auto k = b.translated(0.0f, push);
    /* the key: its walls, a chamfer all round (each side lit by its direction), a slightly dished top */
    g.setColour(Colour(0xff121110));
    g.fillRoundedRectangle(k, 4.0f);
    const float ch = jmin(k.getWidth(), k.getHeight()) * 0.09f;
    auto top = k.reduced(ch);
    Path sides[4];
    sides[0].addQuadrilateral(k.getX(), k.getY(), k.getRight(), k.getY(), top.getRight(), top.getY(), top.getX(), top.getY());          /* up */
    sides[1].addQuadrilateral(k.getX(), k.getBottom(), k.getRight(), k.getBottom(), top.getRight(), top.getBottom(), top.getX(), top.getBottom());
    sides[2].addQuadrilateral(k.getX(), k.getY(), k.getX(), k.getBottom(), top.getX(), top.getBottom(), top.getX(), top.getY());     /* left */
    sides[3].addQuadrilateral(k.getRight(), k.getY(), k.getRight(), k.getBottom(), top.getRight(), top.getBottom(), top.getRight(), top.getY());
    const float facing[4] = {-MathConstants<float>::halfPi, MathConstants<float>::halfPi, MathConstants<float>::pi, 0.0f};
    for (int i = 0; i < 4; i++) {
        g.setColour(Colour(0xff2c2b28).withMultipliedBrightness(0.4f + 1.6f * lit(facing[i], 0.7f)));
        g.fillPath(sides[i]);
    }
    ColourGradient tg(Colour(0xff1f1e1c), top.getX(), top.getY(), Colour(0xff34322e), top.getX(), top.getBottom(), false);
    g.setGradientFill(tg);
    g.fillRect(top);
    ColourGradient worn(Colours::white.withAlpha(0.08f), top.getCentreX(), top.getCentreY() + top.getHeight() * 0.1f, Colours::transparentWhite,
                        top.getCentreX() + top.getWidth() * 0.45f, top.getCentreY(), true);
    g.setGradientFill(worn);
    g.fillRect(top);
    /* its window: a little lens, lit while the voice is open */
    auto win = Rectangle<float>(top.getWidth() * 0.26f, top.getHeight() * 0.17f).withCentre({top.getCentreX(), top.getY() + top.getHeight() * 0.2f});
    if (on) {
        ColourGradient bloom(AMBER_LED.withAlpha(0.5f), win.getCentreX(), win.getCentreY(), AMBER_LED.withAlpha(0.0f), win.getCentreX() + win.getWidth() * 1.4f,
                             win.getCentreY(), true);
        g.setGradientFill(bloom);
        g.fillEllipse(win.expanded(win.getWidth() * 0.9f, win.getHeight() * 1.6f));
    }
    g.setColour(Colour(0xff080706));
    g.fillRoundedRectangle(win.expanded(1.0f), 2.0f);
    ColourGradient lg(on ? Colour(0xfffff0c8) : Colour(0xff4a3418), win.getCentreX(), win.getCentreY(), on ? AMBER_LED.darker(0.2f) : Colour(0xff22180a),
                      win.getRight(), win.getCentreY(), true);
    g.setGradientFill(lg);
    g.fillRoundedRectangle(win, 1.5f);
    g.setColour(Colours::white.withAlpha(0.3f));
    g.fillRect(win.withHeight(jmax(0.5f, win.getHeight() * 0.18f)).reduced(win.getWidth() * 0.1f, 0.0f).translated(0.0f, win.getHeight() * 0.12f));
}

void drawLed(Graphics &g, Point<float> c, float r, float level, Colour col)
{
    level = jlimit(0.0f, 1.0f, level);
    /* the bezel, a ring of dull steel */
    chamfer(g, c, r * 1.05f, r * 1.4f, Colour(0xff7d796f), 0.55f, 0.6f, 40);
    /* the lens: dark when off (its colour just visible), a hot core and its colour when lit, the glow around */
    ColourGradient lg(col.interpolatedWith(Colours::white, 0.85f * level).withMultipliedBrightness(0.25f + 0.75f * level), c.x - r * 0.15f, c.y - r * 0.15f,
                      col.darker(2.4f - 1.6f * level), c.x + r, c.y + r, true);
    lg.addColour(0.45, col.darker(2.0f).interpolatedWith(col, level));
    g.setGradientFill(lg);
    g.fillEllipse(Rectangle<float>(r * 2.1f, r * 2.1f).withCentre(c));
    if (level > 0.02f) {
        ColourGradient bloom(col.withAlpha(0.5f * level), c.x, c.y, col.withAlpha(0.0f), c.x + r * 3.6f, c.y, true);
        bloom.addColour(0.35, col.withAlpha(0.18f * level));
        g.setGradientFill(bloom);
        g.fillEllipse(Rectangle<float>(r * 7.2f, r * 7.2f).withCentre(c));
    }
    g.setColour(Colours::white.withAlpha(0.3f + 0.4f * level));
    g.fillEllipse(Rectangle<float>(r * 0.55f, r * 0.4f).withCentre(c + Point<float>(LX, LY) * r * 0.5f));
}

void drawOrbitWell(Graphics &g, Rectangle<float> r)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    auto c = Rectangle<float>(d, d).withCentre(r.getCentre()).reduced(d * 0.08f);
    const float R = c.getWidth() * 0.5f;
    const Point<float> m = c.getCentre();
    softShadow(g, m, R * 1.0f, R * 1.14f, 0.55f, {R * 0.04f, R * 0.07f});
    chamfer(g, m, R * 0.96f, R * 1.08f, Colour(0xff8a8579), 0.6f, 0.7f, 96);
    ColourGradient well(Colour(0xff1d1c1a), m.x + LX * R * 0.3f, m.y + LY * R * 0.3f, Colour(0xff050505), m.x + R, m.y, true);
    g.setGradientFill(well);
    g.fillEllipse(c);
    g.setColour(Colour(0xff000000));
    g.drawEllipse(c, jmax(0.6f, R * 0.02f));
    const float tr = R * 0.66f;            /* the track, dotted */
    g.setColour(Colour(0xff6b6555).withAlpha(0.55f));
    for (int k = 0; k < 40; k++) {
        float a = MathConstants<float>::twoPi * (float)k / 40.0f;
        g.fillEllipse(Rectangle<float>(R * 0.035f, R * 0.035f).withCentre({m.x + tr * std::sin(a), m.y - tr * std::cos(a)}));
    }
    g.setColour(Colour(0xff3a3730));       /* the hub */
    g.fillEllipse(Rectangle<float>(R * 0.16f, R * 0.16f).withCentre(m));
    Path rim;                              /* the glass catching light */
    rim.addCentredArc(m.x, m.y, R * 0.92f, R * 0.92f, 0.0f, LANG + MathConstants<float>::halfPi - 1.2f, LANG + MathConstants<float>::halfPi + 1.2f, true);
    g.setColour(Colours::white.withAlpha(0.16f));
    g.strokePath(rim, PathStrokeType(R * 0.04f, PathStrokeType::curved, PathStrokeType::rounded));
}

Point<float> orbitLed(Rectangle<float> r, float phase, float *ledR)
{
    const float d = jmin(r.getWidth(), r.getHeight());
    auto c = Rectangle<float>(d, d).withCentre(r.getCentre()).reduced(d * 0.08f);
    const float R = c.getWidth() * 0.5f, tr = R * 0.66f, a = MathConstants<float>::twoPi * phase;
    if (ledR)
        *ledR = R * 0.085f;
    return {c.getCentreX() + tr * std::sin(a), c.getCentreY() - tr * std::cos(a)};
}

/* ORBIT: a black glass well in a steel ring, a faint track, five amber LEDs where the drone's oscillators are on
   their way round (each a fifth of a turn on) */
void drawOrbit(Graphics &g, Rectangle<float> r, float phase)
{
    drawOrbitWell(g, r);
    for (int k = 0; k < 5; k++) {
        float lr = 0.0f;
        auto p = orbitLed(r, phase + 0.2f * (float)k, &lr);
        drawLed(g, p, lr, k == 0 ? 1.0f : 0.72f, ORBIT_LED);
    }
}

/* the light sensor: an old glass dome in a steel ring, milky and yellowed, glowing with what it sees */
void drawDome(Graphics &g, Rectangle<float> r, float level)
{
    level = jlimit(0.0f, 1.0f, level);
    const float d = jmin(r.getWidth(), r.getHeight());
    auto c = Rectangle<float>(d, d).withCentre(r.getCentre()).reduced(d * 0.09f);
    const float R = c.getWidth() * 0.5f;
    const Point<float> m = c.getCentre();
    softShadow(g, m, R * 1.0f, R * 1.16f, 0.6f, {R * 0.04f, R * 0.07f});
    chamfer(g, m, R * 0.97f, R * 1.09f, Colour(0xff8a8579), 0.6f, 0.7f, 96);
    g.setColour(Colour(0xff0e0d0c));
    g.drawEllipse(c, jmax(0.6f, R * 0.015f));
    /* the glass: milk, deeper at the edge (more glass to look through), the light inside warming it */
    Colour milk = Colour(0xffb4aa90).interpolatedWith(Colour(0xffffd590), level);
    ColourGradient gg(milk.brighter(0.35f + 0.3f * level), m.x + LX * R * 0.25f, m.y + LY * R * 0.25f, milk.darker(0.9f - 0.4f * level), m.x + R, m.y, true);
    gg.addColour(0.6, milk.darker(0.1f));
    g.setGradientFill(gg);
    g.fillEllipse(c);
    if (level > 0.03f) {
        ColourGradient in(Colour(0xffffb84a).withAlpha(0.45f * level), m.x, m.y + R * 0.1f, Colour(0xffffb84a).withAlpha(0.0f), m.x + R * 0.8f, m.y, true);
        g.setGradientFill(in);
        g.fillEllipse(c);
    }
    /* the rim: the glass's edge catching light (Fresnel) */
    Path rim;
    rim.addCentredArc(m.x, m.y, R * 0.93f, R * 0.93f, 0.0f, LANG + MathConstants<float>::halfPi - 1.4f, LANG + MathConstants<float>::halfPi + 1.4f, true);
    g.setColour(Colours::white.withAlpha(0.28f));
    g.strokePath(rim, PathStrokeType(R * 0.05f, PathStrokeType::curved, PathStrokeType::rounded));
    Path rim2;
    rim2.addCentredArc(m.x, m.y, R * 0.94f, R * 0.94f, 0.0f, LANG - MathConstants<float>::halfPi - 1.0f, LANG - MathConstants<float>::halfPi + 1.0f, true);
    g.setColour(Colours::white.withAlpha(0.10f));
    g.strokePath(rim2, PathStrokeType(R * 0.03f, PathStrokeType::curved, PathStrokeType::rounded));
    /* the window's reflection, soft, and its hot spot */
    {
        Graphics::ScopedSaveState s(g);
        Path clip;
        clip.addEllipse(c);
        g.reduceClipRegion(clip);
        ColourGradient refl(Colours::white.withAlpha(0.32f), m.x + LX * R * 0.45f, m.y + LY * R * 0.5f, Colours::white.withAlpha(0.0f),
                            m.x + LX * R * 0.45f + R * 0.42f, m.y + LY * R * 0.5f, true);
        g.setGradientFill(refl);
        g.fillEllipse(Rectangle<float>(R * 0.95f, R * 0.6f).withCentre({m.x + LX * R * 0.42f, m.y + LY * R * 0.52f}));
        g.setColour(Colours::white.withAlpha(0.7f));
        g.fillEllipse(Rectangle<float>(R * 0.13f, R * 0.09f).withCentre({m.x + LX * R * 0.5f, m.y + LY * R * 0.58f}));
        /* scratches in the glass, fine as hairs */
        Random rnd((int64)(c.getX() * 11.0f + c.getY()));
        for (int i = 0; i < 26; i++) {
            auto p0 = m + Point<float>(rnd.nextFloat() - 0.5f, rnd.nextFloat() - 0.5f) * R * 1.6f;
            auto p1 = p0 + Point<float>(rnd.nextFloat() - 0.5f, rnd.nextFloat() - 0.5f) * R * 0.6f;
            g.setColour(Colours::white.withAlpha(0.06f + 0.12f * rnd.nextFloat()));
            g.drawLine(Line<float>(p0, p1), jmax(0.3f, R * 0.006f));
        }
    }
}

/* an amber LCD: a steel frame, a glass pane, the backlight warm, a faint dot-matrix grid */
void drawLcd(Graphics &g, Rectangle<float> r)
{
    g.setGradientFill(ColourGradient(Colour(0xff7a766c), r.getX(), r.getY(), Colour(0xff23221f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 3.0f);
    auto s = r.reduced(1.6f);
    g.setColour(Colour(0xff090705));
    g.fillRoundedRectangle(s, 2.2f);
    auto glass = s.reduced(1.0f);
    ColourGradient bl(Colour(0xff3a2406), glass.getCentreX(), glass.getCentreY(), Colour(0xff160d03), glass.getRight(), glass.getY(), true);
    g.setGradientFill(bl);
    g.fillRoundedRectangle(glass, 1.6f);
    g.setColour(Colours::black.withAlpha(0.22f));   /* the matrix's dots */
    for (float x = glass.getX() + 1.0f; x < glass.getRight(); x += 1.6f)
        g.fillRect(x, glass.getY(), 0.35f, glass.getHeight());
    for (float y = glass.getY() + 1.0f; y < glass.getBottom(); y += 1.6f)
        g.fillRect(glass.getX(), y, glass.getWidth(), 0.35f);
    for (int i = 0; i < 3; i++) {        /* the frame's inner shadow on the glass */
        g.setColour(Colours::black.withAlpha(0.3f - 0.09f * (float)i));
        g.drawLine(glass.getX(), glass.getY() + (float)i * 0.6f, glass.getRight(), glass.getY() + (float)i * 0.6f, 0.6f);
    }
    ColourGradient sheen(Colours::white.withAlpha(0.07f), glass.getX(), glass.getY(), Colours::white.withAlpha(0.0f), glass.getX() + glass.getWidth() * 0.4f,
                         glass.getBottom(), false);
    g.setGradientFill(sheen);
    g.fillRoundedRectangle(glass, 1.6f);
}

void drawLegend(Graphics &g, const String &s, Rectangle<float> r, float size, Colour c)
{
    g.setColour(c.isTransparent() ? ink() : c);
    g.setFont(font(Face::label, size));
    g.drawFittedText(s, r.toNearestInt(), Justification::centred, 2, 0.8f);
}

void drawTag(Graphics &g, Rectangle<float> r)
{
    Image tag;
    {
        std::lock_guard<std::mutex> l(resMutex);
        auto &t = resources().tag;
        if (!t.isValid()) {              /* (not through ImageCache: nothing global stays behind) */
            Image src = ImageFileFormat::loadFrom(BinaryData::swtdr34mztag_png, BinaryData::swtdr34mztag_pngSize);
            if (!src.isValid())
                return;
            t = Image(Image::ARGB, src.getWidth(), src.getHeight(), true);   /* (in the print's colour, its own alpha) */
            for (int y = 0; y < src.getHeight(); y++)
                for (int x = 0; x < src.getWidth(); x++) {
                    auto p = src.getPixelAt(x, y);
                    float a = p.getFloatAlpha() * (p.getBrightness() > 0.5f ? 1.0f : p.getBrightness() * 2.0f);
                    t.setPixelAt(x, y, ink().withAlpha(a));
                }
        }
        tag = t;
    }
    auto ri = r.getSmallestIntegerContainer();
    g.setImageResamplingQuality(Graphics::highResamplingQuality);
    g.drawImageWithin(tag, ri.getX(), ri.getY(), ri.getWidth(), ri.getHeight(), RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize);
}

/* SOLARIS in wide bone letters, -23 in rust */
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
