#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/GrainEngine.h"

class OledDisplay final : public juce::Component
{
public:
    void setSnapshot(const GrainveilDsp::TelemetrySnapshot& newSnapshot);
    void paint(juce::Graphics& g) override;

private:
    GrainveilDsp::TelemetrySnapshot snapshot;
};
