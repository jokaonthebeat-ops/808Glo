#pragma once

#include "Voice.h"

#include <array>
#include <cstdint>

namespace glo::dsp
{
class SynthEngine
{
public:
    static constexpr std::size_t maxVoices = 8;

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = std::max (1.0, newSampleRate);
        for (auto& voice : voices)
            voice.prepare (sampleRate);
        reset();
    }

    void reset() noexcept
    {
        for (auto& voice : voices)
            voice.reset();

        heldNotes.fill (false);
        sustainedNotes.fill (false);
        noteOrder.fill (0);
        noteVelocities.fill (1.0);
        eventCounter = 0;
        pitchBend = 0.0;
        sustainPedal = false;
    }

    void noteOn (int midiNote, double velocity, const VoiceParameters& parameters) noexcept
    {
        const auto safeNote = clamp (midiNote, 0, 127);
        const auto hadHeldNote = getMostRecentHeldNote() >= 0;
        const auto noteWasAlreadyHeld = heldNotes[static_cast<std::size_t> (safeNote)];
        heldNotes[static_cast<std::size_t> (safeNote)] = true;
        sustainedNotes[static_cast<std::size_t> (safeNote)] = false;
        noteOrder[static_cast<std::size_t> (safeNote)] = ++eventCounter;
        noteVelocities[static_cast<std::size_t> (safeNote)] = clamp (velocity, 0.0, 1.0);

        if (parameters.voiceMode == 0)
        {
            auto& voice = voices[0];
            const auto wasActive = voice.isActive();
            const auto trueLegatoTransition = parameters.legato && wasActive
                                           && hadHeldNote && ! noteWasAlreadyHeld;
            const auto shouldGlide = trueLegatoTransition && parameters.glideMs > 0.01;
            const auto retrigger = ! trueLegatoTransition;
            voice.startNote (safeNote, velocity, parameters, shouldGlide, retrigger, eventCounter);
            voice.setPitchBend (pitchBend);
            return;
        }

        auto& voice = choosePolyVoice (safeNote);
        voice.startNote (safeNote, velocity, parameters, false, true, eventCounter);
        voice.setPitchBend (pitchBend);
    }

    void noteOff (int midiNote, const VoiceParameters& parameters) noexcept
    {
        const auto safeNote = clamp (midiNote, 0, 127);
        heldNotes[static_cast<std::size_t> (safeNote)] = false;

        if (sustainPedal)
        {
            sustainedNotes[static_cast<std::size_t> (safeNote)] = true;
            return;
        }

        releaseNote (safeNote, parameters);
    }

    void setSustainPedal (bool isDown, const VoiceParameters& parameters) noexcept
    {
        if (sustainPedal == isDown)
            return;

        sustainPedal = isDown;
        if (isDown)
            return;

        for (int note = 0; note < 128; ++note)
        {
            const auto index = static_cast<std::size_t> (note);
            if (sustainedNotes[index] && ! heldNotes[index])
            {
                sustainedNotes[index] = false;
                releaseNote (note, parameters);
            }
        }
    }

    void setPitchBend (double normalisedBend) noexcept
    {
        pitchBend = clamp (normalisedBend, -1.0, 1.0);
        for (auto& voice : voices)
            voice.setPitchBend (pitchBend);
    }

    void allNotesOff (bool allowTail) noexcept
    {
        heldNotes.fill (false);
        sustainedNotes.fill (false);
        sustainPedal = false;

        for (auto& voice : voices)
        {
            if (allowTail)
                voice.forceRelease();
            else
                voice.kill();
        }
    }

    void render (float* const* outputChannels,
                 int numberOfChannels,
                 int startSample,
                 int numberOfSamples,
                 const VoiceParameters& parameters) noexcept
    {
        if (outputChannels == nullptr || numberOfChannels <= 0 || numberOfSamples <= 0)
            return;

        for (auto& voice : voices)
            if (voice.isActive())
                voice.updateParameters (parameters);

        const auto voiceLimit = parameters.voiceMode == 0 ? std::size_t { 1 } : maxVoices;

        for (int sample = 0; sample < numberOfSamples; ++sample)
        {
            auto mixed = 0.0;
            for (std::size_t voiceIndex = 0; voiceIndex < voiceLimit; ++voiceIndex)
                mixed += voices[voiceIndex].renderSample();

            // Polyphonic stacks need predictable headroom without changing the
            // single-note transient or relying on a look-ahead limiter here.
            // Keep the dependency-free voice core linear. All nonlinear peak
            // control runs in the processor's fixed 4x oversampled path.
            mixed *= parameters.voiceMode == 0 ? 0.88 : 0.34;

            for (int channel = 0; channel < numberOfChannels; ++channel)
                outputChannels[channel][startSample + sample] += static_cast<float> (mixed);
        }
    }

    [[nodiscard]] std::size_t getActiveVoiceCount() const noexcept
    {
        std::size_t count = 0;
        for (const auto& voice : voices)
            if (voice.isActive())
                ++count;
        return count;
    }

    [[nodiscard]] const Voice& getVoice (std::size_t index) const noexcept
    {
        return voices[index % maxVoices];
    }

private:
    Voice& choosePolyVoice (int midiNote) noexcept
    {
        for (auto& voice : voices)
            if (voice.isActive() && voice.getNote() == midiNote)
                return voice;

        for (auto& voice : voices)
            if (! voice.isActive())
                return voice;

        auto* oldest = &voices[0];
        for (auto& voice : voices)
        {
            const auto quieter = voice.getEnvelopeValue() < oldest->getEnvelopeValue() - 1.0e-9;
            const auto equallyQuiet = std::abs (voice.getEnvelopeValue() - oldest->getEnvelopeValue()) <= 1.0e-9;
            if (quieter || (equallyQuiet && voice.getStartCounter() < oldest->getStartCounter()))
                oldest = &voice;
        }
        return *oldest;
    }

    void releaseNote (int midiNote, const VoiceParameters& parameters) noexcept
    {
        if (parameters.voiceMode == 0)
        {
            auto& monoVoice = voices[0];
            if (monoVoice.getNote() != midiNote)
                return;

            const auto replacement = getMostRecentHeldNote();
            if (replacement >= 0)
            {
                monoVoice.startNote (replacement,
                                     noteVelocities[static_cast<std::size_t> (replacement)],
                                     parameters,
                                     parameters.glideMs > 0.01,
                                     ! parameters.legato,
                                     ++eventCounter);
                monoVoice.setPitchBend (pitchBend);
            }
            else
            {
                monoVoice.stopNote (parameters);
            }
            return;
        }

        for (auto& voice : voices)
            if (voice.isActive() && voice.getNote() == midiNote)
                voice.stopNote (parameters);
    }

    int getMostRecentHeldNote() const noexcept
    {
        int selected = -1;
        std::uint64_t newestOrder = 0;
        for (int note = 0; note < 128; ++note)
        {
            const auto index = static_cast<std::size_t> (note);
            if (heldNotes[index] && noteOrder[index] >= newestOrder)
            {
                newestOrder = noteOrder[index];
                selected = note;
            }
        }
        return selected;
    }

    std::array<Voice, maxVoices> voices;
    std::array<bool, 128> heldNotes {};
    std::array<bool, 128> sustainedNotes {};
    std::array<std::uint64_t, 128> noteOrder {};
    std::array<double, 128> noteVelocities {};
    double sampleRate { 44100.0 };
    double pitchBend { 0.0 };
    std::uint64_t eventCounter { 0 };
    bool sustainPedal { false };
};
} // namespace glo::dsp
