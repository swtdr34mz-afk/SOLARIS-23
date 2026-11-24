/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
#include "PluginEditor.h"

using namespace s23;
using namespace juce;

/* the header's controls: the program (click: the list), its arrows, SAVE, the readout */
struct SolarisEditor::Header : Component {
    SolarisEditor &ed;
    explicit Header(SolarisEditor &e) : ed(e) {}
    Rectangle<float> prog() const { return {560.0f, 14.0f, 300.0f, 34.0f}; }
    Rectangle<float> prev() const { return {520.0f, 14.0f, 34.0f, 34.0f}; }
    Rectangle<float> next() const { return {866.0f, 14.0f, 34.0f, 34.0f}; }
    Rectangle<float> save() const { return {908.0f, 14.0f, 64.0f, 34.0f}; }
    Rectangle<float> read() const { return {1010.0f, 14.0f, 534.0f, 34.0f}; }
    void paint(Graphics &g) override
    {
        drawLogo(g, {18.0f, 12.0f, 330.0f, 40.0f});
        g.setColour(theme().dim);
        g.setFont(font(Face::label, 11.0f));
        g.drawText(juce::String::fromUTF8("MICROTONAL DRONE  ·  AMBIENT MACHINE"), Rectangle<float>(360.0f, 14.0f, 160.0f, 34.0f), Justification::centredLeft, true);
        auto box = [&](Rectangle<float> r) {
            g.setColour(Colour(0xff0e1018));
            g.fillRoundedRectangle(r, 5.0f);
            g.setColour(secColour(Sec::drone).withAlpha(0.35f));
            g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
        };
        box(prog());
        g.setColour(secColour(Sec::drone));
        g.setFont(font(Face::mono, 18.0f));
        int p = ed.proc.getCurrentProgram();
        g.drawText(String(p + 1).paddedLeft('0', 2) + "  " + ed.proc.getProgramName(p), prog().reduced(10.0f, 0.0f),
                   Justification::centredLeft, true);
        for (int d : {-1, 1}) {
            auto a = d < 0 ? prev() : next();
            drawKey(g, a, false, false, theme().led, false);
            Path t;
            auto c = a.getCentre();
            t.addTriangle(c.x + 4.0f * (float)d, c.y, c.x - 3.0f * (float)d, c.y - 6.0f, c.x - 3.0f * (float)d, c.y + 6.0f);
            g.setColour(theme().legend);
            g.fillPath(t);
        }
        drawKey(g, save(), false, false, theme().led, false);
        g.setColour(theme().legend);
        g.setFont(font(Face::title, 12.0f));
        g.drawText("SAVE", save(), Justification::centred, false);
        box(read());
        if (ed.inHand) {
            g.setFont(font(Face::mono, 15.0f));
            g.setColour(theme().dim);
            g.drawText(ed.inHand->getName(40), read().reduced(12.0f, 0.0f), Justification::centredLeft, true);
            g.setColour(theme().legend);
            g.drawText(ed.inHand->getCurrentValueAsText(), read().reduced(12.0f, 0.0f), Justification::centredRight, true);
        }
    }
    void mouseDown(const MouseEvent &e) override
    {
        int n = ed.proc.getNumPrograms(), p = ed.proc.getCurrentProgram();
        if (prog().contains(e.position))
            ed.programMenu();
        else if (prev().contains(e.position))
            ed.proc.setCurrentProgram((p + n - 1) % n);
        else if (next().contains(e.position))
            ed.proc.setCurrentProgram((p + 1) % n);
        else if (save().contains(e.position))
            ed.saveProgram();
        repaint();
    }
};

