#include "ParameterLayout.h"

#include <algorithm>
#include <cmath>

namespace
{
juce::NormalisableRange<float> skewedRange (float minimum, float maximum, float centre)
{
    juce::NormalisableRange<float> range { minimum, maximum };
    range.setSkewForCentre (centre);
    return range;
}

juce::String percentageText (float value, int)
{
    return juce::String (value, 0) + "%";
}

float percentageValue (const juce::String& text)
{
    return text.getFloatValue();
}

juce::String compactNumber (float value, int decimalPlaces)
{
    auto text = juce::String (value, decimalPlaces);
    if (text.containsChar ('.'))
        text = text.trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
    return text;
}

juce::String parameterText (float value,
                            int,
                            const juce::String& suffix,
                            juce::NormalisableRange<float> range)
{
    if (suffix == "ms")
    {
        if (std::abs (value) >= 1000.0f)
            return compactNumber (value * 0.001f, 2) + " s";
        if (std::abs (value) >= 10.0f)
            return compactNumber (value, 0) + " ms";
        return compactNumber (value, value < 1.0f ? 2 : 1) + " ms";
    }

    if (suffix == "Hz")
    {
        if (std::abs (value) >= 1000.0f)
            return compactNumber (value * 0.001f, value < 10000.0f ? 1 : 0) + " kHz";
        return compactNumber (value, 0) + " Hz";
    }

    if (suffix == "dB")
        return juce::String (value > 0.0005f ? "+" : "") + compactNumber (value, 1) + " dB";

    if (suffix == "st" || suffix == "ct")
    {
        const auto isIntegral = std::abs (value - std::round (value)) < 0.0005f;
        return compactNumber (value, isIntegral ? 0 : 1) + " " + suffix;
    }

    if (suffix.isNotEmpty())
        return compactNumber (value, 2) + " " + suffix;

    if (range.start >= 0.0f && range.end <= 1.0001f)
        return compactNumber (value * 100.0f, 0) + "%";

    if (range.start >= -1.0001f && range.end <= 1.0001f)
        return juce::String (value > 0.0005f ? "+" : "") + compactNumber (value * 100.0f, 0) + "%";

    const auto magnitude = std::max (std::abs (range.start), std::abs (range.end));
    return compactNumber (value, magnitude > 100.0f ? 0 : magnitude > 10.0f ? 1 : 2);
}

float parameterValue (const juce::String& text,
                      const juce::String& suffix,
                      juce::NormalisableRange<float> range)
{
    const auto normalisedText = text.trim().toLowerCase();
    auto value = normalisedText.getFloatValue();

    if (suffix == "ms" && normalisedText.contains ("s") && ! normalisedText.contains ("ms"))
        value *= 1000.0f;
    else if (suffix == "Hz" && (normalisedText.contains ("khz")
                                 || normalisedText.containsChar ('k')))
        value *= 1000.0f;
    else if (suffix.isEmpty() && normalisedText.containsChar ('%')
             && range.start >= -1.0001f && range.end <= 1.0001f)
        value *= 0.01f;

    return value;
}

std::unique_ptr<juce::AudioParameterFloat> floatParameter (const char* id,
                                                           const char* name,
                                                           juce::NormalisableRange<float> range,
                                                           float defaultValue,
                                                           const juce::String& suffix = {})
{
    juce::AudioParameterFloatAttributes attributes;
    attributes = attributes.withLabel (suffix)
                           .withStringFromValueFunction (
                               [suffix, range] (float value, int maximumLength)
                               {
                                   return parameterText (value, maximumLength, suffix, range);
                               })
                           .withValueFromStringFunction (
                               [suffix, range] (const juce::String& text)
                               {
                                   return parameterValue (text, suffix, range);
                               });
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, range, defaultValue, attributes);
}

std::unique_ptr<juce::AudioParameterFloat> percentParameter (const char* id,
                                                             const char* name,
                                                             float defaultValue)
{
    juce::AudioParameterFloatAttributes attributes;
    attributes = attributes.withLabel ("%").withStringFromValueFunction (percentageText)
                           .withValueFromStringFunction (percentageValue);
    return std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> { 0.0f, 100.0f, 0.01f },
        defaultValue, attributes);
}
} // namespace

