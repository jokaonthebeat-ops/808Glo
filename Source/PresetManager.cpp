#include "PresetManager.h"

#include "808GloBinaryData.h"
#include "ParameterLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace
{
constexpr int presetSchemaVersion = 1;
constexpr int expectedFactoryPresetCount = 128;
constexpr auto factoryBankName = "808Glo Pro Factory Bank";
constexpr auto factoryAuthor = "Diamond Loopz";

constexpr std::array expectedCategoryCounts {
    std::pair { "Clean & Sub", 12 },
    std::pair { "Trap", 14 },
    std::pair { "Distorted", 13 },
    std::pair { "Drill", 13 },
    std::pair { "Detroit", 12 },
    std::pair { "West Coast", 12 },
    std::pair { "Long Glide", 12 },
    std::pair { "Short Punch", 12 },
    std::pair { "Experimental", 12 },
    std::pair { "Mix Ready", 16 }
};

bool isIntegerValue (const juce::var& value) noexcept
{
    return value.isInt() || value.isInt64();
}

bool isNumericValue (const juce::var& value) noexcept
{
    return isIntegerValue (value) || value.isDouble();
}

bool readExactInteger (const juce::var& value, int& result) noexcept
{
    if (! isIntegerValue (value))
        return false;

    const auto wide = static_cast<juce::int64> (value);
    if (wide < static_cast<juce::int64> (std::numeric_limits<int>::min())
        || wide > static_cast<juce::int64> (std::numeric_limits<int>::max()))
        return false;

    result = static_cast<int> (wide);
    return true;
}

bool readStateInteger (const juce::var& value, int& result) noexcept
{
    if (readExactInteger (value, result))
        return true;
    if (! value.isString())
        return false;

    const auto text = value.toString().trim();
    const auto parsed = text.getIntValue();
    if (text != juce::String (parsed))
        return false;

    result = parsed;
    return true;
}

bool readStateBoolean (const juce::var& value) noexcept
{
    if (value.isBool())
        return static_cast<bool> (value);
    if (! value.isString())
        return false;

    const auto text = value.toString().trim();
    return text == "1" || text.equalsIgnoreCase ("true");
}

bool hasExactProperties (const juce::DynamicObject& object,
                         std::initializer_list<const char*> expected)
{
    if (object.getProperties().size() != static_cast<int> (expected.size()))
        return false;

    return std::all_of (expected.begin(), expected.end(), [&object] (const char* property)
    {
        return object.hasProperty (property);
    });
}

bool isIntegerParameter (const juce::String& id) noexcept
{
    return id == glo::ids::voiceMode || id == glo::ids::triggerMode
        || id == glo::ids::tune || id == glo::ids::bendRange
        || id == glo::ids::driveMode;
}

bool isKnownCategory (const juce::String& category)
{
    return std::any_of (expectedCategoryCounts.begin(), expectedCategoryCounts.end(),
                        [&category] (const auto& entry) { return category == entry.first; });
}
} // namespace

namespace glo
{
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& stateToUse)
    : state (stateToUse)
{
    const auto parsedFactoryBank = parseFactoryBank();
    jassert (parsedFactoryBank);
    if (! parsedFactoryBank)
        juce::Logger::writeToLog ("808Glo Pro: factory preset bank validation failed; no factory programs were loaded.");
    juce::ignoreUnused (parsedFactoryBank);
    for (const auto& id : parameterIDs())
        state.addParameterListener (id, this);
}

PresetManager::~PresetManager()
{
    for (const auto& id : parameterIDs())
        state.removeParameterListener (id, this);
}

int PresetManager::getNumberOfFactoryPresets() const noexcept
{
    return static_cast<int> (factoryPresets.size());
}

const PresetManager::Preset& PresetManager::getFactoryPreset (int index) const
{
    if (index >= 0 && index < getNumberOfFactoryPresets())
        return factoryPresets[static_cast<std::size_t> (index)];

    jassertfalse;
    static const Preset unavailablePreset;
    return unavailablePreset;
}

