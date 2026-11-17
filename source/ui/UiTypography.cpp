#include "UiTypography.h"
#include <BinaryData.h>

namespace GrainveilUI
{
namespace
{
juce::Typeface::Ptr typeface(bool semibold)
{
    static const auto regular = juce::Typeface::createSystemTypefaceFor(
        BinaryData::LatoRegular_ttf, BinaryData::LatoRegular_ttfSize);
    static const auto medium = juce::Typeface::createSystemTypefaceFor(
        BinaryData::LatoSemiBold_ttf, BinaryData::LatoSemiBold_ttfSize);
    return semibold ? medium : regular;
}
}

juce::Font font(float height, bool semibold)
{
    return juce::Font(juce::FontOptions(typeface(semibold)).withHeight(juce::jmax(1.0f, height)));
}

FittedText fitText(const juce::String& text, juce::Rectangle<float> bounds,
                   float preferredHeight, float minimumHeight, bool semibold)
{
    const auto width = juce::jmax(0.0f, bounds.getWidth());
    const auto height = juce::jmax(0.0f, bounds.getHeight());
    const auto minHeight = juce::jmax(1.0f, minimumHeight);
    const auto maxHeight = juce::jmin(juce::jmax(minHeight, preferredHeight), height);
    if (width <= 0.0f || maxHeight < minHeight || text.isEmpty())
        return { {}, font(minHeight, semibold) };

    auto selectedFont = font(maxHeight, semibold);
    auto measure = [](const juce::Font& f, const juce::String& s)
    {
        return juce::GlyphArrangement::getStringWidth(f, s);
    };

    if (measure(selectedFont, text) <= width)
        return { text, selectedFont };

    // Width is monotonic with font height: search for the largest fitting size.
    float low = minHeight, high = maxHeight;
    for (int i = 0; i < 12; ++i)
    {
        const auto mid = (low + high) * 0.5f;
        if (measure(font(mid, semibold), text) <= width)
            low = mid;
        else
            high = mid;
    }
    selectedFont = font(low, semibold);
    if (measure(selectedFont, text) <= width)
        return { text, selectedFont };

    const juce::String ellipsis = juce::String::charToString(0x2026);
    if (measure(selectedFont, ellipsis) > width)
        return { {}, selectedFont };

    // JUCE String indexes Unicode characters, not UTF-8 bytes.
    int left = 0, right = text.length();
    while (left < right)
    {
        const int mid = left + (right - left + 1) / 2;
        if (measure(selectedFont, text.substring(0, mid) + ellipsis) <= width)
            left = mid;
        else
            right = mid - 1;
    }
    return { text.substring(0, left) + ellipsis, selectedFont };
}

void drawFittedText(juce::Graphics& g, const juce::String& text,
                    juce::Rectangle<float> bounds, float preferredHeight,
                    float minimumHeight, juce::Justification justification,
                    bool semibold)
{
    if (bounds.isEmpty())
        return;
    const auto fitted = fitText(text, bounds, preferredHeight, minimumHeight, semibold);
    if (fitted.text.isEmpty())
        return;
    const juce::Graphics::ScopedSaveState saved(g);
    juce::Path clip;
    clip.addRectangle(bounds);
    g.reduceClipRegion(clip);
    g.setFont(fitted.font);
    g.drawText(fitted.text, bounds, justification, false);
}
}
