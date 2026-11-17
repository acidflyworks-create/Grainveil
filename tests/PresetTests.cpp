#include "PluginProcessor.h"
#include "ui/PresetBar.h"
#include "dsp/TelemetryExchange.h"
#include <atomic>
#include <cmath>
#include <iostream>
#include <thread>

namespace
{
int failures = 0;
void check(bool condition, const char* message)
{
    if (! condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
bool near(float a, float b) { return std::abs(a - b) < 0.001f; }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getNonexistentChildFile("grainveil-preset-test-", {}, false);
    check(root.createDirectory().wasOk(), "temporary folder creation");
    {
        GrainveilAudioProcessor p(root);
        auto& manager = p.getPresetManager();
        auto* mix = p.getAPVTS().getParameter(GrainveilParams::mix);
        auto* pitchRand = p.getAPVTS().getParameter(GrainveilParams::pitchRand);
        check(p.getCurrentProgram() == 0 && p.getProgramName(0) == "Init", "initial program");
        check(! manager.isModified(), "initial state should be unmodified");
        check(p.getNumPrograms() == 9 + PresetManager::maxUserPresets, "stable host program count");

        const float before = mix->getValue();
        std::thread host([&] { p.setCurrentProgram(1); });
        host.join();
        check(p.getCurrentProgram() == 1, "host request visible immediately");
        check(near(mix->getValue(), before), "host request must not apply parameters on caller thread");
        manager.servicePendingProgram();
        check(near(mix->convertFrom0to1(mix->getValue()), .55f), "factory program loads on message thread");
        check(! manager.isModified(), "loaded program unmodified");
        p.setCurrentProgram(5);
        manager.servicePendingProgram();
        check(pitchRand->convertFrom0to1(pitchRand->getValue()) > 0.5f, "pitch cloud override");
        p.setCurrentProgram(2);
        manager.servicePendingProgram();
        check(near(pitchRand->convertFrom0to1(pitchRand->getValue()), 0.0f), "factory load resets unspecified parameters");
        mix->setValueNotifyingHost(mix->convertTo0to1(.22f));
        check(manager.isModified(), "parameter edit sets modified marker");
        mix->setValueNotifyingHost(mix->convertTo0to1(.75f));
        check(! manager.isModified(), "returning to baseline clears modified marker");

        PresetBar bar(manager);
        p.setCurrentProgram(0);
        manager.servicePendingProgram();
        bar.step(-1);
        check(p.getCurrentProgram() == 8, "previous wraps over unused slots");
        manager.servicePendingProgram();
        bar.step(1);
        check(p.getCurrentProgram() == 0, "next wraps to init");
        manager.servicePendingProgram();
        juce::String error;
        check(PresetManager::validateName("../bad").isNotEmpty(), "path traversal rejected");
        check(PresetManager::validateName("CON.txt").isNotEmpty(), "Windows reserved name rejected");
        check(PresetManager::validateName(juce::String::repeatedString("x", 33)).isNotEmpty(), "long name rejected");
        const juce::String longName = "TwentyEight Character Preset XYZ";
        check(longName.length() <= 32, "test name fits maximum");
        mix->setValueNotifyingHost(mix->convertTo0to1(.37f));
        check(manager.saveUserPreset(longName, false, error), "save user preset");
        check(manager.isUserProgram(p.getCurrentProgram()), "saved preset selected");
        const int userProgram = p.getCurrentProgram();
        check(p.getNumPrograms() == 9 + PresetManager::maxUserPresets, "program count unchanged by save");
        check(! manager.isModified(), "saved user preset is unmodified");
        check(! manager.saveUserPreset(longName.toUpperCase(), false, error), "case-insensitive duplicate rejected");
        mix->setValueNotifyingHost(mix->convertTo0to1(.18f));
        check(manager.isModified(), "user preset edit is modified");
        p.setCurrentProgram(userProgram);
        manager.servicePendingProgram();
        check(near(mix->convertFrom0to1(mix->getValue()), .37f), "user preset loads saved value");
        check(! manager.isModified(), "reloaded user preset is unmodified");

        // Second open instance must not collide with the first instance's slot.
        GrainveilAudioProcessor other(root);
        check(other.getProgramName(userProgram) == longName, "user slot stable across instances");
        check(other.getPresetManager().saveUserPreset("Second Voice", false, error), "second instance saves");
        check(other.getCurrentProgram() != userProgram, "different user presets have different stable slots");
        check(manager.saveUserPreset("Third Voice", false, error), "first instance incorporates external slots");
        check(p.getCurrentProgram() != other.getCurrentProgram(), "concurrent instances do not reuse slots");
        p.setCurrentProgram(userProgram);
        manager.servicePendingProgram();
        mix->setValueNotifyingHost(mix->convertTo0to1(.21f));
        juce::MemoryBlock state;
        p.getStateInformation(state);
        GrainveilAudioProcessor restored(root);
        restored.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        auto* restoredMix = restored.getAPVTS().getParameter(GrainveilParams::mix);
        check(near(restoredMix->convertFrom0to1(restoredMix->getValue()), .21f), "session parameter recall");
        check(restored.getProgramName(restored.getCurrentProgram()) == longName, "session preset identity recall");
        check(restored.getPresetManager().isModified(), "session modified marker recall");
        restored.setCurrentProgram(restored.getCurrentProgram());
        restored.getPresetManager().servicePendingProgram();
        check(near(restoredMix->convertFrom0to1(restoredMix->getValue()), .37f), "session stored baseline can be reloaded");

        p.prepareToPlay(48000, 128);
        juce::AudioBuffer<float> audio(2, 128);
        juce::MidiBuffer midi;
        for (int block = 0; block < 20; ++block)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 128; ++i) audio.setSample(ch, i, std::sin((block * 128 + i) * 0.02f) * 0.5f);
            p.processBlock(audio, midi);
        }
        GrainveilDsp::TelemetrySnapshot frame;
        check(p.getTelemetrySnapshot(frame), "live telemetry is published");
        bool anyWave = false;
        for (auto sample : frame.waveform) anyWave = anyWave || std::abs(sample) > 0.001f;
        check(anyWave, "live telemetry contains signal");
    }
    check(root.deleteRecursively(), "temporary folder cleanup");

