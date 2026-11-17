#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PresetManager.h"

class PresetBar final : public juce::Component
{
public:
    explicit PresetBar(PresetManager&);
    ~PresetBar() override;
    void resized() override;
    void refresh();
    void step(int direction);

private:
    class ArrowButton final : public juce::Button
    {
    public:
        explicit ArrowButton(bool next);
        void paintButton(juce::Graphics&, bool, bool) override;
    private:
        bool pointsRight;
    };

    class NameButton final : public juce::Button
    {
    public:
        NameButton();
        void paintButton(juce::Graphics&, bool, bool) override;
        void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void setModified(bool value) { modified = value; repaint(); }
        std::function<void(int)> onWheel;
    private:
        bool modified = false;
    };

    class ActionButton final : public juce::Button
    {
    public:
        explicit ActionButton(const juce::String&);
        void paintButton(juce::Graphics&, bool, bool) override;
    };

    void showMenu();
    void save();
    void saveAs();
    void showError(const juce::String&);
    PresetManager& manager;
    ArrowButton previous { false }, next { true };
    NameButton presetName;
    ActionButton saveButton { "SAVE" }, saveAsButton { "SAVE AS" };
    std::unique_ptr<juce::AlertWindow> saveDialog;
    juce::String displayedName;
    int displayedIndex = -1;
    bool displayedModified = false;
};