juce::StringArray PresetManager::getCategories() const
{
    juce::StringArray categories;
    for (const auto& preset : factoryPresets)
        categories.addIfNotAlreadyThere (preset.category);
    return categories;
}

std::vector<int> PresetManager::search (const juce::String& query,
                                        const juce::String& category) const
{
    std::vector<int> matches;
    const auto needle = query.trim().toLowerCase();

    for (int index = 0; index < getNumberOfFactoryPresets(); ++index)
    {
        const auto& preset = factoryPresets[static_cast<std::size_t> (index)];
        if (category.isNotEmpty() && preset.category != category)
            continue;

        auto haystack = (preset.name + " " + preset.category + " " + preset.description
                         + " " + preset.tags.joinIntoString (" ")).toLowerCase();
        if (needle.isEmpty() || haystack.contains (needle))
            matches.push_back (index);
    }

    return matches;
}

bool PresetManager::loadFactoryPreset (int index)
{
    if (index < 0 || index >= getNumberOfFactoryPresets())
        return false;

    const auto& preset = factoryPresets[static_cast<std::size_t> (index)];
    if (! applyParameterObject (preset.parameters))
        return false;

    {
        const juce::ScopedLock lock (metadataLock);
        currentFactoryIndex.store (index, std::memory_order_release);
        userPresetName.clear();
        userPresetCategory.clear();
    }
    dirty.store (false, std::memory_order_release);
    return true;
}

bool PresetManager::loadNextFactoryPreset (int direction)
{
    if (factoryPresets.empty())
        return false;

    auto index = currentFactoryIndex.load (std::memory_order_acquire);
    if (index < 0 || index >= getNumberOfFactoryPresets())
        index = direction >= 0 ? 0 : getNumberOfFactoryPresets() - 1;
    else
        index = (index + (direction >= 0 ? 1 : -1) + getNumberOfFactoryPresets())
              % getNumberOfFactoryPresets();
    return loadFactoryPreset (index);
}

bool PresetManager::loadInitPreset()
{
    for (const auto& id : parameterIDs())
        if (state.getParameter (id) == nullptr)
            return false;

    applyingPreset.store (true, std::memory_order_release);
    for (const auto& id : parameterIDs())
    {
        auto* parameter = state.getParameter (id);
        jassert (parameter != nullptr);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->getDefaultValue());
        parameter->endChangeGesture();
    }
    applyingPreset.store (false, std::memory_order_release);

    {
        const juce::ScopedLock lock (metadataLock);
        currentFactoryIndex.store (-1, std::memory_order_release);
        userPresetName = "Init 808";
        userPresetCategory = "Init";
    }
    dirty.store (false, std::memory_order_release);
    return true;
}

bool PresetManager::loadUserPreset (const juce::File& file)
{
    if (! file.existsAsFile())
        return false;

    juce::var root;
    if (juce::JSON::parse (file.loadFileAsString(), root).failed())
        return false;

    const auto* object = root.getDynamicObject();
    if (object == nullptr
        || ! hasExactProperties (*object,
                                 { "schemaVersion", "name", "category", "author", "tags", "parameters" }))
        return false;

    int schemaVersion = 0;
    const auto& nameValue = object->getProperty ("name");
    const auto& categoryValue = object->getProperty ("category");
    const auto& authorValue = object->getProperty ("author");
    const auto& tagsValue = object->getProperty ("tags");
    if (! readExactInteger (object->getProperty ("schemaVersion"), schemaVersion)
        || schemaVersion != presetSchemaVersion
        || ! nameValue.isString() || nameValue.toString().trim().isEmpty()
        || ! categoryValue.isString() || categoryValue.toString() != "User"
        || ! authorValue.isString() || authorValue.toString().trim().isEmpty())
        return false;

    const auto* tagArray = tagsValue.getArray();
    if (tagArray == nullptr)
        return false;
    juce::StringArray validatedTags;
    for (const auto& tag : *tagArray)
    {
        if (! tag.isString() || tag.toString().trim().isEmpty()
            || validatedTags.contains (tag.toString().trim(), true))
            return false;
        validatedTags.add (tag.toString().trim());
    }

    const auto& parameterObject = object->getProperty ("parameters");
    if (! applyParameterObject (parameterObject, true))
        return false;

    {
        const juce::ScopedLock lock (metadataLock);
        currentFactoryIndex.store (-1, std::memory_order_release);
        userPresetName = nameValue.toString().trim();
        userPresetCategory = "User";
    }
    dirty.store (false, std::memory_order_release);
    return true;
}

