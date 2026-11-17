#include "PresetBar.h"
#include "Palette.h"
#include "UiTypography.h"

namespace
{
void drawHousing(juce::Graphics& g, juce::Rectangle<float> bounds, bool over, bool down)
{
    g.setColour(down ? juce::Colour(0xff33392f) : (over ? juce::Colour(0xff2e342f) : juce::Colour(0xff191e1c)));
    g.fillRoundedRectangle(bounds.reduced(0.5f), 4.0f);
    g.setColour(over ? GrainveilUI::Palette::warm.withAlpha(0.8f) : juce::Colour(0xff545b53));
    g.drawRoundedRectangle(bounds.reduced(0.7f), 4.0f, 1.0f);
}
}

PresetBar::ArrowButton::ArrowButton(bool nextDirection)
    : juce::Button(nextDirection ? "Next preset" : "Previous preset"), pointsRight(nextDirection)
{
    setTooltip(nextDirection ? "Next preset" : "Previous preset");
    setRepaintsOnMouseActivity(true);
}

void PresetBar::ArrowButton::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto bounds = getLocalBounds().toFloat();
    drawHousing(g, bounds, over, down);
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const float d = pointsRight ? 1.0f : -1.0f;
    juce::Path arrow;
    arrow.startNewSubPath(cx - d * 2.5f, cy - 4.0f);
    arrow.lineTo(cx + d * 2.0f, cy);
    arrow.lineTo(cx - d * 2.5f, cy + 4.0f);
    g.setColour(GrainveilUI::Palette::warm);
    g.strokePath(arrow, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

PresetBar::NameButton::NameButton() : juce::Button("Preset name")
{
    setRepaintsOnMouseActivity(true);
    setTitle("Preset name and menu");
}

void PresetBar::NameButton::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto bounds = getLocalBounds().toFloat();
    drawHousing(g, bounds, over, down);
    auto arrowZone = bounds.removeFromRight(25.0f);
    auto markerZone = bounds.removeFromRight(12.0f);
    bounds = bounds.reduced(10.0f, 2.0f);
    g.setColour(GrainveilUI::Palette::text);
    GrainveilUI::drawFittedText(g, getButtonText(), bounds, 12.5f, 9.0f, juce::Justification::centredLeft, true);
    if (modified)
    {
        g.setColour(GrainveilUI::Palette::warmBright);
        GrainveilUI::drawFittedText(g, "*", markerZone, 14.0f, 9.0f, juce::Justification::centred, true);
    }
    juce::Path arrow;
    const float x = arrowZone.getCentreX(), y = arrowZone.getCentreY();
    arrow.startNewSubPath(x - 3.5f, y - 1.5f);
    arrow.lineTo(x, y + 2.0f);
    arrow.lineTo(x + 3.5f, y - 1.5f);
    g.setColour(GrainveilUI::Palette::warm);
    g.strokePath(arrow, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PresetBar::NameButton::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (onWheel && wheel.deltaY != 0.0f) onWheel(wheel.deltaY < 0.0f ? 1 : -1);
}

PresetBar::ActionButton::ActionButton(const juce::String& text) : juce::Button(text)
{
    setRepaintsOnMouseActivity(true);
}

void PresetBar::ActionButton::paintButton(juce::Graphics& g, bool over, bool down)
{
    auto bounds = getLocalBounds().toFloat();
    drawHousing(g, bounds, over, down);
    g.setColour(GrainveilUI::Palette::text.withAlpha(over ? 1.0f : 0.8f));
    GrainveilUI::drawFittedText(g, getButtonText(), bounds.reduced(5.0f, 2.0f), 10.0f, 8.0f,
                               juce::Justification::centred, true);
}

PresetBar::PresetBar(PresetManager& presetManager) : manager(presetManager)
{
    addAndMakeVisible(previous);
    addAndMakeVisible(presetName);
    addAndMakeVisible(next);
    addAndMakeVisible(saveButton);
    addAndMakeVisible(saveAsButton);
    previous.onClick = [this] { step(-1); };
    next.onClick = [this] { step(1); };
    presetName.onClick = [this] { showMenu(); };
    presetName.onWheel = [this](int direction) { step(direction); };
    saveButton.onClick = [this] { save(); };
    saveAsButton.onClick = [this] { saveAs(); };
    saveButton.setTooltip("Save changes to the current user preset");
    saveAsButton.setTooltip("Save the current settings as a new user preset");
    refresh();
}

PresetBar::~PresetBar()
{
    // PopupMenu holds a non-owning LookAndFeel pointer. Dismiss it before the
    // owning editor's look-and-feel is destroyed.
    juce::PopupMenu::dismissAllActiveMenus();
    if (saveDialog != nullptr) saveDialog->setLookAndFeel(nullptr);
}

void PresetBar::resized()
{
    auto area = getLocalBounds().reduced(0, 3);
    previous.setBounds(area.removeFromLeft(31));
    area.removeFromLeft(4);
    saveAsButton.setBounds(area.removeFromRight(72));
    area.removeFromRight(4);
    saveButton.setBounds(area.removeFromRight(55));
    area.removeFromRight(4);
    next.setBounds(area.removeFromRight(31));
    area.removeFromRight(4);
    presetName.setBounds(area);
}

void PresetBar::refresh()
{
    const auto index = manager.getCurrentProgram();
    const auto modified = manager.isModified();
    const auto name = manager.getProgramName(index);
    if (index != displayedIndex || modified != displayedModified || name != displayedName)
    {
        displayedIndex = index;
        displayedModified = modified;
        displayedName = name;
        presetName.setButtonText(name);
        presetName.setModified(modified);
        presetName.setTooltip(name + (modified ? " (modified)" : ""));
        presetName.setTitle("Preset: " + name + (modified ? " (modified)" : ""));
    }
    const auto error = manager.consumeError();
    if (error.isNotEmpty()) showError(error);
}

void PresetBar::step(int direction)
{
    const int count = manager.getNumPrograms();
    if (count <= 0) return;
    const int current = juce::jlimit(0, count - 1, manager.getCurrentProgram());
    int candidate = current;
    for (int i = 0; i < count; ++i)
    {
        candidate = (candidate + (direction < 0 ? -1 : 1) + count) % count;
        if (manager.isProgramAvailable(candidate)) { manager.requestProgram(candidate); break; }
    }
    refresh();
}

void PresetBar::showMenu()
{
    juce::PopupMenu root;
    root.setLookAndFeel(&getLookAndFeel());
    root.addItem(PresetManager::initProgram() + 1, "Init", true, manager.getCurrentProgram() == PresetManager::initProgram());
    root.addSeparator();
    juce::StringArray categories;
    for (int i = 0; i < manager.getNumPrograms(); ++i)
    {
        if (i == PresetManager::initProgram() || ! manager.isProgramAvailable(i)) continue;
        auto category = manager.getProgramCategory(i);
        if (! categories.contains(category)) categories.add(category);
    }
    for (const auto& category : categories)
    {
        juce::PopupMenu submenu;
        for (int i = 0; i < manager.getNumPrograms(); ++i)
            if (i != PresetManager::initProgram() && manager.isProgramAvailable(i) && manager.getProgramCategory(i) == category)
                submenu.addItem(i + 1, manager.getProgramName(i), true, manager.getCurrentProgram() == i);
        root.addSubMenu(category, submenu);
    }
    juce::Component::SafePointer<PresetBar> safe(this);
    root.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetName), [safe](int result)
    {
        if (safe != nullptr && result > 0)
        {
            safe->manager.requestProgram(result - 1);
            safe->refresh();
        }
    });
}

