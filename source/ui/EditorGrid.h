#pragma once

#include <array>
#include <juce_gui_basics/juce_gui_basics.h>

// All geometry is in design-space units. A single transform maps this table to
// the host window, including child widgets, text, and hit-testing. The unused
// header zone is deliberately reserved for Stage C's preset bar.
namespace GrainveilUI::Grid
{
constexpr int width = 1000;
constexpr int height = 620;
constexpr int minWidth = 750;
constexpr int minHeight = 465;
constexpr int maxWidth = 2000;
constexpr int maxHeight = 1240;

inline const juce::Rectangle<int> presetZone { 270, 30, 460, 40 };
inline const juce::Rectangle<int> modeLabel { 50, 120, 170, 22 };
inline const juce::Rectangle<int> modeField { 50, 146, 170, 32 };
inline const juce::Rectangle<int> freezeField { 780, 145, 170, 40 };
inline const juce::Rectangle<int> display { 270, 96, 460, 140 };

struct KnobCell { juce::Rectangle<int> control, label; };
inline const std::array<KnobCell, 6> mainKnobs {{
    {{  50, 260, 140, 145 }, {  50, 410, 140, 22 }},
    {{ 202, 260, 140, 145 }, { 202, 410, 140, 22 }},
    {{ 354, 260, 140, 145 }, { 354, 410, 140, 22 }},
    {{ 506, 260, 140, 145 }, { 506, 410, 140, 22 }},
    {{ 658, 260, 140, 145 }, { 658, 410, 140, 22 }},
    {{ 810, 260, 140, 145 }, { 810, 410, 140, 22 }}
}};
inline const std::array<KnobCell, 4> secondaryKnobs {{
    {{ 194, 455, 130, 120 }, { 194, 580, 130, 22 }},
    {{ 355, 455, 130, 120 }, { 355, 580, 130, 22 }},
    {{ 516, 455, 130, 120 }, { 516, 580, 130, 22 }},
    {{ 677, 455, 130, 120 }, { 677, 580, 130, 22 }}
}};
}
