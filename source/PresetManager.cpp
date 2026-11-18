#include "PresetManager.h"
#include "FactoryPresets.h"
#include "Parameters.h"
#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>

namespace
{
constexpr auto ids = GrainveilFactory::parameterIds;
static_assert(ids.size() == PresetManager::parameterCount, "Factory bank must cover all parameters");
constexpr int presetFileVersion = 2;
constexpr int sessionVersion = 2;

bool parseValues(const juce::XmlElement& element, PresetManager::Values& values)
{
    const int version = element.getIntAttribute("version", -1);
    if (version != 1 && version != 2) return false;
    std::array<bool, PresetManager::parameterCount> seen {};
    PresetManager::Values parsed {};
    for (const auto* child : element.getChildIterator())
    {
        if (! child->hasTagName("Parameter")) return false;
        const auto id = child->getStringAttribute("id");
        const auto it = std::find_if(ids.begin(), ids.end(), [&id](const char* candidate) { return id == candidate; });
        if (it == ids.end()) return false;
        const auto index = static_cast<size_t>(std::distance(ids.begin(), it));
        if (seen[index] || ! child->hasAttribute("value")) return false;
        const auto raw = child->getStringAttribute("value");
        if (raw.isEmpty() || ! raw.containsOnly("0123456789+-.eE")) return false;
        std::istringstream stream(raw.toStdString());
        stream.imbue(std::locale::classic());
        double value = 0.0;
        char trailing = 0;
        if (! (stream >> value) || (stream >> trailing) || ! std::isfinite(value) || value < 0.0 || value > 1.0) return false;
        parsed[index] = static_cast<float>(value);
        seen[index] = true;
    }
    if (! std::all_of(seen.begin(), seen.end(), [](bool present) { return present; })) return false;
    if (version == 1)
    {
        // V1 stored normalized values using the old 10..500 ms skewed range.
        // Preserve the physical grain size when loading old user/session data.
        const juce::NormalisableRange<float> oldSize(10.0f, 500.0f, 0.5f, 0.5f);
        const juce::NormalisableRange<float> newSize(5.0f, 1200.0f, 0.5f, 0.5f);
        constexpr size_t grainSizeIndex = 7;
        const float physical = oldSize.snapToLegalValue(oldSize.convertFrom0to1(parsed[grainSizeIndex]));
        parsed[grainSizeIndex] = newSize.convertTo0to1(physical);
    }
    values = parsed;
    return true;
}

void writeValues(juce::XmlElement& parent, const PresetManager::Values& values)
{
    for (size_t i = 0; i < ids.size(); ++i)
    {
        auto* child = parent.createNewChildElement("Parameter");
        child->setAttribute("id", ids[i]);
        child->setAttribute("value", juce::String(static_cast<double>(values[i]), 9));
    }
}
}

int PresetManager::initProgram() noexcept { return static_cast<int>(GrainveilFactory::presets.size()); }
int PresetManager::userOffset() noexcept { return initProgram() + 1; }

PresetManager::PresetManager(juce::AudioProcessor& owner, juce::AudioProcessorValueTreeState& state, juce::File directory)
    : processor(owner), apvts(state), presetDirectory(std::move(directory))
{
    if (presetDirectory == juce::File())
    {
       #if JUCE_MAC
        presetDirectory = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
                              .getChildFile("Library/Audio/Presets/Grainveil");
       #else
        presetDirectory = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                              .getChildFile("Grainveil/Presets");
       #endif
    }
    for (size_t i = 0; i < ids.size(); ++i)
    {
        parameters[i] = apvts.getParameter(ids[i]);
        jassert(parameters[i] != nullptr);
    }
    captureBaseline(defaultValues());
    currentIndex.store(initProgram(), std::memory_order_relaxed);

    auto initial = std::make_unique<Catalog>();
    initial->users.resize(static_cast<size_t>(maxUserPresets));
    auto files = presetDirectory.findChildFiles(juce::File::findFiles, false, "*.gvpreset");
    files.sort();
    for (const auto& file : files)
    {
        auto name = file.getFileNameWithoutExtension();
        if (validateName(name).isNotEmpty() || file.getSize() > 65536) continue;
        auto xml = juce::XmlDocument::parse(file);
        Values parsed;
        if (xml == nullptr || ! xml->hasTagName("GrainveilPreset") || ! parseValues(*xml, parsed)) continue;
        const int slot = xml->getIntAttribute("slot", -1);
        if (slot < 0 || slot >= maxUserPresets || initial->users[static_cast<size_t>(slot)].name.isNotEmpty()) continue;
        const auto duplicate = std::any_of(initial->users.begin(), initial->users.end(),
                                           [&name](const UserPreset& user) { return user.name.equalsIgnoreCase(name); });
        if (! duplicate) initial->users[static_cast<size_t>(slot)] = { name, file };
    }
    const auto* catalog = initial.get();
    catalogs.push_back(std::move(initial));
    publishedCatalog.store(catalog, std::memory_order_release);
    startTimerHz(30);
}

