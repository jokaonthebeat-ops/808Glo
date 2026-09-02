#pragma once

#include <JuceHeader.h>

namespace glo::ui
{
struct GloPalette final
{
    [[nodiscard]] static juce::Colour canvas() noexcept;
    [[nodiscard]] static juce::Colour panel() noexcept;
    [[nodiscard]] static juce::Colour panelRaised() noexcept;
    [[nodiscard]] static juce::Colour controlWell() noexcept;
    [[nodiscard]] static juce::Colour controlHover() noexcept;
    [[nodiscard]] static juce::Colour border() noexcept;
    [[nodiscard]] static juce::Colour borderHighlight() noexcept;
    [[nodiscard]] static juce::Colour accent() noexcept;
    [[nodiscard]] static juce::Colour accentHot() noexcept;
    [[nodiscard]] static juce::Colour textPrimary() noexcept;
    [[nodiscard]] static juce::Colour textSecondary() noexcept;
    [[nodiscard]] static juce::Colour textMuted() noexcept;
    [[nodiscard]] static juce::Colour warning() noexcept;
    [[nodiscard]] static juce::Colour clip() noexcept;
};

class GloLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    GloLookAndFeel();
    ~GloLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics&,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosition,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Slider::SliderLayout getSliderLayout (juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&,
                               juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool isMouseOverButton,
                               bool isButtonDown) override;

    void drawButtonText (juce::Graphics&,
                         juce::TextButton&,
                         bool isMouseOverButton,
                         bool isButtonDown) override;

    void drawToggleButton (juce::Graphics&,
                           juce::ToggleButton&,
                           bool isMouseOverButton,
                           bool isButtonDown) override;

    void drawComboBox (juce::Graphics&,
                       int width,
                       int height,
                       bool isButtonDown,
                       int buttonX,
                       int buttonY,
                       int buttonW,
                       int buttonH,
                       juce::ComboBox&) override;

    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

    [[nodiscard]] juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    [[nodiscard]] juce::Font getComboBoxFont (juce::ComboBox&) override;

    static void drawCardBackground (juce::Graphics&,
                                    juce::Rectangle<float> bounds,
                                    float cornerRadius = 16.0f,
                                    bool emphasised = false);

    static void drawBrandLogo (juce::Graphics&,
                               juce::Rectangle<float> bounds,
                               float signalIntensity = 0.0f);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GloLookAndFeel)
};
} // namespace glo::ui
