#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "StimmeDsp.h"

class StimmeAudioProcessor : public juce::AudioProcessor
{
public:
    StimmeAudioProcessor();
    ~StimmeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };
    static const std::vector<Preset>& presets();
    int getNumPrograms() override { return (int) presets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> compGrDb { 0 }, deEssGrDb { 0 };
    std::atomic<float> vuOut { 0 }, peakOut { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters();

    stimme::DeEsser deEsser;
    stimme::Compressor comp;
    enum { hp1, hp2, warmth, presence, air, numStages };
    std::array<eq::Coeffs, numStages> c;
    std::array<std::array<eq::BiquadState, numStages>, 2> st;
    std::array<float, 6> lastFilter {};
    juce::SmoothedValue<float> smMakeup, smOut, smTube;
    float vuState = 0, vuCoef = 0.001f;
    int currentProgram = 0;
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StimmeAudioProcessor)
};
