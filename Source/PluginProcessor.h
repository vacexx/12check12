/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>

//==============================================================================
/**
*/
class _12check12AudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    _12check12AudioProcessor();
    ~_12check12AudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Public APVTS object so the editor can access it
    juce::AudioProcessorValueTreeState apvts;

private:
    //==============================================================================
    juce::AudioBuffer<float> delayBuffer;
    int writePosition{ 0 };
    double currentSampleRate{ 0.0 };

    std::atomic<float>* timeMsParam = nullptr;
    std::atomic<float>* feedbackPercent = nullptr;
    std::atomic<float>* mixPercent = nullptr;
    std::atomic<float>* HPFHz = nullptr;
    std::atomic<float>* LPFHz = nullptr;
    std::atomic<float>* saturationPercent = nullptr;
    std::atomic<float>* syncParam = nullptr;
    std::atomic<float>* divisionParam = nullptr;

    static constexpr int maxChannels = 2;
    juce::dsp::StateVariableTPTFilter<float> hpFilters[maxChannels];
    juce::dsp::StateVariableTPTFilter<float> lpFilters[maxChannels];

    enum {
        saturationIndex
    };

    juce::SmoothedValue<float> smoothedTime;

    juce::dsp::ProcessorChain<juce::dsp::WaveShaper<float>> saturationChain;

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(_12check12AudioProcessor)
};

