#include "PluginProcessor.h"
#include "PluginEditor.h"

GrainveilAudioProcessor::GrainveilAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", GrainveilParams::createParameterLayout())
{
    mixParam         = apvts.getRawParameterValue(GrainveilParams::mix);
    inGainParam      = apvts.getRawParameterValue(GrainveilParams::inGain);
    outGainParam     = apvts.getRawParameterValue(GrainveilParams::outGain);
    freezeParam      = apvts.getRawParameterValue(GrainveilParams::freeze);
    captureTimeParam = apvts.getRawParameterValue(GrainveilParams::captureTime);
    feedbackParam    = apvts.getRawParameterValue(GrainveilParams::feedback);
    fbToneParam      = apvts.getRawParameterValue(GrainveilParams::fbTone);
    grainSizeParam   = apvts.getRawParameterValue(GrainveilParams::grainSize);
    densityParam     = apvts.getRawParameterValue(GrainveilParams::density);
    scatterParam     = apvts.getRawParameterValue(GrainveilParams::scatter);
    reverseAmtParam  = apvts.getRawParameterValue(GrainveilParams::reverseAmt);
    pitchParam       = apvts.getRawParameterValue(GrainveilParams::pitch);
    pitchRandParam   = apvts.getRawParameterValue(GrainveilParams::pitchRand);
    spreadParam      = apvts.getRawParameterValue(GrainveilParams::spread);
    toneParam        = apvts.getRawParameterValue(GrainveilParams::tone);
    modeParam        = apvts.getRawParameterValue(GrainveilParams::mode);
}

const juce::String GrainveilAudioProcessor::getName() const { return JucePlugin_Name; }
bool GrainveilAudioProcessor::acceptsMidi() const { return false; }
bool GrainveilAudioProcessor::producesMidi() const { return false; }
bool GrainveilAudioProcessor::isMidiEffect() const { return false; }
double GrainveilAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int GrainveilAudioProcessor::getNumPrograms() { return 8; }
int GrainveilAudioProcessor::getCurrentProgram() { return currentProgramIndex; }

void GrainveilAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    currentProgramIndex = index;
    applyPreset(index);
}

const juce::String GrainveilAudioProcessor::getProgramName(int index)
{
    static const char* names[8] = {
        "Ambient Wash",
        "Frozen Drone",
        "Reverse Swell",
        "Glitch Rhythm",
        "Pitch Cloud",
        "Tight Echo-Grain",
        "Lo-Fi Loop",
        "Guitar Swell"
    };
    return (index >= 0 && index < 8) ? juce::String(names[index]) : juce::String();
}

void GrainveilAudioProcessor::changeProgramName(int, const juce::String&) {}

