#include "PluginProcessor.h"

#include <JuceHeader.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// Renders every factory preset through the REAL processor (colour stage,
// compressor, clipper, ceiling included - what the player actually hears) and
// extracts audio-domain features. Distinctness is then measured on these
// features, never on parameter values: a parameter-space metric rewards
// unrelated settings and scores an incoherent bank highest.
//
// Every preset is rendered at ONE note, because per-preset registers hide the
// collisions a player hears instantly on a single keyboard. A second render an
// octave up exists only to track the pitch drop, which needs more cycles than
// 43 Hz gives inside a 30 ms envelope.
//
// Usage: 808GloPresetProbe <output.json>

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int blockSize = 512;
constexpr int probeNote = 29;      // F1, 43.65 Hz - the register 808s live in
constexpr int pitchProbeNote = 53; // F3, 174.6 Hz - enough cycles to track the drop
constexpr double renderSeconds = 4.0;
constexpr double gateSeconds = 2.0;

struct Render
{
    std::vector<float> mono;
    bool ceilingHit = false;
};

Render renderPreset (EightOhEightGloProAudioProcessor& processor, int program, int note)
{
    processor.setCurrentProgram (program);
    processor.prepareToPlay (sampleRate, blockSize);

    const auto totalSamples = (int) (renderSeconds * sampleRate);
    const auto gateSample = (int) (gateSeconds * sampleRate);

    Render out;
    out.mono.reserve ((size_t) totalSamples);

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
            out.mono.push_back (0.5f * (buffer.getSample (0, i) + buffer.getSample (1, i)));
    }

    out.ceilingHit = processor.consumeCeilingHit();
    return out;
}

double toDb (double linear)
{
    return 20.0 * std::log10 (std::max (linear, 1.0e-9));
}

// Goertzel magnitude at an exact frequency. A first-difference "brightness"
// reading only sees the fundamental's slope and is blind to where distortion
// actually puts its energy, so harmonic content is measured term by term.
double goertzel (const std::vector<float>& x, size_t from, size_t to, double frequency)
{
    if (to <= from || to > x.size())
        return 0.0;

    const auto n = (double) (to - from);
    const auto k = 2.0 * std::cos (2.0 * juce::MathConstants<double>::pi * frequency / sampleRate);
    double s1 = 0.0, s2 = 0.0;
    for (size_t i = from; i < to; ++i)
    {
        const auto s0 = (double) x[i] + k * s1 - s2;
        s2 = s1;
        s1 = s0;
    }
    return std::sqrt (std::max (0.0, s1 * s1 + s2 * s2 - k * s1 * s2)) / (n * 0.5);
}

// Three cascaded one-pole lowpasses. Without this, a heavily driven preset's
// harmonics cross zero far more often than its fundamental does and the drop
// reads as +75 semitones - impossible, since the parameter maxes at 40.
std::vector<float> lowpassed (const std::vector<float>& x, double cutoff)
{
    auto out = x;
    const auto coefficient = (float) std::exp (-2.0 * juce::MathConstants<double>::pi * cutoff / sampleRate);
    for (int pass = 0; pass < 3; ++pass)
    {
        float state = 0.0f;
        for (auto& sample : out)
        {
            state = sample * (1.0f - coefficient) + state * coefficient;
            sample = state;
        }
    }
    return out;
}

// Interpolated positive-going zero crossings give per-cycle frequency, which
// is what the pitch drop actually is.
std::vector<std::pair<double, double>> instantaneousPitch (const std::vector<float>& x)
{
    std::vector<double> crossings;
    for (size_t i = 1; i < x.size(); ++i)
    {
        if (x[i - 1] <= 0.0f && x[i] > 0.0f)
        {
            const auto denominator = (double) x[i] - (double) x[i - 1];
            const auto fraction = denominator > 1.0e-12 ? -(double) x[i - 1] / denominator : 0.0;
            crossings.push_back (((double) (i - 1) + fraction) / sampleRate);
        }
    }

    std::vector<std::pair<double, double>> pitch;
    for (size_t i = 1; i < crossings.size(); ++i)
    {
        const auto period = crossings[i] - crossings[i - 1];
        if (period > 1.0e-6)
            pitch.emplace_back (crossings[i - 1], 1.0 / period);
    }
    return pitch;
}

double envelopeAt (const std::vector<float>& x, double seconds, double windowMs = 20.0)
{
    const auto centre = (size_t) (seconds * sampleRate);
    const auto half = (size_t) (windowMs * 0.001 * sampleRate * 0.5);
    if (centre >= x.size())
        return 0.0;
    const auto from = centre > half ? centre - half : 0u;
    const auto to = std::min (x.size(), centre + half);

    double sum = 0.0;
    for (size_t i = from; i < to; ++i)
        sum += (double) x[i] * (double) x[i];
    return std::sqrt (sum / std::max<size_t> (1, to - from));
}
} // namespace

