#include "PluginEditor.h"
#include "Parameters.h"
#include "ui/EditorGrid.h"
#include "ui/UiTypography.h"
#include "ui/Palette.h"

GrainveilAudioProcessorEditor::GrainveilAudioProcessorEditor(GrainveilAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), tooltipWindow(this, 650)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(designSurface);
    designSurface.setBounds(0, 0, GrainveilUI::Grid::width, GrainveilUI::Grid::height);
    designSurface.setInterceptsMouseClicks(false, true);

    designSurface.addAndMakeVisible(oledDisplay);

    modeCombo.addItemList(GrainveilParams::getModeNames(), 1);
    modeCombo.setTitle("Grain mode");
    modeCombo.setTooltip("Choose the granular processing mode");
    modeCombo.setRepaintsOnMouseActivity(true);
    designSurface.addAndMakeVisible(modeCombo);
    // ComboBox creates its label before it has a parent; rebuild it using our
    // inherited look-and-feel rather than the host's default theme.
    modeCombo.sendLookAndFeelChange();
    modeLabel.setText("MODE", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9aa092));
    modeLabel.setFont(GrainveilUI::font(12.0f, true));
    designSurface.addAndMakeVisible(modeLabel);
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), GrainveilParams::mode, modeCombo);

    designSurface.addAndMakeVisible(freezeButton);
    freezeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), GrainveilParams::freeze, freezeButton);

    using Unit = GrainveilKnob::Unit;
    setupKnob(mixSlider,      mixLabel,      "MIX",      Unit::percent,      "Blend dry and granular signals");
    setupKnob(sizeSlider,     sizeLabel,     "SIZE",     Unit::milliseconds, "Set grain duration");
    setupKnob(densitySlider,  densityLabel,  "DENSITY",  Unit::hertz,        "Set the grain generation rate");
    setupKnob(scatterSlider,  scatterLabel,  "SCATTER",  Unit::percent,      "Vary grain positions in the buffer");
    setupKnob(feedbackSlider, feedbackLabel, "FEEDBACK", Unit::percent,      "Return processed signal to the buffer");
    setupKnob(pitchSlider,    pitchLabel,    "PITCH",    Unit::semitones,    "Transpose the grains");
    setupKnob(captureSlider,  captureLabel,  "CAPTURE",  Unit::milliseconds, "Set the capture buffer duration");
    setupKnob(toneSlider,     toneLabel,     "TONE",     Unit::percent,      "Adjust the grain tone");
    setupKnob(reverseSlider,  reverseLabel,  "REVERSE",  Unit::percent,      "Set the amount of reversed grains");
    setupKnob(spreadSlider,   spreadLabel,   "SPREAD",   Unit::percent,      "Set the stereo grain spread");

    auto& apvts = audioProcessor.getAPVTS();
    mixAtt      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::mix, mixSlider);
    sizeAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::grainSize, sizeSlider);
    densityAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::density, densitySlider);
    scatterAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::scatter, scatterSlider);
    feedbackAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::feedback, feedbackSlider);
    pitchAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::pitch, pitchSlider);

    captureAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::captureTime, captureSlider);
    toneAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::tone, toneSlider);
    reverseAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::reverseAmt, reverseSlider);
    spreadAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, GrainveilParams::spread, spreadSlider);

    setResizable(true, true);
    setResizeLimits(GrainveilUI::Grid::minWidth, GrainveilUI::Grid::minHeight,
                    GrainveilUI::Grid::maxWidth, GrainveilUI::Grid::maxHeight);
    getConstrainer()->setFixedAspectRatio(static_cast<double>(GrainveilUI::Grid::width) / GrainveilUI::Grid::height);
    setSize(GrainveilUI::Grid::width, GrainveilUI::Grid::height);

    startTimerHz(30);
}

