#include "GrainveilLookAndFeel.h"
#include "UiTypography.h"
#include <cmath>

GrainveilLookAndFeel::GrainveilLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffd8d2b8));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff141718));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff181c1d));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe0d6b3));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff4a5248));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1a1e20));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe0d6b3));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff3e4a38));
}

juce::Font GrainveilLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return GrainveilUI::font(juce::jmin(15.0f, static_cast<float>(box.getHeight()) * 0.7f));
}

juce::Font GrainveilLookAndFeel::getPopupMenuFont() { return GrainveilUI::font(15.0f); }
juce::Font GrainveilLookAndFeel::getSliderPopupFont(juce::Slider&) { return GrainveilUI::font(15.0f, true); }

juce::Label* GrainveilLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->setFont(GrainveilUI::font(13.0f));
    return label;
}

void GrainveilLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    const float alpha = label.isEnabled() ? 1.0f : 0.5f;
    if (! label.isBeingEdited())
    {
        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
        const auto textBounds = getLabelBorderSize(label).subtractedFrom(label.getLocalBounds()).toFloat();
        GrainveilUI::drawFittedText(g, label.getText(), textBounds, label.getFont().getHeight(),
                                   9.0f, label.getJustificationType());
    }
    g.setColour(label.findColour(juce::Label::outlineColourId).withMultipliedAlpha(alpha));
    g.drawRect(label.getLocalBounds());
}

void GrainveilLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                            int x, int y, int width, int height,
                                            float sliderPosProportional,
                                            float rotaryStartAngle,
                                            float rotaryEndAngle,
                                            juce::Slider&)
{
    const float radius = static_cast<float>(juce::jmin(width, height)) * 0.5f - 6.0f;
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Analog tick marks around knob
    g.setColour(juce::Colour(0xff6b7265));
    constexpr int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(numTicks - 1);
        const float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        const float rInner = radius + 2.0f;
        const float rOuter = radius + 5.5f;
        const float x1 = cx + std::sin(tickAngle) * rInner;
        const float y1 = cy - std::cos(tickAngle) * rInner;
        const float x2 = cx + std::sin(tickAngle) * rOuter;
        const float y2 = cy - std::cos(tickAngle) * rOuter;
        g.drawLine(x1, y1, x2, y2, (i == 0 || i == numTicks - 1 || i == numTicks / 2) ? 1.5f : 1.0f);
    }

    // Knob body
    const float knobR = radius - 2.0f;
    juce::ColourGradient grad(juce::Colour(0xff34393b), cx, cy - knobR,
                              juce::Colour(0xff171a1b), cx, cy + knobR, false);
    g.setGradientFill(grad);
    g.fillEllipse(cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f);

    // Outer metallic rim
    g.setColour(juce::Colour(0xff5a6058));
    g.drawEllipse(cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f, 1.2f);

    // Pointer indicator
    const float pLen = knobR * 0.72f;
    const float px = cx + std::sin(angle) * pLen;
    const float py = cy - std::cos(angle) * pLen;
    g.setColour(juce::Colour(0xffe6a832)); // Amber indicator
    g.drawLine(cx, cy, px, py, 2.2f);
}

void GrainveilLookAndFeel::drawComboBox(juce::Graphics& g,
                                        int width, int height,
                                        bool,
                                        int, int, int, int,
                                        juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(1.0f);
    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

    // Small arrow
    juce::Path arrow;
    const float ax = static_cast<float>(width) - 14.0f;
    const float ay = static_cast<float>(height) * 0.5f - 2.0f;
    arrow.addTriangle(ax - 4.0f, ay, ax + 4.0f, ay, ax, ay + 5.0f);
    g.setColour(juce::Colour(0xffe6a832));
    g.fillPath(arrow);
}

void GrainveilLookAndFeel::drawToggleButton(juce::Graphics& g,
                                            juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const bool isOn = button.getToggleState();

    // Stomp pad housing
    g.setColour(isOn ? juce::Colour(0xff2d3328) : juce::Colour(0xff1e2224));
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(shouldDrawButtonAsHighlighted ? juce::Colour(0xff8c9684) : juce::Colour(0xff4e544c));
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

    // LED indicator
    const float ledSize = 10.0f;
    const float ledX = bounds.getX() + 12.0f;
    const float ledY = bounds.getCentreY() - ledSize * 0.5f;
    if (isOn)
    {
        g.setColour(juce::Colour(0x66e6a832));
        g.fillEllipse(ledX - 3.0f, ledY - 3.0f, ledSize + 6.0f, ledSize + 6.0f);
        g.setColour(juce::Colour(0xffffb833));
    }
    else
    {
        g.setColour(juce::Colour(0xff3a3528));
    }
    g.fillEllipse(ledX, ledY, ledSize, ledSize);

    g.setColour(isOn ? juce::Colour(0xffffe099) : juce::Colour(0xffb8b4a4));
    GrainveilUI::drawFittedText(g, button.getButtonText(), bounds.withTrimmedLeft(26.0f),
                               13.0f, 9.0f, juce::Justification::centred, true);
}
