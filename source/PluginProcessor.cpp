#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

GrainveilAudioProcessor::GrainveilAudioProcessor(juce::File presetDirectory)
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
    presetManager = std::make_unique<PresetManager>(*this, apvts, std::move(presetDirectory));
}

const juce::String GrainveilAudioProcessor::getName() const { return JucePlugin_Name; }
bool GrainveilAudioProcessor::acceptsMidi() const { return false; }
bool GrainveilAudioProcessor::producesMidi() const { return false; }
bool GrainveilAudioProcessor::isMidiEffect() const { return false; }
double GrainveilAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int GrainveilAudioProcessor::getNumPrograms() { return presetManager->getNumPrograms(); }
int GrainveilAudioProcessor::getCurrentProgram() { return presetManager->getCurrentProgram(); }
void GrainveilAudioProcessor::setCurrentProgram(int index) { presetManager->requestProgram(index); }
const juce::String GrainveilAudioProcessor::getProgramName(int index) { return presetManager->getProgramName(index); }
void GrainveilAudioProcessor::changeProgramName(int, const juce::String&) {}

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
    params.mix           = (mixParam != nullptr) ? mixParam->load() : 0.65f;
    params.inGainDb      = (inGainParam != nullptr) ? inGainParam->load() : 0.0f;
    params.outGainDb     = (outGainParam != nullptr) ? outGainParam->load() : 0.0f;
    params.freeze        = (freezeParam != nullptr) && (freezeParam->load() >= 0.5f);
    params.captureTimeMs = (captureTimeParam != nullptr) ? captureTimeParam->load() : 1000.0f;
    params.feedback      = (feedbackParam != nullptr) ? feedbackParam->load() : 0.16f;
    params.fbTone        = (fbToneParam != nullptr) ? fbToneParam->load() : 0.5f;
    params.grainSizeMs   = (grainSizeParam != nullptr) ? grainSizeParam->load() : 160.0f;
    params.densityHz     = (densityParam != nullptr) ? densityParam->load() : 14.0f;
    params.scatter       = (scatterParam != nullptr) ? scatterParam->load() : 0.35f;
    params.reverseAmt    = (reverseAmtParam != nullptr) ? reverseAmtParam->load() : 0.0f;
    params.pitchSt       = (pitchParam != nullptr) ? pitchParam->load() : 0.0f;
    params.pitchRand     = (pitchRandParam != nullptr) ? pitchRandParam->load() : 0.0f;
    params.spread        = (spreadParam != nullptr) ? spreadParam->load() : 0.5f;
    params.tone          = (toneParam != nullptr) ? toneParam->load() : 0.7f;
    const float modeValue = (modeParam != nullptr) ? modeParam->load() : 0.0f;
    params.mode          = std::isfinite(modeValue) ? static_cast<int>(std::clamp(modeValue, 0.0f, 7.0f) + 0.5f) : 0;

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
    if (xml != nullptr)
    {
        presetManager->writeSessionMetadata(*xml);
        copyXmlToBinary(*xml, destData);
    }
}

void GrainveilAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        auto* meta = xmlState->getChildByName("GrainveilProgram");
        std::unique_ptr<juce::XmlElement> savedMeta(meta != nullptr ? new juce::XmlElement(*meta) : nullptr);
        if (meta != nullptr) xmlState->removeChildElement(meta, true);
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        presetManager->restoreSessionMetadata(savedMeta.get());
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrainveilAudioProcessor();
}