GrainveilAudioProcessorEditor::~GrainveilAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void GrainveilAudioProcessorEditor::setupKnob(GrainveilKnob& slider, juce::Label& label,
                                                 const juce::String& text, GrainveilKnob::Unit unit,
                                                 const juce::String& tooltip)
{
    slider.configure(text, unit, tooltip);
    slider.addListener(this);
    designSurface.addAndMakeVisible(slider);
    // Recreate the value label after the component inherits our look-and-feel.
    slider.sendLookAndFeelChange();

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffb8b4a4));
    label.setFont(GrainveilUI::font(12.0f, true));
    label.setTooltip(tooltip);
    designSurface.addAndMakeVisible(label);
}

void GrainveilAudioProcessorEditor::timerCallback()
{
    // The old DSP telemetry object is not a safe cross-thread snapshot. Avoid
    // touching it until its producer can be changed under an agreed scope.
    oledDisplay.setControlState(modeCombo.getText(), freezeButton.getToggleState());
    oledDisplay.advanceAnimation();
}

void GrainveilAudioProcessorEditor::sliderDragStarted(juce::Slider* slider)
{
    if (auto* knob = dynamic_cast<GrainveilKnob*>(slider))
    {
        activeKnob = knob;
        oledDisplay.beginParameter(knob->getDisplayName(), knob->getDisplayValue());
    }
}

void GrainveilAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == activeKnob && activeKnob != nullptr)
        oledDisplay.updateParameter(activeKnob->getDisplayName(), activeKnob->getDisplayValue());
}

void GrainveilAudioProcessorEditor::sliderDragEnded(juce::Slider* slider)
{
    if (slider == activeKnob)
    {
        activeKnob = nullptr;
        oledDisplay.endParameter();
    }
}

void GrainveilAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Letterbox defensively if a host ignores the editor's aspect constraint.
    g.fillAll(juce::Colour(0xff141718));
}

