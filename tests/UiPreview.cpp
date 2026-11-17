#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/EditorGrid.h"
#include "ui/OledDisplay.h"
#include <cmath>
#include <iostream>

namespace
{
bool saveComponent(juce::Component& component, const juce::File& file, float dpiScale)
{
    const int w = juce::roundToInt(static_cast<float>(component.getWidth()) * dpiScale);
    const int h = juce::roundToInt(static_cast<float>(component.getHeight()) * dpiScale);
    juce::Image image(juce::Image::ARGB, w, h, true);
    {
        juce::Graphics g(image);
        g.addTransform(juce::AffineTransform::scale(dpiScale));
        component.paintEntireComponent(g, true);
    }
    if (! file.getParentDirectory().createDirectory()) return false;
    auto stream = file.createOutputStream();
    if (stream == nullptr) return false;
    juce::PNGImageFormat png;
    return png.writeImageToStream(image, *stream);
}
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc != 2) { std::cerr << "Expected output directory\n"; return 1; }
    auto dir = juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[1]));
    GrainveilAudioProcessor processor;
    GrainveilAudioProcessorEditor editor(processor);
    editor.setVisible(true);

    struct Size { int w, h; const char* name; };
    const Size sizes[] = { {750, 465, "min"}, {1000, 620, "default"}, {2000, 1240, "max"} };
    struct Scale { float value; const char* name; };
    const Scale dpis[] = { {1.0f, "100"}, {1.5f, "150"}, {2.0f, "200"} };
    for (const auto& size : sizes)
    {
        editor.setSize(size.w, size.h);
        for (const auto& dpi : dpis)
        {
            const auto name = juce::String(size.name) + "-dpi" + dpi.name + ".png";
            if (! saveComponent(editor, dir.getChildFile(name), dpi.value))
            {
                std::cerr << "Unable to write " << name << '\n';
                return 1;
            }
        }
    }

    // Exercise the display's drawing path with explicitly synthetic data. No
    // unsafe audio-thread telemetry is accessed by this standalone fixture.
    OledDisplay display;
    display.setSize(460, 140);
    display.setControlState("ReverseSwarm", true);
    std::array<float, OledDisplay::waveformPoints> wave {};
    std::array<float, OledDisplay::maxGrains> grains {};
    for (size_t i = 0; i < wave.size(); ++i)
        wave[i] = 0.7f * std::sin(static_cast<float>(i) * 0.2f);
    grains[0] = 0.24f; grains[1] = 0.55f; grains[2] = 0.78f;
    display.setMonitorData(wave, grains, 3);
    display.beginParameter("FEEDBACK", "49.5 %");
    for (int i = 0; i < 8; ++i) display.advanceAnimation();
    if (! saveComponent(display, dir.getChildFile("display-synthetic-fixture.png"), 2.0f)) return 1;
    std::cout << "Rendered editor at 3 sizes and 3 simulated DPIs; display fixture is synthetic.\n";
    return 0;
}
