#include "PluginProcessor.h"
#include "FactoryPresets.h"
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
        check(p.getCurrentProgram() == PresetManager::initProgram() && p.getProgramName(p.getCurrentProgram()) == "Init", "initial program");
        check(p.getProgramName(0) == "Ambient Wash", "original factory program indices preserved");
        check(! manager.isModified(), "initial state should be unmodified");
        check(p.getNumPrograms() == 46 + PresetManager::maxUserPresets, "stable host program count");
        check(GrainveilFactory::dataVersion == 2 && PresetManager::initProgram() == 45, "factory bank version and count");
        // Original v1 programs, including values that were formerly implicit defaults.
        const std::array<const char*, 8> originalNames {{ "Ambient Wash", "Frozen Drone", "Reverse Swell", "Glitch Rhythm", "Pitch Cloud", "Tight Echo-Grain", "Lo-Fi Loop", "Guitar Swell" }};
        const std::array<std::array<float, 19>, 8> originalValues {{
            {{.55f,0,0,0,1000,.45f,.5f,220,18,.35f,0,0,.15f,.8f,.7f,0,.5f,0,0}},
            {{.75f,0,0,1,1000,.3f,.5f,360,12,.25f,0,0,.2f,.7f,.55f,3,.5f,0,0}},
            {{.65f,0,0,0,1000,.4f,.5f,260,14,.3f,0,0,.9f,.65f,.65f,2,.5f,0,0}},
            {{.6f,0,0,0,1000,.35f,.5f,45,26,.75f,0,0,.35f,.85f,.8f,4,.5f,0,0}},
            {{.55f,0,0,0,1000,.5f,.5f,190,16,.4f,12,.6f,0,.75f,.65f,5,.5f,0,0}},
            {{.45f,0,0,0,420,.45f,.5f,110,8,.1f,0,0,0,.4f,.75f,1,.5f,0,0}},
            {{.65f,0,0,0,1200,.55f,.5f,160,11,.45f,0,0,0,.5f,.4f,7,.5f,0,0}},
            {{.6f,0,0,0,1000,.4f,.5f,320,16,.2f,0,0,.25f,.75f,.72f,6,.5f,0,0}}
        }};
        for (size_t i = 0; i < originalNames.size(); ++i)
        {
            check(p.getProgramName(static_cast<int>(i)) == originalNames[i], "v1 factory name and index preserved");
            for (size_t j = 0; j < originalValues[i].size(); ++j)
                check(near(GrainveilFactory::presets[i].values[j], originalValues[i][j]), "v1 factory values preserved");
        }
        juce::StringArray categories { "Ambient Wash", "Frozen Drones", "Reverse Swells", "Glitch & Stutter", "Pitch Clouds", "Echo Grains", "Lo-Fi / Degraded", "Guitar Textures", "Sound Design / Experimental" };
        for (const auto& category : categories)
        {
            int count = 0;
            for (int i = 0; i < PresetManager::initProgram(); ++i)
                if (manager.getProgramCategory(i) == category) ++count;
            check(count == 5, "factory category contains five programs");
        }
        for (int i = 0; i < PresetManager::initProgram(); ++i)
        {
            const auto& factory = GrainveilFactory::presets[static_cast<size_t>(i)];
            check(categories.contains(factory.category), "factory category recognised");
            for (size_t j = 0; j < factory.values.size(); ++j)
            {
                auto* param = p.getAPVTS().getParameter(GrainveilFactory::parameterIds[j]);
                check(param != nullptr, "factory parameter exists");
                if (param == nullptr) continue;
                const auto actual = factory.values[j];
                const auto legal = param->convertFrom0to1(param->convertTo0to1(actual));
                check(std::isfinite(actual) && std::abs(actual - legal) < .002f, "factory value is legal and quantised");
                // Deliberately disturb every parameter before each load, to catch omitted values.
                param->setValueNotifyingHost(param->convertTo0to1(actual) < .5f ? 1.0f : 0.0f);
            }
            p.setCurrentProgram(i);
            manager.servicePendingProgram();
            check(p.getCurrentProgram() == i && ! manager.isModified(), "factory selected and unmodified");
            for (size_t j = 0; j < factory.values.size(); ++j)
            {
                auto* param = p.getAPVTS().getParameter(GrainveilFactory::parameterIds[j]);
                if (param != nullptr)
                    check(std::abs(param->convertFrom0to1(param->getValue()) - factory.values[j]) < .002f, "factory loads every parameter");
            }
        }
        p.setCurrentProgram(7);
        manager.servicePendingProgram();
        juce::XmlElement legacyState("LegacySession");
        manager.writeSessionMetadata(legacyState);
        auto* legacyMeta = legacyState.getChildByName("GrainveilProgram");
        check(legacyMeta != nullptr && legacyMeta->getIntAttribute("version") == 1, "session metadata remains v1 compatible");
        p.setCurrentProgram(PresetManager::initProgram());
        manager.servicePendingProgram();
        manager.restoreSessionMetadata(legacyMeta);
        check(p.getCurrentProgram() == 7 && p.getProgramName(7) == "Guitar Swell", "v1 factory metadata resolved by name");
        check(manager.isModified(), "legacy baseline remains distinct from current parameters");
        p.setCurrentProgram(PresetManager::initProgram());
        manager.servicePendingProgram();

        const float before = mix->getValue();
        std::thread host([&] { p.setCurrentProgram(0); });
        host.join();
        check(p.getCurrentProgram() == 0, "host request visible immediately");
        check(near(mix->getValue(), before), "host request must not apply parameters on caller thread");
        manager.servicePendingProgram();
        check(near(mix->convertFrom0to1(mix->getValue()), .55f), "factory program loads on message thread");
        check(! manager.isModified(), "loaded program unmodified");
        p.setCurrentProgram(4);
        manager.servicePendingProgram();
        check(pitchRand->convertFrom0to1(pitchRand->getValue()) > 0.5f, "pitch cloud override");
        p.setCurrentProgram(1);
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
        check(p.getCurrentProgram() == PresetManager::initProgram(), "previous wraps over unused slots");
        manager.servicePendingProgram();
        bar.step(1);
        check(p.getCurrentProgram() == 0, "next wraps to first factory preset");
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
        check(p.getNumPrograms() == 46 + PresetManager::maxUserPresets, "program count unchanged by save");
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

        const auto presetFile = root.getChildFile(longName + ".gvpreset");
        const auto validFile = presetFile.loadFileAsString();
        check(presetFile.replaceWithText("<GrainveilPreset version=\"1\"><Parameter id=\"mix\" value=\"nan\"/></GrainveilPreset>"), "write corrupt test preset");
        p.setCurrentProgram(PresetManager::initProgram());
        manager.servicePendingProgram();
        p.setCurrentProgram(userProgram);
        manager.servicePendingProgram();
        check(p.getCurrentProgram() == PresetManager::initProgram(), "invalid preset must not partially load or change selection");
        check(manager.consumeError().isNotEmpty(), "invalid preset reports an error");
        check(presetFile.replaceWithText(validFile), "restore test preset");

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
