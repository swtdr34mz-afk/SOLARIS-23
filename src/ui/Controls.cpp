/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIUM-23A) */
#include "Controls.h"
#include "solaris.h"

namespace s23
{
using namespace juce;

/* a right click: the host's menu for this parameter (MIDI learn, automation ...), never a change of its value.
   True when the click was a right click (shown or not: either way it does nothing else) */
bool hostMenu(Component &c, const MouseEvent &e, RangedAudioParameter *param)
{
    if (!e.mods.isPopupMenu())
        return false;
    if (param != nullptr)
        if (auto *ed = c.findParentComponentOfClass<AudioProcessorEditor>())
            if (auto *ctx = ed->getHostContext())
                if (auto menu = ctx->getContextMenuForParameter(param))
                    menu->showNativeMenu(ed->getMouseXYRelative());
    return true;
}

static bool inHost(Component &c)
{
    auto *ed = c.findParentComponentOfClass<AudioProcessorEditor>();
    return ed != nullptr && ed->getHostContext() != nullptr;
}

static float LXf() { return -0.63f; }   /* (the light's direction, as Look.cpp's) */
static float LYf() { return -0.78f; }

/* ----------------------------------------------------------------------------------------- Knob --- */
Knob::Knob(APVTS &s, const String &id, const String &leg, KS st, Touch *t)
    : param(*s.getParameter(id)), legend(leg), style(st), touch(t)
{
    setSliderStyle(Slider::RotaryVerticalDrag);
    setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    setMouseDragSensitivity(220);
    setVelocityModeParameters(1.0, 1, 0.0, true, ModifierKeys::shiftModifier);   /* SHIFT: fine */
    att = std::make_unique<APVTS::SliderAttachment>(s, id, *this);
    setDoubleClickReturnValue(true, param.convertFrom0to1(param.getDefaultValue()));
    if (dynamic_cast<AudioParameterChoice *>(&param))
        showValue = true;
    int i = sol::paramIndex(id.toRawUTF8());
    bipolar = i >= 0 && sol::PARAMS[i].kind == sol::K_BIP;
    onValueChange = [this] {
        if (touch)
            touch->touched(param);
        repaint();
    };
}

Rectangle<float> Knob::knobArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromBottom((legend.isNotEmpty() ? 13.0f : 0.0f) + (showValue ? 15.0f : 0.0f));
    float d = knobSize > 0.0f ? knobSize : jmin(r.getWidth(), r.getHeight());
    d = jmin(d, r.getWidth(), r.getHeight());
    return Rectangle<float>(d, d).withCentre(r.getCentre());
}

bool liveResizing(const Component &c)
{
    auto *ed = c.findParentComponentOfClass<AudioProcessorEditor>();
    return ed != nullptr && (bool)ed->getProperties()["liveResize"];
}

/* (drawn again only when it turns: the Mac may repaint everything at once, and a resize only stretches it) */
void Knob::paint(Graphics &g)
{
    String key = String(param.getValue(), 6) + (showValue ? param.getCurrentValueAsText() : String());
    snap.paint(*this, g, key, [this](Graphics &ig) { paintFace(ig); });
}

void Knob::paintFace(Graphics &g)
{
    drawKnob(g, knobArea(), param.getValue(), style, bipolar, ticks);
    auto r = getLocalBounds().toFloat();
    if (legend.isNotEmpty())
        drawLegend(g, legend, r.removeFromBottom(13.0f), 10.5f);
    if (showValue) {
        auto v = r.removeFromBottom(15.0f).reduced(2.0f, 1.0f);
        drawLcd(g, v);
        g.setColour(lcdText());
        g.setFont(font(Face::mono, 10.5f));
        g.drawFittedText(param.getCurrentValueAsText().toUpperCase(), v.toNearestInt(), Justification::centred, 1, 0.7f);
    }
}

void Knob::mouseEnter(const MouseEvent &e)
{
    if (touch)
        touch->touched(param);
    Slider::mouseEnter(e);
}

