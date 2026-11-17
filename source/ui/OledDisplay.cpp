#include "OledDisplay.h"
#include "../Parameters.h"

void OledDisplay::setSnapshot(const GrainveilDsp::TelemetrySnapshot& newSnapshot)
{
    snapshot = newSnapshot;
    repaint();
}

void OledDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // OLED bezel & screen
    g.setColour(juce::Colour(0xff0b0d0c));
    g.fillRoundedRectangle(bounds, 5.0f);
    g.setColour(juce::Colour(0xff383f38));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 5.0f, 1.2f);

    auto inner = bounds.reduced(8.0f, 6.0f);

    // Header: Mode name + Freeze badge
    auto header = inner.removeFromTop(18.0f);
    const auto modeNames = GrainveilParams::getModeNames();
    const juce::String modeName = (snapshot.mode >= 0 && snapshot.mode < modeNames.size())
                                      ? modeNames[snapshot.mode]
                                      : "Cloud";

    g.setColour(juce::Colour(0xffe6a832));
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText(modeName.toUpperCase(), header, juce::Justification::centredLeft, false);

    if (snapshot.frozen)
    {
        g.setColour(juce::Colour(0xff7ed957));
        g.drawText("FREEZE", header, juce::Justification::centredRight, false);
    }

    // Waveform area
    auto waveArea = inner.reduced(0.0f, 4.0f);
    const float midY = waveArea.getCentreY();
    const float halfH = waveArea.getHeight() * 0.45f;

    g.setColour(juce::Colour(0xff1e2820));
    g.drawHorizontalLine(static_cast<int>(midY), waveArea.getX(), waveArea.getRight());

    juce::Path wavePath;
    const int n = GrainveilDsp::TelemetrySnapshot::waveformPoints;
    for (int i = 0; i < n; ++i)
    {
        const float x = waveArea.getX() + (static_cast<float>(i) / static_cast<float>(n - 1)) * waveArea.getWidth();
        const float y = midY - juce::jlimit(-1.0f, 1.0f, snapshot.waveform[static_cast<size_t>(i)]) * halfH;
        if (i == 0)
            wavePath.startNewSubPath(x, y);
        else
            wavePath.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xff7ed957).withAlpha(0.85f));
    g.strokePath(wavePath, juce::PathStrokeType(1.3f));

    // Grain position ticks
    g.setColour(juce::Colour(0xffe6a832).withAlpha(0.8f));
    for (int i = 0; i < snapshot.activeGrainCount; ++i)
    {
        const float normX = snapshot.grainPositions[static_cast<size_t>(i)];
        const float gx = waveArea.getX() + normX * waveArea.getWidth();
        g.drawVerticalLine(static_cast<int>(gx), waveArea.getY(), waveArea.getBottom());
    }
}