SolarisEditor::SolarisEditor(SolarisProcessor &p) : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&look);
    Builder b{proc.apvts, page, this};
    b.keys = &proc.keyboardState;
    b.meters.light = [this] { return proc.light.load(); };
    b.meters.lfo = [this](int l) { return proc.lfo[l].load(); };
    b.meters.voice = [this](int v) { return proc.voice[v].load(); };
    b.meters.step = [this] { return proc.seqStep.load(); };

    /* row 1: DRONE 1, DRONE 2, the effector, DRONE 4, DRONE 5 */
    b.classic(1, {16, 64, 300, 290});
    b.classic(2, {324, 64, 300, 290});
    b.effector({632, 64, 296, 290});
    b.classic(4, {936, 64, 300, 290});
    b.classic(5, {1244, 64, 300, 290});
    /* row 2: DRONE 3, VCO A, the mixer and the filter, VCO B, DRONE 6 */
    b.chaos(3, {16, 362, 300, 290});
    b.vco(0, {324, 362, 300, 290});
    b.mixer({632, 362, 296, 140});
    b.filter({632, 510, 296, 142});
    b.vco(1, {936, 362, 300, 290});
    b.chaos(6, {1244, 362, 300, 290});
    /* row 3: the modulation */
    b.lfo(0, {16, 660, 190, 150});
    b.joystick({214, 660, 270, 150});
    b.sequencer({492, 660, 636, 150});
    b.light({1136, 660, 210, 150});
    b.lfo(1, {1354, 660, 190, 150});
    /* row 4: the touch plates, the drone keys, glide and tuning */
    b.plates({16, 818, 1040, 140});
    b.droneKeys({1064, 818, 260, 140});
    b.global({1332, 818, 212, 140});
    page.extra = [](Graphics &g) {
        drawTag(g, {1340.0f, 966.0f, 200.0f, 26.0f});
        g.setColour(theme().dim);
        g.setFont(font(Face::label, 10.5f));
        g.drawText(juce::String::fromUTF8("SOLARIS-23 by swtdr34mz  ·  unofficial, inspired by the drone machines of the 2020s"), Rectangle<float>(18.0f, 968.0f, 900.0f, 22.0f),
                   Justification::centredLeft, false);
    };
    page.setSize(W, H);
    addAndMakeVisible(page);
    header = std::make_unique<Header>(*this);
    header->setBounds(0, 0, W, 60);
    page.addAndMakeVisible(*header);
    for (auto *c : page.getChildren())
        if (dynamic_cast<Lamp *>(c) || dynamic_cast<Plates *>(c))
            lamps.push_back(c);

    setResizable(true, true);
    float s = proc.uiScale.load();
    getConstrainer()->setFixedAspectRatio((double)W / (double)H);
    setResizeLimits(W / 2, H / 2, (int)(W * 1.5f), (int)(H * 1.5f));
    setSize((int)std::lround(W * s), (int)std::lround(H * s));
    startTimerHz(30);
}

SolarisEditor::~SolarisEditor() { setLookAndFeel(nullptr); }

void SolarisEditor::resized()
{
    float s = (float)getWidth() / (float)W;
    page.setTransform(AffineTransform::scale(s));
    proc.uiScale = s;
}

void SolarisEditor::timerCallback()
{
    for (auto *c : lamps)
        c->repaint();
    static int tick = 0;
    if (++tick % 6 == 0)
        header->repaint();
}

void SolarisEditor::touched(RangedAudioParameter &p)
{
    inHand = &p;
    header->repaint();
}

void SolarisEditor::programMenu()
{
    PopupMenu m;
    for (int i = 0; i < sol::numPresets(); i++)
        m.addItem(i + 1, String(i + 1).paddedLeft('0', 2) + "  " + proc.getProgramName(i), true, i == proc.getCurrentProgram());
    if (!proc.userPresets.empty()) {
        m.addSeparator();
        m.addSectionHeader("YOUR OWN");
        for (int i = sol::numPresets(); i < proc.getNumPrograms(); i++)
            m.addItem(i + 1, String(i + 1).paddedLeft('0', 2) + "  " + proc.getProgramName(i), true, i == proc.getCurrentProgram());
    }
    m.addSeparator();
    m.addItem(-2, "Open the presets folder");
    m.showMenuAsync(PopupMenu::Options().withTargetComponent(header.get()), [this](int r) {
        if (r > 0)
            proc.setCurrentProgram(r - 1);
        else if (r == -2) {
            SolarisProcessor::presetFolder().createDirectory();
            SolarisProcessor::presetFolder().startAsProcess();
        }
        header->repaint();
    });
}

void SolarisEditor::saveProgram()
{
    auto *w = new AlertWindow("SAVE", "The sound as it is now, under a name of its own (the same name again replaces it):",
                              MessageBoxIconType::NoIcon, this);
    w->addTextEditor("name", proc.getProgramName(proc.getCurrentProgram()));
    w->addButton("SAVE", 1, KeyPress(KeyPress::returnKey));
    w->addButton("CANCEL", 0, KeyPress(KeyPress::escapeKey));
    w->enterModalState(true, ModalCallbackFunction::create([this, w](int r) {
                           if (r == 1)
                               proc.writePreset(w->getTextEditorContents("name"));
                           header->repaint();
                       }),
                       true);
}

juce::AudioProcessorEditor *SolarisProcessor::createEditor() { return new SolarisEditor(*this); }