void GrainveilAudioProcessor::applyPreset(int index)
{
    auto setParam = [this](const char* id, float value)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(value));
    };

    switch (index)
    {
        case 0: // Ambient Wash
            setParam(GrainveilParams::mode, 0.0f);
            setParam(GrainveilParams::mix, 0.55f);
            setParam(GrainveilParams::grainSize, 220.0f);
            setParam(GrainveilParams::density, 18.0f);
            setParam(GrainveilParams::scatter, 0.35f);
            setParam(GrainveilParams::feedback, 0.45f);
            setParam(GrainveilParams::pitch, 0.0f);
            setParam(GrainveilParams::reverseAmt, 0.15f);
            setParam(GrainveilParams::spread, 0.8f);
            setParam(GrainveilParams::tone, 0.7f);
            setParam(GrainveilParams::freeze, 0.0f);
            break;
        case 1: // Frozen Drone
            setParam(GrainveilParams::mode, 3.0f);
            setParam(GrainveilParams::mix, 0.75f);
            setParam(GrainveilParams::grainSize, 360.0f);
            setParam(GrainveilParams::density, 12.0f);
            setParam(GrainveilParams::scatter, 0.25f);
            setParam(GrainveilParams::feedback, 0.30f);
            setParam(GrainveilParams::pitch, 0.0f);
            setParam(GrainveilParams::reverseAmt, 0.2f);
            setParam(GrainveilParams::spread, 0.7f);
            setParam(GrainveilParams::tone, 0.55f);
            setParam(GrainveilParams::freeze, 1.0f);
            break;
        case 2: // Reverse Swell
            setParam(GrainveilParams::mode, 2.0f);
            setParam(GrainveilParams::mix, 0.65f);
            setParam(GrainveilParams::grainSize, 260.0f);
            setParam(GrainveilParams::density, 14.0f);
            setParam(GrainveilParams::scatter, 0.3f);
            setParam(GrainveilParams::feedback, 0.4f);
            setParam(GrainveilParams::pitch, 0.0f);
            setParam(GrainveilParams::reverseAmt, 0.9f);
            setParam(GrainveilParams::spread, 0.65f);
            setParam(GrainveilParams::tone, 0.65f);
            setParam(GrainveilParams::freeze, 0.0f);
            break;
        case 3: // Glitch Rhythm
            setParam(GrainveilParams::mode, 4.0f);
            setParam(GrainveilParams::mix, 0.60f);
            setParam(GrainveilParams::grainSize, 45.0f);
            setParam(GrainveilParams::density, 26.0f);
            setParam(GrainveilParams::scatter, 0.75f);
            setParam(GrainveilParams::feedback, 0.35f);
            setParam(GrainveilParams::pitch, 0.0f);
            setParam(GrainveilParams::reverseAmt, 0.35f);
            setParam(GrainveilParams::spread, 0.85f);
            setParam(GrainveilParams::tone, 0.8f);
            setParam(GrainveilParams::freeze, 0.0f);
            break;
        case 4: // Pitch Cloud
            setParam(GrainveilParams::mode, 5.0f);
            setParam(GrainveilParams::mix, 0.55f);
            setParam(GrainveilParams::grainSize, 190.0f);
            setParam(GrainveilParams::density, 16.0f);
            setParam(GrainveilParams::scatter, 0.4f);
            setParam(GrainveilParams::feedback, 0.5f);
            setParam(GrainveilParams::pitch, 12.0f);
            setParam(GrainveilParams::pitchRand, 0.6f);
            setParam(GrainveilParams::spread, 0.75f);
            setParam(GrainveilParams::tone, 0.65f);
            setParam(GrainveilParams::freeze, 0.0f);
            break;
        case 5: // Tight Echo-Grain
            setParam(GrainveilParams::mode, 1.0f);
            setParam(GrainveilParams::mix, 0.45f);
            setParam(GrainveilParams::captureTime, 420.0f);
            setParam(GrainveilParams::grainSize, 110.0f);
            setParam(GrainveilParams::density, 8.0f);
            setParam(GrainveilParams::scatter, 0.1f);
            setParam(GrainveilParams::feedback, 0.45f);
            setParam(GrainveilParams::pitch, 0.0f);
            setParam(GrainveilParams::spread, 0.4f);
            setParam(GrainveilParams::tone, 0.75f);
            setParam(GrainveilParams::freeze, 0.0f);
            break;
        case 6: // Lo-Fi Loop
            setParam(GrainveilParams::mode, 7.0f);
            setParam(GrainveilParams::mix, 0.65f);
            setParam(GrainveilParams::captureTime, 1200.0f);
            setParam(GrainveilParams::grainSize, 160.0f);
            setParam(GrainveilParams::density, 11.0f);
            setParam(GrainveilParams::scatter, 0.45f);
            setParam(GrainveilParams::feedback, 0.55f);
            setParam(GrainveilParams::tone, 0.4f);
            setParam(GrainveilParams::spread, 0.5f);
            setParam(GrainveilParams::freeze, 0.0f);
            break;
        case 7: // Guitar Swell
            setParam(GrainveilParams::mode, 6.0f);
            setParam(GrainveilParams::mix, 0.60f);
            setParam(GrainveilParams::grainSize, 320.0f);
            setParam(GrainveilParams::density, 16.0f);
            setParam(GrainveilParams::scatter, 0.2f);
            setParam(GrainveilParams::feedback, 0.4f);
            setParam(GrainveilParams::pitch, 0.0f);
            setParam(GrainveilParams::reverseAmt, 0.25f);
            setParam(GrainveilParams::spread, 0.75f);
            setParam(GrainveilParams::tone, 0.72f);
            setParam(GrainveilParams::freeze, 0.0f);
            break;
        default:
            break;
    }
}

void GrainveilAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    grainEngine.prepare(sampleRate);
}

void GrainveilAudioProcessor::releaseResources()
{
    grainEngine.reset();
}

bool GrainveilAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void GrainveilAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples             = buffer.getNumSamples();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);

    GrainveilDsp::EngineParams params;
    params.mix           = (mixParam != nullptr) ? mixParam->load() : 0.5f;
    params.inGainDb      = (inGainParam != nullptr) ? inGainParam->load() : 0.0f;
    params.outGainDb     = (outGainParam != nullptr) ? outGainParam->load() : 0.0f;
    params.freeze        = (freezeParam != nullptr) && (freezeParam->load() >= 0.5f);
    params.captureTimeMs = (captureTimeParam != nullptr) ? captureTimeParam->load() : 1000.0f;
    params.feedback      = (feedbackParam != nullptr) ? feedbackParam->load() : 0.25f;
    params.fbTone        = (fbToneParam != nullptr) ? fbToneParam->load() : 0.5f;
    params.grainSizeMs   = (grainSizeParam != nullptr) ? grainSizeParam->load() : 120.0f;
    params.densityHz     = (densityParam != nullptr) ? densityParam->load() : 10.0f;
    params.scatter       = (scatterParam != nullptr) ? scatterParam->load() : 0.2f;
    params.reverseAmt    = (reverseAmtParam != nullptr) ? reverseAmtParam->load() : 0.0f;
    params.pitchSt       = (pitchParam != nullptr) ? pitchParam->load() : 0.0f;
    params.pitchRand     = (pitchRandParam != nullptr) ? pitchRandParam->load() : 0.0f;
    params.spread        = (spreadParam != nullptr) ? spreadParam->load() : 0.5f;
    params.tone          = (toneParam != nullptr) ? toneParam->load() : 0.7f;
    params.mode          = (modeParam != nullptr) ? static_cast<int>(modeParam->load() + 0.5f) : 0;

    grainEngine.process(buffer, params);
}

bool GrainveilAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* GrainveilAudioProcessor::createEditor()
{
    return new GrainveilAudioProcessorEditor(*this);
}

void GrainveilAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GrainveilAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrainveilAudioProcessor();
}
