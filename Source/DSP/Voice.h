#pragma once

#include "Envelope.h"
#include "OnePoleFilter.h"

#include <cstdint>

namespace glo::dsp
{
struct VoiceParameters
{
    int voiceMode { 0 };             // 0 = mono, 1 = poly
    bool legato { true };
    bool oneShot { true };
    double velocitySensitivity { 0.55 };
    double tuneSemitones { 0.0 };
    double fineCents { 0.0 };
    double bendRangeSemitones { 12.0 };
    double glideMs { 85.0 };

    double bodyShape { 0.08 };
    double harmonics { 0.08 };
    double harmonicBalance { 0.0 };  // -1 = odd, +1 = even
    double pitchDropSemitones { 19.0 };
    double pitchDecayMs { 42.0 };
    double pitchCurve { 1.25 };

    double attackMs { 0.5 };
    double holdMs { 0.0 };
    double decayMs { 1050.0 };
    double sustain { 0.0 };
    double releaseMs { 85.0 };
    double ampCurve { 1.7 };

    double clickAmount { 0.12 };
    double clickToneHz { 4200.0 };
    double clickDecayMs { 3.5 };
    double punch { 0.3 };

    double toneHz { 6200.0 };
    double toneKeytrack { 0.12 };
};

class Voice
{
public:
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = std::max (1.0, newSampleRate);
        amplitudeEnvelope.prepare (sampleRate);
        toneFilter.prepare (sampleRate);
        clickSmoother.prepare (sampleRate);
        dcBlocker.prepare (sampleRate);
        reset();
    }

    void reset() noexcept
    {
        amplitudeEnvelope.reset();
        toneFilter.reset();
        clickSmoother.reset();
        dcBlocker.reset();
        phase = 0.0;
        clickPhase = 0.0;
        pitchEnvelopeSamples = 0;
        clickSamples = 0;
        currentPitch = -1.0;
        targetPitch = -1.0;
        glideStartPitch = -1.0;
        glideSamplesElapsed = 0;
        glideSamplesTotal = 0;
        velocityGain = 0.0;
        pitchBend = 0.0;
        lastOutput = 0.0;
        stolenTail = 0.0;
        filterTailActive = false;
        stealFadeSamplesRemaining = 0;
        stealFadeSamplesTotal = 0;
        note = -1;
        startCounter = 0;
    }

    void startNote (int midiNote,
                    double velocity,
                    const VoiceParameters& parameters,
                    bool glideFromCurrent,
                    bool retriggerEnvelope,
                    std::uint64_t newStartCounter) noexcept
    {
        const auto envelopeWasActive = amplitudeEnvelope.isActive();
        const auto startsFreshEnvelope = retriggerEnvelope || ! envelopeWasActive;
        const auto hadRenderedSignal = envelopeWasActive || filterTailActive
                                    || stealFadeSamplesRemaining > 0;
        if (startsFreshEnvelope && hadRenderedSignal)
        {
            stolenTail = lastOutput;
            stealFadeSamplesTotal = std::max<std::uint64_t> (
                1, static_cast<std::uint64_t> (std::llround (sampleRate * 0.003)));
            stealFadeSamplesRemaining = stealFadeSamplesTotal;
        }

        note = clamp (midiNote, 0, 127);
        startCounter = newStartCounter;
        targetPitch = notePitch (parameters);

        if (! glideFromCurrent || currentPitch < 0.0)
        {
            currentPitch = targetPitch;
            glideStartPitch = targetPitch;
            glideSamplesElapsed = 0;
            glideSamplesTotal = 0;
        }
        else
        {
            glideStartPitch = currentPitch;
            glideSamplesElapsed = 0;
            glideSamplesTotal = std::max<std::uint64_t> (
                1, static_cast<std::uint64_t> (std::llround (parameters.glideMs * 0.001 * sampleRate)));
        }

        const auto safeVelocity = clamp (velocity, 0.0, 1.0);
        const auto velocityCurve = std::pow (safeVelocity, 0.72);
        velocityGain = (1.0 - parameters.velocitySensitivity)
                     + parameters.velocitySensitivity * velocityCurve;

        if (startsFreshEnvelope)
        {
            phase = 0.0;
            clickPhase = 0.0;
            pitchEnvelopeSamples = 0;
            clickSamples = 0;
            clickSmoother.reset();
            toneFilter.reset();
            dcBlocker.reset();
            filterTailActive = false;
        }

        EnvelopeParameters envelope;
        envelope.attackMs = parameters.attackMs;
        envelope.holdMs = parameters.holdMs;
        envelope.decayMs = parameters.decayMs;
        envelope.sustain = parameters.sustain;
        envelope.releaseMs = parameters.releaseMs;
        envelope.curve = parameters.ampCurve;
        envelope.oneShot = parameters.oneShot;
        if (startsFreshEnvelope)
            amplitudeEnvelope.noteOn (envelope, true);

        updateParameters (parameters);
        random.seed (0x808000u ^ (static_cast<std::uint32_t> (note) * 2654435761u)
                     ^ static_cast<std::uint32_t> (newStartCounter));
    }