bool PresetManager::saveUserPreset (const juce::File& file,
                                    const juce::String& name,
                                    const juce::StringArray& tags)
{
    auto* rootObject = new juce::DynamicObject();
    rootObject->setProperty ("schemaVersion", 1);
    rootObject->setProperty ("name", name.trim().isNotEmpty() ? name.trim()
                                                                : file.getFileNameWithoutExtension());
    rootObject->setProperty ("category", "User");
    rootObject->setProperty ("author", "Joka Beatz / Diamond Loopz");

    juce::Array<juce::var> tagValues;
    juce::StringArray uniqueTags;
    for (const auto& tag : tags)
    {
        const auto cleanTag = tag.trim();
        if (cleanTag.isNotEmpty() && ! uniqueTags.contains (cleanTag, true))
        {
            uniqueTags.add (cleanTag);
            tagValues.add (cleanTag);
        }
    }
    rootObject->setProperty ("tags", juce::var (tagValues));
    rootObject->setProperty ("parameters", createCurrentParameterObject());

    const juce::var root (rootObject);
    const auto destination = file.hasFileExtension ("808glo") ? file
                                                               : file.withFileExtension ("808glo");
    const auto parentDirectory = destination.getParentDirectory();
    if (! parentDirectory.isDirectory() && parentDirectory.createDirectory().failed())
        return false;
    if (! destination.replaceWithText (juce::JSON::toString (root, true)))
        return false;

    {
        const juce::ScopedLock lock (metadataLock);
        currentFactoryIndex.store (-1, std::memory_order_release);
        userPresetName = rootObject->getProperty ("name").toString();
        userPresetCategory = "User";
    }
    dirty.store (false, std::memory_order_release);
    return true;
}

int PresetManager::getCurrentFactoryPresetIndex() const noexcept
{
    return currentFactoryIndex.load (std::memory_order_acquire);
}

juce::String PresetManager::getCurrentPresetName() const
{
    const auto index = getCurrentFactoryPresetIndex();
    if (index >= 0 && index < getNumberOfFactoryPresets())
        return factoryPresets[static_cast<std::size_t> (index)].name;

    const juce::ScopedLock lock (metadataLock);
    return userPresetName;
}

juce::String PresetManager::getCurrentPresetCategory() const
{
    const auto index = getCurrentFactoryPresetIndex();
    if (index >= 0 && index < getNumberOfFactoryPresets())
        return factoryPresets[static_cast<std::size_t> (index)].category;

    const juce::ScopedLock lock (metadataLock);
    return userPresetCategory;
}

bool PresetManager::isDirty() const noexcept
{
    return dirty.load (std::memory_order_acquire);
}

void PresetManager::markClean() noexcept
{
    dirty.store (false, std::memory_order_release);
}

juce::File PresetManager::getDefaultUserPresetDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Diamond Loopz")
        .getChildFile ("808Glo Pro")
        .getChildFile ("Presets");
}

