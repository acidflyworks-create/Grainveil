#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace GrainveilUI
{
// Typefaces are loaded once from embedded data, never from the host OS.
juce::Font font(float height, bool semibold = false);

struct FittedText
{
    juce::String text;
    juce::Font font;
};

// Single-line layout: reduce the height to the minimum before truncating.
// An empty string is returned when even the ellipsis cannot fit. The returned
// glyph advance and font height are bounded by the supplied rectangle.
FittedText fitText(const juce::String& text, juce::Rectangle<float> bounds,
                   float preferredHeight, float minimumHeight, bool semibold = false);

// Clips ink to the rectangle as glyph bearings can extend beyond their advance.
void drawFittedText(juce::Graphics& g, const juce::String& text,
                    juce::Rectangle<float> bounds, float preferredHeight,
                    float minimumHeight, juce::Justification justification,
                    bool semibold = false);
}
