#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace glo::dsp
{
constexpr double pi = 3.1415926535897932384626433832795;
constexpr double twoPi = pi * 2.0;

template <typename T>
constexpr T clamp (T value, T low, T high) noexcept
{
    return std::min (high, std::max (low, value));
}

inline double midiNoteToHz (double note) noexcept
{
    return 440.0 * std::exp2 ((note - 69.0) / 12.0);
}

inline double decibelsToGain (double decibels) noexcept
{
    return std::pow (10.0, decibels / 20.0);
}

inline double gainToDecibels (double gain, double floorDb = -120.0) noexcept
{
    if (gain <= 0.0)
        return floorDb;

    return std::max (floorDb, 20.0 * std::log10 (gain));
}

inline double smoothstep (double x) noexcept
{
    x = clamp (x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

inline double wrapPhase (double phase) noexcept
{
    phase -= std::floor (phase);
    return phase;
}

inline double fastTanh (double x) noexcept
{
    // Stable, bounded approximation with enough accuracy for the voice-level
    // harmonic shaper. The final colour stage uses JUCE's 4x oversampling.
    const auto x2 = x * x;
    const auto numerator = x * (27.0 + x2);
    const auto denominator = 27.0 + 9.0 * x2;
    return clamp (numerator / denominator, -1.0, 1.0);
}

class XorShift32
{
public:
    explicit XorShift32 (std::uint32_t initialSeed = 0x8086a11u) noexcept
        : state (initialSeed == 0 ? 0x8086a11u : initialSeed)
    {
    }

    void seed (std::uint32_t newSeed) noexcept
    {
        state = newSeed == 0 ? 0x8086a11u : newSeed;
    }

    std::uint32_t nextUInt() noexcept
    {
        auto value = state;
        value ^= value << 13;
        value ^= value >> 17;
        value ^= value << 5;
        state = value;
        return value;
    }

    double nextBipolar() noexcept
    {
        constexpr auto scale = 1.0 / static_cast<double> (std::numeric_limits<std::uint32_t>::max());
        return static_cast<double> (nextUInt()) * scale * 2.0 - 1.0;
    }

private:
    std::uint32_t state;
};
} // namespace glo::dsp

