#include "PluginProcessor.h"

#include <JuceHeader.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// Renders ARBITRARY parameter sets through the real processor and returns the
// same audio features the preset probe reports.
//
// This exists so candidate presets can be auditioned without regenerating the
// factory JSON and relinking the plugin. A full regenerate-rebuild-probe cycle
// costs minutes, which makes a search over candidates impractical; this makes
// each candidate a render. Features and thresholds are kept identical to
// PresetProbe.cpp so the two are directly comparable.
//
// Usage: 808GloPresetEval <candidates.json> <out.json>
//   candidates.json: [{"id": "...", "parameters": {"decay": 1200, ...}}, ...]

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 512;
constexpr int probeNote = 29;
constexpr int pitchProbeNote = 53;
constexpr double renderSeconds = 4.0;
constexpr double gateSeconds = 2.0;

double toDb (double linear) { return 20.0 * std::log10 (std::max (linear, 1.0e-9)); }

double goertzel (const std::vector<float>& x, size_t from, size_t to, double frequency)
{
    if (to <= from || to > x.size()) return 0.0;
    const auto n = (double) (to - from);
    const auto k = 2.0 * std::cos (2.0 * juce::MathConstants<double>::pi * frequency / sampleRate);
    double s1 = 0.0, s2 = 0.0;
    for (size_t i = from; i < to; ++i)
    {
        const auto s0 = (double) x[i] + k * s1 - s2;
        s2 = s1; s1 = s0;
    }
    return std::sqrt (std::max (0.0, s1 * s1 + s2 * s2 - k * s1 * s2)) / (n * 0.5);
}

std::vector<float> lowpassed (const std::vector<float>& x, double cutoff)
{
    auto out = x;
    const auto c = (float) std::exp (-2.0 * juce::MathConstants<double>::pi * cutoff / sampleRate);
    for (int pass = 0; pass < 3; ++pass)
    {
        float state = 0.0f;
        for (auto& s : out) { state = s * (1.0f - c) + state * c; s = state; }
    }
    return out;
}

std::vector<std::pair<double, double>> instantaneousPitch (const std::vector<float>& x)
{
    std::vector<double> crossings;
    for (size_t i = 1; i < x.size(); ++i)
        if (x[i - 1] <= 0.0f && x[i] > 0.0f)
        {
            const auto d = (double) x[i] - (double) x[i - 1];
            const auto f = d > 1.0e-12 ? -(double) x[i - 1] / d : 0.0;
            crossings.push_back (((double) (i - 1) + f) / sampleRate);
        }
    std::vector<std::pair<double, double>> pitch;
    for (size_t i = 1; i < crossings.size(); ++i)
    {
        const auto period = crossings[i] - crossings[i - 1];
        if (period > 1.0e-6) pitch.emplace_back (crossings[i - 1], 1.0 / period);
    }
    return pitch;
}

double envelopeAt (const std::vector<float>& x, double seconds, double windowMs = 20.0)
{
    const auto centre = (size_t) (seconds * sampleRate);
    const auto half = (size_t) (windowMs * 0.001 * sampleRate * 0.5);
    if (centre >= x.size()) return 0.0;
    const auto from = centre > half ? centre - half : 0u;
    const auto to = std::min (x.size(), centre + half);
    double sum = 0.0;
    for (size_t i = from; i < to; ++i) sum += (double) x[i] * (double) x[i];
    return std::sqrt (sum / std::max<size_t> (1, to - from));
}

void applyParameters (EightOhEightGloProAudioProcessor& processor, const juce::var& parameters)
{
    auto& state = processor.getParameterState();
    if (auto* object = parameters.getDynamicObject())
        for (const auto& property : object->getProperties())
            if (auto* parameter = state.getParameter (property.name.toString()))
                parameter->setValueNotifyingHost (
                    parameter->convertTo0to1 ((float) (double) property.value));
}

std::vector<float> render (EightOhEightGloProAudioProcessor& processor, int note)
{
    processor.prepareToPlay (sampleRate, blockSize);
    const auto totalSamples = (int) (renderSeconds * sampleRate);
    const auto gateSample = (int) (gateSeconds * sampleRate);

    std::vector<float> mono;
    mono.reserve ((size_t) totalSamples);
    juce::AudioBuffer<float> buffer (2, blockSize);
    for (int position = 0; position < totalSamples; position += blockSize)
    {
        juce::MidiBuffer midi;
        if (position == 0)
            midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        if (position <= gateSample && gateSample < position + blockSize)
            midi.addEvent (juce::MidiMessage::noteOff (1, note), gateSample - position);
        buffer.clear();
        processor.processBlock (buffer, midi);
        for (int i = 0; i < blockSize && position + i < totalSamples; ++i)
            mono.push_back (0.5f * (buffer.getSample (0, i) + buffer.getSample (1, i)));
    }
    return mono;
}
} // namespace

