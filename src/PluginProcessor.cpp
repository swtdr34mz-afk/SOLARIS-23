/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIUM-23A) */
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace sol;

juce::AudioProcessorValueTreeState::ParameterLayout SolarisProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < NPARAMS; i++) {
        const ParamDef &d = PARAMS[i];
        juce::ParameterID pid{d.id, 1};
        if (d.kind == K_CH) {
            juce::StringArray ch;
            for (int k = 0; k < d.nChoices; k++)
                ch.add(d.choices[k]);
            layout.add(std::make_unique<juce::AudioParameterChoice>(pid, d.name, ch, (int)d.def));
        } else if (d.kind == K_SW) {   /* (a choice of two: its value always 0 or 1, whatever a host sets) */
            layout.add(std::make_unique<juce::AudioParameterChoice>(pid, d.name, juce::StringArray{"OFF", "ON"}, d.def >= 0.5f ? 1 : 0));
        } else {
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                pid, d.name, juce::NormalisableRange<float>(0.0f, 1.0f), d.def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction([i](float v, int) {
                    char t[32];
                    paramText(i, v, t, sizeof t);
                    return juce::String(t);
                })));
        }
    }
    return layout;
}

SolarisProcessor::SolarisProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "SOLARIS23", createLayout())
{
    for (int i = 0; i < NPARAMS; i++)
        raw.push_back(apvts.getRawParameterValue(paramId(i)));
    for (auto &a : lfo)
        a = 0.0f;
    for (auto &a : voice)
        a = 0.0f;
    loadUserPresets();
    setCurrentProgram(1);                /* (a program that sounds when the plugin opens) */
}

/* ----------------------------------------------------------------------------------- programs --- */
juce::File SolarisProcessor::presetFolder()
{
    auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    auto dir = docs.getChildFile("SOLARIUM-23A").getChildFile("Presets");
    /* (the plugin was called SOLARIS-23 before v1.3: its own programs come along, copied once) */
    auto old = docs.getChildFile("SOLARIS-23").getChildFile("Presets");
    if (!dir.isDirectory() && old.isDirectory() && dir.createDirectory())
        for (auto &f : old.findChildFiles(juce::File::findFiles, false, "*.s23"))
            f.copyFileTo(dir.getChildFile(f.getFileName()));
    return dir;
}

void SolarisProcessor::loadUserPresets()
{
    userPresets.clear();
    auto files = presetFolder().findChildFiles(juce::File::findFiles, false, "*.s23");
    files.sort();
    for (auto &f : files)
        if (auto xml = juce::XmlDocument::parse(f)) {
            if (!xml->hasTagName("SOLARIS23PRESET"))
                continue;
            UserPreset u;
            u.name = xml->getStringAttribute("name", f.getFileNameWithoutExtension());
            u.v.resize(NPARAMS);
            for (int i = 0; i < NPARAMS; i++)   /* (a parameter the file lacks: its default) */
                u.v[(size_t)i] = juce::jlimit(0.0f, paramMax(i), (float)xml->getDoubleAttribute(PARAMS[i].id, PARAMS[i].def));
            userPresets.push_back(std::move(u));
        }
}

int SolarisProcessor::writePreset(const juce::String &name)
{
    auto clean = name.trim().toUpperCase().substring(0, 16);
    if (clean.isEmpty())
        return -1;
    juce::XmlElement xml("SOLARIS23PRESET");
    xml.setAttribute("name", clean);
    xml.setAttribute("version", 1);
    for (int i = 0; i < NPARAMS; i++)
        xml.setAttribute(PARAMS[i].id, raw[(size_t)i]->load());
    auto folder = presetFolder();
    folder.createDirectory();
    if (!xml.writeTo(folder.getChildFile(juce::File::createLegalFileName(clean) + ".s23")))
        return -1;
    loadUserPresets();
    for (size_t i = 0; i < userPresets.size(); i++)
        if (userPresets[i].name == clean) {
            program = numPresets() + (int)i;
            updateHostDisplay(ChangeDetails().withProgramChanged(true));
            return program;
        }
    return -1;
}