void PresetBar::save()
{
    const int index = manager.getCurrentProgram();
    if (! manager.isUserProgram(index)) { saveAs(); return; }
    juce::String error;
    if (! manager.saveUserPreset(manager.getProgramName(index), true, error)) showError(error);
    refresh();
}

void PresetBar::saveAs()
{
    if (saveDialog != nullptr) return;
    saveDialog = std::make_unique<juce::AlertWindow>("Save user preset", "Enter a new preset name (up to 32 characters).",
                                                     juce::MessageBoxIconType::NoIcon, this);
    saveDialog->setLookAndFeel(&getLookAndFeel());
    saveDialog->addTextEditor("name", {}, "Preset name");
    saveDialog->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    saveDialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PresetBar> safe(this);
    saveDialog->enterModalState(true, juce::ModalCallbackFunction::create([safe](int result)
    {
        if (safe == nullptr || safe->saveDialog == nullptr) return;
        const auto name = safe->saveDialog->getTextEditorContents("name");
        safe->saveDialog->setLookAndFeel(nullptr);
        safe->saveDialog.reset();
        if (result == 1)
        {
            juce::String error;
            if (! safe->manager.saveUserPreset(name, false, error)) safe->showError(error);
            safe->refresh();
        }
    }), false);
}

void PresetBar::showError(const juce::String& error)
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Grainveil presets", error, "OK", this);
}
