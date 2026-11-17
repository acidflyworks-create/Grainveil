#include "ui/EditorGrid.h"
#include "ui/UiTypography.h"
#include <iostream>
#include <vector>

namespace
{
int failures = 0;
void check(bool condition, const char* message)
{
    if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    namespace grid = GrainveilUI::Grid;
    std::vector<juce::Rectangle<int>> regions { grid::presetZone, grid::modeLabel, grid::modeField,
                                                 grid::freezeField, grid::display };
    for (const auto& cell : grid::mainKnobs) { regions.push_back(cell.control); regions.push_back(cell.label); }
    for (const auto& cell : grid::secondaryKnobs) { regions.push_back(cell.control); regions.push_back(cell.label); }
    const juce::Rectangle<int> canvas(0, 0, grid::width, grid::height);
    for (size_t i = 0; i < regions.size(); ++i)
    {
        check(canvas.contains(regions[i]), "region extends beyond design canvas");
        for (size_t j = i + 1; j < regions.size(); ++j)
            check(! regions[i].intersects(regions[j]), "design regions overlap");
    }

    // Uniform editor scaling and DPI scaling preserve the grid's geometry.
    for (const auto scale : { 0.75f, 1.0f, 2.0f })
        for (const auto dpi : { 1.0f, 1.5f, 2.0f })
        {
            const auto transform = juce::AffineTransform::scale(scale * dpi);
            for (size_t i = 0; i < regions.size(); ++i)
                for (size_t j = i + 1; j < regions.size(); ++j)
                    check(! regions[i].toFloat().transformedBy(transform).intersects(
                                 regions[j].toFloat().transformedBy(transform)), "scaled regions overlap");
        }

    const juce::String samples[] = { "Mix", "ReverseSwarm", "32 character preset name example",
                                     juce::String::fromUTF8("0123456789 100% -24 dB 20 Hz ±12 st …") };
    for (const auto& sample : samples)
        for (const auto width : { 20.0f, 80.0f, 300.0f })
        {
            const auto fitted = GrainveilUI::fitText(sample, { 0, 0, width, 22 }, 15, 9);
            check(juce::GlyphArrangement::getStringWidth(fitted.font, fitted.text) <= width + 0.01f,
                  "fitted text exceeds its width");
            check(fitted.font.getHeight() <= 22.0f, "fitted text exceeds its height");
        }
    const auto truncated = GrainveilUI::fitText(samples[2], { 0, 0, 45, 22 }, 15, 9);
    check(truncated.text.endsWith(juce::String::charToString(0x2026)), "long text should end in ellipsis");
    check(GrainveilUI::fitText("A", { 0, 0, 10, 4 }, 15, 9).text.isEmpty(), "undersized zone should draw nothing");

    // Check the drawing helper really clips ink, not just its measured advance.
    juce::Image image(juce::Image::ARGB, 120, 50, true);
    juce::Graphics graphics(image);
    graphics.setColour(juce::Colours::white);
    GrainveilUI::drawFittedText(graphics, samples[2], { 20, 10, 45, 22 }, 15, 9, juce::Justification::centred);
    bool hasInk = false, escaped = false;
    for (int y = 0; y < image.getHeight(); ++y)
        for (int x = 0; x < image.getWidth(); ++x)
            if (image.getPixelAt(x, y).getAlpha() != 0)
            {
                hasInk = true;
                if (! juce::Rectangle<int>(20, 10, 45, 22).contains(x, y)) escaped = true;
            }
    check(hasInk, "text did not render");
    check(! escaped, "text ink escaped its rectangle");
    std::cout << (failures == 0 ? "UI layout and text tests passed\n" : "UI tests failed\n");
    return failures == 0 ? 0 : 1;
}
