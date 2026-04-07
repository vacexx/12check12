#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
_12check12AudioProcessor::_12check12AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // cache parametrů — jednorázově, bez string lookup v processBlock
    timeMsParam = apvts.getRawParameterValue("TIME");
    feedbackPercent = apvts.getRawParameterValue("FEEDBACK");
    mixPercent = apvts.getRawParameterValue("MIX");
    HPFHz = apvts.getRawParameterValue("HPF");
    LPFHz = apvts.getRawParameterValue("LPF");
    saturationPercent = apvts.getRawParameterValue("SATURATION");
    syncParam = apvts.getRawParameterValue("SYNC");
    divisionParam = apvts.getRawParameterValue("DIVISION");

    auto& saturation = saturationChain.template get<saturationIndex>();
    saturation.functionToUse = [](float x) {
        return std::tanh(x);
        };
}

_12check12AudioProcessor::~_12check12AudioProcessor() {}

//==============================================================================
void _12check12AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // alokace zpožďovacího bufferu
    auto delayBufferSize = static_cast<int>(10.0 * sampleRate);
    delayBuffer.setSize(getTotalNumOutputChannels(), delayBufferSize);
    delayBuffer.clear();
    writePosition = 0;

    // inicializace ProcessSpec
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1; // filtry zpracovávají kanály samostatně

    // inicializace vyhlazování času
    smoothedTime.reset(sampleRate, 0.05);
	smoothedTime.setCurrentAndTargetValue(timeMsParam->load() /1000.0f);

    // inicializace StateVariableTPTFilter pro každý kanál
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        hpFilters[ch].prepare(spec);
        hpFilters[ch].setType(
            juce::dsp::StateVariableTPTFilterType::highpass);
        hpFilters[ch].reset();

        lpFilters[ch].prepare(spec);
        lpFilters[ch].setType(
            juce::dsp::StateVariableTPTFilterType::lowpass);
        lpFilters[ch].reset();
    }

    saturationChain.prepare(spec);
}

void _12check12AudioProcessor::releaseResources() {}

//==============================================================================
#ifndef JucePlugin_PreferredChannelConfigurations
bool _12check12AudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif
    return true;
#endif
}
#endif

//==============================================================================
void _12check12AudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numInputChannels; ch < numOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    // načtení cached parametrů — pouze atomické load(), žádný string lookup
    const bool  isSyncOn = syncParam->load() > 0.5f;
    const float HPFFreq = HPFHz->load();
    const float LPFFreq = LPFHz->load();
    const float feedback = juce::jlimit(0.0f, 0.98f,
        feedbackPercent->load() / 100.0f);
    const float mix = juce::jlimit(0.0f, 1.0f,
        mixPercent->load() / 100.0f);
    const float saturationAmount = saturationPercent->load() / 100.0f;
    const float driveInDb = saturationAmount * 30.0f;
    const float driveGain = juce::Decibels::decibelsToGain(driveInDb);
    const float outputGain = juce::Decibels::decibelsToGain(
        -(driveInDb * 0.2f));

    // výpočet doby zpoždění
    float delayTimeSec = 0.0f;

    if (isSyncOn)
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (pos->getBpm().hasValue())
                    bpm = *pos->getBpm();

        const float quarterNoteSec = 60.0f / static_cast<float>(bpm);
        const int   divisionIndex = static_cast<int>(divisionParam->load());

        float multiplier = 1.0f;
        switch (divisionIndex)
        {
        case 0: multiplier = 4.0f;              break; // 1/1
        case 1: multiplier = 2.0f;              break; // 1/2
        case 2: multiplier = 1.0f;              break; // 1/4
        case 3: multiplier = 0.5f;              break; // 1/8
        case 4: multiplier = 0.25f;             break; // 1/16
        case 5: multiplier = 1.0f * (2.0f / 3.0f); break; // 1/4 triplet
        case 6: multiplier = 0.5f * (2.0f / 3.0f); break; // 1/8 triplet
        case 7: multiplier = 1.5f;              break; // 1/4 dotted
        case 8: multiplier = 0.75f;             break; // 1/8 dotted
        default: break;
        }

		smoothedTime.setTargetValue(quarterNoteSec * multiplier);
    }
    else
    {
		smoothedTime.setTargetValue(timeMsParam->load() / 1000.0f);
    }

    const int delayBufferLength = delayBuffer.getNumSamples();
    if (delayBufferLength == 0)
        return;

    const float delayInSamplesFloat = juce::jlimit(0.0f,
        static_cast<float>(delayBufferLength - 1),
        delayTimeSec * static_cast<float>(currentSampleRate));

    // nastavení frekvencí filtrů (jednou za blok, ne pro každý vzorek)
    for (int ch = 0; ch < juce::jmin(numInputChannels, maxChannels); ++ch)
    {
        hpFilters[ch].setCutoffFrequency(HPFFreq);
        lpFilters[ch].setCutoffFrequency(LPFFreq);
    }

    auto& saturation = saturationChain.template get<saturationIndex>();

    // raw pointery — bez alokace v audio vlákně
    float* const* channelData = buffer.getArrayOfWritePointers();
    float* const* delayData = delayBuffer.getArrayOfWritePointers();

    int localWritePos = writePosition;

    for (int i = 0; i < numSamples; ++i)
    {
        // získání vyhlazené hodnoty pro vzorek
        float currentDelaySec = smoothedTime.getNextValue();

        // výpočet zpoždění ve vzorcích
        float delayInSamplesFloat = currentDelaySec * static_cast<float>(currentSampleRate);
        delayInSamplesFloat = juce::jlimit(0.0f,
            static_cast<float>(delayBufferLength - 1),
            delayInSamplesFloat);

        // získání přesné desetinné pozice
        float exactReadPos = static_cast<float>(localWritePos) - delayInSamplesFloat;

        // ošetření bufferu (když čteme za začátkem)
        if (exactReadPos < 0.0f)
            exactReadPos += static_cast<float>(delayBufferLength);

        // rozdělení na celočíselný index a zlomkovou část pro interpolaci
        int index0 = static_cast<int>(exactReadPos);
        float fraction = exactReadPos - static_cast<float>(index0);

        // určení sousedního indexu (s ošetřením konce bufferu)
        int index1 = index0 + 1;
        if (index1 >= delayBufferLength)
            index1 -= delayBufferLength;

        for (int ch = 0; ch < juce::jmin(numInputChannels, maxChannels); ++ch)
        {
            const float inSample = channelData[ch][i];

            // načtení dvou sousedních vzorků
            const float sample0 = delayData[ch][index0];
            const float sample1 = delayData[ch][index1];

            // lineární interpolace zpožděného vzorku
            const float delayedSample = sample0 + fraction * (sample1 - sample0);

            // filtrace zpožděného vzorku
            float filtered = hpFilters[ch].processSample(0, delayedSample);
            filtered = lpFilters[ch].processSample(0, filtered);

            // zpětná vazba + saturace (před zápisem do bufferu)
            float delayInput = inSample + (filtered * feedback);
            float driven = delayInput * driveGain;
            float saturated = saturation.processSample(driven);
            delayData[ch][localWritePos] = saturated / driveGain;

            // dry/wet mix
            float wetOutput = filtered * outputGain;
            channelData[ch][i] = inSample * (1.0f - mix)
                + wetOutput * mix;
        }

        if (++localWritePos >= delayBufferLength)
            localWritePos = 0;
    }

    writePosition = localWritePos;
}

