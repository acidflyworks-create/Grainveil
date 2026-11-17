#include "PluginEditor.h"
#include "Parameters.h"

GrainveilAudioProcessorEditor::GrainveilAudioProcessorEditor(GrainveilAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(oledDisplay);

    modeCombo.addItemList(GrainveilParams::getModeNames(), 1);
    addAndMakeVisible(modeCombo);
    modeLabel.setText("MODE", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9aa092));
    addAndMakeVisible(modeLabel);
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), GrainveilParams::mode, modeCombo);

    addAndMakeVisible(freezeButton);
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
    setResizeLimits(640, 380, 1100, 700);
    setSize(740, 440);

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
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffb8b4a4));
    label.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
    addAndMakeVisible(label);
}

void GrainveilAudioProcessorEditor::timerCallback()
{
    audioProcessor.getTelemetrySnapshot(telemetrySnapshot);
    oledDisplay.setSnapshot(telemetrySnapshot);
}

void GrainveilAudioProcessorEditor::paint(juce::Graphics& g)
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
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText("GRAINVEIL", plate.getX() + 30.0f, plate.getY() + 8.0f, 120.0f, 18.0f, juce::Justification::centredLeft, false);
}

void GrainveilAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(26);

    // Top section: OLED + Mode + Freeze
    auto topRow = area.removeFromTop(110);
    auto leftTop = topRow.removeFromLeft(150).reduced(4);
    modeLabel.setBounds(leftTop.removeFromTop(18));
    modeCombo.setBounds(leftTop.removeFromTop(30));
    leftTop.removeFromTop(10);
    freezeButton.setBounds(leftTop.removeFromTop(36));

    oledDisplay.setBounds(topRow.reduced(8, 2));

    area.removeFromTop(12);

    // Main 6 knobs
    auto mainRow = area.removeFromTop(145);
    const int mainW = mainRow.getWidth() / 6;
    auto placeKnob = [](juce::Rectangle<int> cell, juce::Slider& s, juce::Label& l)
    {
        l.setBounds(cell.removeFromTop(18));
        s.setBounds(cell.reduced(2));
    };

    placeKnob(mainRow.removeFromLeft(mainW), mixSlider,      mixLabel);
    placeKnob(mainRow.removeFromLeft(mainW), sizeSlider,     sizeLabel);
    placeKnob(mainRow.removeFromLeft(mainW), densitySlider,  densityLabel);
    placeKnob(mainRow.removeFromLeft(mainW), scatterSlider,  scatterLabel);
    placeKnob(mainRow.removeFromLeft(mainW), feedbackSlider, feedbackLabel);
    placeKnob(mainRow,                       pitchSlider,    pitchLabel);

    area.removeFromTop(8);

    // Secondary 4 knobs centered
    auto secRow = area.reduced(area.getWidth() / 8, 0);
    const int secW = secRow.getWidth() / 4;
    placeKnob(secRow.removeFromLeft(secW), captureSlider, captureLabel);
    placeKnob(secRow.removeFromLeft(secW), toneSlider,    toneLabel);
    placeKnob(secRow.removeFromLeft(secW), reverseSlider, reverseLabel);
    placeKnob(secRow,                      spreadSlider,  spreadLabel);
}