void PresetManager::restoreMetadataFromState (const juce::ValueTree& restoredState)
{
    auto restoredFactoryIndex = -1;
    int storedIndex = -1;
    if (readStateInteger (restoredState.getProperty ("factoryPresetIndex", -1), storedIndex)
        && storedIndex >= 0 && storedIndex < getNumberOfFactoryPresets())
        restoredFactoryIndex = storedIndex;

    // The name makes factory identity survive a future insertion or reordering,
    // while the numeric index remains backwards compatible with existing state.
    const auto storedFactoryName = restoredState.getProperty ("factoryPresetName").toString();
    if (storedFactoryName.isNotEmpty())
    {
        const auto match = std::find_if (factoryPresets.begin(), factoryPresets.end(),
                                         [&storedFactoryName] (const Preset& preset)
                                         {
                                             return preset.name == storedFactoryName;
                                         });
        if (match != factoryPresets.end())
            restoredFactoryIndex = static_cast<int> (std::distance (factoryPresets.begin(), match));
    }

    const auto restoredDirty = readStateBoolean (restoredState.getProperty ("presetDirty", false));

    auto restoredUserName = restoredState.getProperty ("userPresetName", "Init 808").toString().trim();
    auto restoredUserCategory = restoredState.getProperty ("userPresetCategory", "Init").toString().trim();
    if (restoredFactoryIndex >= 0)
    {
        restoredUserName.clear();
        restoredUserCategory.clear();
    }
    else
    {
        if (restoredUserName.isEmpty())
            restoredUserName = "Init 808";
        if (restoredUserCategory.isEmpty())
            restoredUserCategory = "Init";
    }

    {
        const juce::ScopedLock lock (metadataLock);
        currentFactoryIndex.store (restoredFactoryIndex, std::memory_order_release);
        userPresetName = std::move (restoredUserName);
        userPresetCategory = std::move (restoredUserCategory);
    }
    dirty.store (restoredDirty, std::memory_order_release);
}

void PresetManager::writeMetadataToState (juce::ValueTree& stateSnapshot) const
{
    if (! stateSnapshot.isValid())
        return;

    const juce::ScopedLock lock (metadataLock);
    const auto factoryIndex = currentFactoryIndex.load (std::memory_order_acquire);
    stateSnapshot.setProperty ("factoryPresetIndex", factoryIndex, nullptr);
    stateSnapshot.setProperty ("factoryPresetName",
                               factoryIndex >= 0 && factoryIndex < getNumberOfFactoryPresets()
                                   ? factoryPresets[static_cast<std::size_t> (factoryIndex)].name
                                   : juce::String(),
                               nullptr);
    stateSnapshot.setProperty ("presetDirty", dirty.load (std::memory_order_acquire), nullptr);
    stateSnapshot.setProperty ("userPresetName", userPresetName, nullptr);
    stateSnapshot.setProperty ("userPresetCategory", userPresetCategory, nullptr);
}

void PresetManager::parameterChanged (const juce::String&, float)
{
    if (! applyingPreset.load (std::memory_order_acquire))
        dirty.store (true, std::memory_order_release);
}

