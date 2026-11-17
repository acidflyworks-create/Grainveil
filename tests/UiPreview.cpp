#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ui/EditorGrid.h"
#include "ui/OledDisplay.h"
#include <cmath>
#include <iostream>
#include <vector>

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

// Check the actual components, not just the design-grid constants. Only peers
// are compared: a slider's editable value is intentionally inside the slider.
bool checkPeers(juce::Component& parent, const juce::AffineTransform& transform,
                juce::Rectangle<float> viewport, const char* description)
{
    std::vector<juce::Rectangle<float>> regions;
    for (auto* child : parent.getChildren())
    {
        if (! child->isVisible()) continue;
        const auto rect = child->getBounds();
        if (rect.isEmpty() || ! parent.getLocalBounds().contains(rect))
        {
            std::cerr << description << ": child outside parent or empty\n";
            return false;
        }
        const auto mapped = rect.toFloat().transformedBy(transform);
        if (! viewport.expanded(0.01f).contains(mapped))
        {
            std::cerr << description << ": child outside viewport\n";
            return false;
        }
        for (const auto& other : regions)
            if (mapped.intersects(other))
            {
                std::cerr << description << ": visible siblings overlap\n";
                return false;
            }
        regions.push_back(mapped);
    }
    return true;
}
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc != 2) { std::cerr << "Expected output directory\n"; return 1; }
    auto dir = juce::File::getCurrentWorkingDirectory().getChildFile(juce::String::fromUTF8(argv[1]));
    const auto tempRoot = juce::File::getSpecialLocation(juce::File::tempDirectory)
                              .getNonexistentChildFile("grainveil-preview-", {}, false);
    if (tempRoot.createDirectory().failed()) return 1;
    GrainveilAudioProcessor processor(tempRoot);
    GrainveilAudioProcessorEditor editor(processor);
    editor.setVisible(true);

    // The controls create internal labels before parenting. Ensure that the
    // editor rebuilt those labels with its embedded font and own theme.
    int sliderCount = 0, correctlyStyled = 0;
    std::function<void(juce::Component&)> inspect = [&](juce::Component& component)
    {
        if (auto* slider = dynamic_cast<juce::Slider*>(&component))
        {
            ++sliderCount;
            for (auto* child : slider->getChildren())
                if (auto* label = dynamic_cast<juce::Label*>(child))
                    if (label->getFont().getTypefacePtr()->getName().containsIgnoreCase("Lato")
                        && label->findColour(juce::Label::outlineColourId).isTransparent())
                        ++correctlyStyled;
        }
        for (auto* child : component.getChildren()) inspect(*child);
    };
    inspect(editor);
    if (sliderCount != 10 || correctlyStyled != sliderCount)
    {
        std::cerr << "Slider value fields did not inherit the embedded-font theme: "
                  << correctlyStyled << "/" << sliderCount << '\\n';
        return 1;
    }

    juce::Component* surface = nullptr;
    PresetBar* presetBar = nullptr;
    OledDisplay* oled = nullptr;
    for (auto* candidate : editor.getChildren())
        for (auto* child : candidate->getChildren())
        {
            if (auto* found = dynamic_cast<PresetBar*>(child)) { surface = candidate; presetBar = found; }
            if (auto* found = dynamic_cast<OledDisplay*>(child)) oled = found;
        }
    if (surface == nullptr || presetBar == nullptr || oled == nullptr || surface->getNumChildComponents() != 25)
    {
        std::cerr << "Unexpected editor component hierarchy\n";
        return 1;
    }
    if (! checkPeers(*presetBar, juce::AffineTransform(), presetBar->getLocalBounds().toFloat(), "preset bar")) return 1;
    for (auto* child : surface->getChildren())
        if (auto* slider = dynamic_cast<juce::Slider*>(child))
            for (auto* part : slider->getChildren())
                if (dynamic_cast<juce::Label*>(part) != nullptr &&
                    (part->getBounds().isEmpty() || ! slider->getLocalBounds().contains(part->getBounds())))
                {
                    std::cerr << "Slider value field outside its control\n";
                    return 1;
                }

    struct Size { int w, h; const char* name; };
    const Size sizes[] = { {750, 465, "min"}, {1000, 620, "default"}, {2000, 1240, "max"} };
    struct Scale { float value; const char* name; };
    const Scale dpis[] = { {1.0f, "100"}, {1.5f, "150"}, {2.0f, "200"} };
    for (const auto& size : sizes)
    {
        editor.setSize(size.w, size.h);
        for (const auto& dpi : dpis)
        {
            const auto transform = surface->getTransform().followedBy(juce::AffineTransform::scale(dpi.value));
            const auto viewport = editor.getLocalBounds().toFloat().transformedBy(juce::AffineTransform::scale(dpi.value));
            if (! checkPeers(*surface, transform, viewport, "editor surface")) return 1;
            const auto name = juce::String(size.name) + "-dpi" + dpi.name + ".png";
            if (! saveComponent(editor, dir.getChildFile(name), dpi.value))
            {
                std::cerr << "Unable to write " << name << '\n';
                return 1;
            }
        }
    }

    // Stress layout with the longest allowed name, modified marker, longest mode
    // and parameter extremes. This is synthetic UI state, not a host screenshot.
    auto& manager = processor.getPresetManager();
    processor.setCurrentProgram(2); // ReverseSwarm
    manager.servicePendingProgram();
    const juce::String longName = "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW";
    juce::String error;
    if (! manager.saveUserPreset(longName, false, error))
    {
        std::cerr << "Cannot prepare long-name fixture: " << error << '\n';
        return 1;
    }
    for (auto* child : surface->getChildren())
        if (auto* slider = dynamic_cast<juce::Slider*>(child)) slider->setValue(slider->getMaximum(), juce::sendNotificationSync);
    presetBar->refresh();
    oled->setPresetName(longName, true);
    oled->setControlState("ReverseSwarm", true);
    bool fullTooltip = false;
    for (auto* child : presetBar->getChildren())
        if (auto* button = dynamic_cast<juce::Button*>(child))
            if (button->getTooltip() == longName + " (modified)") fullTooltip = true;
    if (! manager.isModified() || ! fullTooltip)
    {
        std::cerr << "Long-name tooltip or modified marker fixture not set\n";
        return 1;
    }
    for (const auto& size : sizes)
    {
        editor.setSize(size.w, size.h);
        for (const auto& dpi : dpis)
        {
            const auto name = juce::String("stress-") + size.name + "-dpi" + dpi.name + ".png";
            if (! saveComponent(editor, dir.getChildFile(name), dpi.value)) return 1;
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
    if (! tempRoot.deleteRecursively()) { std::cerr << "Could not clean preview fixture\n"; return 1; }
    std::cout << "Checked actual peer geometry and rendered normal/stress editor at 3 sizes and 3 simulated DPIs; display fixture is synthetic.\n";
    return 0;
}
