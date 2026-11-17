#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/GrainveilLookAndFeel.h"
#include "ui/OledDisplay.h"

class GrainveilAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit GrainveilAudioProcessorEditor(GrainveilAudioProcessor&);
    ~GrainveilAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text);

    GrainveilAudioProcessor& audioProcessor;
    GrainveilLookAndFeel lookAndFeel;
    OledDisplay oledDisplay;
    GrainveilDsp::TelemetrySnapshot telemetrySnapshot;

    // 6 main knobs
    juce::Slider mixSlider, sizeSlider, densitySlider, scatterSlider, feedbackSlider, pitchSlider;
    juce::Label  mixLabel,  sizeLabel,  densityLabel,  scatterLabel,  feedbackLabel,  pitchLabel;

    // 4 secondary knobs
    juce::Slider captureSlider, toneSlider, reverseSlider, spreadSlider;
    juce::Label  captureLabel,  toneLabel,  reverseLabel,  spreadLabel;

    // Mode + Freeze
    juce::ComboBox modeCombo;
    juce::Label    modeLabel;
    juce::ToggleButton freezeButton { "FREEZE" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        mixAtt, sizeAtt, densityAtt, scatterAtt, feedbackAtt, pitchAtt,
        captureAtt, toneAtt, reverseAtt, spreadAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   freezeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainveilAudioProcessorEditor)
};
