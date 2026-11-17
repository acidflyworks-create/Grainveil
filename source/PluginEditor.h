#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/GrainveilLookAndFeel.h"
#include "ui/OledDisplay.h"
#include "ui/Knob.h"
#include "ui/FreezeButton.h"

class GrainveilAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer,
                                            private juce::Slider::Listener
{
public:
    explicit GrainveilAudioProcessorEditor(GrainveilAudioProcessor&);
    ~GrainveilAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void sliderValueChanged(juce::Slider*) override;
    void sliderDragStarted(juce::Slider*) override;
    void sliderDragEnded(juce::Slider*) override;
    void setupKnob(GrainveilKnob&, juce::Label&, const juce::String&, GrainveilKnob::Unit, const juce::String& tooltip);

    class DesignSurface final : public juce::Component
    {
    public:
        void paint(juce::Graphics&) override;
    };

    GrainveilAudioProcessor& audioProcessor;
    GrainveilLookAndFeel lookAndFeel;
    DesignSurface designSurface;
    OledDisplay oledDisplay;
    juce::TooltipWindow tooltipWindow;
    GrainveilKnob* activeKnob = nullptr;

    // 6 main knobs
    GrainveilKnob mixSlider, sizeSlider, densitySlider, scatterSlider, feedbackSlider, pitchSlider;
    juce::Label  mixLabel,  sizeLabel,  densityLabel,  scatterLabel,  feedbackLabel,  pitchLabel;

    // 4 secondary knobs
    GrainveilKnob captureSlider, toneSlider, reverseSlider, spreadSlider;
    juce::Label  captureLabel,  toneLabel,  reverseLabel,  spreadLabel;

    // Mode + Freeze
    juce::ComboBox modeCombo;
    juce::Label    modeLabel;
    FreezeButton freezeButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        mixAtt, sizeAtt, densityAtt, scatterAtt, feedbackAtt, pitchAtt,
        captureAtt, toneAtt, reverseAtt, spreadAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   freezeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainveilAudioProcessorEditor)
};