void Knob::mouseDown(const MouseEvent &e)
{
    auto *ch = dynamic_cast<AudioParameterChoice *>(&param);
    if (ch && e.mods.isPopupMenu() && !inHost(*this)) {   /* a choice, the standalone app: its list on a right click */
        PopupMenu m;
        for (int i = 0; i < ch->choices.size(); i++)
            m.addItem(i + 1, ch->choices[i], true, i == ch->getIndex());
        m.showMenuAsync(PopupMenu::Options().withTargetComponent(this), [ch](int r) {
            if (r > 0) {
                ch->beginChangeGesture();
                ch->setValueNotifyingHost(ch->convertTo0to1((float)(r - 1)));
                ch->endChangeGesture();
            }
        });
        return;
    }
    if (hostMenu(*this, e, &param))      /* (in a host: its menu) */
        return;
    Slider::mouseDown(e);
}

void Knob::mouseDrag(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())            /* (a right drag turns nothing) */
        return;
    Slider::mouseDrag(e);
}

void Knob::mouseUp(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())
        return;
    Slider::mouseUp(e);
}

void Knob::mouseDoubleClick(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())
        return;
    Slider::mouseDoubleClick(e);
}

/* --------------------------------------------------------------------------------------- Switch --- */
Switch::Switch(APVTS &s, const String &id, const String &leg, SW st, Touch *t)
    : param(*s.getParameter(id)), legend(leg), style(st), touch(t), att(param, [this](float) { repaint(); }, nullptr)
{
    att.sendInitialUpdate();
}

void Switch::paint(Graphics &g)
{
    snap.paint(*this, g, String(on() ? 1 : 0) + (down ? "d" : "u"), [this](Graphics &ig) { paintFace(ig); });
}

void Switch::paintFace(Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    if (style == SW::key) {              /* a drone key: its number printed below */
        auto lg = r.removeFromBottom(14.0f);
        drawDroneKey(g, r, on(), down);
        drawLegend(g, legend, lg, 11.0f);
        return;
    }
    auto lg = legend.isNotEmpty() ? r.removeFromBottom(13.0f) : Rectangle<float>();
    if (style == SW::toggle)
        drawToggle(g, r.withSizeKeepingCentre(jmin(r.getWidth(), 22.0f), jmin(r.getHeight(), 34.0f)), on());
    else
        /* (the black ON buttons stay dark: their state is in the row of lamps, as on the instrument) */
        drawButton(g, r.withSizeKeepingCentre(jmin(r.getWidth(), 22.0f), jmin(r.getHeight(), 22.0f)), style == SW::buttonGrey && on(),
                   down, style == SW::buttonGrey);
    if (legend.isNotEmpty())
        drawLegend(g, legend, lg, 10.0f);
}

void Switch::mouseDown(const MouseEvent &e)
{
    if (hostMenu(*this, e, &param))
        return;
    down = true;
    att.beginGesture();
    att.setValueAsPartOfGesture(momentary ? 1.0f : on() ? 0.0f : 1.0f);
    if (!momentary)
        att.endGesture();
    if (touch)
        touch->touched(param);
    repaint();
}

void Switch::mouseUp(const MouseEvent &)
{
    if (momentary && down) {             /* (the key let go: the voice closes, unless HOLD keeps it) */
        att.setValueAsPartOfGesture(0.0f);
        att.endGesture();
    }
    down = false;
    repaint();
}

void Switch::mouseEnter(const MouseEvent &)
{
    if (touch)
        touch->touched(param);
}

/* ---------------------------------------------------------------------------------------- XYPad --- */
XYPad::XYPad(APVTS &s, const String &xid, const String &yid, Touch *t)
    : px(*s.getParameter(xid)), py(*s.getParameter(yid)), touch(t), ax(px, [this](float) { repaint(); }, nullptr),
      ay(py, [this](float) { repaint(); }, nullptr)
{
    ax.sendInitialUpdate();
    ay.sendInitialUpdate();
}

/* a recessed black field with the four arrows; the stick: a rubber boot with its folds, a steel shaft, a knurled black
 * top, leaning where X and Y are */
