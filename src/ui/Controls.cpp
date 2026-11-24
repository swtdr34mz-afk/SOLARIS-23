/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
#include "Controls.h"
#include "solaris.h"

namespace s23
{
using namespace juce;

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

void Knob::paint(Graphics &g)
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
    if (ch && e.mods.isPopupMenu()) {    /* a choice: its list on a right click */
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
    Slider::mouseDown(e);
}

/* --------------------------------------------------------------------------------------- Switch --- */
Switch::Switch(APVTS &s, const String &id, const String &leg, SW st, Touch *t)
    : param(*s.getParameter(id)), legend(leg), style(st), touch(t), att(param, [this](float) { repaint(); }, nullptr)
{
    att.sendInitialUpdate();
}

void Switch::paint(Graphics &g)
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

void Switch::mouseDown(const MouseEvent &)
{
    down = true;
    att.beginGesture();
    att.setValueAsPartOfGesture(on() ? 0.0f : 1.0f);
    att.endGesture();
    if (touch)
        touch->touched(param);
    repaint();
}

void Switch::mouseUp(const MouseEvent &)
{
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

/* a black field with the four arrows, the stick's round cap where X and Y are */
void XYPad::paint(Graphics &g)
{
    auto r = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(r.translated(0.0f, 1.5f), 5.0f);
    g.setGradientFill(ColourGradient(Colour(0xff2a2a2c), r.getX(), r.getY(), Colour(0xff141415), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 5.0f);
    const Colour w = Colour(0xffe6e6e6);
    auto c = r.getCentre();
    auto tri = [&](float dx, float dy) {
        Path p;
        Point<float> t(c.x + dx * (r.getWidth() * 0.5f - 9.0f), c.y + dy * (r.getHeight() * 0.5f - 9.0f));
        p.addTriangle(t.x + dx * 5.0f, t.y + dy * 5.0f, t.x - dy * 5.0f - dx * 3.0f, t.y - dx * 5.0f - dy * 3.0f,
                      t.x + dy * 5.0f - dx * 3.0f, t.y + dx * 5.0f - dy * 3.0f);
        g.setColour(w);
        g.fillPath(p);
    };
    tri(0, -1);
    tri(0, 1);
    tri(-1, 0);
    tri(1, 0);
    g.setColour(w.withAlpha(0.14f));
    g.drawLine(r.getX() + 16.0f, c.y, r.getRight() - 16.0f, c.y, 1.0f);
    g.drawLine(c.x, r.getY() + 16.0f, c.x, r.getBottom() - 16.0f, 1.0f);
    auto area = r.reduced(18.0f);
    Point<float> p(area.getX() + px.getValue() * area.getWidth(), area.getBottom() - py.getValue() * area.getHeight());
    g.setColour(Colours::black.withAlpha(0.5f));
    g.fillEllipse(Rectangle<float>(34.0f, 34.0f).withCentre(p.translated(1.0f, 3.0f)));
    g.setGradientFill(ColourGradient(Colour(0xff5c5c60), p.x - 8.0f, p.y - 10.0f, Colour(0xff0e0e0f), p.x + 10.0f, p.y + 12.0f, true));
    g.fillEllipse(Rectangle<float>(32.0f, 32.0f).withCentre(p));
    g.setColour(Colour(0xff3a3a3d));
    g.fillEllipse(Rectangle<float>(16.0f, 16.0f).withCentre(p));
    g.setColour(Colours::white.withAlpha(0.1f));
    g.drawEllipse(Rectangle<float>(32.0f, 32.0f).withCentre(p), 1.0f);
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
    /* the slot with the cartridge in it (a green board, its label), the LCD, the 1-2-3 switch */
    auto slot = r.removeFromTop(30.0f);
    g.setColour(Colour(0xff0d0d0e));
    g.fillRoundedRectangle(slot, 3.0f);
    auto card = slot.reduced(5.0f, 7.0f);
    g.setGradientFill(ColourGradient(Colour(0xff2f5a3a), card.getX(), card.getY(), Colour(0xff1d3a25), card.getX(), card.getBottom(), false));
    g.fillRect(card);
    for (int i = 0; i < 6; i++) {        /* its contacts */
        g.setColour(Colour(0xffc9a24a));
        g.fillRect(card.getX() + 3.0f + (float)i * 3.0f, card.getY() + 2.0f, 1.6f, card.getHeight() - 4.0f);
    }
    auto label = card.reduced(card.getWidth() * 0.2f, 2.0f);
    g.setColour(Colour(0xffefe7d3));
    g.fillRect(label);
    g.setColour(Colour(0xff1a1a1a));
    g.setFont(font(Face::title, 9.5f));
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
Plates::Plates(MidiKeyboardState &k) : ks(k) {}

Rectangle<float> Plates::arrow(int dir) const
{
    auto r = getLocalBounds().toFloat();
    auto a = dir < 0 ? r.withWidth(40.0f) : r.withTrimmedLeft(r.getWidth() - 40.0f);
    return a.withSizeKeepingCentre(26.0f, 26.0f).translated(0.0f, 8.0f);
}

/* the plates in a gentle wave */
Rectangle<float> Plates::plate(int i) const
{
    auto r = getLocalBounds().toFloat().reduced(46.0f, 4.0f);
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

void Plates::paint(Graphics &g)
{
    for (int i = 0; i < 12; i++) {
        auto p = plate(i);
        bool lit = ks.isNoteOn(1, octave * 12 + 12 + i);
        if (lit) {
            g.setColour(AMBER_LED.withAlpha(0.25f));
            g.fillRect(p.expanded(4.0f));
        }
        /* brass, darkened with age, rubbed bright where it is touched */
        g.setColour(Colours::black.withAlpha(0.6f));
        g.fillRect(p.expanded(1.5f).translated(0.0f, 1.5f));
        g.setGradientFill(ColourGradient(lit ? Colour(0xffe8c278) : Colour(0xffa98a4c), p.getX(), p.getY(),
                                         lit ? Colour(0xffa8803a) : Colour(0xff5e4a24), p.getRight(), p.getBottom(), false));
        g.fillRect(p);
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

void Plates::mouseDown(const MouseEvent &e)
{
    for (int d : {-1, 1})
        if (arrow(d).expanded(6.0f).contains(e.position)) {
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