void GrainveilAudioProcessorEditor::DesignSurface::paint(juce::Graphics& g)
{
    using namespace GrainveilUI;
    const auto bounds = getLocalBounds().toFloat();
    g.fillAll(Palette::background);

    // Procedural graphite chassis: vector geometry and deterministic fine
    // brushing, with no texture bitmaps or dependence on display scale.
    auto bezel = bounds.reduced(3.0f);
    juce::ColourGradient edge(juce::Colour(0xff60645d), bezel.getTopLeft(),
                              juce::Colour(0xff191d1b), bezel.getBottomLeft(), false);
    g.setGradientFill(edge);
    g.fillRoundedRectangle(bezel, 13.0f);
    auto plate = bounds.reduced(10.0f);
    juce::ColourGradient face(juce::Colour(0xff343835), plate.getTopLeft(),
                              juce::Colour(0xff222725), plate.getBottomRight(), false);
    g.setGradientFill(face);
    g.fillRoundedRectangle(plate, 9.0f);
    {
        const juce::Graphics::ScopedSaveState saved(g);
        juce::Path clip;
        clip.addRoundedRectangle(plate.reduced(1.0f), 8.0f);
        g.reduceClipRegion(clip);
        for (int y = 14; y < Grid::height - 14; y += 3)
        {
            const float alpha = static_cast<float>((y * 17) % 11 + 2) / 1100.0f;
            g.setColour(juce::Colours::white.withAlpha(alpha));
            g.drawHorizontalLine(y, plate.getX() + 1.0f, plate.getRight() - 1.0f);
        }
        juce::ColourGradient vignette(juce::Colours::transparentBlack, plate.getCentreX(), plate.getCentreY(),
                                      juce::Colours::black.withAlpha(0.16f), plate.getRight(), plate.getBottom(), true);
        g.setGradientFill(vignette);
        g.fillRect(plate);
    }
    g.setColour(juce::Colour(0xff6a7066).withAlpha(0.5f));
    g.drawRoundedRectangle(plate.reduced(0.5f), 9.0f, 1.0f);

    // A small original linear mark paired with the Grainveil wordmark.
    const float markX = 49.0f, markY = 40.0f;
    g.setColour(Palette::warm);
    juce::Path mark;
    for (int i = 0; i < 3; ++i)
    {
        const float x = markX + static_cast<float>(i) * 6.0f;
        mark.startNewSubPath(x, markY + 11.0f);
        mark.cubicTo(x + 7.0f, markY + 8.0f, x - 2.0f, markY + 2.0f, x + 5.0f, markY - 2.0f);
    }
    g.strokePath(mark, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(Palette::text);
    drawFittedText(g, "GRAINVEIL", { 78.0f, 33.0f, 155.0f, 23.0f }, 18.0f, 12.0f, juce::Justification::centredLeft, true);
    g.setColour(Palette::muted);
    drawFittedText(g, "STEREO GRANULAR PROCESSOR", Grid::presetZone.toFloat(), 10.0f, 8.0f,
                   juce::Justification::centred, true);
    drawFittedText(g, "SIGNAL  /  SPACE", { 782.0f, 34.0f, 168.0f, 20.0f }, 10.0f, 8.0f,
                   juce::Justification::centredRight, true);

    g.setColour(juce::Colour(0xff8b9085).withAlpha(0.28f));
    g.drawHorizontalLine(79, 49.0f, 951.0f);
    g.setColour(Palette::muted.withAlpha(0.8f));
    drawFittedText(g, "01 / ALGORITHM", { 50.0f, 98.0f, 170.0f, 18.0f }, 9.0f, 8.0f,
                   juce::Justification::centredLeft, true);
    drawFittedText(g, "02 / BUFFER MONITOR", { 270.0f, 79.0f, 460.0f, 16.0f }, 9.0f, 8.0f,
                   juce::Justification::centredLeft, true);
    drawFittedText(g, "03 / MEMORY HOLD", { 780.0f, 120.0f, 170.0f, 18.0f }, 9.0f, 8.0f,
                   juce::Justification::centredLeft, true);
    g.setColour(juce::Colour(0xffa0a59a).withAlpha(0.17f));
    g.drawHorizontalLine(247, 49.0f, 951.0f);
    g.drawHorizontalLine(443, 49.0f, 951.0f);
}

void GrainveilAudioProcessorEditor::resized()
{
    using namespace GrainveilUI::Grid;
    const float scale = juce::jmin(static_cast<float>(getWidth()) / static_cast<float>(width),
                                   static_cast<float>(getHeight()) / static_cast<float>(height));
    const float offsetX = (static_cast<float>(getWidth()) - static_cast<float>(width) * scale) * 0.5f;
    const float offsetY = (static_cast<float>(getHeight()) - static_cast<float>(height) * scale) * 0.5f;
    designSurface.setTransform(juce::AffineTransform::scale(scale).translated(offsetX, offsetY));

    this->modeLabel.setBounds(GrainveilUI::Grid::modeLabel);
    modeCombo.setBounds(modeField);
    freezeButton.setBounds(freezeField);
    oledDisplay.setBounds(display);

    auto place = [](const KnobCell& cell, juce::Slider& slider, juce::Label& label)
    {
        slider.setBounds(cell.control); // Value box remains inside this cell.
        label.setBounds(cell.label);    // Dedicated row below, never over a value.
    };
    place(mainKnobs[0], mixSlider, mixLabel);
    place(mainKnobs[1], sizeSlider, sizeLabel);
    place(mainKnobs[2], densitySlider, densityLabel);
    place(mainKnobs[3], scatterSlider, scatterLabel);
    place(mainKnobs[4], feedbackSlider, feedbackLabel);
    place(mainKnobs[5], pitchSlider, pitchLabel);
    place(secondaryKnobs[0], captureSlider, captureLabel);
    place(secondaryKnobs[1], toneSlider, toneLabel);
    place(secondaryKnobs[2], reverseSlider, reverseLabel);
    place(secondaryKnobs[3], spreadSlider, spreadLabel);
}