void XYPad::paint(Graphics &g)
{
    auto r = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(Colour(0xff060606));
    g.fillRoundedRectangle(r, 5.0f);
    auto f = r.reduced(2.0f);
    g.setGradientFill(ColourGradient(Colour(0xff0f0f0e), f.getX(), f.getY(), Colour(0xff201f1c), f.getX(), f.getBottom(), false));
    g.fillRoundedRectangle(f, 4.0f);
    for (int i = 0; i < 5; i++) {        /* the well's inner shadow */
        auto e = f.reduced((float)i * 0.8f);
        g.setColour(Colours::black.withAlpha(0.35f - 0.06f * (float)i));
        g.drawLine(e.getX() + 3.0f, e.getY(), e.getRight() - 3.0f, e.getY(), 0.8f);
        g.drawLine(e.getX(), e.getY() + 3.0f, e.getX(), e.getBottom() - 3.0f, 0.8f);
    }
    const Colour w = ink();
    auto c = f.getCentre();
    auto tri = [&](float dx, float dy) {
        Path p;
        Point<float> t(c.x + dx * (f.getWidth() * 0.5f - 9.0f), c.y + dy * (f.getHeight() * 0.5f - 9.0f));
        p.addTriangle(t.x + dx * 5.0f, t.y + dy * 5.0f, t.x - dy * 5.0f - dx * 3.0f, t.y - dx * 5.0f - dy * 3.0f, t.x + dy * 5.0f - dx * 3.0f,
                      t.y + dx * 5.0f - dy * 3.0f);
        g.setColour(w.withAlpha(0.85f));
        g.fillPath(p);
    };
    tri(0, -1);
    tri(0, 1);
    tri(-1, 0);
    tri(1, 0);
    auto area = f.reduced(20.0f);
    Point<float> p(area.getX() + px.getValue() * area.getWidth(), area.getBottom() - py.getValue() * area.getHeight());
    /* the boot: concentric folds, darker in their valleys, around the centre */
    const float B = jmin(f.getWidth(), f.getHeight()) * 0.30f;
    for (int i = 6; i >= 0; i--) {
        float rr = B * (0.35f + 0.1f * (float)i);
        auto bc = c + (p - c) * (0.06f * (float)(6 - i));
        ColourGradient fold(Colour(0xff3a3935), bc.x + LXf() * rr * 0.4f, bc.y + LYf() * rr * 0.4f, Colour(0xff0b0b0a), bc.x + rr, bc.y, true);
        g.setGradientFill(fold);
        g.fillEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(bc));
    }
    /* the shaft and the top */
    g.setColour(Colours::black.withAlpha(0.5f));
    g.drawLine(Line<float>(c, p).withShortenedStart(0.0f), 7.0f);
    ColourGradient sh(Colour(0xff4a4740), c.x - 3.0f, c.y, Colour(0xffd8d3c6), c.x + 1.0f, c.y, false);
    g.setGradientFill(sh);
    g.drawLine(Line<float>(c, p), 4.0f);
    g.setColour(Colours::black.withAlpha(0.55f));
    g.fillEllipse(Rectangle<float>(34.0f, 34.0f).withCentre(p.translated(2.0f, 4.0f)));
    const float kr = 15.0f;
    for (int i = 0; i < 36; i++) {       /* its knurled edge */
        float a0 = MathConstants<float>::twoPi * (float)i / 36.0f, a1 = a0 + MathConstants<float>::twoPi / 36.0f;
        Path w2;
        w2.addPieSegment(p.x - kr, p.y - kr, kr * 2.0f, kr * 2.0f, a0, a1 + 0.01f, 0.8f);
        float l = 0.5f + 0.5f * std::cos(a0 - MathConstants<float>::halfPi - std::atan2(LYf(), LXf()));
        g.setColour(Colour(0xff222220).withMultipliedBrightness(0.6f + 1.6f * l * l));
        g.fillPath(w2);
    }
    ColourGradient top(Colour(0xff4a4945), p.x + LXf() * 6.0f, p.y + LYf() * 6.0f, Colour(0xff0c0c0b), p.x + kr, p.y, true);
    g.setGradientFill(top);
    g.fillEllipse(Rectangle<float>(kr * 1.6f, kr * 1.6f).withCentre(p));
    g.setColour(Colours::white.withAlpha(0.18f));
    g.fillEllipse(Rectangle<float>(5.0f, 3.5f).withCentre(p + Point<float>(LXf(), LYf()) * 6.0f));
}

void XYPad::moveTo(Point<float> m)
{
    auto area = getLocalBounds().toFloat().reduced(20.0f);
    ax.setValueAsPartOfGesture(jlimit(0.0f, 1.0f, (m.x - area.getX()) / area.getWidth()));
    ay.setValueAsPartOfGesture(jlimit(0.0f, 1.0f, (area.getBottom() - m.y) / area.getHeight()));
    if (touch)
        touch->touched(px);
}

