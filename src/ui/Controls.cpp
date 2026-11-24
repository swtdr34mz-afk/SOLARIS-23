/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
#include "Controls.h"
#include "solaris.h"

namespace s23
{
using namespace juce;

/* ----------------------------------------------------------------------------------------- Knob --- */
Knob::Knob(APVTS &s, const String &id, const String &leg, Sec sc, Touch *t)
    : param(*s.getParameter(id)), legend(leg), sec(sc), touch(t)
{
    setSliderStyle(Slider::RotaryVerticalDrag);
    setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    setRotaryParameters(-2.36f + MathConstants<float>::pi * 1.5f, 2.36f + MathConstants<float>::pi * 1.5f, true);
    setMouseDragSensitivity(220);
    setVelocityModeParameters(1.0, 1, 0.0, true, ModifierKeys::shiftModifier);   /* SHIFT: fine */
    att = std::make_unique<APVTS::SliderAttachment>(s, id, *this);
    setDoubleClickReturnValue(true, param.convertFrom0to1(param.getDefaultValue()));
    if (dynamic_cast<AudioParameterChoice *>(&param))
        showValue = true;
    onValueChange = [this] {
        if (touch)
            touch->touched(param);
        repaint();
    };
}

Rectangle<float> Knob::knobArea() const
{
    auto r = getLocalBounds().toFloat();
    float below = (legend.isNotEmpty() ? 13.0f : 0.0f) + (showValue ? 13.0f : 0.0f);
    r.removeFromBottom(below);
    float d = knobSize > 0.0f ? knobSize : jmin(r.getWidth(), r.getHeight());
    d = jmin(d, r.getWidth(), r.getHeight());
    return Rectangle<float>(d, d).withCentre({r.getCentreX(), r.getY() + d * 0.5f + (r.getHeight() - d) * 0.5f});
}

void Knob::paint(Graphics &g)
{
    auto *ch = dynamic_cast<AudioParameterChoice *>(&param);
    bool bip = false;
    for (int i = 0; i < sol::NPARAMS; i++)
        if (param.paramID == sol::PARAMS[i].id) {
            bip = sol::PARAMS[i].kind == sol::K_BIP;
            break;
        }
    auto r = getLocalBounds().toFloat();
    if (paintMode == 1) {
        if (legend.isNotEmpty())
            drawLegend(g, legend, r.removeFromBottom(13.0f), 10.5f);
        return;
    }
    drawKnob(g, knobArea(), param.getValue(), secColour(sec), bip, ch != nullptr);
    if (paintMode == 2 && legend.isNotEmpty())
        r.removeFromBottom(13.0f);
    if (showValue) {
        auto v = r.removeFromBottom(13.0f);
        g.setColour(Colour(0xff0e1018));
        g.fillRoundedRectangle(v.reduced(1.0f, 0.5f), 2.5f);
        g.setColour(secColour(sec).brighter(0.3f));
        g.setFont(font(Face::mono, 11.0f));
        g.drawFittedText(param.getCurrentValueAsText().toUpperCase(), v.toNearestInt(), Justification::centred, 1, 0.7f);
    }
    if (legend.isNotEmpty() && paintMode == 0)
        drawLegend(g, legend, r.removeFromBottom(13.0f), 10.5f);
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
    if (ch && e.mods.isPopupMenu()) {    /* a choice: its list on a right click */
        PopupMenu m;
        for (int i = 0; i < ch->choices.size(); i++)
            m.addItem(i + 1, ch->choices[i], true, i == ch->getIndex());
        m.showMenuAsync(PopupMenu::Options().withTargetComponent(this), [this, ch](int r) {
            if (r > 0) {
                ch->beginChangeGesture();
                ch->setValueNotifyingHost(ch->convertTo0to1((float)(r - 1)));
                ch->endChangeGesture();
            }
        });
        return;
    }
    Slider::mouseDown(e);
}

/* ------------------------------------------------------------------------------------------ Key --- */
Key::Key(APVTS &s, const String &id, const String &leg, Sec sc, Touch *t, bool b)
    : param(*s.getParameter(id)), legend(leg), sec(sc), big(b), touch(t), att(param, [this](float) { repaint(); }, nullptr)
{
    att.sendInitialUpdate();
}

void Key::paint(Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    auto lg = legend.isNotEmpty() ? r.removeFromBottom(13.0f) : Rectangle<float>();
    if (paintMode == 1) {
        if (legend.isNotEmpty() && !(big && legend.length() <= 2))
            drawLegend(g, legend, lg, 10.5f);
        return;
    }
    auto k = big ? r.reduced(2.0f) : r.withSizeKeepingCentre(jmin(r.getWidth() - 4.0f, 30.0f), jmin(r.getHeight() - 2.0f, 20.0f));
    drawKey(g, k, on(), down, big ? secColour(sec) : theme().led, big);
    if (big) {
        g.setColour(theme().legend);
        g.setFont(font(Face::title, k.getHeight() * 0.32f));
        if (legend.isNotEmpty() && legend.length() <= 2)
            g.drawText(legend, k.translated(0.0f, down ? 1.0f : 0.0f), Justification::centred, false);
    }
    if (legend.isNotEmpty() && !(big && legend.length() <= 2) && paintMode == 0)
        drawLegend(g, legend, lg, 10.5f);
}

void Key::mouseDown(const MouseEvent &)
{
    down = true;
    att.beginGesture();
    att.setValueAsPartOfGesture(on() ? 0.0f : 1.0f);
    att.endGesture();
    if (touch)
        touch->touched(param);
    repaint();
}

void Key::mouseUp(const MouseEvent &)
{
    down = false;
    repaint();
}

void Key::mouseEnter(const MouseEvent &)
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

void XYPad::paint(Graphics &g)
{
    auto r = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(Colour(0xff0e1018));
    g.fillRoundedRectangle(r, 8.0f);
    g.setColour(secColour(Sec::mod).withAlpha(0.35f));
    for (int i = 1; i < 4; i++) {
        float x = r.getX() + r.getWidth() * (float)i / 4.0f, y = r.getY() + r.getHeight() * (float)i / 4.0f;
        g.drawLine(x, r.getY() + 4.0f, x, r.getBottom() - 4.0f, i == 2 ? 1.2f : 0.6f);
        g.drawLine(r.getX() + 4.0f, y, r.getRight() - 4.0f, y, i == 2 ? 1.2f : 0.6f);
    }
    g.setColour(secColour(Sec::mod).withAlpha(0.6f));
    g.drawRoundedRectangle(r, 8.0f, 1.2f);
    Point<float> p(r.getX() + px.getValue() * r.getWidth(), r.getBottom() - py.getValue() * r.getHeight());
    g.setColour(Colours::black.withAlpha(0.5f));
    g.fillEllipse(Rectangle<float>(26.0f, 26.0f).withCentre(p.translated(0.0f, 2.0f)));
    g.setGradientFill(ColourGradient(Colour(0xff5a5f78), p.x - 6.0f, p.y - 8.0f, Colour(0xff1d2030), p.x + 8.0f, p.y + 10.0f, true));
    g.fillEllipse(Rectangle<float>(24.0f, 24.0f).withCentre(p));
    drawLed(g, p, 3.0f, 1.0f, secColour(Sec::mod).brighter(0.5f));
}

void XYPad::moveTo(Point<float> m)
{
    auto r = getLocalBounds().toFloat().reduced(2.0f);
    ax.setValueAsPartOfGesture(jlimit(0.0f, 1.0f, (m.x - r.getX()) / r.getWidth()));
    ay.setValueAsPartOfGesture(jlimit(0.0f, 1.0f, (r.getBottom() - m.y) / r.getHeight()));
    if (touch)
        touch->touched(px);
}

void XYPad::mouseDown(const MouseEvent &e)
{
    ax.beginGesture();
    ay.beginGesture();
    moveTo(e.position);
}
void XYPad::mouseDrag(const MouseEvent &e) { moveTo(e.position); }
void XYPad::mouseUp(const MouseEvent &)
{
    ax.endGesture();
    ay.endGesture();
}
void XYPad::mouseDoubleClick(const MouseEvent &)   /* back to the middle */
{
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
    /* the slot, the cartridge's label, the program's three lamps */
    g.setColour(Colour(0xff0e1018));
    g.fillRoundedRectangle(r, 6.0f);
    auto lab = r.reduced(6.0f).withTrimmedBottom(16.0f);
    Colour c = secColour(Sec::fx);
    static const uint32 CART_COL[8] = {0xff9d7bd8, 0xff4fa3c7, 0xffe36f9a, 0xff5fbf8f, 0xffd89c3c, 0xff6a6f8c, 0xff58c7c0, 0xffb48a62};
    Colour cc(CART_COL[cart]);
    g.setGradientFill(ColourGradient(cc.brighter(0.15f), lab.getX(), lab.getY(), cc.darker(0.35f), lab.getX(), lab.getBottom(), false));
    g.fillRoundedRectangle(lab, 4.0f);
    g.setColour(Colours::black.withAlpha(0.8f));
    g.setFont(font(Face::title, 11.5f));
    g.drawText(side + "  " + sol::cartName(cart), lab.withHeight(16.0f).reduced(6.0f, 0.0f), Justification::centredLeft, false);
    g.setFont(font(Face::logo, jmin(20.0f, lab.getHeight() - 16.0f)));
    g.drawFittedText(sol::fxName(f), lab.withTrimmedTop(15.0f).reduced(6.0f, 0.0f).toNearestInt(), Justification::centredLeft, 1, 0.7f);
    auto lamps = r.removeFromBottom(16.0f).reduced(8.0f, 2.0f);
    for (int i = 0; i < 3; i++) {
        auto cell = lamps.withWidth(lamps.getWidth() / 3.0f).translated(lamps.getWidth() / 3.0f * (float)i, 0.0f);
        drawLed(g, {cell.getX() + 6.0f, cell.getCentreY()}, 3.0f, i == prog ? 1.0f : 0.0f, c.brighter(0.4f));
        g.setColour(theme().dim);
        g.setFont(font(Face::label, 10.5f));
        g.drawText(String(i + 1), cell.withTrimmedLeft(13.0f), Justification::centredLeft, false);
    }
}

void Cartridge::mouseDown(const MouseEvent &e)
{
    if (touch)
        touch->touched(param);
    auto set = [this](int f) {
        att.setValueAsCompleteGesture((float)jlimit(0, sol::NFX - 1, f));
    };
    auto r = getLocalBounds().toFloat();
    if (e.position.y > r.getBottom() - 16.0f) {   /* a program lamp: that program of this cartridge */
        int i = jlimit(0, 2, (int)((e.position.x - 8.0f) / ((r.getWidth() - 16.0f) / 3.0f)));
        set(fx() / 3 * 3 + i);
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
    g.setFont(font(Face::label, jmin(10.0f, (float)getHeight() * 0.85f)));
    g.setColour(theme().dim);
    g.drawText(side + " " + sol::fxLabel(f, k), getLocalBounds().toFloat(), Justification::centred, false);
}

/* --------------------------------------------------------------------------------------- Plates --- */
Plates::Plates(MidiKeyboardState &k) : ks(k) {}

Rectangle<float> Plates::arrow(int dir) const
{
    auto r = getLocalBounds().toFloat();
    return dir < 0 ? r.withWidth(34.0f).withSizeKeepingCentre(30.0f, 30.0f)
                   : r.withTrimmedLeft(r.getWidth() - 34.0f).withSizeKeepingCentre(30.0f, 30.0f);
}

Rectangle<float> Plates::plate(int i) const
{
    auto r = getLocalBounds().toFloat().reduced(40.0f, 4.0f);
    float w = r.getWidth() / 12.0f;
    static const float H[12] = {0.62f, 0.72f, 0.82f, 0.72f, 0.66f, 0.58f, 0.58f, 0.66f, 0.72f, 0.82f, 0.72f, 0.62f};
    auto p = Rectangle<float>(r.getX() + w * (float)i, r.getY(), w, r.getHeight()).reduced(4.0f, 0.0f);
    return p.withSizeKeepingCentre(p.getWidth(), r.getHeight() * H[i]);
}

int Plates::plateAt(Point<float> p) const
{
    for (int i = 0; i < 12; i++)
        if (plate(i).expanded(2.0f, 8.0f).contains(p))
            return i;
    return -1;
}

void Plates::paint(Graphics &g)
{
    static const char *const NAMES[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    for (int i = 0; i < 12; i++) {
        auto p = plate(i);
        int note = octave * 12 + 12 + i;
        bool lit = ks.isNoteOn(1, note);
        g.setColour(Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(p.translated(0.0f, 2.0f), 5.0f);
        g.setColour(lit ? theme().plateLit : theme().plate);
        g.fillRoundedRectangle(p, 5.0f);
        g.setColour((lit ? Colours::black : theme().legend).withAlpha(lit ? 0.35f : 0.12f));
        for (float y = p.getY() + 8.0f; y < p.getBottom() - 8.0f; y += 5.0f)   /* the plate's grooves */
            g.fillRect(p.getX() + 6.0f, y, p.getWidth() - 12.0f, 2.0f);
        g.setColour(lit ? Colours::black : theme().dim);
        g.setFont(font(Face::label, 10.0f));
        g.drawText(NAMES[i], p.withTrimmedTop(p.getHeight() - 14.0f), Justification::centred, false);
    }
    for (int d : {-1, 1}) {
        auto a = arrow(d);
        drawKey(g, a, false, false, theme().led, false);
        Path tri;
        auto c = a.getCentre();
        tri.addTriangle(c.x - 5.0f, c.y + 3.0f * (float)d, c.x + 5.0f, c.y + 3.0f * (float)d, c.x, c.y - 4.0f * (float)d);
        g.setColour(theme().legend);
        g.fillPath(tri);
    }
    g.setColour(theme().dim);
    g.setFont(font(Face::label, 10.0f));
    g.drawText("C" + String(octave), arrow(-1).translated(0.0f, 22.0f), Justification::centred, false);
}

void Plates::mouseDown(const MouseEvent &e)
{
    for (int d : {-1, 1})
        if (arrow(d).contains(e.position)) {
            octave = jlimit(0, 7, octave + d);
            repaint();
            return;
        }
    int i = plateAt(e.position);
    if (i >= 0) {
        held = octave * 12 + 12 + i;
        ks.noteOn(1, held, jlimit(0.3f, 1.0f, 1.0f - (e.position.y - plate(i).getY()) / plate(i).getHeight() * 0.6f));
        repaint();
    }
}

void Plates::mouseDrag(const MouseEvent &e)
{
    int i = plateAt(e.position);
    int n = i >= 0 ? octave * 12 + 12 + i : -1;
    if (n != held && n >= 0) {           /* sliding over the plates: legato */
        int was = held;
        held = n;
        ks.noteOn(1, held, 0.8f);
        if (was >= 0)
            ks.noteOff(1, was, 0.0f);
        repaint();
    }
}

void Plates::mouseUp(const MouseEvent &)
{
    if (held >= 0)
        ks.noteOff(1, held, 0.0f);
    held = -1;
    repaint();
}
} // namespace s23