bool PresetManager::parseFactoryBank()
{
    const auto json = juce::String::fromUTF8 (
        EightOhEightAssets::FactoryPresets_json,
        EightOhEightAssets::FactoryPresets_jsonSize);
    juce::var root;
    if (juce::JSON::parse (json, root).failed())
        return false;

    const auto* rootObject = root.getDynamicObject();
    if (rootObject == nullptr
        || ! hasExactProperties (*rootObject,
                                 { "schemaVersion", "bankName", "author", "presetCount", "presets" }))
        return false;

    int schemaVersion = 0;
    int declaredPresetCount = 0;
    const auto& bankNameValue = rootObject->getProperty ("bankName");
    const auto& authorValue = rootObject->getProperty ("author");
    if (! readExactInteger (rootObject->getProperty ("schemaVersion"), schemaVersion)
        || schemaVersion != presetSchemaVersion
        || ! readExactInteger (rootObject->getProperty ("presetCount"), declaredPresetCount)
        || declaredPresetCount != expectedFactoryPresetCount
        || ! bankNameValue.isString() || bankNameValue.toString() != factoryBankName
        || ! authorValue.isString() || authorValue.toString() != factoryAuthor)
        return false;

    const auto* presets = rootObject->getProperty ("presets").getArray();
    if (presets == nullptr || presets->size() != expectedFactoryPresetCount)
        return false;

    std::vector<Preset> parsedPresets;
    parsedPresets.reserve (static_cast<std::size_t> (presets->size()));
    juce::StringArray names;
    juce::StringArray descriptions;
    for (const auto& presetValue : *presets)
    {
        const auto* object = presetValue.getDynamicObject();
        if (object == nullptr
            || ! hasExactProperties (*object,
                                     { "schemaVersion", "name", "category", "description",
                                       "tags", "author", "parameters" }))
            return false;

        int presetVersion = 0;
        const auto& nameValue = object->getProperty ("name");
        const auto& categoryValue = object->getProperty ("category");
        const auto& descriptionValue = object->getProperty ("description");
        const auto& presetAuthorValue = object->getProperty ("author");
        const auto& parameters = object->getProperty ("parameters");

        if (! readExactInteger (object->getProperty ("schemaVersion"), presetVersion)
            || presetVersion != presetSchemaVersion
            || ! nameValue.isString() || nameValue.toString().trim().isEmpty()
            || ! categoryValue.isString() || ! isKnownCategory (categoryValue.toString())
            || ! descriptionValue.isString() || descriptionValue.toString().trim().isEmpty()
            || ! presetAuthorValue.isString() || presetAuthorValue.toString() != factoryAuthor
            || ! validateParameterObject (parameters, false))
            return false;

        const auto name = nameValue.toString().trim();
        const auto description = descriptionValue.toString().trim();
        if (names.contains (name, true) || descriptions.contains (description, true))
            return false;
        names.add (name);
        descriptions.add (description);

        const auto* tags = object->getProperty ("tags").getArray();
        if (tags == nullptr || tags->size() < 3)
            return false;

        juce::StringArray parsedTags;
        for (const auto& tagValue : *tags)
        {
            if (! tagValue.isString() || tagValue.toString().trim().isEmpty())
                return false;
            const auto tag = tagValue.toString().trim();
            if (parsedTags.contains (tag, true))
                return false;
            parsedTags.add (tag);
        }

        const auto* parameterValues = parameters.getDynamicObject();
        jassert (parameterValues != nullptr);
        const auto voiceMode = static_cast<int> (static_cast<double> (
            parameterValues->getProperty (ids::voiceMode)));
        const auto legato = static_cast<bool> (parameterValues->getProperty (ids::legato));
        const auto glide = static_cast<double> (parameterValues->getProperty (ids::glide));
        const auto hasGlideTag = std::any_of (parsedTags.begin(), parsedTags.end(), [] (const auto& tag)
        {
            return ("-" + tag + "-").contains ("-glide-");
        });
        if ((parsedTags.contains ("mono", true) && voiceMode != 0)
            || ((parsedTags.contains ("poly", true) || parsedTags.contains ("polyphonic", true))
                && voiceMode != 1)
            || (parsedTags.contains ("legato", true) && ! legato)
            || (hasGlideTag && glide <= 0.0)
            || (parsedTags.contains ("mono-legato", true) && (voiceMode != 0 || ! legato)))
            return false;

        Preset preset;
        preset.name = name;
        preset.category = categoryValue.toString();
        preset.description = description;
        preset.tags = std::move (parsedTags);
        preset.author = presetAuthorValue.toString();
        preset.parameters = parameters;
        parsedPresets.push_back (std::move (preset));
    }

    for (const auto& [category, expectedCount] : expectedCategoryCounts)
    {
        const auto actualCount = std::count_if (parsedPresets.begin(), parsedPresets.end(),
                                                [category] (const Preset& preset)
                                                {
                                                    return preset.category == category;
                                                });
        if (actualCount != expectedCount)
            return false;
    }

    factoryPresets = std::move (parsedPresets);
    return true;
}