void XYPad::mouseDown(const MouseEvent &e)
{
    /* a right click: the host's menu for X in the left half of the field, for Y in the right half */
    if (hostMenu(*this, e, e.position.x < (float)getWidth() * 0.5f ? &px : &py))
        return;
    dragging = true;
    ax.beginGesture();
    ay.beginGesture();
    moveTo(e.position);
}
void XYPad::mouseDrag(const MouseEvent &e)
{
    if (dragging)
        moveTo(e.position);
}
void XYPad::mouseUp(const MouseEvent &)
{
    if (!dragging)
        return;
    dragging = false;
    ax.endGesture();
    ay.endGesture();
}
void XYPad::mouseDoubleClick(const MouseEvent &e)   /* back to the middle */
{
    if (e.mods.isPopupMenu())
        return;
    ax.setValueAsCompleteGesture(0.5f);
    ay.setValueAsCompleteGesture(0.5f);
}

/* ------------------------------------------------------------------------------------ Cartridge --- */
Cartridge::Cartridge(APVTS &s, const String &id, const String &sd, Touch *t)
    : param(*s.getParameter(id)), side(sd), touch(t), att(param, [this](float) { repaint(); }, nullptr)
{
    att.sendInitialUpdate();
}

int Cartridge::fx() const { return jlimit(0, sol::NFX - 1, (int)std::lround(param.convertFrom0to1(param.getValue()))); }

void Cartridge::paint(Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    int f = fx(), cart = f / 3, prog = f % 3;
    /* the slot (a dark well, its walls in shadow), the cartridge in it: a green board with gold fingers and traces,
     * a paper label, a little dog-eared */
    auto slot = r.removeFromTop(30.0f);
    g.setColour(Colour(0xff050505));
    g.fillRoundedRectangle(slot, 3.0f);
    for (int i = 0; i < 4; i++) {
        g.setColour(Colours::black.withAlpha(0.5f - 0.1f * (float)i));
        g.drawLine(slot.getX() + 2.0f, slot.getY() + (float)i * 0.7f + 0.5f, slot.getRight() - 2.0f, slot.getY() + (float)i * 0.7f + 0.5f, 0.7f);
    }
    g.setColour(Colour(0xffd8d2c4).withAlpha(0.15f));
    g.drawLine(slot.getX() + 3.0f, slot.getBottom() - 0.5f, slot.getRight() - 3.0f, slot.getBottom() - 0.5f, 0.8f);
    auto card = slot.reduced(5.0f, 6.0f);
    g.setColour(Colours::black.withAlpha(0.6f));
    g.fillRect(card.translated(0.8f, 1.2f));
    ColourGradient pcb(Colour(0xff2f6a40), card.getX(), card.getY(), Colour(0xff163a22), card.getX(), card.getBottom(), false);
    pcb.addColour(0.15, Colour(0xff3d7c4e));
    g.setGradientFill(pcb);
    g.fillRect(card);
    Random rnd(cart * 31 + 7);
    for (int i = 0; i < 8; i++) {        /* traces */
        float y = card.getY() + 2.0f + rnd.nextFloat() * (card.getHeight() - 4.0f);
        float x0 = card.getX() + card.getWidth() * (0.2f + 0.6f * rnd.nextFloat());
        g.setColour(Colour(0xff5fa06c).withAlpha(0.5f));
        g.drawLine(x0, y, jmin(card.getRight() - 2.0f, x0 + 6.0f + rnd.nextFloat() * 20.0f), y, 0.5f);
    }
    for (int i = 0; i < 7; i++) {        /* the gold fingers */
        auto f = Rectangle<float>(card.getX() + 2.5f + (float)i * 2.8f, card.getY() + 2.0f, 1.7f, card.getHeight() - 4.0f);
        ColourGradient au(Colour(0xfff2d27a), f.getX(), f.getY(), Colour(0xff8a6420), f.getRight(), f.getY(), false);
        g.setGradientFill(au);
        g.fillRect(f);
    }
    auto label = card.reduced(card.getWidth() * 0.2f, 2.0f).withTrimmedLeft(4.0f);
    g.setColour(Colours::black.withAlpha(0.4f));
    g.fillRect(label.translated(0.5f, 0.7f));
    ColourGradient paper(Colour(0xfff1e8d0), label.getX(), label.getY(), Colour(0xffcfc2a0), label.getRight(), label.getBottom(), false);
    g.setGradientFill(paper);
    g.fillRect(label);
    g.setColour(Colour(0xff8a7a58).withAlpha(0.35f));   /* a stain */
    g.fillEllipse(label.getRight() - label.getHeight() * 1.4f, label.getY() + 1.0f, label.getHeight() * 1.1f, label.getHeight() * 0.8f);
    Path ear;                            /* the dog-ear */
    ear.addTriangle(label.getRight() - 4.0f, label.getY(), label.getRight(), label.getY(), label.getRight(), label.getY() + 4.0f);
    g.setColour(Colour(0xff163a22));
    g.fillPath(ear);
    g.setColour(Colour(0xff1a1a1a));
    g.setFont(font(Face::title, 9.5f).withExtraKerningFactor(0.08f));
    g.drawText(sol::cartName(cart), label, Justification::centred, false);
    r.removeFromTop(4.0f);
    auto row = r.removeFromTop(26.0f);
    auto lcd = row.withTrimmedRight(30.0f);
    drawLcd(g, lcd);
    g.setColour(lcdText());
    g.setFont(font(Face::mono, 12.0f));
    g.drawFittedText(side + " " + String(prog + 1) + " " + sol::fxName(f), lcd.reduced(5.0f, 0.0f).toNearestInt(), Justification::centredLeft, 1, 0.7f);
    auto sw = row.withTrimmedLeft(row.getWidth() - 26.0f);
    drawToggle(g, sw.withSizeKeepingCentre(20.0f, 26.0f), prog == 0);
}

