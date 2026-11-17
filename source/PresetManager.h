#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

// Program requests can originate on any host thread. Only an atomic integer is
// touched there; all parameter notification, file access and catalog changes
// are performed on the message thread by the timer.
class PresetManager final : private juce::Timer
{
public:
    static constexpr size_t parameterCount = 19;
    // Fixed slots keep JUCE's VST3 program-change step count stable for the
    // lifetime of an instance, including when the user saves a new preset.
    static constexpr int maxUserPresets = 64;
    using Values = std::array<float, parameterCount>;

    PresetManager(juce::AudioProcessor&, juce::AudioProcessorValueTreeState&, juce::File directory = {});
    ~PresetManager() override;

    int getNumPrograms() const noexcept;
    int getCurrentProgram() const noexcept;
    juce::String getProgramName(int index) const;
    juce::String getProgramCategory(int index) const;
    void requestProgram(int index) noexcept;
    void servicePendingProgram(); // message thread; exposed for deterministic tests
    bool isModified() const noexcept;
    bool isUserProgram(int index) const noexcept;
    bool isProgramAvailable(int index) const noexcept;
    bool saveUserPreset(const juce::String& name, bool overwrite, juce::String& error);
    static juce::String validateName(const juce::String& name);
    juce::File getDirectory() const { return presetDirectory; }

    void writeSessionMetadata(juce::XmlElement& state) const;
    void restoreSessionMetadata(const juce::XmlElement* metadata);
    juce::String consumeError(); // message thread

private:
    struct UserPreset { juce::String name; juce::File file; };
    struct Catalog { std::vector<UserPreset> users; };
    static constexpr int factoryOffset = 1; // index zero is Init
    static int userOffset() noexcept;
    void timerCallback() override;
    Values currentValues() const noexcept;
    Values defaultValues() const noexcept;
    Values factoryValues(int index) const;
    void applyValues(const Values& values);
    void captureBaseline(const Values& values) noexcept;
    int findUser(const juce::String& name) const noexcept;
    int appendUser(const UserPreset& entry, int slot);
    bool refreshCatalogFromDisk(juce::String& error);
    bool readUser(const juce::File&, Values&, juce::String& error) const;
    void notifyHost();

    juce::AudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    juce::File presetDirectory;
    std::array<juce::RangedAudioParameter*, parameterCount> parameters {};
    std::array<std::atomic<float>, parameterCount> baseline {};
    std::atomic<int> currentIndex { 0 };
    std::atomic<int> pendingIndex { -1 };
    std::atomic<const Catalog*> publishedCatalog { nullptr };
    // Bounded to maxUserPresets + 1 immutable catalog snapshots. Published
    // snapshots remain alive for concurrent host enumeration until destruction.
    std::vector<std::unique_ptr<const Catalog>> catalogs;
    juce::String lastError;
};
