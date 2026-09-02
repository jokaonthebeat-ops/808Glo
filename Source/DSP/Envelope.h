#pragma once

#include "Math.h"

namespace glo::dsp
{
struct EnvelopeParameters
{
    double attackMs { 2.0 };
    double holdMs { 0.0 };
    double decayMs { 900.0 };
    double sustain { 0.0 };
    double releaseMs { 80.0 };
    double curve { 1.65 };
    bool oneShot { true };
};

class AmpEnvelope
{
public:
    enum class Stage
    {
        idle,
        attack,
        hold,
        decay,
        sustain,
        release
    };

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = std::max (1.0, newSampleRate);
        reset();
    }

    void reset() noexcept
    {
        stage = Stage::idle;
        current = 0.0;
        attackStart = 0.0;
        decayStart = 1.0;
        samplesInStage = 0;
        stageLengthSamples = 0;
        releaseStart = 0.0;
        sustainRampStart = 0.0;
        sustainRampSamples = 0;
        sustainRampSamplesElapsed = 0;
    }

    void noteOn (const EnvelopeParameters& newParameters, bool retrigger) noexcept
    {
        if (! retrigger && stage != Stage::idle)
        {
            updateParameters (newParameters);
            return;
        }

        parameters = sanitise (newParameters);
        current = 0.0;
        attackStart = 0.0;
        stage = Stage::attack;
        samplesInStage = 0;
        stageLengthSamples = millisecondsToSamples (parameters.attackMs);
    }

    void updateParameters (const EnvelopeParameters& newParameters) noexcept
    {
        const auto updated = sanitise (newParameters);

        switch (stage)
        {
            case Stage::attack:
                if (hasChanged (updated.attackMs, parameters.attackMs)
                    || hasChanged (updated.curve, parameters.curve))
                {
                    attackStart = current;
                    stageLengthSamples = remapRemainingStageLength (updated.attackMs);
                    samplesInStage = 0;
                }
                break;

            case Stage::hold:
                if (hasChanged (updated.holdMs, parameters.holdMs))
                {
                    stageLengthSamples = remapRemainingStageLength (updated.holdMs);
                    samplesInStage = 0;
                }
                break;

            case Stage::decay:
                if (hasChanged (updated.decayMs, parameters.decayMs)
                    || hasChanged (updated.sustain, parameters.sustain)
                    || hasChanged (updated.curve, parameters.curve))
                {
                    decayStart = current;
                    stageLengthSamples = remapRemainingStageLength (updated.decayMs);
                    samplesInStage = 0;
                }
                break;

            case Stage::sustain:
                if (hasChanged (updated.sustain, parameters.sustain))
                {
                    sustainRampStart = current;
                    sustainRampSamples = std::max (1, millisecondsToSamples (5.0));
                    sustainRampSamplesElapsed = 0;
                }
                break;

            case Stage::release:
                if (hasChanged (updated.releaseMs, parameters.releaseMs)
                    || hasChanged (updated.curve, parameters.curve))
                {
                    releaseStart = current;
                    stageLengthSamples = remapRemainingStageLength (updated.releaseMs);
                    samplesInStage = 0;
                }
                break;

            case Stage::idle:
                break;
        }

        parameters = updated;
    }

    void noteOff() noexcept
    {
        if (stage == Stage::idle || (parameters.oneShot && stage != Stage::sustain))
            return;

        beginRelease();
    }