void Cartridge::mouseDown(const MouseEvent &e)
{
    if (hostMenu(*this, e, &param))
        return;
    if (touch)
        touch->touched(param);
    auto set = [this](int f) { att.setValueAsCompleteGesture((float)jlimit(0, sol::NFX - 1, f)); };
    auto r = getLocalBounds().toFloat();
    if (e.position.y > 32.0f && e.position.x > r.getRight() - 30.0f) {   /* the 1-2-3 switch: this cartridge's next program */
        set(fx() / 3 * 3 + (fx() % 3 + 1) % 3);
        return;
    }
    PopupMenu m;
    for (int c = 0; c < sol::NCART; c++) {
        PopupMenu sub;
        for (int p = 0; p < 3; p++)
            sub.addItem(c * 3 + p + 1, String(p + 1) + "  " + sol::fxName(c * 3 + p), true, fx() == c * 3 + p);
        m.addSubMenu(sol::cartName(c), sub, true, nullptr, fx() / 3 == c);
    }
    m.showMenuAsync(PopupMenu::Options().withTargetComponent(this), [set](int res) {
        if (res > 0)
            set(res - 1);
    });
}

/* -------------------------------------------------------------------------------------- FxLabel --- */
FxLabel::FxLabel(APVTS &s, const String &id, const String &sd, int kk)
    : param(*s.getParameter(id)), side(sd), k(kk), att(param, [this](float) { repaint(); }, nullptr)
{
    setInterceptsMouseClicks(false, false);
    att.sendInitialUpdate();
}

void FxLabel::paint(Graphics &g)
{
    int f = jlimit(0, sol::NFX - 1, (int)std::lround(param.convertFrom0to1(param.getValue())));
    g.setFont(font(Face::label, 9.5f));
    g.setColour(ink().withAlpha(0.8f));
    g.drawText(side + " " + String(sol::fxLabel(f, k)).toLowerCase(), getLocalBounds().toFloat(), Justification::centred, false);
}

