#include "DSP/SynthEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
constexpr int sampleRate = 48000;
constexpr int sectionSamples = sampleRate * 3;
constexpr int sectionCount = 4;
constexpr int totalSamples = sectionSamples * sectionCount;

void writeLittleEndian16 (std::ofstream& stream, std::uint16_t value)
{
    const std::array bytes {
        static_cast<char> (value & 0xffu),
        static_cast<char> ((value >> 8u) & 0xffu)
    };
    stream.write (bytes.data(), static_cast<std::streamsize> (bytes.size()));
}

void writeLittleEndian32 (std::ofstream& stream, std::uint32_t value)
{
    const std::array bytes {
        static_cast<char> (value & 0xffu),
        static_cast<char> ((value >> 8u) & 0xffu),
        static_cast<char> ((value >> 16u) & 0xffu),
        static_cast<char> ((value >> 24u) & 0xffu)
    };
    stream.write (bytes.data(), static_cast<std::streamsize> (bytes.size()));
}

bool writeWav (const std::string& destination,
               const std::vector<float>& left,
               const std::vector<float>& right)
{
    if (left.size() != right.size())
        return false;

    std::ofstream stream (destination, std::ios::binary | std::ios::trunc);
    if (! stream)
        return false;

    constexpr std::uint16_t channels = 2;
    constexpr std::uint16_t bits = 16;
    constexpr std::uint16_t blockAlign = channels * (bits / 8u);
    constexpr std::uint32_t byteRate = sampleRate * blockAlign;
    const auto dataBytes = static_cast<std::uint32_t> (left.size() * blockAlign);

    stream.write ("RIFF", 4);
    writeLittleEndian32 (stream, 36u + dataBytes);
    stream.write ("WAVEfmt ", 8);
    writeLittleEndian32 (stream, 16u);
    writeLittleEndian16 (stream, 1u);
    writeLittleEndian16 (stream, channels);
    writeLittleEndian32 (stream, sampleRate);
    writeLittleEndian32 (stream, byteRate);
    writeLittleEndian16 (stream, blockAlign);
    writeLittleEndian16 (stream, bits);
    stream.write ("data", 4);
    writeLittleEndian32 (stream, dataBytes);

    for (std::size_t i = 0; i < left.size(); ++i)
    {
        const auto l = static_cast<std::int16_t> (std::lrint (
            std::clamp (left[i], -1.0f, 1.0f) * 32767.0f));
        const auto r = static_cast<std::int16_t> (std::lrint (
            std::clamp (right[i], -1.0f, 1.0f) * 32767.0f));
        writeLittleEndian16 (stream, static_cast<std::uint16_t> (l));
        writeLittleEndian16 (stream, static_cast<std::uint16_t> (r));
    }
    return static_cast<bool> (stream);
}

glo::dsp::VoiceParameters sectionParameters (int section)
{
    glo::dsp::VoiceParameters p;
    p.oneShot = true;
    p.legato = true;
    p.velocitySensitivity = 0.45;
    p.toneKeytrack = 0.08;

    switch (section)
    {
        case 0: // clean sub
            p.bodyShape = 0.02;
            p.harmonics = 0.015;
            p.pitchDropSemitones = 18.0;
            p.pitchDecayMs = 42.0;
            p.decayMs = 1350.0;
            p.clickAmount = 0.04;
            p.punch = 0.20;
            p.toneHz = 950.0;
            break;

        case 1: // modern trap
            p.bodyShape = 0.22;
            p.harmonics = 0.28;
            p.harmonicBalance = -0.2;
            p.pitchDropSemitones = 30.0;
            p.pitchDecayMs = 29.0;
            p.pitchCurve = 1.45;
            p.decayMs = 820.0;
            p.clickAmount = 0.30;
            p.clickToneHz = 5200.0;
            p.clickDecayMs = 2.1;
            p.punch = 0.62;
            p.toneHz = 2900.0;
            break;

        case 2: // driven speaker-translating 808
            p.bodyShape = 0.48;
            p.harmonics = 0.66;
            p.harmonicBalance = -0.55;
            p.pitchDropSemitones = 24.0;
            p.pitchDecayMs = 36.0;
            p.decayMs = 1050.0;
            p.clickAmount = 0.22;
            p.clickToneHz = 7600.0;
            p.punch = 0.48;
            p.toneHz = 6200.0;
            break;

        default: // drill glide
            p.oneShot = false;
            p.sustain = 0.72;
            p.releaseMs = 220.0;
            p.glideMs = 260.0;
            p.bodyShape = 0.16;
            p.harmonics = 0.18;
            p.pitchDropSemitones = 16.0;
            p.pitchDecayMs = 58.0;
            p.decayMs = 1600.0;
            p.clickAmount = 0.12;
            p.punch = 0.28;
            p.toneHz = 1400.0;
            break;
    }
    return p;
}
} // namespace

int main (int argc, char** argv)
{
    const std::string destination = argc > 1 ? argv[1] : "808GloPro_Core_AudioPreview.wav";
    std::vector<float> left (totalSamples, 0.0f);
    std::vector<float> right (totalSamples, 0.0f);
    float* channels[] { left.data(), right.data() };
    glo::dsp::SynthEngine engine;
    engine.prepare (sampleRate);

    constexpr std::array pattern { 36, 36, 31, 34, 29, 36, 39, 34 };
    constexpr int step = sectionSamples / static_cast<int> (pattern.size());

    for (int section = 0; section < sectionCount; ++section)
    {
        engine.allNotesOff (false);
        auto parameters = sectionParameters (section);
        const auto sectionStart = section * sectionSamples;
        auto nextStep = 0;

        for (int local = 0; local < sectionSamples; ++local)
        {
            if (local == nextStep && local / step < static_cast<int> (pattern.size()))
            {
                const auto patternIndex = local / step;
                if (section == 3 && patternIndex > 0)
                    engine.noteOn (pattern[static_cast<std::size_t> (patternIndex)], 0.92, parameters);
                else
                {
                    engine.allNotesOff (section == 3);
                    engine.noteOn (pattern[static_cast<std::size_t> (patternIndex)],
                                   patternIndex % 3 == 0 ? 1.0 : 0.82,
                                   parameters);
                }
                nextStep += step;
            }

            engine.render (channels, 2, sectionStart + local, 1, parameters);

            if (section == 2)
            {
                const auto index = static_cast<std::size_t> (sectionStart + local);
                left[index] = std::tanh (left[index] * 3.4f) * 0.68f;
                right[index] = left[index];
            }
        }
    }

    glo::dsp::DcBlocker previewDcLeft;
    glo::dsp::DcBlocker previewDcRight;
    previewDcLeft.prepare (sampleRate, 5.0);
    previewDcRight.prepare (sampleRate, 5.0);
    auto peak = 0.0f;
    for (std::size_t i = 0; i < left.size(); ++i)
    {
        left[i] = static_cast<float> (previewDcLeft.process (left[i]));
        right[i] = static_cast<float> (previewDcRight.process (right[i]));
        peak = std::max (peak, std::max (std::abs (left[i]), std::abs (right[i])));
    }
    const auto target = std::pow (10.0f, -1.0f / 20.0f);
    const auto gain = peak > 0.0f ? target / peak : 1.0f;
    for (std::size_t i = 0; i < left.size(); ++i)
    {
        left[i] *= gain;
        right[i] *= gain;
    }

    if (! writeWav (destination, left, right))
    {
        std::cerr << "Failed to write " << destination << '\n';
        return 1;
    }

    std::cout << "Rendered 12-second 808Glo core preview to " << destination
              << " (peak normalized to -1 dBFS)\n";
    return 0;
}
