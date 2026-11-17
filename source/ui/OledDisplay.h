#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

// All methods are called on the message thread. The display deliberately has no
// direct access to DSP memory; a future safe handoff can provide monitor data.
class OledDisplay final : public juce::Component, public juce::SettableTooltipClient
{
public:
    static constexpr size_t waveformPoints = 128;
    static constexpr size_t maxGrains = 32;

    void setControlState(const juce::String& mode, bool frozen);
    void setPresetName(const juce::String& name, bool modified);
    void setMonitorData(const std::array<float, waveformPoints>& wave,
                        const std::array<float, maxGrains>& markers, int count);
    void setMonitorUnavailable();
    void beginParameter(const juce::String& name, const juce::String& value);
    void updateParameter(const juce::String& name, const juce::String& value);
    void endParameter();
    void advanceAnimation();
    void paint(juce::Graphics&) override;

private:
    std::array<float, waveformPoints> waveform {};
    std::array<float, maxGrains> grainPositions {};
    int grainCount = 0;
    bool monitorAvailable = false;
    bool isFrozen = false;
    bool parameterHeld = false;
    juce::String modeName { "Cloud" }, presetName { "Init" }, parameterName, parameterValue;
    bool presetModified = false;
    double fadeDeadlineMs = 0.0;
    float parameterOpacity = 0.0f;
};