/* --------------------------------------------------------------------------------------- Plates --- */
namespace
{
const char *const NOTE_NAME[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
}

Plates::Plates(MidiKeyboardState &k, APVTS &s, int ch) : ks(k), apvts(s), channel(ch) {}

Plates::~Plates()
{
    if (preview >= 0)
        ks.noteOff(channel, preview, 0.0f);
    if (held >= 0)
        ks.noteOff(channel, held, 0.0f);
}

float Plates::offset(int i) const
{
    float v = apvts.getRawParameterValue("plate_" + String(i + 1))->load();
    float st = (v - 0.5f) * 24.0f;
    bool grid = apvts.getRawParameterValue("grid")->load() >= 0.5f;
    return grid ? std::round(st) : st;
}

String Plates::plateName(int i) const
{
    float pitch = (float)baseNote(i) + offset(i);
    int n = (int)std::lround(pitch);
    int ct = (int)std::lround((pitch - (float)n) * 100.0f);
    String s = String(NOTE_NAME[((n % 12) + 12) % 12]) + String(n / 12 - 1);
    if (ct != 0)
        s << " " << (ct > 0 ? "+" : "") << ct << " ct";
    return s;
}

Rectangle<float> Plates::arrow(int dir) const
{
    auto r = getLocalBounds().toFloat();
    auto a = dir < 0 ? r.withWidth(40.0f) : r.withTrimmedLeft(r.getWidth() - 40.0f);
    return a.withSizeKeepingCentre(26.0f, 26.0f).translated(0.0f, 8.0f);
}

/* the plates in a gentle wave, room for their notes below */
Rectangle<float> Plates::plate(int i) const
{
    auto r = getLocalBounds().toFloat().reduced(46.0f, 2.0f).withTrimmedBottom(14.0f);
    float w = r.getWidth() / 12.0f;
    static const float H[12] = {0.62f, 0.74f, 0.88f, 0.74f, 0.64f, 0.56f, 0.56f, 0.64f, 0.74f, 0.88f, 0.74f, 0.62f};
    auto p = Rectangle<float>(r.getX() + w * (float)i, r.getY(), w, r.getHeight()).reduced(w * 0.14f, 0.0f);
    float h = r.getHeight() * H[i];
    return p.withHeight(h).withY(r.getBottom() - h - (r.getHeight() - h) * 0.5f);
}

int Plates::plateAt(Point<float> p) const
{
    for (int i = 0; i < 12; i++)
        if (plate(i).expanded(2.0f, 6.0f).contains(p))
            return i;
    return -1;
}

juce::int64 Plates::signature() const
{
    juce::int64 sig = (juce::int64)octave * 1000003 + (edit ? 7 : 0) + (juce::int64)(selected + 1) * 131;
    for (int i = 0; i < 12; i++) {
        if (ks.isNoteOn(channel, baseNote(i)))
            sig ^= (juce::int64)1 << (40 + i);
        sig = sig * 31 + (juce::int64)std::lround(offset(i) * 1000.0f);
    }
    return sig;
}

bool Plates::changed() { return signature() != drawnSig; }

void Plates::paint(Graphics &g)
{
    drawnSig = signature();
    for (int i = 0; i < 12; i++) {
        auto p = plate(i);
        bool lit = ks.isNoteOn(channel, baseNote(i));
        bool sel = edit && i == selected;
        if (lit || sel) {
            g.setColour(AMBER_LED.withAlpha(sel ? 0.4f : 0.25f));
            g.fillRect(p.expanded(sel ? 5.0f : 4.0f));
        }
        /* brass, darkened with age, rubbed bright where it is touched */
        g.setColour(Colours::black.withAlpha(0.6f));
        g.fillRect(p.expanded(1.5f).translated(0.0f, 1.5f));
        g.setGradientFill(ColourGradient(lit ? Colour(0xffe8c278) : Colour(0xffa98a4c), p.getX(), p.getY(),
                                         lit ? Colour(0xffa8803a) : Colour(0xff5e4a24), p.getRight(), p.getBottom(), false));
        g.fillRect(p);
        g.setColour(Colour(0xfff0d89a).withAlpha(0.45f));   /* its edges: lit above and left, dark below and right */
        g.drawLine(p.getX(), p.getY() + 0.4f, p.getRight(), p.getY() + 0.4f, 0.8f);
        g.drawLine(p.getX() + 0.4f, p.getY(), p.getX() + 0.4f, p.getBottom(), 0.8f);
        g.setColour(Colour(0xff1c150a).withAlpha(0.8f));
        g.drawLine(p.getX(), p.getBottom() - 0.4f, p.getRight(), p.getBottom() - 0.4f, 0.8f);
        g.drawLine(p.getRight() - 0.4f, p.getY(), p.getRight() - 0.4f, p.getBottom(), 0.8f);
        g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.16f), p.getCentreX(), p.getCentreY(), Colours::transparentWhite,
                                         p.getCentreX() + p.getWidth(), p.getCentreY(), true));
        g.fillRect(p);
        for (float y = p.getY() + 4.0f; y < p.getBottom() - 3.0f; y += 4.0f) {   /* its grooves */
            g.setColour(Colour(0xff1c150a).withAlpha(0.85f));
            g.fillRect(p.getX() + 3.0f, y, p.getWidth() - 6.0f, 1.4f);
            g.setColour(Colours::white.withAlpha(0.08f));
            g.fillRect(p.getX() + 3.0f, y + 1.4f, p.getWidth() - 6.0f, 0.6f);
        }
        Random rnd(i * 77 + 5);          /* verdigris and dirt */
        for (int k = 0; k < 10; k++) {
            g.setColour((rnd.nextBool() ? Colour(0xff4f7a5c) : Colour(0xff20170c)).withAlpha(0.15f + 0.2f * rnd.nextFloat()));
            g.fillEllipse(p.getX() + rnd.nextFloat() * p.getWidth(), p.getY() + rnd.nextFloat() * p.getHeight(), 2.0f + rnd.nextFloat() * 4.0f,
                          2.0f + rnd.nextFloat() * 3.0f);
        }
        /* the note it plays, printed under it (a tuned plate's in amber) */
        bool tuned = std::fabs(offset(i)) > 0.004f;
        g.setColour(sel ? AMBER_LED : tuned ? AMBER_LED.withAlpha(0.85f) : ink().withAlpha(0.75f));
        g.setFont(font(Face::label, 9.5f));
        g.drawFittedText(plateName(i), Rectangle<int>((int)p.getX() - 8, getHeight() - 14, (int)p.getWidth() + 16, 13), Justification::centred, 1, 0.7f);
    }
    for (int d : {-1, 1}) {
        auto a = arrow(d);
        drawButton(g, a, false, false, false);
        g.setColour(Colour(0xffe8e8e8));
        Path t;
        auto c = a.getCentre().translated(0.0f, -26.0f);
        t.addTriangle(c.x - 6.0f, c.y + 4.0f * (float)d, c.x + 6.0f, c.y + 4.0f * (float)d, c.x, c.y - 6.0f * (float)d);
        g.fillPath(t);
    }
    g.setColour(Colour(0xffbdbdbd));
    g.setFont(font(Face::mono, 11.0f));
    g.drawText("C" + String(octave), arrow(-1).translated(0.0f, 24.0f), Justification::centred, false);
}