int main (int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: 808GloPresetEval <candidates.json> <out.json>\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI initialiseJuce;

    const auto inputFile = juce::File::getCurrentWorkingDirectory()
                               .getChildFile (juce::String::fromUTF8 (argv[1]));
    auto parsed = juce::JSON::parse (inputFile.loadFileAsString());
    auto* candidates = parsed.getArray();
    if (candidates == nullptr) { std::cerr << "candidates must be a JSON array\n"; return 2; }

    const auto fundamental = 440.0 * std::pow (2.0, (probeNote - 69) / 12.0);
    const auto pitchTarget = 440.0 * std::pow (2.0, (pitchProbeNote - 69) / 12.0);

    juce::String json = "[\n";
    bool first = true;

    for (const auto& candidate : *candidates)
    {
        // A fresh processor per candidate: parameter smoothing and filter state
        // from the previous render would otherwise leak into this one and make
        // a candidate's measurement depend on the order it was evaluated in.
        EightOhEightGloProAudioProcessor processor;
        applyParameters (processor, candidate.getProperty ("parameters", {}));

        const auto x = render (processor, probeNote);

        double peak = 0.0, sumSquares = 0.0;
        size_t peakIndex = 0;
        for (size_t i = 0; i < x.size(); ++i)
        {
            const auto magnitude = std::abs ((double) x[i]);
            if (magnitude > peak) { peak = magnitude; peakIndex = i; }
            sumSquares += (double) x[i] * (double) x[i];
        }
        const auto rms = std::sqrt (sumSquares / std::max<size_t> (1, x.size()));
        const auto attackMs = 1000.0 * (double) peakIndex / sampleRate;
        const auto peakEnvelope = envelopeAt (x, (double) peakIndex / sampleRate);

        double t20 = -1.0, t40 = -1.0;
        for (double t = (double) peakIndex / sampleRate; t < renderSeconds; t += 0.005)
        {
            const auto drop = toDb (envelopeAt (x, t)) - toDb (peakEnvelope);
            if (t20 < 0.0 && drop <= -20.0) t20 = t - (double) peakIndex / sampleRate;
            if (t40 < 0.0 && drop <= -40.0) { t40 = t - (double) peakIndex / sampleRate; break; }
        }

        constexpr double minimumWindow = 4.0 / 43.653;
        double audibleEnd = 0.0;
        for (double t = 0.005; t < renderSeconds; t += 0.005)
            if (envelopeAt (x, t) > peakEnvelope * 0.02) audibleEnd = t;
        double harmonicStart = 0.030;
        double harmonicEnd = std::min (harmonicStart + 0.5, std::max (audibleEnd, 0.0));
        if (harmonicEnd - harmonicStart < minimumWindow)
        {
            harmonicEnd = std::max (audibleEnd, minimumWindow + 0.002);
            harmonicStart = std::max (0.002, harmonicEnd - minimumWindow);
        }
        const auto from = (size_t) (harmonicStart * sampleRate);
        const auto to = std::min (x.size(), (size_t) (harmonicEnd * sampleRate));
        const auto h1 = goertzel (x, from, to, fundamental);
        const auto h2 = goertzel (x, from, to, fundamental * 2.0);
        const auto h3 = goertzel (x, from, to, fundamental * 3.0);
        const auto h5 = goertzel (x, from, to, fundamental * 5.0);
        const auto h7 = goertzel (x, from, to, fundamental * 7.0);
        const auto reference = std::max (h1 + h2 + h3 + h5 + h7, 1.0e-9);

        const auto clickWindow = (size_t) (0.030 * sampleRate);
        double attackPeak = 0.0;
        for (size_t i = 0; i < std::min (clickWindow, x.size()); ++i)
            attackPeak = std::max (attackPeak, std::abs ((double) x[i]));
        const auto punchWindow = std::min (x.size(), (size_t) (0.150 * sampleRate));
        double punchSum = 0.0;
        for (size_t i = 0; i < punchWindow; ++i) punchSum += (double) x[i] * (double) x[i];
        const auto punchRms = std::sqrt (punchSum / std::max<size_t> (1, punchWindow));
        const auto punchDb = toDb (attackPeak) - toDb (std::max (punchRms, 1.0e-9));

        double attackHf = 0.0;
        for (size_t i = 1; i < std::min (clickWindow, x.size()); ++i)
        {
            const auto d = (double) x[i] - (double) x[i - 1];
            attackHf += d * d;
        }
        attackHf = std::sqrt (attackHf / std::max<size_t> (1, clickWindow));

        EightOhEightGloProAudioProcessor pitchProcessor;
        applyParameters (pitchProcessor, candidate.getProperty ("parameters", {}));
        const auto pitchRaw = render (pitchProcessor, pitchProbeNote);
        const auto pitchSignal = lowpassed (pitchRaw, pitchTarget * 1.5);
        const auto pitch = instantaneousPitch (pitchSignal);
        double pitchPeak = 0.0;
        for (auto s : pitchSignal) pitchPeak = std::max (pitchPeak, std::abs ((double) s));
        const auto floorLevel = pitchPeak * 0.01;

        std::vector<double> early;
        for (const auto& [t, f] : pitch)
            if (t > 0.005 && t < 0.20 && envelopeAt (pitchSignal, t, 30.0) > floorLevel)
                early.push_back (f);
        double dropSemitones = 0.0, settleMs = 0.0;
        if (! early.empty())
        {
            auto sorted = early;
            std::sort (sorted.begin(), sorted.end());
            dropSemitones = 12.0 * std::log2 (
                std::max (sorted[(size_t) ((double) (sorted.size() - 1) * 0.9)], 1.0) / pitchTarget);
        }
        for (const auto& [t, f] : pitch)
        {
            if (t < 0.005) continue;
            if (envelopeAt (pitchSignal, t, 30.0) <= floorLevel) break;
            if (std::abs (12.0 * std::log2 (std::max (f, 1.0) / pitchTarget)) < 0.5)
            { settleMs = t * 1000.0; break; }
        }

        json << (first ? "  {" : ",\n  {");
        first = false;
        json << "\"id\":\"" << candidate.getProperty ("id", "").toString().replace ("\"", "'") << "\""
             << ",\"peakDb\":" << juce::String (toDb (peak), 3)
             << ",\"rmsDb\":" << juce::String (toDb (rms), 3)
             << ",\"crestDb\":" << juce::String (toDb (peak) - toDb (rms), 3)
             << ",\"attackMs\":" << juce::String (attackMs, 3)
             << ",\"t20Ms\":" << juce::String (t20 * 1000.0, 2)
             << ",\"t40Ms\":" << juce::String (t40 * 1000.0, 2)
             << ",\"env100\":" << juce::String (toDb (envelopeAt (x, 0.10)) - toDb (peakEnvelope), 2)
             << ",\"env250\":" << juce::String (toDb (envelopeAt (x, 0.25)) - toDb (peakEnvelope), 2)
             << ",\"env500\":" << juce::String (toDb (envelopeAt (x, 0.50)) - toDb (peakEnvelope), 2)
             << ",\"env1000\":" << juce::String (toDb (envelopeAt (x, 1.00)) - toDb (peakEnvelope), 2)
             << ",\"env2000\":" << juce::String (toDb (envelopeAt (x, 2.00)) - toDb (peakEnvelope), 2)
             << ",\"h2\":" << juce::String (h2 / reference, 4)
             << ",\"h3\":" << juce::String (h3 / reference, 4)
             << ",\"h5\":" << juce::String (h5 / reference, 4)
             << ",\"h7\":" << juce::String (h7 / reference, 4)
             << ",\"punchDb\":" << juce::String (punchDb, 2)
             << ",\"attackHfDb\":" << juce::String (toDb (attackHf) - toDb (peak), 2)
             << ",\"dropSemitones\":" << juce::String (dropSemitones, 2)
             << ",\"settleMs\":" << juce::String (settleMs, 1)
             << "}";
        std::cerr << "." << std::flush;
    }

    json << "\n]\n";
    juce::File out (juce::File::getCurrentWorkingDirectory()
                        .getChildFile (juce::String::fromUTF8 (argv[2])));
    out.getParentDirectory().createDirectory();
    out.replaceWithText (json);
    std::cerr << "\n";
    std::cout << out.getFullPathName() << "\n";
    return 0;
}
