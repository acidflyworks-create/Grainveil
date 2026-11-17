#include "OledDisplay.h"
#include "Palette.h"
#include "UiTypography.h"
#include <cmath>

void OledDisplay::setControlState(const juce::String& mode, bool frozen)
{
    if (modeName != mode || isFrozen != frozen)
    {
        modeName = mode;
        isFrozen = frozen;
        repaint();
    }
}

void OledDisplay::setPresetName(const juce::String& name, bool modified)
{
    if (presetName != name || presetModified != modified)
    {
        presetName = name;
        presetModified = modified;
        setTooltip(name + (modified ? " (modified)" : ""));
        repaint();
    }
}

void OledDisplay::setMonitorData(const std::array<float, waveformPoints>& wave,
                                 const std::array<float, maxGrains>& markers, int count)
{
    waveform = wave;
    grainPositions = markers;
    grainCount = juce::jlimit(0, static_cast<int>(maxGrains), count);
    monitorAvailable = true;
    repaint();
}

void OledDisplay::setMonitorUnavailable()
{
    if (monitorAvailable)
    {
        monitorAvailable = false;
        repaint();
    }
}

void OledDisplay::beginParameter(const juce::String& name, const juce::String& value)
{
    parameterHeld = true;
    updateParameter(name, value);
}

void OledDisplay::updateParameter(const juce::String& name, const juce::String& value)
{
    parameterName = name;
    parameterValue = value;
    fadeDeadlineMs = juce::Time::getMillisecondCounterHiRes() + 700.0;
    repaint();
}

void OledDisplay::endParameter()
{
    parameterHeld = false;
    fadeDeadlineMs = juce::Time::getMillisecondCounterHiRes() + 700.0;
}

void OledDisplay::advanceAnimation()
{
    const auto visible = parameterHeld || juce::Time::getMillisecondCounterHiRes() < fadeDeadlineMs;
    const auto previous = parameterOpacity;
    parameterOpacity = juce::jlimit(0.0f, 1.0f, parameterOpacity + (visible ? 0.16f : -0.10f));
    if (parameterOpacity != previous)
        repaint();
}