bool PresetManager::applyParameterObject (const juce::var& parameterObject,
                                          bool allowLegacyNumericTypes)
{
    if (! validateParameterObject (parameterObject, allowLegacyNumericTypes))
        return false;

    const auto* object = parameterObject.getDynamicObject();
    jassert (object != nullptr);

    applyingPreset.store (true, std::memory_order_release);

    for (const auto& id : parameterIDs())
    {
        auto* parameter = state.getParameter (id);
        jassert (parameter != nullptr);

        const auto actualValue = static_cast<float> (static_cast<double> (object->getProperty (id)));
        const auto normalised = parameter->convertTo0to1 (actualValue);
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (normalised);
        parameter->endChangeGesture();
    }

    applyingPreset.store (false, std::memory_order_release);
    return true;
}

bool PresetManager::validateParameterObject (const juce::var& parameterObject,
                                             bool allowLegacyNumericTypes) const
{
    const auto* object = parameterObject.getDynamicObject();
    if (object == nullptr || object->getProperties().size() != static_cast<int> (parameterIDs().size()))
        return false;

    for (const auto& id : parameterIDs())
    {
        const auto identifier = juce::Identifier (id);
        if (! object->hasProperty (identifier))
            return false;

        const auto& value = object->getProperty (identifier);
        if (id == ids::legato)
        {
            if (! value.isBool())
            {
                if (! allowLegacyNumericTypes || ! isNumericValue (value))
                    return false;
                const auto legacyBoolean = static_cast<double> (value);
                if (std::abs (legacyBoolean) > 1.0e-12
                    && std::abs (legacyBoolean - 1.0) > 1.0e-12)
                    return false;
            }
        }
        else if (isIntegerParameter (id))
        {
            if (! isIntegerValue (value))
            {
                if (! allowLegacyNumericTypes || ! isNumericValue (value))
                    return false;
                const auto legacyInteger = static_cast<double> (value);
                if (! std::isfinite (legacyInteger)
                    || std::abs (std::round (legacyInteger) - legacyInteger) > 1.0e-12)
                    return false;
            }
        }
        else if (! isNumericValue (value))
        {
            return false;
        }

        const auto actual = static_cast<double> (value);
        if (! std::isfinite (actual))
            return false;

        const auto* parameter = state.getParameter (id);
        if (parameter == nullptr)
            return false;

        const auto& range = parameter->getNormalisableRange();
        const auto actualFloat = static_cast<float> (actual);
        if (! std::isfinite (actualFloat)
            || actualFloat < range.start
            || actualFloat > range.end)
            return false;

        const auto snapped = range.snapToLegalValue (actualFloat);
        const auto snapTolerance = std::max (1.0e-6f, std::abs (range.interval) * 1.0e-3f);
        if (! std::isfinite (snapped) || std::abs (snapped - actualFloat) > snapTolerance)
            return false;
    }

    return true;
}

juce::var PresetManager::createCurrentParameterObject() const
{
    auto* object = new juce::DynamicObject();
    for (const auto& id : parameterIDs())
    {
        if (const auto* parameter = state.getParameter (id))
        {
            const auto value = parameter->convertFrom0to1 (parameter->getValue());
            if (id == ids::legato)
                object->setProperty (id, value >= 0.5f);
            else if (isIntegerParameter (id))
                object->setProperty (id, juce::roundToInt (value));
            else
                object->setProperty (id, static_cast<double> (value));
        }
    }
    return juce::var (object);
}

const std::vector<juce::String>& PresetManager::parameterIDs()
{
    static const std::vector<juce::String> idsList {
        ids::voiceMode, ids::legato, ids::triggerMode, ids::velocitySens,
        ids::tune, ids::fine, ids::bendRange, ids::glide,
        ids::body, ids::harmonics, ids::harmonicBalance,
        ids::pitchDrop, ids::pitchDecay, ids::pitchCurve,
        ids::attack, ids::hold, ids::decay, ids::sustain, ids::release, ids::ampCurve,
        ids::click, ids::clickTone, ids::clickDecay, ids::punch,
        ids::tone, ids::toneKeytrack, ids::drive, ids::driveMode,
        ids::compressor, ids::clipper, ids::output
    };
    return idsList;
}
} // namespace glo
