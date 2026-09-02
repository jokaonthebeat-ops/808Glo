#pragma once

#include <JuceHeader.h>

#include "DSP/OnePoleFilter.h"
#include "DSP/SynthEngine.h"
#include "ParameterLayout.h"
#include "PresetManager.h"

#include <array>
#include <atomic>
#include <cstdint>

class EightOhEightGloProAudioProcessor final : public juce::AudioProcessor,
                                               private juce::MidiKeyboardState::Listener
{
public:
    EightOhEightGloProAudioProcessor();
    ~EightOhEightGloProAudioProcessor() override;

    using juce::AudioProcessor::processBlock;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destinationData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameterState() noexcept { return parameters; }
    glo::PresetManager& getPresetManager() noexcept { return presetManager; }
    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

    [[nodiscard]] float getPeakLevel (int channel) const noexcept;
    [[nodiscard]] float getRmsLevel (int channel) const noexcept;
    [[nodiscard]] bool consumeCeilingHit() noexcept;
    int copyLatestScopeSamples (float* destination, int maximumSamples) const noexcept;

    // Message-thread only: drains the host-MIDI mirror queue into keyboardState
    // so the UI keyboard lights for notes played from the host or a controller.
    void applyPendingHostNotesToKeyboard();

private:
    static constexpr std::size_t scopeCapacity = 16384;
    static constexpr std::uint32_t uiNoteQueueCapacity = 128;
    static constexpr std::uint32_t hostNoteQueueCapacity = 256;

    struct UiNoteEvent
    {
        float velocity { 0.0f };
        std::uint8_t note { 0 };
        bool noteOn { false };
    };

    static_assert ((uiNoteQueueCapacity & (uiNoteQueueCapacity - 1u)) == 0u);
    static_assert ((hostNoteQueueCapacity & (hostNoteQueueCapacity - 1u)) == 0u);
    static_assert (std::atomic<std::uint32_t>::is_always_lock_free);
    static_assert (std::atomic<bool>::is_always_lock_free);

    void handleMidiMessage (const juce::MidiMessage&, const glo::dsp::VoiceParameters&) noexcept;
    void handleNoteOn (juce::MidiKeyboardState*, int midiChannel,
                       int midiNoteNumber, float velocity) noexcept override;
    void handleNoteOff (juce::MidiKeyboardState*, int midiChannel,
                        int midiNoteNumber, float velocity) noexcept override;
    void enqueueUiNote (int midiNoteNumber, float velocity, bool isNoteOn) noexcept;
    void enqueueHostNoteMirror (int midiNoteNumber, float velocity, bool isNoteOn) noexcept;
    void drainUiNoteQueue (const glo::dsp::VoiceParameters&) noexcept;
    void discardUiNoteQueueAndPanic() noexcept;
    void processColourAndDynamics (juce::AudioBuffer<float>& buffer) noexcept;
    void publishMetersAndScope (const juce::AudioBuffer<float>& buffer) noexcept;
    static float waveshape (float sample, float driveGain, int mode) noexcept;

    juce::AudioProcessorValueTreeState parameters;
    glo::ParameterPointers parameterPointers;
    glo::PresetManager presetManager;
    juce::MidiKeyboardState keyboardState;
    glo::dsp::SynthEngine synthEngine;

    juce::dsp::Oversampling<float> oversampling {
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true
    };
    std::array<glo::dsp::DcBlocker, 2> outputDcBlockers;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> driveGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGainSmoothed;
    juce::SmoothedValue<float> compressionSmoothed;
    juce::SmoothedValue<float> clipperSmoothed;
    float compressorEnvelope { 0.0f };
    float compressorAttack { 0.0f };
    float compressorRelease { 0.0f };
    double currentSampleRate { 44100.0 };
    int maximumProcessingBlockSize { 512 };
    int previousVoiceMode { 0 };

    // The editor/message thread is the sole producer and the audio thread is
    // the sole consumer. Publishing an index releases the corresponding cell.
    std::array<UiNoteEvent, uiNoteQueueCapacity> uiNoteQueue {};
    alignas (64) std::atomic<std::uint32_t> uiNoteWriteIndex { 0 };
    alignas (64) std::atomic<std::uint32_t> uiNoteReadIndex { 0 };
    std::atomic<bool> uiNoteOverflowPanic { false };

    // Mirror of host MIDI notes for the UI keyboard display. The audio thread
    // is the sole producer and the editor timer the sole consumer; on overflow
    // or all-notes-off the display is resynchronised via hostNoteMirrorReset
    // rather than risking a stuck-lit key. applyingHostNoteMirror is touched by
    // the message thread only and stops mirrored notes re-entering uiNoteQueue.
    std::array<UiNoteEvent, hostNoteQueueCapacity> hostNoteQueue {};
    alignas (64) std::atomic<std::uint32_t> hostNoteWriteIndex { 0 };
    alignas (64) std::atomic<std::uint32_t> hostNoteReadIndex { 0 };
    std::atomic<bool> hostNoteMirrorReset { false };
    bool applyingHostNoteMirror { false };

    std::array<std::atomic<float>, 2> peakLevels {};
    std::array<std::atomic<float>, 2> rmsLevels {};
    std::atomic<bool> ceilingHit { false };
    std::array<std::atomic<float>, scopeCapacity> scopeSamples {};
    std::atomic<std::uint64_t> scopeWriteCounter { 0 };
    int scopeDecimationCounter { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EightOhEightGloProAudioProcessor)
};
