#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/GrainEngine.h"

class GrainveilAudioProcessor final : public juce::AudioProcessor
{
public:
    GrainveilAudioProcessor();
    ~GrainveilAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    void getTelemetrySnapshot(GrainveilDsp::TelemetrySnapshot& dest) const noexcept
    {
        grainEngine.copyTelemetry(dest);
    }

private:
    void applyPreset(int index);
    int currentProgramIndex = 0;
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float>* mixParam         = nullptr;
    std::atomic<float>* inGainParam      = nullptr;
    std::atomic<float>* outGainParam     = nullptr;
    std::atomic<float>* freezeParam      = nullptr;
    std::atomic<float>* captureTimeParam = nullptr;
    std::atomic<float>* feedbackParam    = nullptr;
    std::atomic<float>* fbToneParam      = nullptr;
    std::atomic<float>* grainSizeParam   = nullptr;
    std::atomic<float>* densityParam     = nullptr;
    std::atomic<float>* scatterParam     = nullptr;
    std::atomic<float>* reverseAmtParam  = nullptr;
    std::atomic<float>* pitchParam       = nullptr;
    std::atomic<float>* pitchRandParam   = nullptr;
    std::atomic<float>* spreadParam      = nullptr;
    std::atomic<float>* toneParam        = nullptr;
    std::atomic<float>* modeParam        = nullptr;

    GrainveilDsp::GrainEngine grainEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainveilAudioProcessor)
};
