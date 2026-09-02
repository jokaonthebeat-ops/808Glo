#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "UI/GloComponents.h"
#include "UI/GloLookAndFeel.h"

#include <memory>

class EightOhEightGloProAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                     private juce::Timer
{
public:
    explicit EightOhEightGloProAudioProcessorEditor (EightOhEightGloProAudioProcessor&);
    ~EightOhEightGloProAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    class PresetBrowserOverlay;

    void timerCallback() override;
    void updatePresetDisplay();
    void showPresetBrowser();
    void saveUserPreset();
    void loadUserPreset();
    [[nodiscard]] juce::Rectangle<int> designRect (float x, float y, float width, float height) const;

    EightOhEightGloProAudioProcessor& processor;
    glo::ui::GloLookAndFeel gloLookAndFeel;
    juce::TooltipWindow tooltips { this, 650 };

    glo::ui::GloCard headerCard;
    glo::ui::GloLogo logo;
    juce::TextButton previousPresetButton { juce::CharPointer_UTF8 ("\xe2\x80\xb9") };
    juce::TextButton presetNameButton { "INIT 808" };
    juce::TextButton nextPresetButton { juce::CharPointer_UTF8 ("\xe2\x80\xba") };
    juce::TextButton browserButton { "BROWSE" };
    juce::TextButton saveButton { "SAVE AS" };
    juce::TextButton loadButton { "LOAD" };
    juce::Label presetCategoryLabel;

    glo::ui::ParameterPanel voicePanel;
    glo::ui::GloCard scopeCard { "LIVE 808 SIGNAL", "REAL OUTPUT  /  12 kHz DISPLAY" };
    glo::ui::ScopeComponent scope;
    glo::ui::ParameterKnob punchHero;
    glo::ui::ParameterKnob gloHero;
    glo::ui::ParameterKnob tailHero;
    glo::ui::ParameterPanel masterPanel;
    glo::ui::StereoMeter outputMeter;

    glo::ui::ParameterPanel pitchPanel;
    glo::ui::ParameterPanel envelopePanel;
    glo::ui::ParameterPanel tonePanel;
    glo::ui::ParameterPanel transientPanel;

    glo::ui::GloCard keyboardCard { "PLAY", juce::String::fromUTF8 ("C1–C6  /  MIDI + MOUSE") };
    juce::MidiKeyboardComponent keyboard;
    juce::Label statusLabel;

    std::unique_ptr<PresetBrowserOverlay> presetBrowser;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EightOhEightGloProAudioProcessorEditor)
};