int main (int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: 808GloPresetProbe <output.json>\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI initialiseJuce;
    EightOhEightGloProAudioProcessor processor;

    const auto presetCount = processor.getPresetManager().getNumberOfFactoryPresets();
    const auto fundamental = 440.0 * std::pow (2.0, (probeNote - 69) / 12.0);

    juce::String json = "[\n";

    for (int preset = 0; preset < presetCount; ++preset)
    {
        const auto program = preset + 1;
        const auto render = renderPreset (processor, program, probeNote);
        const auto& x = render.mono;

        // --- level and shape -------------------------------------------------
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

        // Decay times measured from the peak, using a short-window envelope.
        const auto peakEnvelope = envelopeAt (x, (double) peakIndex / sampleRate);
        double t20 = -1.0, t40 = -1.0;
        for (double t = (double) peakIndex / sampleRate; t < renderSeconds; t += 0.005)
        {
            const auto level = envelopeAt (x, t);
            const auto drop = toDb (level) - toDb (peakEnvelope);
            if (t20 < 0.0 && drop <= -20.0) t20 = t - (double) peakIndex / sampleRate;
            if (t40 < 0.0 && drop <= -40.0) { t40 = t - (double) peakIndex / sampleRate; break; }
        }

        // --- harmonic structure ---------------------------------------------
        // The window follows the preset's own envelope. A fixed 300-800 ms
        // window is silence on a short preset, and Goertzel on silence returns
        // the shape of the noise floor: a preset with harmonics=0 and body=0
        // was reporting more third harmonic than presets that actually have one.
        // At least four cycles of the fundamental are required, or the bins are
        // too wide to separate adjacent partials.
        constexpr double minimumWindow = 4.0 / 43.653;
        double audibleEnd = 0.0;
        for (double t = 0.005; t < renderSeconds; t += 0.005)
            if (envelopeAt (x, t) > peakEnvelope * 0.02) // within 34 dB of the peak
                audibleEnd = t;

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
        // Each partial as a FRACTION of measured partial energy. Dividing by
        // the fundamental alone explodes to 112x on a detuned poly preset
        // whose energy simply is not at f0; a fraction stays bounded and still
        // says how much of the sound each partial owns.
        const auto reference = std::max (h1 + h2 + h3 + h5 + h7, 1.0e-9);

        // --- transient / punch ------------------------------------------------
        // Transient dominance inside the window a listener hears as "the hit".
        // Comparing the attack with a FIXED 250-500 ms body window is degenerate:
        // on a short preset that window is silence, so the ratio exploded to
        // 70-98 dB and the metric ended up ranking decay length rather than
        // knock (it correlated -0.69 with log decay time). A crest factor over
        // the first 150 ms is well defined for every preset, long or short.
        const auto clickWindow = (size_t) (0.030 * sampleRate);
        double attackPeak = 0.0;
        for (size_t i = 0; i < std::min (clickWindow, x.size()); ++i)
            attackPeak = std::max (attackPeak, std::abs ((double) x[i]));

        const auto punchWindow = std::min (x.size(), (size_t) (0.150 * sampleRate));
        double punchSum = 0.0;
        for (size_t i = 0; i < punchWindow; ++i)
            punchSum += (double) x[i] * (double) x[i];
        const auto punchRms = std::sqrt (punchSum / std::max<size_t> (1, punchWindow));
        const auto punchDb = toDb (attackPeak) - toDb (std::max (punchRms, 1.0e-9));

        // How far the body sits below the hit, measured only where a body
        // actually exists. Reported with a validity flag instead of a number
        // that silently means "this preset had already stopped".
        const auto bodyFrom = (size_t) (0.25 * sampleRate);
        const auto bodyTo = std::min (x.size(), (size_t) (0.50 * sampleRate));
        double bodySum = 0.0;
        for (size_t i = bodyFrom; i < bodyTo; ++i)
            bodySum += (double) x[i] * (double) x[i];
        const auto bodyRms = bodyTo > bodyFrom
                               ? std::sqrt (bodySum / (double) (bodyTo - bodyFrom))
                               : 0.0;
        const auto bodyValid = bodyRms > peak * 0.001; // body above -60 dB of the hit
        const auto bodyRatioDb = bodyValid ? toDb (attackPeak) - toDb (bodyRms) : 0.0;

        // High-frequency content of the attack: first difference rejects the
        // sub fundamental, so what remains is the click's own band.
        double attackHf = 0.0;
        for (size_t i = 1; i < std::min (clickWindow, x.size()); ++i)
        {
            const auto difference = (double) x[i] - (double) x[i - 1];
            attackHf += difference * difference;
        }
        attackHf = std::sqrt (attackHf / std::max<size_t> (1, clickWindow));

        // --- pitch drop, tracked an octave up --------------------------------
        const auto pitchRender = renderPreset (processor, program, pitchProbeNote);
        const auto target = 440.0 * std::pow (2.0, (pitchProbeNote - 69) / 12.0);
        // Lowpass just above the fundamental so harmonics cannot be counted as
        // cycles, and gate every reading on the note still sounding: a preset
        // that has decayed to silence was reporting the pitch of its own
        // denormal noise, 7 octaves sharp.
        const auto pitchSignal = lowpassed (pitchRender.mono, target * 1.5);
        const auto pitch = instantaneousPitch (pitchSignal);

        double pitchPeak = 0.0;
        for (auto sample : pitchSignal)
            pitchPeak = std::max (pitchPeak, std::abs ((double) sample));
        const auto audibleFloor = pitchPeak * 0.01; // -40 dB from this preset's own peak

        const auto audibleAt = [&] (double seconds)
        {
            return envelopeAt (pitchSignal, seconds, 30.0) > audibleFloor;
        };

        double startSemitones = 0.0, settleMs = 0.0, landingCents = 0.0;
        bool landingMeasured = false;

        // Drop height, ignoring the first 5 ms: the click is broadband and its
        // crossings are not the oscillator's. A high percentile rather than the
        // maximum keeps one ragged cycle from setting the number.
        std::vector<double> earlyFrequencies;
        for (const auto& [time, frequency] : pitch)
            if (time > 0.005 && time < 0.20 && audibleAt (time))
                earlyFrequencies.push_back (frequency);
        if (! earlyFrequencies.empty())
        {
            auto sorted = earlyFrequencies;
            std::sort (sorted.begin(), sorted.end());
            const auto p90 = sorted[(size_t) ((double) (sorted.size() - 1) * 0.9)];
            startSemitones = 12.0 * std::log2 (std::max (p90, 1.0) / target);
        }

        for (const auto& [time, frequency] : pitch)
        {
            if (time < 0.005) continue;
            if (! audibleAt (time)) break;
            if (std::abs (12.0 * std::log2 (std::max (frequency, 1.0) / target)) < 0.5)
            { settleMs = time * 1000.0; break; }
        }

        // Where the note actually lands. Zero crossings cannot answer this for
        // a folded or hard-clipped preset - the shaper adds crossings of its
        // own - so the settled window is scanned with Goertzel and the bin with
        // the most energy wins. That is what finally settled the sibling
        // project's octave-flipped detectors too.
        {
            const auto scanFrom = (size_t) (0.30 * sampleRate);
            const auto scanTo = std::min (pitchSignal.size(), (size_t) (0.80 * sampleRate));
            if (scanTo > scanFrom && envelopeAt (pitchRender.mono, 0.4, 60.0) > audibleFloor)
            {
                double bestMagnitude = 0.0, bestCents = 0.0;
                for (double cents = -700.0; cents <= 700.0; cents += 20.0)
                {
                    const auto candidate = target * std::pow (2.0, cents / 1200.0);
                    const auto magnitude = goertzel (pitchRender.mono, scanFrom, scanTo, candidate);
                    if (magnitude > bestMagnitude) { bestMagnitude = magnitude; bestCents = cents; }
                }
                landingCents = bestCents;
                landingMeasured = true;
            }
        }

        json << (preset == 0 ? "  {" : ",\n  {");
        json << "\"index\":" << preset
             << ",\"name\":\"" << processor.getProgramName (program).replace ("\"", "'") << "\""
             << ",\"category\":\"" << processor.getPresetManager().getCurrentPresetCategory() << "\""
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
             << ",\"bodyRatioDb\":" << juce::String (bodyRatioDb, 2)
             << ",\"bodyValid\":" << (bodyValid ? "true" : "false")
             << ",\"harmWindowMs\":" << juce::String ((harmonicEnd - harmonicStart) * 1000.0, 1)
             << ",\"attackHfDb\":" << juce::String (toDb (attackHf) - toDb (peak), 2)
             << ",\"dropSemitones\":" << juce::String (startSemitones, 2)
             << ",\"settleMs\":" << juce::String (settleMs, 1)
             << ",\"landingCents\":" << juce::String (landingCents, 1)
             << ",\"landingMeasured\":" << (landingMeasured ? "true" : "false")
             << ",\"ceilingHit\":" << (render.ceilingHit ? "true" : "false")
             << "}";

        std::cerr << "." << std::flush;
    }

    json << "\n]\n";

    juce::File out (juce::File::getCurrentWorkingDirectory()
                        .getChildFile (juce::String::fromUTF8 (argv[1])));
    out.getParentDirectory().createDirectory();
    out.replaceWithText (json);
    std::cerr << "\n";
    std::cout << out.getFullPathName() << "\n";
    return 0;
}
