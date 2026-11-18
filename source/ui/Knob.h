#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// JUCE Slider retains keyboard, text-entry, automation and accessibility support;
// its visual treatment lives in GrainveilLookAndFeel.
class GrainveilKnob final : public juce::Slider
{
public:
    enum class Unit { percent, milliseconds, hertz, semitones, density };

    GrainveilKnob()
    {
        setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                            juce::MathConstants<float>::pi * 2.75f, true);
        setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 20);
        setRepaintsOnMouseActivity(true);
        setScrollWheelEnabled(true);
    }

    void configure(const juce::String& name, Unit newUnit, const juce::String& description)
    {
        displayName = name;
        unit = newUnit;
        setTitle(name);
        setDescription(description);
        setTooltip(description);
    }

    const juce::String& getDisplayName() const noexcept { return displayName; }
    juce::String getDisplayValue() const
    {
        const double value = getValue();
        switch (unit)
        {
            case Unit::percent:      return juce::String(value * 100.0, 1) + " %";
            case Unit::milliseconds: return juce::String(value, value < 100.0 ? 1 : 0) + " ms";
            case Unit::hertz:        return juce::String(value, 1) + " Hz";
            case Unit::semitones:    return (value > 0.0 ? "+" : "") + juce::String(value, 1) + " st";
            case Unit::density:      return juce::String(value, 1) + " / 40";
        }
        return {};
    }

private:
    juce::String displayName;
    Unit unit = Unit::percent;
};
