/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (SOLARIS-23) */
/* SOLARIS-23 as a plugin (VST3, AU on the Mac) and a standalone app: the engine (dsp/solaris.h), all its parameters
 * for the host (automation, saved with the project), its programs as the host's programs (the factory's, then your
 * own: files in Documents/SOLARIS-23/Presets), MIDI in (notes to the VCOs, pitch bend, the mod wheel to the
 * joystick's Y, CC 120/123), the host's tempo and position for the sequencer. */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "solaris.h"

class SolarisProcessor : public juce::AudioProcessor
{
public:
    SolarisProcessor();
    ~SolarisProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SOLARIS-23"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return sol::numPresets() + (int)userPresets.size(); }
    int getCurrentProgram() override { return program; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String &) override {}

    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    /* your own programs */
    struct UserPreset {
        juce::String name;
        std::vector<float> v;
    };
    std::vector<UserPreset> userPresets;
    static juce::File presetFolder();
    void loadUserPresets();
    int writePreset(const juce::String &name);   /* the sound as it is, kept under this name: its program number */

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::String paramId(int i) { return sol::PARAMS[i].id; }

    juce::MidiKeyboardState keyboardState;   /* the touch plates */
    /* for the panel */
    std::atomic<float> light{0.0f}, lfo[2], voice[8];
    std::atomic<int> seqStep{0};
    std::atomic<float> uiScale{0.8f};
    std::atomic<int> uiFinish{0};            /* the panel's finish (Look.h), kept with the project */

private:
    sol::Engine engine;
    std::vector<std::atomic<float> *> raw;
    float sent[sol::NPARAMS] = {};
    bool fresh = true;
    int program = 0;
    void handleMidi(const juce::MidiMessage &m);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SolarisProcessor)
};