PresetManager::~PresetManager() { stopTimer(); }

int PresetManager::getNumPrograms() const noexcept
{
    return userOffset() + maxUserPresets;
}

int PresetManager::getCurrentProgram() const noexcept
{
    const int requested = pendingIndex.load(std::memory_order_acquire);
    return requested >= 0 ? requested : currentIndex.load(std::memory_order_acquire);
}

juce::String PresetManager::getProgramName(int index) const
{
    if (index == initProgram()) return "Init";
    if (index >= 0 && index < initProgram())
        return GrainveilFactory::presets[static_cast<size_t>(index)].name;
    const auto* catalog = publishedCatalog.load(std::memory_order_acquire);
    const int user = index - userOffset();
    if (catalog == nullptr || user < 0 || user >= maxUserPresets) return {};
    const auto& name = catalog->users[static_cast<size_t>(user)].name;
    return name.isNotEmpty() ? name : "Unused user slot " + juce::String(user + 1);
}

juce::String PresetManager::getProgramCategory(int index) const
{
    if (index == initProgram()) return "Init";
    if (index >= 0 && index < initProgram())
        return GrainveilFactory::presets[static_cast<size_t>(index)].category;
    return isUserProgram(index) ? juce::String("User") : juce::String();
}

bool PresetManager::isUserProgram(int index) const noexcept
{
    const auto* catalog = publishedCatalog.load(std::memory_order_acquire);
    const int slot = index - userOffset();
    return catalog != nullptr && slot >= 0 && slot < maxUserPresets && catalog->users[static_cast<size_t>(slot)].name.isNotEmpty();
}

bool PresetManager::isProgramAvailable(int index) const noexcept
{
    return (index >= 0 && index < userOffset()) || isUserProgram(index);
}

void PresetManager::requestProgram(int index) noexcept
{
    if (isProgramAvailable(index)) pendingIndex.store(index, std::memory_order_release);
}

void PresetManager::timerCallback() { servicePendingProgram(); }

void PresetManager::servicePendingProgram()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    const int index = pendingIndex.exchange(-1, std::memory_order_acq_rel);
    if (! isProgramAvailable(index)) return;
    Values values;
    if (index == initProgram()) values = defaultValues();
    else if (index < initProgram()) values = factoryValues(index);
    else
    {
        const auto* catalog = publishedCatalog.load(std::memory_order_acquire);
        juce::String error;
        if (catalog == nullptr || ! readUser(catalog->users[static_cast<size_t>(index - userOffset())].file, values, error))
        {
            lastError = error.isNotEmpty() ? error : "User preset is no longer available.";
            return;
        }
    }
    applyValues(values);
    captureBaseline(currentValues()); // captures the parameter's legal quantised values
    currentIndex.store(index, std::memory_order_release);
    notifyHost();
}

PresetManager::Values PresetManager::currentValues() const noexcept
{
    Values result {};
    for (size_t i = 0; i < result.size(); ++i)
        if (parameters[i] != nullptr) result[i] = parameters[i]->getValue();
    return result;
}

PresetManager::Values PresetManager::defaultValues() const noexcept
{
    Values result {};
    for (size_t i = 0; i < result.size(); ++i)
        if (parameters[i] != nullptr) result[i] = parameters[i]->getDefaultValue();
    return result;
}