    void stopNote (const VoiceParameters& parameters) noexcept
    {
        updateEnvelopeParameters (parameters);
        amplitudeEnvelope.noteOff();
    }

    void forceRelease() noexcept
    {
        amplitudeEnvelope.forceRelease();
    }

    void kill() noexcept
    {
        reset();
    }

    void setPitchBend (double normalisedBend) noexcept
    {
        pitchBend = clamp (normalisedBend, -1.0, 1.0);
    }

    void updateParameters (const VoiceParameters& parameters) noexcept
    {
        cachedParameters = parameters;
        const auto updatedTarget = notePitch (parameters);
        targetPitch = updatedTarget;
        if (glideSamplesTotal == 0)
            currentPitch = updatedTarget;

        const auto keyOffset = static_cast<double> (std::max (0, note) - 36) / 12.0;
        const auto trackedCutoff = parameters.toneHz * std::exp2 (keyOffset * parameters.toneKeytrack);
        toneFilter.setCutoff (trackedCutoff);
        clickSmoother.setCutoff (clamp (parameters.clickToneHz * 0.42, 80.0, 8000.0));
        updateEnvelopeParameters (parameters);
    }

    double renderSample() noexcept
    {
        if (! amplitudeEnvelope.isActive())
        {
            note = -1;
            auto output = renderFilterTail();
            output = addRetriggerTail (output);
            lastOutput = output;
            return output;
        }

        updateGlide();

        const auto pitchEnvelope = getPitchEnvelopeSemitones();
        const auto bendSemitones = pitchBend * cachedParameters.bendRangeSemitones;
        const auto instantaneousFrequency = midiNoteToHz (currentPitch + pitchEnvelope + bendSemitones);
        const auto increment = clamp (instantaneousFrequency / sampleRate, 0.0, 0.475);
        const auto oscillatorGain = clamp ((sampleRate * 0.475 - instantaneousFrequency)
                                           / (sampleRate * 0.075), 0.0, 1.0);

        phase = wrapPhase (phase + increment);
        const auto angle = phase * twoPi;
        const auto sine = std::sin (angle);
        const auto nyquistGuard = sampleRate * 0.45;
        const auto h2Guard = clamp ((nyquistGuard - instantaneousFrequency * 2.0) / (sampleRate * 0.10), 0.0, 1.0);
        const auto h3Guard = clamp ((nyquistGuard - instantaneousFrequency * 3.0) / (sampleRate * 0.10), 0.0, 1.0);
        const auto h5Guard = clamp ((nyquistGuard - instantaneousFrequency * 5.0) / (sampleRate * 0.10), 0.0, 1.0);

        // BODY is an additive rounded-shape morph rather than an unbounded
        // asin(sine) triangle, keeping the voice oscillator band-limited.
        auto body = sine
                  + cachedParameters.bodyShape
                        * (0.22 * h2Guard * std::sin (angle * 2.0)
                           + 0.12 * h3Guard * std::sin (angle * 3.0));
        body /= 1.0 + cachedParameters.bodyShape * 0.22;

        const auto balance = clamp (cachedParameters.harmonicBalance, -1.0, 1.0);
        const auto evenWeight = 0.5 * (balance + 1.0);
        const auto oddWeight = 1.0 - evenWeight;
        const auto harmonics = cachedParameters.harmonics
                             * (0.34 * evenWeight * h2Guard * std::sin (angle * 2.0)
                                + 0.25 * oddWeight * h3Guard * std::sin (angle * 3.0)
                                + 0.08 * oddWeight * h5Guard * std::sin (angle * 5.0));
        body = (body + harmonics) / (1.0 + cachedParameters.harmonics * 0.45);

        const auto click = renderClick();
        const auto amp = amplitudeEnvelope.process();
        auto initialPunch = 1.0;
        if (pitchEnvelopeSamples < static_cast<std::uint64_t> (sampleRate * 0.20))
            initialPunch += cachedParameters.punch * 0.72
                          * std::exp (-static_cast<double> (pitchEnvelopeSamples)
                                      / std::max (1.0, sampleRate * 0.024));

        ++pitchEnvelopeSamples;
        auto output = (body * oscillatorGain * initialPunch + click) * amp * velocityGain * 0.63;
        output = toneFilter.process (output);
        output = dcBlocker.process (output);

        output = addRetriggerTail (output);

        lastOutput = output;

        if (! amplitudeEnvelope.isActive())
        {
            note = -1;
            filterTailActive = true;
        }

        return output;
    }

    [[nodiscard]] bool isActive() const noexcept { return amplitudeEnvelope.isActive(); }
    [[nodiscard]] int getNote() const noexcept { return note; }
    [[nodiscard]] std::uint64_t getStartCounter() const noexcept { return startCounter; }
    [[nodiscard]] double getEnvelopeValue() const noexcept { return amplitudeEnvelope.getCurrentValue(); }
    [[nodiscard]] double getCurrentFrequency() const noexcept
    {
        return currentPitch < 0.0 ? 0.0 : midiNoteToHz (currentPitch);
    }

private:
    double notePitch (const VoiceParameters& parameters) const noexcept
    {
        return static_cast<double> (std::max (0, note))
             + parameters.tuneSemitones
             + parameters.fineCents * 0.01;
    }