void Plates::select(int i)
{
    if (preview >= 0)                    /* (the plate before: quiet) */
        ks.noteOff(channel, preview, 0.0f);
    preview = -1;
    selected = i;
    if (edit && i >= 0) {                /* the selected plate sounds while it is selected */
        preview = baseNote(i);
        ks.noteOn(channel, preview, 0.8f);
    }
    if (onSelect)
        onSelect();
    repaint();
}

void Plates::setEdit(bool on)
{
    edit = on;
    select(on ? jmax(0, selected) : -1);
}

void Plates::mouseDown(const MouseEvent &e)
{
    if (e.mods.isPopupMenu()) {          /* a right click: the host's menu for that plate's tuning, no note */
        int i = plateAt(e.position);
        hostMenu(*this, e, i >= 0 ? apvts.getParameter("plate_" + String(i + 1)) : nullptr);
        return;
    }
    for (int d : {-1, 1})
        if (arrow(d).expanded(6.0f).contains(e.position)) {
            bool was = edit && selected >= 0;
            if (was && preview >= 0)
                ks.noteOff(channel, preview, 0.0f);
            preview = -1;
            octave = jlimit(0, 7, octave + d);
            if (was)
                select(selected);
            repaint();
            return;
        }
    int i = plateAt(e.position);
    if (i < 0)
        return;
    if (edit) {
        select(i);
        return;
    }
    held = baseNote(i);
    ks.noteOn(channel, held, jlimit(0.3f, 1.0f, 1.0f - (e.position.y - plate(i).getY()) / plate(i).getHeight() * 0.6f));
    repaint();
}