    // Verify a reader never sees a torn multi-field frame while the producer
    // publishes on another thread. No JUCE or file operations run in writer.
    GrainveilDsp::TelemetryExchange exchange;
    std::atomic<bool> done { false };
    std::thread writer([&]
    {
        for (int i = 1; i <= 5000; ++i)
        {
            GrainveilDsp::TelemetrySnapshot frame;
            const float token = static_cast<float>(i);
            frame.waveform.fill(token);
            frame.grainPositions.fill(token);
            frame.activeGrainCount = i;
            frame.mode = i;
            frame.frozen = (i % 2) != 0;
            exchange.publish(frame);
        }
        done.store(true);
    });
    int reads = 0;
    while (! done.load())
    {
        GrainveilDsp::TelemetrySnapshot frame;
        if (! exchange.copy(frame)) continue;
        ++reads;
        const float token = frame.waveform[0];
        bool coherent = frame.mode == static_cast<int>(token)
                     && frame.activeGrainCount == frame.mode && frame.frozen == ((frame.mode % 2) != 0);
        for (auto v : frame.waveform) coherent = coherent && v == token;
        for (auto v : frame.grainPositions) coherent = coherent && v == token;
        check(coherent, "telemetry frame must not tear");
    }
    writer.join();
    GrainveilDsp::TelemetrySnapshot finalFrame;
    check(exchange.copy(finalFrame) && finalFrame.mode == 5000, "final telemetry frame published");
    check(reads > 0, "concurrent telemetry was observed");
    std::cout << (failures == 0 ? "Preset and telemetry tests passed\n" : "Preset and telemetry tests failed\n");
    return failures == 0 ? 0 : 1;
}