void OledDisplay::paint(juce::Graphics& g)
{
    using namespace GrainveilUI;
    const auto outer = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff0c0f0f));
    g.fillRoundedRectangle(outer, 8.0f);
    g.setColour(juce::Colour(0xff474b46));
    g.drawRoundedRectangle(outer.reduced(0.7f), 8.0f, 1.0f);
    auto screen = outer.reduced(7.0f);
    juce::ColourGradient glass(juce::Colour(0xff1b2b26), screen.getTopLeft(),
                               juce::Colour(0xff0c1715), screen.getBottomRight(), false);
    g.setGradientFill(glass);
    g.fillRoundedRectangle(screen, 3.5f);
    g.setColour(juce::Colour(0xff263b32));
    g.drawRoundedRectangle(screen.reduced(0.5f), 3.5f, 1.0f);

    auto inner = screen.reduced(13.0f, 8.0f);
    auto header = inner.removeFromTop(21.0f);
    auto session = header.removeFromLeft(180.0f);
    header.removeFromLeft(6.0f);
    auto state = header.removeFromRight(72.0f);
    header.removeFromRight(6.0f);
    auto mode = header;

    auto modifiedZone = session.removeFromRight(12.0f);
    g.setColour(juce::Colour(0xffd5e3c9));
    drawFittedText(g, presetName, session, 11.0f, 8.0f, juce::Justification::centredLeft, true);
    if (presetModified)
    {
        g.setColour(Palette::warmBright);
        drawFittedText(g, "*", modifiedZone, 12.0f, 8.0f, juce::Justification::centred, true);
    }
    g.setColour(Palette::screenText);
    drawFittedText(g, modeName.toUpperCase(), mode, 11.5f, 8.0f, juce::Justification::centred, true);
    g.setColour(isFrozen ? Palette::warmBright : Palette::screenText.withAlpha(0.5f));
    drawFittedText(g, isFrozen ? "FROZEN" : "LIVE", state, 10.0f, 8.0f, juce::Justification::centredRight, true);

    auto footer = inner.removeFromBottom(25.0f);
    auto waveArea = inner.reduced(0.0f, 2.0f);
    g.setColour(juce::Colour(0xff2b4337).withAlpha(0.75f));
    g.drawHorizontalLine(static_cast<int>(waveArea.getCentreY()), waveArea.getX(), waveArea.getRight());
    for (int i = 1; i < 8; ++i)
    {
        const float x = waveArea.getX() + waveArea.getWidth() * static_cast<float>(i) / 8.0f;
        g.setColour(juce::Colour(0xff253b31).withAlpha(0.65f));
        g.drawVerticalLine(static_cast<int>(x), waveArea.getY(), waveArea.getBottom());
    }
    if (monitorAvailable)
    {
        const juce::Graphics::ScopedSaveState saved(g);
        juce::Path clip;
        clip.addRectangle(waveArea);
        g.reduceClipRegion(clip);
        juce::Path path;
        for (size_t i = 0; i < waveform.size(); ++i)
        {
            const float sample = std::isfinite(waveform[i]) ? juce::jlimit(-1.0f, 1.0f, waveform[i]) : 0.0f;
            const float x = waveArea.getX() + static_cast<float>(i) * waveArea.getWidth() / static_cast<float>(waveform.size() - 1);
            const float y = waveArea.getCentreY() - sample * waveArea.getHeight() * 0.42f;
            if (i == 0) path.startNewSubPath(x, y); else path.lineTo(x, y);
        }
        g.setColour(Palette::screenText);
        g.strokePath(path, juce::PathStrokeType(1.25f));
        g.setColour(Palette::warm.withAlpha(0.75f));
        for (int i = 0; i < grainCount; ++i)
        {
            const float pos = grainPositions[static_cast<size_t>(i)];
            if (! std::isfinite(pos)) continue;
            const float x = waveArea.getX() + juce::jlimit(0.0f, 1.0f, pos) * waveArea.getWidth();
            g.drawVerticalLine(static_cast<int>(x), waveArea.getY(), waveArea.getBottom());
        }
    }
    else
    {
        g.setColour(Palette::screenText.withAlpha(0.55f));
        drawFittedText(g, "BUFFER MONITOR UNAVAILABLE", waveArea.reduced(8.0f), 10.0f, 8.0f,
                       juce::Justification::centred, true);
    }

    g.setColour(juce::Colour(0xff31483b));
    g.drawHorizontalLine(static_cast<int>(footer.getY()), footer.getX(), footer.getRight());
    if (parameterOpacity > 0.0f)
    {
        auto popup = footer.reduced(0.0f, 3.0f);
        g.setColour(juce::Colour(0xff334c3b).withAlpha(0.60f * parameterOpacity));
        g.fillRoundedRectangle(popup, 3.0f);
        auto valueZone = popup.removeFromRight(120.0f).reduced(6.0f, 0.0f);
        auto nameZone = popup.reduced(8.0f, 0.0f);
        g.setColour(Palette::screenText.withAlpha(parameterOpacity));
        drawFittedText(g, parameterName.toUpperCase(), nameZone, 10.0f, 8.0f, juce::Justification::centredLeft, true);
        g.setColour(juce::Colour(0xffe5edce).withAlpha(parameterOpacity));
        drawFittedText(g, parameterValue, valueZone, 12.0f, 8.0f, juce::Justification::centredRight, true);
    }
    else
    {
        g.setColour(Palette::screenText.withAlpha(0.4f));
        drawFittedText(g, "GRAINVEIL  /  GRANULAR PROCESSOR", footer, 9.0f, 8.0f,
                       juce::Justification::centredLeft, true);
    }
}