    void updateGlide() noexcept
    {
        if (glideSamplesTotal == 0 || glideSamplesElapsed >= glideSamplesTotal)
        {
            currentPitch = targetPitch;
            glideSamplesTotal = 0;
            return;
        }

        ++glideSamplesElapsed;
        const auto progress = clamp (static_cast<double> (glideSamplesElapsed)
                                     / static_cast<double> (glideSamplesTotal), 0.0, 1.0);
        const auto shaped = smoothstep (progress);
        currentPitch = glideStartPitch + (targetPitch - glideStartPitch) * shaped;

        if (glideSamplesElapsed >= glideSamplesTotal)
        {
            currentPitch = targetPitch;
            glideSamplesTotal = 0;
        }
    }

    double getPitchEnvelopeSemitones() const noexcept
    {
        const auto decaySamples = std::max (1.0, cachedParameters.pitchDecayMs * 0.001 * sampleRate);
        const auto progress = static_cast<double> (pitchEnvelopeSamples) / decaySamples;
        if (progress >= 1.0)
            return 0.0;

        return cachedParameters.pitchDropSemitones
             * std::exp (-6.907755278982137 * std::pow (progress, cachedParameters.pitchCurve));
    }

    double renderClick() noexcept
    {
        const auto age = clickSamples++;
        if (cachedParameters.clickAmount <= 0.0)
            return 0.0;

        const auto decaySamples = std::max (1.0, cachedParameters.clickDecayMs * 0.001 * sampleRate);
        if (static_cast<double> (age) > decaySamples * 13.82)
            return 0.0;

        const auto envelope = std::exp (-static_cast<double> (age) / decaySamples);

        if (envelope < 1.0e-6)
            return 0.0;

        clickPhase = wrapPhase (clickPhase + cachedParameters.clickToneHz / sampleRate);
        const auto tonal = std::sin (clickPhase * twoPi);
        const auto rawNoise = random.nextBipolar();
        const auto highNoise = rawNoise - clickSmoother.process (rawNoise);
        return cachedParameters.clickAmount * envelope * (0.62 * tonal + 0.38 * highNoise);
    }

    double renderFilterTail() noexcept
    {
        if (! filterTailActive)
            return 0.0;

        auto output = toneFilter.process (0.0);
        output = dcBlocker.process (output);

        if (! toneFilter.hasResidual() && ! dcBlocker.hasResidual())
        {
            toneFilter.reset();
            dcBlocker.reset();
            filterTailActive = false;
            return 0.0;
        }

        return output;
    }

    double addRetriggerTail (double output) noexcept
    {
        if (stealFadeSamplesRemaining == 0)
            return output;

        const auto fade = static_cast<double> (stealFadeSamplesRemaining)
                        / static_cast<double> (stealFadeSamplesTotal);
        output += stolenTail * fade;
        --stealFadeSamplesRemaining;
        if (stealFadeSamplesRemaining == 0)
            stolenTail = 0.0;
        return output;
    }

    void updateEnvelopeParameters (const VoiceParameters& parameters) noexcept
    {
        EnvelopeParameters envelope;
        envelope.attackMs = parameters.attackMs;
        envelope.holdMs = parameters.holdMs;
        envelope.decayMs = parameters.decayMs;
        envelope.sustain = parameters.sustain;
        envelope.releaseMs = parameters.releaseMs;
        envelope.curve = parameters.ampCurve;
        envelope.oneShot = parameters.oneShot;
        amplitudeEnvelope.updateParameters (envelope);
    }

    VoiceParameters cachedParameters;
    AmpEnvelope amplitudeEnvelope;
    OnePoleLowPass toneFilter;
    OnePoleLowPass clickSmoother;
    DcBlocker dcBlocker;
    XorShift32 random;
    double sampleRate { 44100.0 };
    double phase { 0.0 };
    double clickPhase { 0.0 };
    double currentPitch { -1.0 };
    double targetPitch { -1.0 };
    double glideStartPitch { -1.0 };
    double velocityGain { 0.0 };
    double pitchBend { 0.0 };
    double lastOutput { 0.0 };
    double stolenTail { 0.0 };
    bool filterTailActive { false };
    std::uint64_t pitchEnvelopeSamples { 0 };
    std::uint64_t clickSamples { 0 };
    std::uint64_t glideSamplesElapsed { 0 };
    std::uint64_t glideSamplesTotal { 0 };
    std::uint64_t stealFadeSamplesRemaining { 0 };
    std::uint64_t stealFadeSamplesTotal { 0 };
    std::uint64_t startCounter { 0 };
    int note { -1 };
};
} // namespace glo::dsp
