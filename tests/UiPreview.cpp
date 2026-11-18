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

// Exercise the actual slider paint path, including JUCE's rotary bounds and
// editable value box, at both endpoints and halfway. No host mouse is needed.
bool checkKnobPainting(juce::Slider& slider)
{
    const auto original = slider.getValue();
    const auto bounds = slider.getLookAndFeel().getSliderLayout(slider).sliderBounds.toFloat();
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const float radius = juce::jmax(1.0f, juce::jmin(bounds.getWidth(), bounds.getHeight()) * .5f - 9.0f);
    const float cap = radius * .73f;
    const auto angles = slider.getRotaryParameters();
    auto imageAt = [&](double proportion)
    {
        slider.setValue(slider.proportionOfLengthToValue(proportion), juce::sendNotificationSync);
        juce::Image image(juce::Image::ARGB, slider.getWidth(), slider.getHeight(), true);
        { juce::Graphics graphics(image); slider.paintEntireComponent(graphics, true); }
        return image;
    };
    const auto low = imageAt(0.0), mid = imageAt(0.5), high = imageAt(1.0);
    slider.setValue(original, juce::sendNotificationSync);

    auto isWarm = [](juce::Colour c)
    {
        return c.getAlpha() > 180 && c.getRed() > 100 &&
               c.getRed() > c.getBlue() * 1.35f && c.getGreen() > c.getBlue() * 1.20f;
    };
    auto warmAt = [&](const juce::Image& image, float angle, float distance)
    {
        const int x = juce::roundToInt(cx + std::sin(angle) * distance);
        const int y = juce::roundToInt(cy - std::cos(angle) * distance);
        for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx)
            if (image.getBounds().contains(x + dx, y + dy) && isWarm(image.getPixelAt(x + dx, y + dy))) return true;
        return false;
    };
    bool ok = true;
    if (std::abs(cy - slider.getHeight() * .5f) < 1.0f)
    {
        std::cerr << slider.getTitle() << ": rotary centre unexpectedly includes the value box\n"; ok = false;
    }
    const float start = angles.startAngleRadians, end = angles.endAngleRadians;
    if (! warmAt(low, start, cap * .58f) || ! warmAt(mid, (start + end) * .5f, cap * .58f)
        || ! warmAt(high, end, cap * .58f))
    {
        std::cerr << slider.getTitle() << ": pointer did not follow rotary angle\n"; ok = false;
    }
    if (warmAt(low, (start + end) * .5f, cap * .58f) || warmAt(high, start, cap * .58f))
    {
        std::cerr << slider.getTitle() << ": pointer remained at an earlier angle\n"; ok = false;
    }
    // These samples lie on the value track, away from the cap and tick ring.
    const float quarter = start + (end - start) * .25f, threeQuarter = start + (end - start) * .75f;
    if (warmAt(low, quarter, radius - 1.0f) || ! warmAt(mid, quarter, radius - 1.0f)
        || warmAt(mid, threeQuarter, radius - 1.0f) || ! warmAt(high, threeQuarter, radius - 1.0f))
    {
        std::cerr << slider.getTitle() << ": value arc did not grow along the fixed track\n"; ok = false;
    }
    int tickPixels = 0, capChanges = 0;
    for (int y = 0; y < slider.getHeight(); ++y) for (int x = 0; x < slider.getWidth(); ++x)
    {
        const float dx = x + .5f - cx, dy = y + .5f - cy;
        const float d = std::sqrt(dx * dx + dy * dy);
        if (d >= radius + 3.5f && d <= radius + 8.0f && low.getPixelAt(x,y).getAlpha() != 0)
        {
            ++tickPixels;
            if (low.getPixelAt(x,y) != mid.getPixelAt(x,y) || low.getPixelAt(x,y) != high.getPixelAt(x,y))
            { std::cerr << slider.getTitle() << ": tick ring changed with the value\n"; return false; }
        }
        // At the centre the pointer cannot reach: rotating cap gradient/detail
        // must itself change independently of the moving pointer or value arc.
        if (d < cap * .25f && low.getPixelAt(x,y) != mid.getPixelAt(x,y)) ++capChanges;
    }
    if (tickPixels < 20 || capChanges < 10)
    {
        std::cerr << slider.getTitle() << ": missing static tick ink or rotating cap detail\n"; ok = false;
    }
    return ok;
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
    int checkedKnobs = 0;
    for (auto* child : surface->getChildren())
        if (auto* slider = dynamic_cast<juce::Slider*>(child))
            if (slider->getTitle() == "MIX" || slider->getTitle() == "CAPTURE")
            {
                if (! checkKnobPainting(*slider)) return 1;
                ++checkedKnobs;
            }
    if (checkedKnobs != 2) { std::cerr << "Missing large or small knob paint fixture\n"; return 1; }
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
    if (auto* freeze = processor.getAPVTS().getParameter(GrainveilParams::freeze))
        freeze->setValueNotifyingHost(1.0f);
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
