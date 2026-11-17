#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace GrainveilParams
{
    inline constexpr const char* mix         = "mix";
    inline constexpr const char* inGain      = "inGain";
    inline constexpr const char* outGain     = "outGain";
    inline constexpr const char* freeze      = "freeze";
    inline constexpr const char* captureTime = "captureTime";
    inline constexpr const char* feedback    = "feedback";
    inline constexpr const char* fbTone      = "fbTone";
    inline constexpr const char* grainSize   = "grainSize";
    inline constexpr const char* density     = "density";
    inline constexpr const char* scatter     = "scatter";
    inline constexpr const char* pitch       = "pitch";
    inline constexpr const char* pitchRand   = "pitchRand";
    inline constexpr const char* reverseAmt  = "reverseAmt";
    inline constexpr const char* spread      = "spread";
    inline constexpr const char* tone        = "tone";
    inline constexpr const char* mode        = "mode";
    inline constexpr const char* lfoRate     = "lfoRate";
    inline constexpr const char* lfoDepth    = "lfoDepth";
    inline constexpr const char* tempoSync   = "tempoSync";

    inline juce::StringArray getModeNames()
    {
        return {
            "Cloud",
            "EchoGrain",
            "ReverseSwarm",
            "IceHold",
            "Fracture",
            "PitchBloom",
            "Smear",
            "Ash"
        };
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ mix, 1 }, "Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ inGain, 1 }, "Input Gain",
            juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ outGain, 1 }, "Output Gain",
            juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ freeze, 1 }, "Freeze", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ captureTime, 1 }, "Capture Time",
            juce::NormalisableRange<float>(50.0f, 4000.0f, 1.0f, 0.5f), 1000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ feedback, 1 }, "Feedback",
            juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.25f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ fbTone, 1 }, "FB Tone",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ grainSize, 1 }, "Grain Size",
            juce::NormalisableRange<float>(10.0f, 500.0f, 0.5f, 0.5f), 120.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ density, 1 }, "Density",
            juce::NormalisableRange<float>(1.0f, 40.0f, 0.1f, 0.5f), 10.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ scatter, 1 }, "Scatter",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ pitch, 1 }, "Pitch",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ pitchRand, 1 }, "Pitch Rand",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ reverseAmt, 1 }, "Reverse",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ spread, 1 }, "Spread",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ tone, 1 }, "Tone",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{ mode, 1 }, "Mode", getModeNames(), 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ lfoRate, 1 }, "LFO Rate",
            juce::NormalisableRange<float>(0.05f, 10.0f, 0.01f, 0.5f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{ lfoDepth, 1 }, "LFO Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{ tempoSync, 1 }, "Tempo Sync", false));

        return { params.begin(), params.end() };
    }
}
