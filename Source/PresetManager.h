#pragma once

#include <JuceHeader.h>

#include <atomic>
#include <vector>

namespace glo
{
class PresetManager final : private juce::AudioProcessorValueTreeState::Listener
{
public:
    struct Preset
    {
        juce::String name;
        juce::String category;
        juce::String description;
        juce::StringArray tags;
        juce::String author;
        juce::var parameters;
    };

    explicit PresetManager (juce::AudioProcessorValueTreeState& stateToUse);
    ~PresetManager() override;

    PresetManager (const PresetManager&) = delete;
    PresetManager& operator= (const PresetManager&) = delete;

    [[nodiscard]] int getNumberOfFactoryPresets() const noexcept;
    [[nodiscard]] const Preset& getFactoryPreset (int index) const;
    [[nodiscard]] juce::StringArray getCategories() const;
    [[nodiscard]] std::vector<int> search (const juce::String& query,
                                           const juce::String& category = {}) const;

    bool loadFactoryPreset (int index);
    bool loadNextFactoryPreset (int direction);
    bool loadInitPreset();
    bool loadUserPreset (const juce::File& file);
    bool saveUserPreset (const juce::File& file,
                         const juce::String& name,
                         const juce::StringArray& tags = {});

    [[nodiscard]] int getCurrentFactoryPresetIndex() const noexcept;
    [[nodiscard]] juce::String getCurrentPresetName() const;
    [[nodiscard]] juce::String getCurrentPresetCategory() const;
    [[nodiscard]] bool isDirty() const noexcept;
    void markClean() noexcept;

    [[nodiscard]] juce::File getDefaultUserPresetDirectory() const;

    void restoreMetadataFromState (const juce::ValueTree& restoredState);
    void writeMetadataToState (juce::ValueTree& stateSnapshot) const;

private:
    void parameterChanged (const juce::String&, float) override;
    bool parseFactoryBank();
    bool validateParameterObject (const juce::var& parameterObject,
                                  bool allowLegacyNumericTypes) const;
    bool applyParameterObject (const juce::var& parameterObject,
                               bool allowLegacyNumericTypes = false);
    juce::var createCurrentParameterObject() const;
    static const std::vector<juce::String>& parameterIDs();

    juce::AudioProcessorValueTreeState& state;
    std::vector<Preset> factoryPresets;
    mutable juce::CriticalSection metadataLock;
    juce::String userPresetName { "Init 808" };
    juce::String userPresetCategory { "Init" };
    std::atomic<int> currentFactoryIndex { -1 };
    std::atomic<bool> dirty { false };
    std::atomic<bool> applyingPreset { false };
};
} // namespace glo
