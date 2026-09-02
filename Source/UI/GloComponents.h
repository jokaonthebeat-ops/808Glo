#pragma once

#include <JuceHeader.h>

#include "GloLookAndFeel.h"

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace glo::ui
{
class GloCard : public juce::Component
{
public:
    explicit GloCard (juce::String title = {}, juce::String subtitle = {});
    ~GloCard() override = default;

    void paint (juce::Graphics&) override;

    void setTitle (juce::String newTitle);
    void setSubtitle (juce::String newSubtitle);
    void setEmphasised (bool shouldEmphasise);

protected:
    [[nodiscard]] juce::Rectangle<int> getContentBounds() const;

private:
    juce::String cardTitle;
    juce::String cardSubtitle;
    bool emphasised { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GloCard)
};

class GloLogo final : public juce::Component
{
public:
    GloLogo();
    ~GloLogo() override = default;

    void paint (juce::Graphics&) override;
    void setSignalIntensity (float newIntensity);

private:
    float signalIntensity { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GloLogo)
};

/**
    Message-thread oscilloscope. The provider is called by the UI timer and must
    copy up to maxSamples into destination, returning the number copied. A typical
    provider drains a lock-free FIFO owned by the processor.
*/
class ScopeComponent final : public juce::Component,
                             private juce::Timer
{
public:
    using SampleProvider = std::function<int (float* destination, int maxSamples)>;

    ScopeComponent();
    ~ScopeComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    void setSampleProvider (SampleProvider newProvider);
    void setRefreshRateHz (int newRateHz);
    void setReducedMotion (bool shouldReduceMotion);
    void setFrozen (bool shouldFreeze);
    void setTraceGain (float newGain);
    void clear();

    [[nodiscard]] bool isFrozen() const noexcept { return frozen; }
    [[nodiscard]] float getDisplayedPeak() const noexcept { return displayedPeak; }
    [[nodiscard]] float getDisplayedRms() const noexcept { return displayedRms; }

private:
    void timerCallback() override;
    void updateTimerState();
    void drawGrid (juce::Graphics&, juce::Rectangle<float>) const;
    void drawWaveform (juce::Graphics&, juce::Rectangle<float>) const;

    static constexpr int sampleCapacity = 4096;
    std::array<float, sampleCapacity> sampleBuffer {};
    SampleProvider sampleProvider;
    int validSampleCount { 0 };
    int refreshRateHz { 60 };
    float traceGain { 1.0f };
    float displayedPeak { 0.0f };
    float displayedRms { 0.0f };
    bool reducedMotion { false };
    bool frozen { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeComponent)
};

/** A stereo peak meter whose sources contain linear gain values. */
class StereoMeter final : public juce::Component,
                          private juce::Timer
{
public:
    StereoMeter();
    ~StereoMeter() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    /**
        Source atomics must outlive this meter and must be detached with nullptr
        before they are destroyed. Call this only on the message thread, or while
        holding a MessageManagerLock, so the timer cannot dereference an old source.
    */
    void setLevelSources (const std::atomic<float>* leftSource,
                          const std::atomic<float>* rightSource) noexcept;

    /** Audio-thread-safe alternative that writes to meter-owned atomics. */
    void setLevels (float leftLinear, float rightLinear, bool ceilingWasHit = false) noexcept;
    void setRefreshRateHz (int newRateHz);
    void clearClip() noexcept;

private:
    void timerCallback() override;
    void updateTimerState();
    void drawMeterLane (juce::Graphics&,
                        juce::Rectangle<float> bounds,
                        float linearLevel,
                        float heldLevel,
                        juce::StringRef label) const;

    std::atomic<float> ownedLeft { 0.0f };
    std::atomic<float> ownedRight { 0.0f };
    std::atomic<bool> ownedCeilingHit { false };
    const std::atomic<float>* leftLevelSource { &ownedLeft };
    const std::atomic<float>* rightLevelSource { &ownedRight };

    float displayedLeft { 0.0f };
    float displayedRight { 0.0f };
    float heldLeft { 0.0f };
    float heldRight { 0.0f };
    int leftHoldFrames { 0 };
    int rightHoldFrames { 0 };
    int refreshRateHz { 30 };
    bool clipLatched { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StereoMeter)
};

class ParameterKnob final : public juce::Component
{
public:
    ParameterKnob (juce::AudioProcessorValueTreeState&,
                   const juce::String& parameterID,
                   juce::String displayName);
    ~ParameterKnob() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setFeatured (bool shouldBeFeatured);

    [[nodiscard]] juce::Slider& getSlider() noexcept { return slider; }
    [[nodiscard]] const juce::Slider& getSlider() const noexcept { return slider; }

private:
    juce::Label nameLabel;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    bool featured { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterKnob)
};

class ParameterToggle final : public juce::Component
{
public:
    ParameterToggle (juce::AudioProcessorValueTreeState&,
                     const juce::String& parameterID,
                     juce::String displayName);
    ~ParameterToggle() override = default;

    void resized() override;

    [[nodiscard]] juce::ToggleButton& getButton() noexcept { return button; }

private:
    juce::ToggleButton button;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterToggle)
};

class ParameterChoice final : public juce::Component
{
public:
    ParameterChoice (juce::AudioProcessorValueTreeState&,
                     const juce::String& parameterID,
                     juce::String displayName,
                     const juce::StringArray& choices);
    ~ParameterChoice() override = default;

    void resized() override;

    [[nodiscard]] juce::ComboBox& getComboBox() noexcept { return comboBox; }

private:
    juce::Label nameLabel;
    juce::ComboBox comboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterChoice)
};

class ParameterPanel final : public GloCard
{
public:
    ParameterPanel (juce::String title,
                    juce::AudioProcessorValueTreeState& parameterState);
    ~ParameterPanel() override = default;

    ParameterKnob& addKnob (const juce::String& parameterID,
                            juce::String displayName);
    ParameterToggle& addToggle (const juce::String& parameterID,
                                juce::String displayName);
    ParameterChoice& addChoice (const juce::String& parameterID,
                                juce::String displayName,
                                const juce::StringArray& choices);

    void setColumns (int newColumnCount);
    void resized() override;

private:
    template <typename Control, typename... Args>
    Control& addControl (Args&&... args)
    {
        auto control = std::make_unique<Control> (std::forward<Args> (args)...);
        auto& result = *control;
        addAndMakeVisible (result);
        controls.push_back (std::move (control));
        resized();
        return result;
    }

    juce::AudioProcessorValueTreeState& state;
    std::vector<std::unique_ptr<juce::Component>> controls;
    int columns { 4 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterPanel)
};
} // namespace glo::ui