PresetManager::Values PresetManager::factoryValues(int index) const
{
    Values result {};
    const auto& preset = GrainveilFactory::presets[static_cast<size_t>(index)];
    for (size_t i = 0; i < result.size(); ++i)
        if (parameters[i] != nullptr)
            result[i] = parameters[i]->convertTo0to1(preset.values[i]);
    return result;
}

void PresetManager::applyValues(const Values& values)
{
    for (size_t i = 0; i < values.size(); ++i)
        if (auto* parameter = parameters[i])
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, values[i]));
            parameter->endChangeGesture();
        }
}

void PresetManager::captureBaseline(const Values& values) noexcept
{
    for (size_t i = 0; i < values.size(); ++i) baseline[i].store(values[i], std::memory_order_release);
}

bool PresetManager::isModified() const noexcept
{
    for (size_t i = 0; i < ids.size(); ++i)
        if (parameters[i] != nullptr && std::abs(parameters[i]->getValue() - baseline[i].load(std::memory_order_acquire)) > 1.0e-5f)
            return true;
    return false;
}

juce::String PresetManager::validateName(const juce::String& name)
{
    if (name.isEmpty() || name.length() > 32) return "Use a name of 1 to 32 characters.";
    if (name != name.trim() || name.endsWithChar('.') || name.startsWithChar('.')) return "Names cannot begin or end with spaces or dots.";
    if (name.containsAnyOf("<>:\"/\\|?*")) return "The name contains a character that cannot be used in a filename.";
    for (auto c : name) if (c < 32) return "Control characters are not permitted in preset names.";
    const auto base = name.upToFirstOccurrenceOf(".", false, false).toUpperCase();
    static const juce::StringArray reserved { "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
    if (reserved.contains(base)) return "This name is reserved by Windows.";
    return {};
}

int PresetManager::findUser(const juce::String& name) const noexcept
{
    const auto* catalog = publishedCatalog.load(std::memory_order_acquire);
    if (catalog != nullptr)
        for (size_t i = 0; i < catalog->users.size(); ++i)
            if (catalog->users[i].name.equalsIgnoreCase(name)) return userOffset() + static_cast<int>(i);
    return -1;
}

int PresetManager::appendUser(const UserPreset& user, int slot)
{
    const auto existing = findUser(user.name);
    if (existing >= 0) return existing;
    const auto* old = publishedCatalog.load(std::memory_order_acquire);
    if (old == nullptr || slot < 0 || slot >= maxUserPresets || old->users[static_cast<size_t>(slot)].name.isNotEmpty()) return -1;
    auto replacement = std::make_unique<Catalog>(*old);
    replacement->users[static_cast<size_t>(slot)] = user;
    const int index = userOffset() + slot;
    const auto* next = replacement.get();
    catalogs.push_back(std::move(replacement));
    publishedCatalog.store(next, std::memory_order_release);
    return index;
}

bool PresetManager::refreshCatalogFromDisk(juce::String& error)
{
    auto files = presetDirectory.findChildFiles(juce::File::findFiles, false, "*.gvpreset");
    for (const auto& file : files)
    {
        const auto name = file.getFileNameWithoutExtension();
        if (validateName(name).isNotEmpty() || file.getSize() > 65536) continue;
        auto xml = juce::XmlDocument::parse(file);
        Values parsed;
        if (xml == nullptr || ! xml->hasTagName("GrainveilPreset") || ! parseValues(*xml, parsed)) continue;
        const int slot = xml->getIntAttribute("slot", -1);
        if (slot < 0 || slot >= maxUserPresets) continue;
        const int existing = findUser(name);
        if (existing >= 0)
        {
            if (existing != userOffset() + slot)
            {
                error = "A user preset has conflicting slot metadata. Reopen the plugin after resolving duplicate files.";
                return false;
            }
            continue;
        }
        if (appendUser({ name, file }, slot) < 0)
        {
            error = "Two user presets use the same slot. Reopen after resolving duplicate files.";
            return false;
        }
    }
    return true;
}

bool PresetManager::saveUserPreset(const juce::String& name, bool overwrite, juce::String& error)
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    error = validateName(name);
    if (error.isNotEmpty()) return false;
    if (const auto result = presetDirectory.createDirectory(); result.failed())
    {
        error = "Cannot create the preset folder: " + result.getErrorMessage(); return false;
    }
    // File operations and this bounded inter-process lock are message-thread
    // only. It prevents two plugin instances assigning the same user slot.
    juce::InterProcessLock lock("Grainveil-presets-" + juce::String(presetDirectory.getFullPathName().hashCode64()));
    if (! lock.enter(500)) { error = "The preset folder is busy; please try again."; return false; }
    struct Unlock { juce::InterProcessLock& lock; ~Unlock() { lock.exit(); } } unlock { lock };
    if (! refreshCatalogFromDisk(error)) return false;
    const int existing = findUser(name);
    const auto* catalog = publishedCatalog.load(std::memory_order_acquire);
    const auto freeSlot = std::find_if(catalog->users.begin(), catalog->users.end(),
                                       [](const UserPreset& entry) { return entry.name.isEmpty(); });
    if (existing < 0 && freeSlot == catalog->users.end())
    {
        error = "The user preset limit (64) has been reached."; return false;
    }
    const int slot = existing >= 0 ? existing - userOffset() : static_cast<int>(std::distance(catalog->users.begin(), freeSlot));
    const auto file = existing >= 0 ? catalog->users[static_cast<size_t>(existing - userOffset())].file
                                    : presetDirectory.getChildFile(name + ".gvpreset");
    if (! overwrite && file.exists()) { error = "A preset with this name already exists."; return false; }
    auto values = currentValues();
    juce::XmlElement xml("GrainveilPreset");
    xml.setAttribute("version", presetFileVersion);
    xml.setAttribute("name", name);
    xml.setAttribute("slot", slot);
    writeValues(xml, values);
    if (! file.replaceWithText(xml.toString())) { error = "Could not write preset: " + file.getFullPathName(); return false; }
    const int index = existing >= 0 ? existing : appendUser({ name, file }, slot);
    if (index < 0) { error = "The preset was saved but could not be added to the program list."; return false; }
    pendingIndex.store(-1, std::memory_order_release);
    captureBaseline(values);
    currentIndex.store(index, std::memory_order_release);
    notifyHost();
    return true;
}

