#include "PluginProcessor.h"

#include <JuceHeader.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace
{
int failures = 0;

void check (bool condition, const std::string& message)
{
    if (condition)
        return;

    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}
} // namespace

int main()
{
    EightOhEightGloProAudioProcessor processor;

    check (processor.getPresetManager().getNumberOfFactoryPresets() == 128,
           "the complete embedded factory bank must load atomically");
    check (processor.getNumPrograms() == 129,
           "host programs must expose Init plus all 128 factory presets");
    check (processor.getParameters().size() == 31,
           "the public automation contract must contain exactly 31 parameters");

    for (int preset = 0; preset < processor.getPresetManager().getNumberOfFactoryPresets(); ++preset)
    {
        const auto hostProgram = preset + 1;
        processor.setCurrentProgram (hostProgram);
        check (processor.getCurrentProgram() == hostProgram,
               "host program selection must retain a valid factory index");
        check (processor.getProgramName (hostProgram).isNotEmpty(),
               "every factory program must have a host-visible name");

        for (const auto* parameter : processor.getParameters())
        {
            const auto value = parameter->getValue();
            check (std::isfinite (value) && value >= 0.0f && value <= 1.0f,
                   "factory parameters must remain finite and normalised");
        }
    }

    // Deliberately prepare for 64 samples, then render 4096. Some hosts use
    // larger offline blocks than the size passed to prepareToPlay().
    processor.prepareToPlay (48000.0, 64);
    juce::AudioBuffer<float> audio (2, 4096);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 36, static_cast<juce::uint8> (120)), 0);
    processor.processBlock (audio, midi);

    auto peak = 0.0f;
    auto energy = 0.0;
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
    {
        const auto* samples = audio.getReadPointer (channel);
        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
        {
            const auto value = samples[sample];
            check (std::isfinite (value), "rendered samples must be finite");
            peak = std::max (peak, std::abs (value));
            energy += static_cast<double> (value) * static_cast<double> (value);
        }
    }

    check (energy > 1.0e-6, "a MIDI note must produce audio");
    check (peak <= 0.913f, "the post-reconstruction safety ceiling must hold");
    check (processor.getLatencySamples() > 0, "4x oversampling latency must be reported");

    // UI keyboard messages must reach the synth through the processor's fixed
    // SPSC queue without mutating or growing the host-provided MidiBuffer.
    EightOhEightGloProAudioProcessor keyboardProcessor;
    const auto setActualParameter = [&keyboardProcessor] (const char* parameterID, float actualValue)
    {
        auto* parameter = keyboardProcessor.getParameterState().getParameter (parameterID);
        check (parameter != nullptr, std::string ("missing test parameter: ") + parameterID);
        if (parameter != nullptr)
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (actualValue));
    };

    setActualParameter (glo::ids::triggerMode, 1.0f);
    setActualParameter (glo::ids::attack, 0.05f);
    setActualParameter (glo::ids::decay, 50.0f);
    setActualParameter (glo::ids::sustain, 1.0f);
    setActualParameter (glo::ids::release, 5.0f);
    keyboardProcessor.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> keyboardAudio (2, 512);
    juce::MidiBuffer emptyMidi;
    keyboardProcessor.getKeyboardState().noteOn (1, 48, 1.0f);
    keyboardProcessor.processBlock (keyboardAudio, emptyMidi);

    auto keyboardEnergy = 0.0;
    for (int channel = 0; channel < keyboardAudio.getNumChannels(); ++channel)
        for (int sampleIndex = 0; sampleIndex < keyboardAudio.getNumSamples(); ++sampleIndex)
        {
            const auto sample = keyboardAudio.getSample (channel, sampleIndex);
            keyboardEnergy += static_cast<double> (sample) * static_cast<double> (sample);
        }
    check (keyboardEnergy > 1.0e-6,
           "the allocation-free UI note queue must produce audio");
    check (emptyMidi.isEmpty(),
           "UI note injection must not mutate the host MIDI buffer");

    // Force overflow while a gated voice is sustaining. The dropped batch must
    // trigger an audio-thread panic; otherwise the missing note-off could leave
    // the voice sounding indefinitely.
    for (int event = 0; event < 512; ++event)
        keyboardProcessor.getKeyboardState().noteOn (1, 48, 1.0f);

    auto finalPanicPeak = 0.0f;
    for (int block = 0; block < 40; ++block)
    {
        keyboardProcessor.processBlock (keyboardAudio, emptyMidi);
        if (block == 39)
        {
            for (int channel = 0; channel < keyboardAudio.getNumChannels(); ++channel)
                finalPanicPeak = std::max (
                    finalPanicPeak,
                    keyboardAudio.getMagnitude (channel, 0, keyboardAudio.getNumSamples()));
        }
    }
    check (finalPanicPeak < 1.0e-4f,
           "UI queue overflow must panic instead of leaving a stuck gated note");

    keyboardProcessor.getKeyboardState().noteOff (1, 48, 0.0f);
    keyboardProcessor.releaseResources();

    setActualParameter (glo::ids::attack, 50.0f);
    setActualParameter (glo::ids::hold, 250.0f);
    setActualParameter (glo::ids::decay, 8000.0f);
    setActualParameter (glo::ids::release, 3000.0f);
    check (keyboardProcessor.getTailLengthSeconds() >= 11.5,
           "tail reporting must include attack, hold, decay, release, and residual filters");

    // Host MIDI must light the UI keyboard through the lock-free mirror, and a
    // mirrored note must never re-enter the UI note queue and re-trigger the
    // synth. Sustain level zero makes an accidental re-trigger audible.
    EightOhEightGloProAudioProcessor mirrorProcessor;
    const auto setMirrorParameter = [&mirrorProcessor] (const char* parameterID, float actualValue)
    {
        auto* parameter = mirrorProcessor.getParameterState().getParameter (parameterID);
        check (parameter != nullptr, std::string ("missing mirror test parameter: ") + parameterID);
        if (parameter != nullptr)
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (actualValue));
    };
    setMirrorParameter (glo::ids::triggerMode, 1.0f);
    setMirrorParameter (glo::ids::attack, 0.05f);
    setMirrorParameter (glo::ids::decay, 30.0f);
    setMirrorParameter (glo::ids::sustain, 0.0f);
    setMirrorParameter (glo::ids::release, 5.0f);
    mirrorProcessor.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> mirrorAudio (2, 512);
    juce::MidiBuffer mirrorMidi;
    mirrorMidi.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    mirrorProcessor.processBlock (mirrorAudio, mirrorMidi);
    mirrorMidi.clear();

    for (int block = 0; block < 40; ++block)
        mirrorProcessor.processBlock (mirrorAudio, mirrorMidi);
    const auto silentBeforeMirror = std::max (mirrorAudio.getMagnitude (0, 0, 512),
                                              mirrorAudio.getMagnitude (1, 0, 512));
    check (silentBeforeMirror < 1.0e-4f,
           "a zero-sustain gated note must decay to silence while held");

    mirrorProcessor.applyPendingHostNotesToKeyboard();
    check (mirrorProcessor.getKeyboardState().isNoteOn (1, 60),
           "a host note-on must light the UI keyboard after the mirror drain");

    mirrorProcessor.processBlock (mirrorAudio, mirrorMidi);
    const auto peakAfterMirror = std::max (mirrorAudio.getMagnitude (0, 0, 512),
                                           mirrorAudio.getMagnitude (1, 0, 512));
    check (peakAfterMirror < 1.0e-4f,
           "mirroring host notes to the keyboard must not re-trigger the synth");

    mirrorMidi.addEvent (juce::MidiMessage::noteOff (1, 60, static_cast<juce::uint8> (0)), 0);
    mirrorProcessor.processBlock (mirrorAudio, mirrorMidi);
    mirrorMidi.clear();
    mirrorProcessor.applyPendingHostNotesToKeyboard();
    check (! mirrorProcessor.getKeyboardState().isNoteOn (1, 60),
           "a host note-off must unlight the UI keyboard after the mirror drain");

    mirrorMidi.addEvent (juce::MidiMessage::noteOn (1, 64, static_cast<juce::uint8> (100)), 0);
    mirrorMidi.addEvent (juce::MidiMessage::allNotesOff (1), 64);
    mirrorProcessor.processBlock (mirrorAudio, mirrorMidi);
    mirrorMidi.clear();
    mirrorProcessor.applyPendingHostNotesToKeyboard();
    check (! mirrorProcessor.getKeyboardState().isNoteOn (1, 64),
           "all-notes-off must clear the mirrored UI keyboard display");
    mirrorProcessor.releaseResources();

    processor.setCurrentProgram (37);
    juce::MemoryBlock state;
    processor.getStateInformation (state);
    check (! state.isEmpty(), "DAW state serialization must produce data");

    processor.setCurrentProgram (2);
    processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    check (processor.getCurrentProgram() == 37,
           "DAW state round-trip must restore factory preset identity");
    check (processor.getPresetManager().getCurrentPresetName()
               == processor.getProgramName (processor.getCurrentProgram()),
           "restored preset metadata must match the restored host program");

    processor.releaseResources();

    if (failures != 0)
    {
        std::cerr << failures << " JUCE integration check(s) failed\n";
        return 1;
    }

    std::cout << "808Glo Pro JUCE integration checks passed\n";
    return 0;
}
