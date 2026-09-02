#pragma once

#include "Math.h"

namespace glo::dsp
{
class OnePoleLowPass
{
public:
    void prepare (double newSampleRate) noexcept
    {
        sampleRate = std::max (1.0, newSampleRate);
        reset();
    }

    void reset() noexcept { state = 0.0; }

    void setCutoff (double hertz) noexcept
    {
        const auto safeCutoff = clamp (hertz, 5.0, sampleRate * 0.475);
        coefficient = 1.0 - std::exp (-twoPi * safeCutoff / sampleRate);
    }

    double process (double input) noexcept
    {
        state += coefficient * (input - state);
        if (std::abs (state) < 1.0e-20)
            state = 0.0;
        return state;
    }

    [[nodiscard]] bool hasResidual (double threshold = 1.0e-9) const noexcept
    {
        return std::abs (state) >= threshold;
    }

private:
    double sampleRate { 44100.0 };
    double coefficient { 1.0 };
    double state { 0.0 };
};

class DcBlocker
{
public:
    void prepare (double newSampleRate, double cutoffHz = 12.0) noexcept
    {
        const auto safeRate = std::max (1.0, newSampleRate);
        coefficient = std::exp (-twoPi * clamp (cutoffHz, 1.0, 40.0) / safeRate);
        reset();
    }

    void reset() noexcept
    {
        previousInput = 0.0;
        previousOutput = 0.0;
    }

    double process (double input) noexcept
    {
        const auto output = input - previousInput + coefficient * previousOutput;
        previousInput = input;
        previousOutput = output;
        if (std::abs (previousOutput) < 1.0e-20)
            previousOutput = 0.0;
        return previousOutput;
    }

    [[nodiscard]] bool hasResidual (double threshold = 1.0e-9) const noexcept
    {
        return std::abs (previousInput) >= threshold
            || std::abs (previousOutput) >= threshold;
    }

private:
    double coefficient { 0.998 };
    double previousInput { 0.0 };
    double previousOutput { 0.0 };
};
} // namespace glo::dsp