void SolarisProcessor::setCurrentProgram(int index)
{
    std::vector<float> v((size_t)NPARAMS);
    if (index >= 0 && index < numPresets())
        presetValues(index, v.data());
    else if (index >= numPresets() && index < getNumPrograms())
        v = userPresets[(size_t)(index - numPresets())].v;
    else
        return;
    program = index;
    for (int i = 0; i < NPARAMS; i++)
        if (auto *p = apvts.getParameter(paramId(i)))
            p->setValueNotifyingHost(p->convertTo0to1(v[(size_t)i]));
}

const juce::String SolarisProcessor::getProgramName(int index)
{
    if (index >= 0 && index < numPresets())
        return presetName(index);
    if (index >= numPresets() && index < getNumPrograms())
        return userPresets[(size_t)(index - numPresets())].name;
    return {};
}

/* -------------------------------------------------------------------------------------- audio --- */
bool SolarisProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SolarisProcessor::prepareToPlay(double sampleRate, int)
{
    engine.prepare(sampleRate);
    fresh = true;
}

void SolarisProcessor::handleMidi(const juce::MidiMessage &m)
{
    const bool plate = m.getChannel() == PLATE_CHANNEL;   /* (the touch plates play on their own channel) */
    if (m.isNoteOn())
        engine.noteOn(m.getNoteNumber(), m.getFloatVelocity(), plate);
    else if (m.isNoteOff())
        engine.noteOff(m.getNoteNumber(), plate);
    else if (m.isPitchWheel())
        engine.bend((float)(m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isAllSoundOff())
        engine.panic();
    else if (m.isAllNotesOff())
        engine.allNotesOff();
    else if (m.isController()) {
        int cc = m.getControllerNumber(), v = m.getControllerValue();
        if (cc == 1)
            engine.modWheel((float)v / 127.0f);
        else if (cc == 121) {
            engine.bend(0.0f);
            engine.modWheel(0.0f);
        }
    }
}

void SolarisProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    float *L = buffer.getWritePointer(0), *R = buffer.getWritePointer(buffer.getNumChannels() > 1 ? 1 : 0);
    for (int i = 0; i < NPARAMS; i++) {  /* the parameters, when they change */
        float v = raw[(size_t)i]->load();
        if (fresh || v != sent[i]) {
            engine.set(i, v);
            sent[i] = v;
        }
    }
    fresh = false;
    if (auto *ph = getPlayHead())        /* the host's tempo and position (the sequencer's clock) */
        if (auto pos = ph->getPosition()) {
            if (auto bpm = pos->getBpm())
                engine.tempo(*bpm);
            auto ppq = pos->getPpqPosition();
            engine.position(ppq ? *ppq : 0.0, pos->getIsPlaying() && ppq.hasValue());
        }
    keyboardState.processNextMidiBuffer(midi, 0, n, true);   /* the touch plates join the host's notes */
    int done = 0;
    for (const auto meta : midi) {
        int at = juce::jlimit(0, n, meta.samplePosition);
        if (at > done) {
            engine.render(L + done, R + done, at - done);
            done = at;
        }
        handleMidi(meta.getMessage());
    }
    if (n > done)
        engine.render(L + done, R + done, n - done);
    if (buffer.getNumChannels() > 2)
        for (int c = 2; c < buffer.getNumChannels(); c++)
            buffer.clear(c, 0, n);
    light = engine.light();
    seqStep = engine.seqStep();
    for (int l = 0; l < 2; l++)
        lfo[l] = engine.lfo(l);
    for (int v = 0; v < 8; v++)
        voice[v] = engine.voiceLevel(v);
}

/* -------------------------------------------------------------------------------------- state --- */
void SolarisProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    state.setProperty("program", program, nullptr);
    state.setProperty("uiScale", uiScale.load(), nullptr);
    state.setProperty("uiFinish", uiFinish.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void SolarisProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType())) {
            auto state = juce::ValueTree::fromXml(*xml);
            program = juce::jlimit(0, juce::jmax(0, getNumPrograms() - 1), (int)state.getProperty("program", 0));
            uiScale = juce::jlimit(0.5f, 1.5f, (float)state.getProperty("uiScale", 0.8f));
            uiFinish = juce::jlimit(0, 4, (int)state.getProperty("uiFinish", 0));
            apvts.replaceState(state);
        }
}

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new SolarisProcessor(); }
