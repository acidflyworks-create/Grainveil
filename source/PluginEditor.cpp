#include "PluginEditor.h"
#include "Parameters.h"
#include "ui/EditorGrid.h"
#include "ui/UiTypography.h"

GrainveilAudioProcessorEditor::GrainveilAudioProcessorEditor(GrainveilAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(designSurface);
    designSurface.setBounds(0, 0, GrainveilUI::Grid::width, GrainveilUI::Grid::height);
    designSurface.setInterceptsMouseClicks(false, true);

    designSurface.addAndMakeVisible(oledDisplay);

    modeCombo.addItemList(GrainveilParams::getModeNames(), 1);
    designSurface.addAndMakeVisible(modeCombo);
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

    setupKnob(mixSlider,      mixLabel,      "MIX");
    setupKnob(sizeSlider,     sizeLabel,     "SIZE");
    setupKnob(densitySlider,  densityLabel,  "DENSITY");
    setupKnob(scatterSlider,  scatterLabel,  "SCATTER");
    setupKnob(feedbackSlider, feedbackLabel, "FEEDBACK");
    setupKnob(pitchSlider,    pitchLabel,    "PITCH");

    setupKnob(captureSlider,  captureLabel,  "CAPTURE");
    setupKnob(toneSlider,     toneLabel,     "TONE");
    setupKnob(reverseSlider,  reverseLabel,  "REVERSE");
    setupKnob(spreadSlider,   spreadLabel,   "SPREAD");

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

void GrainveilAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    designSurface.addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffb8b4a4));
    label.setFont(GrainveilUI::font(12.0f, true));
    label.setTooltip(text);
    slider.setTooltip(text);
    designSurface.addAndMakeVisible(label);
}

void GrainveilAudioProcessorEditor::timerCallback()
{
    audioProcessor.getTelemetrySnapshot(telemetrySnapshot);
    oledDisplay.setSnapshot(telemetrySnapshot);
}

void GrainveilAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Letterbox defensively if a host ignores the editor's aspect constraint.
    g.fillAll(juce::Colour(0xff141718));
}

void GrainveilAudioProcessorEditor::DesignSurface::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Outer metallic bezel
    g.fillAll(juce::Colour(0xff141718));
    juce::ColourGradient bezelGrad(juce::Colour(0xff424744), 0.0f, 0.0f,
                                   juce::Colour(0xff202422), 0.0f, bounds.getHeight(), false);
    g.setGradientFill(bezelGrad);
    g.fillRoundedRectangle(bounds.reduced(4.0f), 12.0f);

    // Recessed charcoal/olive inner plate
    auto plate = bounds.reduced(14.0f);
    g.setColour(juce::Colour(0xff1d2120));
    g.fillRoundedRectangle(plate, 8.0f);
    g.setColour(juce::Colour(0xff383f3a));
    g.drawRoundedRectangle(plate, 8.0f, 1.5f);

    // Subtle corner screws
    auto drawScrew = [&g](float cx, float cy)
    {
        g.setColour(juce::Colour(0xff525852));
        g.fillEllipse(cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        g.setColour(juce::Colour(0xff181b1a));
        g.drawLine(cx - 3.0f, cy - 2.0f, cx + 3.0f, cy + 2.0f, 1.4f);
    };
    drawScrew(plate.getX() + 14.0f, plate.getY() + 14.0f);
    drawScrew(plate.getRight() - 14.0f, plate.getY() + 14.0f);
    drawScrew(plate.getX() + 14.0f, plate.getBottom() - 14.0f);
    drawScrew(plate.getRight() - 14.0f, plate.getBottom() - 14.0f);

    // Subtle title
    g.setColour(juce::Colour(0xff7c8276));
    GrainveilUI::drawFittedText(g, "GRAINVEIL", { plate.getX() + 30.0f, plate.getY() + 8.0f, 160.0f, 22.0f },
                               12.0f, 9.0f, juce::Justification::centredLeft, true);
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
