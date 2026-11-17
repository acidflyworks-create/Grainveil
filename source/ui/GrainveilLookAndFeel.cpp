#include "GrainveilLookAndFeel.h"
#include "Palette.h"
#include "UiTypography.h"
#include <cmath>

using namespace GrainveilUI;

GrainveilLookAndFeel::GrainveilLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, Palette::text.withAlpha(0.8f));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxHighlightColourId, Palette::warm.withAlpha(0.35f));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff171b1b));
    setColour(juce::ComboBox::textColourId, Palette::text);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff505751));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff1b201e));
    setColour(juce::PopupMenu::textColourId, Palette::text);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff383f36));
    setColour(juce::AlertWindow::backgroundColourId, juce::Colour(0xff202522));
    setColour(juce::AlertWindow::textColourId, Palette::text);
    setColour(juce::AlertWindow::outlineColourId, Palette::muted);
    setColour(juce::TextEditor::textColourId, Palette::text);
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff161b18));
    setColour(juce::TextEditor::highlightColourId, Palette::warm.withAlpha(0.35f));
}

juce::Font GrainveilLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return font(juce::jmin(13.5f, static_cast<float>(box.getHeight()) * 0.6f), true);
}
juce::Font GrainveilLookAndFeel::getPopupMenuFont() { return font(13.0f); }
juce::Font GrainveilLookAndFeel::getSliderPopupFont(juce::Slider&) { return font(13.0f, true); }
juce::Font GrainveilLookAndFeel::getAlertWindowTitleFont() { return font(18.0f, true); }
juce::Font GrainveilLookAndFeel::getAlertWindowMessageFont() { return font(15.0f); }
juce::Font GrainveilLookAndFeel::getAlertWindowFont() { return font(13.0f); }
juce::Font GrainveilLookAndFeel::getTextButtonFont(juce::TextButton&, int height)
{
    return font(juce::jmin(13.0f, static_cast<float>(height) * 0.65f), true);
}

void GrainveilLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                const juce::Colour&, bool highlighted, bool down)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(down ? juce::Colour(0xff3b4438) : (highlighted ? juce::Colour(0xff343b34) : juce::Colour(0xff1a201d)));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(highlighted ? Palette::warm : Palette::muted.withAlpha(0.7f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void GrainveilLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    g.setColour(Palette::text.withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.45f));
    drawFittedText(g, button.getButtonText(), button.getLocalBounds().toFloat().reduced(8.0f, 3.0f),
                   getTextButtonFont(button, button.getHeight()).getHeight(), 9.0f, juce::Justification::centred, true);
}

juce::Label* GrainveilLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox(slider);
    label->setFont(font(12.0f));
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
        drawFittedText(g, label.getText(), textBounds, label.getFont().getHeight(), 9.0f, label.getJustificationType());
    }
    g.setColour(label.findColour(juce::Label::outlineColourId).withMultipliedAlpha(alpha));
    g.drawRect(label.getLocalBounds());
}

void GrainveilLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height, juce::TextEditor&)
{
    g.setColour(juce::Colour(0xff161b18));
    g.fillRoundedRectangle(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 3.0f);
}

void GrainveilLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor&)
{
    g.setColour(Palette::warm.withAlpha(0.8f));
    g.drawRoundedRectangle(0.5f, 0.5f, static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f, 3.0f, 1.0f);
}

void GrainveilLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float position, float start, float end, juce::Slider& slider)
{
    const float cx = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float radius = juce::jmax(1.0f, static_cast<float>(juce::jmin(width, height)) * 0.5f - 9.0f);
    const float angle = start + juce::jlimit(0.0f, 1.0f, position) * (end - start);
    const bool active = slider.isMouseOverOrDragging();

    juce::Path arc;
    arc.addCentredArc(cx, cy, radius - 1.0f, radius - 1.0f, 0.0f, start, end, true);
    g.setColour(juce::Colour(0xff565c55).withAlpha(0.55f));
    g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path valueArc;
    valueArc.addCentredArc(cx, cy, radius - 1.0f, radius - 1.0f, 0.0f, start, angle, true);
    g.setColour(active ? Palette::warmBright : Palette::warm);
    g.strokePath(valueArc, juce::PathStrokeType(2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    constexpr int ticks = 21;
    for (int i = 0; i < ticks; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(ticks - 1);
        const float a = start + t * (end - start);
        const float inner = radius + 4.0f, outer = radius + (i % 5 == 0 ? 7.0f : 5.5f);
        g.setColour((t <= position ? Palette::warm : Palette::muted).withAlpha(t <= position ? 0.8f : 0.45f));
        g.drawLine(cx + std::sin(a) * inner, cy - std::cos(a) * inner,
                   cx + std::sin(a) * outer, cy - std::cos(a) * outer, 1.0f);
    }

    const float cap = radius * 0.73f;
    auto capBounds = juce::Rectangle<float>(cx - cap, cy - cap, cap * 2.0f, cap * 2.0f);
    g.setColour(juce::Colours::black.withAlpha(0.38f));
    g.fillEllipse(capBounds.translated(0.0f, 3.0f).expanded(2.0f));
    juce::ColourGradient capGradient(active ? juce::Colour(0xff696d65) : juce::Colour(0xff5d625b),
                                     cx - cap * 0.35f, cy - cap,
                                     juce::Colour(0xff202422), cx + cap * 0.3f, cy + cap, false);
    g.setGradientFill(capGradient);
    g.fillEllipse(capBounds);
    g.setColour(juce::Colour(0xff8b9086).withAlpha(active ? 0.8f : 0.5f));
    g.drawEllipse(capBounds.reduced(0.5f), 1.0f);
    g.setColour(juce::Colour(0xff181b1a));
    g.drawEllipse(capBounds.reduced(cap * 0.15f), 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.10f));
    g.drawEllipse(capBounds.reduced(cap * 0.22f).withTrimmedBottom(cap * 0.55f), 0.7f);
    const float innerPointer = cap * 0.38f, outerPointer = cap * 0.77f;
    g.setColour(active ? Palette::warmBright : Palette::warm);
    g.drawLine(cx + std::sin(angle) * innerPointer, cy - std::cos(angle) * innerPointer,
               cx + std::sin(angle) * outerPointer, cy - std::cos(angle) * outerPointer, 2.7f);
}

void GrainveilLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool down,
                                        int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(0.5f);
    g.setColour(box.isMouseOverOrDragging() || down ? juce::Colour(0xff303731) : juce::Colour(0xff191e1d));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(box.isMouseOverOrDragging() ? Palette::warm.withAlpha(0.8f) : juce::Colour(0xff505751));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    g.setColour(juce::Colour(0xff505751));
    g.drawVerticalLine(width - 32, 6.0f, static_cast<float>(height) - 6.0f);
    const float cx = static_cast<float>(width) - 16.0f, cy = static_cast<float>(height) * 0.5f;
    juce::Path arrow;
    arrow.startNewSubPath(cx - 4.0f, cy - 2.0f);
    arrow.lineTo(cx, cy + 2.0f);
    arrow.lineTo(cx + 4.0f, cy - 2.0f);
    g.setColour(Palette::warm);
    g.strokePath(arrow, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void GrainveilLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(9, 1, juce::jmax(0, box.getWidth() - 44), box.getHeight() - 2);
    label.setFont(getComboBoxFont(box));
    label.setJustificationType(juce::Justification::centredLeft);
}

void GrainveilLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                            bool highlighted, bool down)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const bool on = button.getToggleState();
    g.setColour(juce::Colours::black.withAlpha(0.30f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 2.0f), 5.0f);
    juce::ColourGradient gradient(on ? juce::Colour(0xff4b493a) : juce::Colour(0xff3a403c), bounds.getTopLeft(),
                                  down ? juce::Colour(0xff151816) : juce::Colour(0xff202421), bounds.getBottomLeft(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(highlighted ? Palette::warm : juce::Colour(0xff61665b));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.0f);
    const float x = bounds.getX() + 19.0f, y = bounds.getCentreY();
    if (on)
    {
        juce::ColourGradient glow(Palette::warmBright.withAlpha(0.48f), x, y,
                                  Palette::warmBright.withAlpha(0.0f), x + 12.0f, y, true);
        g.setGradientFill(glow);
        g.fillEllipse(x - 12.0f, y - 12.0f, 24.0f, 24.0f);
    }
    g.setColour(on ? Palette::warmBright : juce::Colour(0xff504b38));
    g.fillEllipse(x - 3.5f, y - 3.5f, 7.0f, 7.0f);
    g.setColour(on ? juce::Colour(0xfffff0cf) : Palette::text.withAlpha(0.8f));
    drawFittedText(g, button.getButtonText(), bounds.withTrimmedLeft(35.0f).withTrimmedRight(10.0f),
                   12.0f, 9.0f, juce::Justification::centred, true);
}

void GrainveilLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(juce::Colour(0xff1b201e));
    g.setColour(juce::Colour(0xff656a5d));
    g.drawRect(0, 0, width, height, 1);
}

void GrainveilLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                             bool separator, bool active, bool highlighted, bool ticked, bool subMenu,
                                             const juce::String& text, const juce::String& shortcut,
                                             const juce::Drawable*, const juce::Colour* colour)
{
    auto r = area.toFloat().reduced(4.0f, 1.0f);
    if (separator)
    {
        g.setColour(Palette::muted.withAlpha(0.3f));
        g.drawHorizontalLine(static_cast<int>(r.getCentreY()), r.getX() + 4.0f, r.getRight() - 4.0f);
        return;
    }
    if (highlighted && active)
    {
        g.setColour(juce::Colour(0xff3b4339));
        g.fillRoundedRectangle(r, 3.0f);
    }
    auto tickZone = r.removeFromLeft(19.0f);
    auto rightZone = r.removeFromRight(subMenu ? 18.0f : (shortcut.isNotEmpty() ? 68.0f : 0.0f));
    g.setColour(Palette::warm.withAlpha(active ? 1.0f : 0.4f));
    if (ticked) g.fillEllipse(tickZone.getCentreX() - 2.0f, tickZone.getCentreY() - 2.0f, 4.0f, 4.0f);
    if (subMenu)
    {
        juce::Path arrow;
        arrow.addTriangle(rightZone.getCentreX() - 2.0f, rightZone.getCentreY() - 4.0f,
                          rightZone.getCentreX() + 2.0f, rightZone.getCentreY(),
                          rightZone.getCentreX() - 2.0f, rightZone.getCentreY() + 4.0f);
        g.fillPath(arrow);
    }
    else if (shortcut.isNotEmpty())
        drawFittedText(g, shortcut, rightZone, 11.0f, 8.0f, juce::Justification::centredRight);
    g.setColour((colour != nullptr ? *colour : Palette::text).withMultipliedAlpha(active ? 1.0f : 0.4f));
    drawFittedText(g, text, r, 13.0f, 9.0f, juce::Justification::centredLeft);
}

void GrainveilLookAndFeel::drawPopupMenuSectionHeader(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                       const juce::String& text)
{
    g.setColour(Palette::warm.withAlpha(0.8f));
    drawFittedText(g, text, area.toFloat().reduced(12.0f, 2.0f), 11.0f, 8.0f, juce::Justification::centredLeft, true);
}

juce::Rectangle<int> GrainveilLookAndFeel::getTooltipBounds(const juce::String& text, juce::Point<int> pos,
                                                            juce::Rectangle<int> parent)
{
    const auto textWidth = juce::GlyphArrangement::getStringWidth(font(12.0f), text);
    const int w = juce::jmin(juce::jmax(1, parent.getWidth()), juce::jmax(80, static_cast<int>(std::ceil(textWidth)) + 24));
    return juce::Rectangle<int>(pos.x + 16, pos.y + 20, w, 28).constrainedWithin(parent);
}

void GrainveilLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height)
{
    g.fillAll(juce::Colour(0xff191d1b));
    g.setColour(Palette::warm.withAlpha(0.65f));
    g.drawRect(0, 0, width, height, 1);
    g.setColour(Palette::text);
    drawFittedText(g, text, juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)).reduced(10.0f, 3.0f),
                   12.0f, 9.0f, juce::Justification::centredLeft);
}

void GrainveilLookAndFeel::drawCornerResizer(juce::Graphics& g, int width, int height, bool over, bool dragging)
{
    g.setColour(Palette::warm.withAlpha(over || dragging ? 0.8f : 0.3f));
    for (int i = 0; i < 3; ++i)
    {
        const float offset = static_cast<float>(5 + i * 4);
        g.drawLine(static_cast<float>(width) - offset, static_cast<float>(height) - 3.0f,
                   static_cast<float>(width) - 3.0f, static_cast<float>(height) - offset, 1.0f);
    }
}