//==============================================================================
bool _12check12AudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* _12check12AudioProcessor::createEditor()
{
    return new _12check12AudioProcessorEditor(*this);
}

//==============================================================================
void _12check12AudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void _12check12AudioProcessor::setStateInformation(
    const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(
        getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
const juce::String _12check12AudioProcessor::getName() const
{
    return JucePlugin_Name;
}
bool _12check12AudioProcessor::acceptsMidi() const { return false; }
bool _12check12AudioProcessor::producesMidi() const { return false; }
bool _12check12AudioProcessor::isMidiEffect() const { return false; }
double _12check12AudioProcessor::getTailLengthSeconds() const { return 0.0; }
int _12check12AudioProcessor::getNumPrograms() { return 1; }
int _12check12AudioProcessor::getCurrentProgram() { return 0; }
void _12check12AudioProcessor::setCurrentProgram(int) {}
const juce::String _12check12AudioProcessor::getProgramName(int)
{
    return {};
}
void _12check12AudioProcessor::changeProgramName(int,
    const juce::String&) {
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new _12check12AudioProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout
_12check12AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "TIME", "Time",
        juce::NormalisableRange<float>(1.0f, 10000.0f, 1.0f, 0.3f),
        250.0f,
        juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction(
            [](float value, int) {
                if (value >= 1000.0f)
                    return juce::String(value / 1000.0f, 2) + " s";
                else
                    return juce::String((int)value) + " ms";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "FEEDBACK", "Feedback", 0.0f, 100.0f, 25.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "MIX", "Mix", 0.0f, 100.0f, 50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "HPF", "HPF",
        juce::NormalisableRange<float>(20.0f, 2000.0f, 0.0f, 0.3f),
        20.0f,
        juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction(
            [](float value, int) {
                return juce::String((int)value) + " Hz";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "LPF", "LPF",
        juce::NormalisableRange<float>(200.0f, 20000.0f, 0.0f, 0.3f),
        20000.0f,
        juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction(
            [](float value, int) {
                return juce::String((int)value) + " Hz";
            })));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SATURATION", "Saturation", 0.0f, 100.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "SYNC", "Sync", false));

    juce::StringArray noteLengths{
        "1/1", "1/2", "1/4", "1/8", "1/16",
        "1/4 Triplet", "1/8 Triplet",
        "1/4 Dotted",  "1/8 Dotted"
    };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "DIVISION", "Division", noteLengths, 2));

    return { params.begin(), params.end() };
}