    double process() noexcept
    {
        switch (stage)
        {
            case Stage::idle:
                return 0.0;

            case Stage::attack:
            {
                const auto length = stageLengthSamples;
                if (length <= 1)
                {
                    current = 1.0;
                    enterPostAttackStage();
                    return current;
                }

                const auto progress = clamp (static_cast<double> (samplesInStage++) / static_cast<double> (length), 0.0, 1.0);
                current = attackStart + (1.0 - attackStart) * std::pow (progress, 1.0 / parameters.curve);
                if (samplesInStage >= length)
                {
                    current = 1.0;
                    enterPostAttackStage();
                }
                break;
            }

            case Stage::hold:
                current = 1.0;
                if (++samplesInStage >= stageLengthSamples)
                    enterDecay();
                break;

            case Stage::decay:
            {
                const auto length = stageLengthSamples;
                if (length <= 1)
                {
                    finishDecay();
                    break;
                }

                const auto progress = clamp (static_cast<double> (samplesInStage++) / static_cast<double> (length), 0.0, 1.0);
                const auto shaped = std::pow (progress, parameters.curve);
                current = decayStart + (parameters.sustain - decayStart) * shaped;
                if (samplesInStage >= length)
                    finishDecay();
                break;
            }

            case Stage::sustain:
                if (sustainRampSamplesElapsed < sustainRampSamples)
                {
                    const auto progress = clamp (
                        static_cast<double> (sustainRampSamplesElapsed++)
                            / static_cast<double> (sustainRampSamples),
                        0.0, 1.0);
                    current = sustainRampStart
                            + (parameters.sustain - sustainRampStart) * smoothstep (progress);

                    if (sustainRampSamplesElapsed >= sustainRampSamples)
                        current = parameters.sustain;
                }
                else
                {
                    current = parameters.sustain;
                }
                break;

            case Stage::release:
            {
                const auto length = stageLengthSamples;
                if (length <= 1)
                {
                    reset();
                    return 0.0;
                }

                const auto progress = clamp (static_cast<double> (samplesInStage++) / static_cast<double> (length), 0.0, 1.0);
                current = releaseStart * std::pow (1.0 - progress, parameters.curve);
                if (samplesInStage >= length || current < 1.0e-7)
                {
                    reset();
                    return 0.0;
                }
                break;
            }
        }

        return current;
    }

    [[nodiscard]] bool isActive() const noexcept { return stage != Stage::idle; }
    [[nodiscard]] Stage getStage() const noexcept { return stage; }
    [[nodiscard]] double getCurrentValue() const noexcept { return current; }

    void forceRelease() noexcept { beginRelease(); }

private:
    static bool hasChanged (double first, double second) noexcept
    {
        return std::abs (first - second) > 1.0e-12;
    }

    EnvelopeParameters sanitise (EnvelopeParameters input) const noexcept
    {
        input.attackMs = clamp (input.attackMs, 0.0, 10000.0);
        input.holdMs = clamp (input.holdMs, 0.0, 10000.0);
        input.decayMs = clamp (input.decayMs, 0.0, 30000.0);
        input.sustain = clamp (input.sustain, 0.0, 1.0);
        input.releaseMs = clamp (input.releaseMs, 0.0, 10000.0);
        input.curve = clamp (input.curve, 0.2, 5.0);
        return input;
    }

    int millisecondsToSamples (double milliseconds) const noexcept
    {
        return std::max (0, static_cast<int> (std::round (milliseconds * 0.001 * sampleRate)));
    }

    int remapRemainingStageLength (double newMilliseconds) const noexcept
    {
        const auto elapsed = stageLengthSamples > 0
            ? clamp (static_cast<double> (samplesInStage)
                         / static_cast<double> (stageLengthSamples),
                     0.0, 1.0)
            : 0.0;
        const auto remaining = 1.0 - elapsed;
        return std::max (0, static_cast<int> (std::round (
            static_cast<double> (millisecondsToSamples (newMilliseconds)) * remaining)));
    }

    void enterPostAttackStage() noexcept
    {
        samplesInStage = 0;
        if (millisecondsToSamples (parameters.holdMs) > 0)
        {
            stage = Stage::hold;
            stageLengthSamples = millisecondsToSamples (parameters.holdMs);
        }
        else
            enterDecay();
    }

    void enterDecay() noexcept
    {
        stage = Stage::decay;
        decayStart = current;
        samplesInStage = 0;
        stageLengthSamples = millisecondsToSamples (parameters.decayMs);
    }

    void finishDecay() noexcept
    {
        current = parameters.sustain;
        samplesInStage = 0;
        stageLengthSamples = 0;

        if (parameters.oneShot || parameters.sustain <= 0.0)
            beginRelease();
        else
        {
            stage = Stage::sustain;
            sustainRampStart = current;
            sustainRampSamples = 0;
            sustainRampSamplesElapsed = 0;
        }
    }

    void beginRelease() noexcept
    {
        if (stage == Stage::idle || stage == Stage::release)
            return;

        releaseStart = current;
        stage = Stage::release;
        samplesInStage = 0;
        stageLengthSamples = millisecondsToSamples (parameters.releaseMs);
    }

    EnvelopeParameters parameters;
    Stage stage { Stage::idle };
    double sampleRate { 44100.0 };
    double current { 0.0 };
    double attackStart { 0.0 };
    double decayStart { 1.0 };
    double releaseStart { 0.0 };
    double sustainRampStart { 0.0 };
    int samplesInStage { 0 };
    int stageLengthSamples { 0 };
    int sustainRampSamples { 0 };
    int sustainRampSamplesElapsed { 0 };
};
} // namespace glo::dsp
