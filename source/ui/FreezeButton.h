#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// ToggleButton retains JUCE's accessibility and APVTS ButtonAttachment behaviour.
// The LED and switch housing are drawn by GrainveilLookAndFeel.
class FreezeButton final : public juce::ToggleButton
{
public:
    FreezeButton() : juce::ToggleButton("FREEZE")
    {
        setTitle("Freeze");
        setDescription("Hold the current audio buffer");
        setTooltip("Hold or release the current audio buffer");
        setRepaintsOnMouseActivity(true);
    }
};