bool PresetManager::readUser(const juce::File& file, Values& values, juce::String& error) const
{
    if (! file.existsAsFile() || file.getSize() > 65536)
    {
        error = "Preset file is missing or too large: " + file.getFileName(); return false;
    }
    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr || ! xml->hasTagName("GrainveilPreset") || ! parseValues(*xml, values))
    {
        error = "Preset file is invalid or uses an unsupported version: " + file.getFileName(); return false;
    }
    return true;
}

void PresetManager::writeSessionMetadata(juce::XmlElement& state) const
{
    auto* meta = state.createNewChildElement("GrainveilProgram");
    meta->setAttribute("version", sessionVersion);
    const auto index = currentIndex.load(std::memory_order_acquire);
    meta->setAttribute("type", index == initProgram() ? "init" : (isUserProgram(index) ? "user" : "factory"));
    meta->setAttribute("name", getProgramName(index));
    Values values {};
    for (size_t i = 0; i < values.size(); ++i) values[i] = baseline[i].load(std::memory_order_acquire);
    writeValues(*meta, values);
}

void PresetManager::restoreSessionMetadata(const juce::XmlElement* meta)
{
    Values values = defaultValues();
    int index = initProgram();
    if (meta != nullptr && parseValues(*meta, values))
    {
        const auto type = meta->getStringAttribute("type");
        const auto name = meta->getStringAttribute("name");
        if (type == "user")
        {
            const auto found = findUser(name);
            if (found >= 0) index = found;
        }
        else if (type == "factory")
            for (int i = 0; i < initProgram(); ++i)
                if (getProgramName(i) == name) { index = i; break; }
    }
    pendingIndex.store(-1, std::memory_order_release);
    captureBaseline(values);
    currentIndex.store(index, std::memory_order_release);
}

juce::String PresetManager::consumeError()
{
    auto result = lastError;
    lastError.clear();
    return result;
}

void PresetManager::notifyHost()
{
    processor.updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withProgramChanged(true));
}