void Plates::mouseDrag(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())
        return;
    if (edit)
        return;
    int i = plateAt(e.position);
    int n = i >= 0 ? baseNote(i) : -1;
    if (n != held && n >= 0) {           /* sliding over the plates: legato */
        int was = held;
        held = n;
        ks.noteOn(channel, held, 0.8f);
        if (was >= 0)
            ks.noteOff(channel, was, 0.0f);
        repaint();
    }
}

void Plates::mouseUp(const MouseEvent &e)
{
    if (e.mods.isPopupMenu())
        return;
    if (held >= 0)
        ks.noteOff(channel, held, 0.0f);
    held = -1;
    repaint();
}

/* ---------------------------------------------------------------------------------- PlateEditor --- */
PlateEditor::PlateEditor(Plates &p, APVTS &s, Touch *t) : plates(p), apvts(s), touch(t)
{
    knob.setSliderStyle(Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    knob.setRange(0.0, 1.0);
    knob.setMouseDragSensitivity(400);   /* (two octaves over a long drag: fine enough for cents; SHIFT finer) */
    knob.setVelocityModeParameters(1.0, 1, 0.0, true, ModifierKeys::shiftModifier);
    knob.setDoubleClickReturnValue(true, 0.5);
    knob.setAlpha(0.0f);                 /* (the knob is drawn by paint; the slider only takes the mouse) */
    addAndMakeVisible(knob);
    knob.onDragStart = [this] {
        if (auto *p = param())
            p->beginChangeGesture();
    };
    knob.onDragEnd = [this] {
        if (auto *p = param())
            p->endChangeGesture();
    };
    knob.onValueChange = [this] {
        if (auto *p = param()) {
            p->setValueNotifyingHost((float)knob.getValue());
            if (touch)
                touch->touched(*p);
        }
        repaint();
    };
    plates.onSelect = [this] {
        if (auto *p = param())
            knob.setValue(p->getValue(), dontSendNotification);
        knob.setEnabled(param() != nullptr);
        repaint();
    };
    knob.setEnabled(false);
    startTimerHz(15);                    /* (a preset or the host may change a tuning) */
}

RangedAudioParameter *PlateEditor::param() const
{
    int i = plates.selectedPlate();
    return plates.editing() && i >= 0 ? apvts.getParameter("plate_" + String(i + 1)) : nullptr;
}

void PlateEditor::resized() { knob.setBounds(knobBox().toNearestInt()); }

void PlateEditor::timerCallback()
{
    if (auto *p = param())
        if (!knob.isMouseButtonDown() && std::fabs(knob.getValue() - p->getValue()) > 1e-6)
            knob.setValue(p->getValue(), dontSendNotification);
    repaint();
}

void PlateEditor::paint(Graphics &g)
{
    const bool on = plates.editing();
    drawButton(g, editBox(), on, false, true);
    g.setColour(ink());
    g.setFont(font(Face::label, 10.0f));
    g.drawText("EDIT", editBox().translated(0.0f, 44.0f).withHeight(12.0f), Justification::centred, false);
    auto *p = param();
    auto kb = knobBox();
    drawKnob(g, kb.withTrimmedBottom(14.0f).withSizeKeepingCentre(60.0f, 60.0f), p ? p->getValue() : 0.5f, KS::blue, true, true);
    g.setColour(ink().withAlpha(on ? 1.0f : 0.45f));
    g.drawText("NOTE", kb.withTrimmedTop(kb.getHeight() - 13.0f), Justification::centred, false);
    drawLcd(g, lcdBox());
    g.setColour(lcdText().withAlpha(on ? 1.0f : 0.35f));
    g.setFont(font(Face::mono, 11.5f));
    String t = on && plates.selectedPlate() >= 0 ? "PLATE " + String(plates.selectedPlate() + 1) + "  " + plates.plateName(plates.selectedPlate())
                                                 : String("EDIT: TUNE A PLATE");
    g.drawFittedText(t, lcdBox().reduced(6.0f, 0.0f).toNearestInt(), Justification::centredLeft, 1, 0.7f);
}

void PlateEditor::mouseDown(const MouseEvent &e)
{
    if (hostMenu(*this, e, param()))     /* (the NOTE of the selected plate) */
        return;
    if (editBox().expanded(4.0f).contains(e.position))
        plates.setEdit(!plates.editing());
    repaint();
}
} // namespace s23