namespace glo
{
ParameterPointers::ParameterPointers (juce::AudioProcessorValueTreeState& state)
    : voiceMode (state.getRawParameterValue (ids::voiceMode)),
      legato (state.getRawParameterValue (ids::legato)),
      triggerMode (state.getRawParameterValue (ids::triggerMode)),
      velocitySens (state.getRawParameterValue (ids::velocitySens)),
      tune (state.getRawParameterValue (ids::tune)),
      fine (state.getRawParameterValue (ids::fine)),
      bendRange (state.getRawParameterValue (ids::bendRange)),
      glide (state.getRawParameterValue (ids::glide)),
      body (state.getRawParameterValue (ids::body)),
      harmonics (state.getRawParameterValue (ids::harmonics)),
      harmonicBalance (state.getRawParameterValue (ids::harmonicBalance)),
      pitchDrop (state.getRawParameterValue (ids::pitchDrop)),
      pitchDecay (state.getRawParameterValue (ids::pitchDecay)),
      pitchCurve (state.getRawParameterValue (ids::pitchCurve)),
      attack (state.getRawParameterValue (ids::attack)),
      hold (state.getRawParameterValue (ids::hold)),
      decay (state.getRawParameterValue (ids::decay)),
      sustain (state.getRawParameterValue (ids::sustain)),
      release (state.getRawParameterValue (ids::release)),
      ampCurve (state.getRawParameterValue (ids::ampCurve)),
      click (state.getRawParameterValue (ids::click)),
      clickTone (state.getRawParameterValue (ids::clickTone)),
      clickDecay (state.getRawParameterValue (ids::clickDecay)),
      punch (state.getRawParameterValue (ids::punch)),
      tone (state.getRawParameterValue (ids::tone)),
      toneKeytrack (state.getRawParameterValue (ids::toneKeytrack)),
      drive (state.getRawParameterValue (ids::drive)),
      driveMode (state.getRawParameterValue (ids::driveMode)),
      compressor (state.getRawParameterValue (ids::compressor)),
      clipper (state.getRawParameterValue (ids::clipper)),
      output (state.getRawParameterValue (ids::output))
{
    jassert (voiceMode != nullptr && output != nullptr);
}

dsp::VoiceParameters ParameterPointers::getVoiceParameters() const noexcept
{
    dsp::VoiceParameters result;
    result.voiceMode = static_cast<int> (voiceMode->load (std::memory_order_relaxed));
    result.legato = legato->load (std::memory_order_relaxed) >= 0.5f;
    result.oneShot = triggerMode->load (std::memory_order_relaxed) < 0.5f;
    result.velocitySensitivity = velocitySens->load (std::memory_order_relaxed) * 0.01;
    result.tuneSemitones = tune->load (std::memory_order_relaxed);
    result.fineCents = fine->load (std::memory_order_relaxed);
    result.bendRangeSemitones = bendRange->load (std::memory_order_relaxed);
    result.glideMs = glide->load (std::memory_order_relaxed);
    result.bodyShape = body->load (std::memory_order_relaxed);
    result.harmonics = harmonics->load (std::memory_order_relaxed);
    result.harmonicBalance = harmonicBalance->load (std::memory_order_relaxed);
    result.pitchDropSemitones = pitchDrop->load (std::memory_order_relaxed);
    result.pitchDecayMs = pitchDecay->load (std::memory_order_relaxed);
    result.pitchCurve = pitchCurve->load (std::memory_order_relaxed);
    result.attackMs = attack->load (std::memory_order_relaxed);
    result.holdMs = hold->load (std::memory_order_relaxed);
    result.decayMs = decay->load (std::memory_order_relaxed);
    result.sustain = sustain->load (std::memory_order_relaxed);
    result.releaseMs = release->load (std::memory_order_relaxed);
    result.ampCurve = ampCurve->load (std::memory_order_relaxed);
    result.clickAmount = click->load (std::memory_order_relaxed);
    result.clickToneHz = clickTone->load (std::memory_order_relaxed);
    result.clickDecayMs = clickDecay->load (std::memory_order_relaxed);
    result.punch = punch->load (std::memory_order_relaxed);
    result.toneHz = tone->load (std::memory_order_relaxed);
    result.toneKeytrack = toneKeytrack->load (std::memory_order_relaxed);
    return result;
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::voiceMode, 1 }, "Voice Mode", juce::StringArray { "Mono", "Poly" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ids::legato, 1 }, "Legato", true));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::triggerMode, 1 }, "Trigger", juce::StringArray { "One Shot", "Gate" }, 0));
    layout.add (percentParameter (ids::velocitySens, "Velocity", 55.0f));
    layout.add (floatParameter (ids::tune, "Tune", { -24.0f, 24.0f, 1.0f }, 0.0f, "st"));
    layout.add (floatParameter (ids::fine, "Fine", { -100.0f, 100.0f, 0.1f }, 0.0f, "ct"));
    layout.add (floatParameter (ids::bendRange, "Bend Range", { 1.0f, 24.0f, 1.0f }, 12.0f, "st"));
    layout.add (floatParameter (ids::glide, "Glide", skewedRange (0.0f, 2000.0f, 160.0f), 85.0f, "ms"));

    layout.add (floatParameter (ids::body, "Body Shape", { 0.0f, 1.0f, 0.0001f }, 0.08f));
    layout.add (floatParameter (ids::harmonics, "Harmonics", { 0.0f, 1.0f, 0.0001f }, 0.08f));
    layout.add (floatParameter (ids::harmonicBalance, "Harmonic Balance", { -1.0f, 1.0f, 0.0001f }, 0.0f));
    layout.add (floatParameter (ids::pitchDrop, "Pitch Drop", { 0.0f, 48.0f, 0.01f }, 19.0f, "st"));
    layout.add (floatParameter (ids::pitchDecay, "Pitch Time", skewedRange (5.0f, 500.0f, 55.0f), 42.0f, "ms"));
    layout.add (floatParameter (ids::pitchCurve, "Pitch Curve", { 0.25f, 4.0f, 0.0001f }, 1.25f));

    layout.add (floatParameter (ids::attack, "Attack", skewedRange (0.05f, 50.0f, 2.0f), 0.5f, "ms"));
    layout.add (floatParameter (ids::hold, "Hold", skewedRange (0.0f, 250.0f, 25.0f), 0.0f, "ms"));
    layout.add (floatParameter (ids::decay, "Decay", skewedRange (50.0f, 8000.0f, 1000.0f), 1050.0f, "ms"));
    layout.add (floatParameter (ids::sustain, "Sustain", { 0.0f, 1.0f, 0.0001f }, 0.0f));
    layout.add (floatParameter (ids::release, "Release", skewedRange (5.0f, 3000.0f, 120.0f), 85.0f, "ms"));
    layout.add (floatParameter (ids::ampCurve, "Amp Curve", { 0.25f, 4.0f, 0.0001f }, 1.7f));

    layout.add (floatParameter (ids::click, "Click", { 0.0f, 1.0f, 0.0001f }, 0.12f));
    layout.add (floatParameter (ids::clickTone, "Click Tone", skewedRange (500.0f, 16000.0f, 4200.0f), 4200.0f, "Hz"));
    layout.add (floatParameter (ids::clickDecay, "Click Length", skewedRange (0.1f, 30.0f, 3.0f), 3.5f, "ms"));
    layout.add (floatParameter (ids::punch, "Punch", { 0.0f, 1.0f, 0.0001f }, 0.3f));
    layout.add (floatParameter (ids::tone, "Tone", skewedRange (45.0f, 18000.0f, 1600.0f), 6200.0f, "Hz"));
    layout.add (floatParameter (ids::toneKeytrack, "Tone Keytrack", { 0.0f, 1.0f, 0.0001f }, 0.12f));

    layout.add (floatParameter (ids::drive, "Drive", { 0.0f, 36.0f, 0.01f }, 3.0f, "dB"));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ids::driveMode, 1 }, "Drive Mode",
        juce::StringArray { "Clean", "Warm", "Hard", "Fold" }, 1));
    layout.add (floatParameter (ids::compressor, "Compression", { 0.0f, 1.0f, 0.0001f }, 0.22f));
    layout.add (floatParameter (ids::clipper, "Clip", { 0.0f, 1.0f, 0.0001f }, 0.18f));
    layout.add (floatParameter (ids::output, "Output", { -24.0f, 12.0f, 0.01f }, -3.0f, "dB"));

    return layout;
}
} // namespace glo
