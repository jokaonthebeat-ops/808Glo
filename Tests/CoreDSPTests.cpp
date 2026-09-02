#include "DSP/Envelope.h"
#include "DSP/Math.h"
#include "DSP/SynthEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace
{
int failures = 0;

void check (bool condition, std::string_view message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void checkNear (double actual, double expected, double tolerance, std::string_view message)
{
    if (! std::isfinite (actual) || ! std::isfinite (expected)
        || std::abs (actual - expected) > tolerance)
    {
        ++failures;
        std::cerr << "FAIL: " << message << " (actual=" << actual
                  << ", expected=" << expected << ", tolerance=" << tolerance << ")\n";
    }
}

glo::dsp::VoiceParameters cleanParameters()
{
    glo::dsp::VoiceParameters p;
    p.voiceMode = 0;
    p.legato = true;
    p.oneShot = true;
    p.velocitySensitivity = 0.0;
    p.glideMs = 100.0;
    p.bodyShape = 0.0;
    p.harmonics = 0.0;
    p.pitchDropSemitones = 0.0;
    p.attackMs = 0.05;
    p.holdMs = 0.0;
    p.decayMs = 2000.0;
    p.sustain = 0.0;
    p.releaseMs = 20.0;
    p.ampCurve = 1.0;
    p.clickAmount = 0.0;
    p.punch = 0.0;
    p.toneHz = 18000.0;
    p.toneKeytrack = 0.0;
    return p;
}

void testMath()
{
    checkNear (glo::dsp::midiNoteToHz (69.0), 440.0, 1.0e-12, "A4 tuning");
    checkNear (glo::dsp::midiNoteToHz (36.0), 65.4063913, 1.0e-6, "C2 tuning");
    checkNear (glo::dsp::gainToDecibels (glo::dsp::decibelsToGain (-18.0)), -18.0, 1.0e-10,
               "gain/decibel round trip");
    checkNear (glo::dsp::wrapPhase (2.25), 0.25, 1.0e-12, "positive phase wrapping");
    checkNear (glo::dsp::wrapPhase (-0.25), 0.75, 1.0e-12, "negative phase wrapping");
}

void testEnvelopeLifecycle()
{
    glo::dsp::AmpEnvelope envelope;
    envelope.prepare (1000.0);

    glo::dsp::EnvelopeParameters parameters;
    parameters.attackMs = 10.0;
    parameters.holdMs = 5.0;
    parameters.decayMs = 50.0;
    parameters.sustain = 0.0;
    parameters.releaseMs = 10.0;
    parameters.curve = 1.0;
    parameters.oneShot = true;

    envelope.noteOn (parameters, true);
    for (int i = 0; i < 10; ++i)
        static_cast<void> (envelope.process());
    check (envelope.getCurrentValue() > 0.8, "attack reaches its peak");

    for (int i = 0; i < 80; ++i)
        static_cast<void> (envelope.process());
    check (! envelope.isActive(), "one-shot envelope terminates");
}

void testEnvelopeParameterUpdateContinuity()
{
    glo::dsp::EnvelopeParameters parameters;
    parameters.attackMs = 1000.0;
    parameters.holdMs = 0.0;
    parameters.decayMs = 1000.0;
    parameters.sustain = 0.1;
    parameters.releaseMs = 1000.0;
    parameters.curve = 1.0;
    parameters.oneShot = false;

    glo::dsp::AmpEnvelope attackEnvelope;
    attackEnvelope.prepare (1000.0);
    attackEnvelope.noteOn (parameters, true);
    for (int i = 0; i < 401; ++i)
        static_cast<void> (attackEnvelope.process());

    const auto attackBefore = attackEnvelope.getCurrentValue();
    parameters.attackMs = 100.0;
    parameters.curve = 4.0;
    attackEnvelope.updateParameters (parameters);
    const auto attackAfter = attackEnvelope.process();
    checkNear (attackAfter, attackBefore, 1.0e-12,
               "attack automation preserves the active envelope level");
    check (attackEnvelope.getStage() == glo::dsp::AmpEnvelope::Stage::attack,
           "shortening attack remaps remaining progress instead of completing instantly");

    parameters.attackMs = 0.0;
    parameters.decayMs = 1000.0;
    parameters.sustain = 0.1;
    parameters.curve = 1.0;

    glo::dsp::AmpEnvelope decayEnvelope;
    decayEnvelope.prepare (1000.0);
    decayEnvelope.noteOn (parameters, true);
    static_cast<void> (decayEnvelope.process());
    for (int i = 0; i < 401; ++i)
        static_cast<void> (decayEnvelope.process());

    const auto decayBefore = decayEnvelope.getCurrentValue();
    parameters.decayMs = 100.0;
    parameters.sustain = 0.85;
    parameters.curve = 4.0;
    decayEnvelope.updateParameters (parameters);
    const auto decayAfter = decayEnvelope.process();
    checkNear (decayAfter, decayBefore, 1.0e-12,
               "decay time, curve and sustain automation preserve level continuity");
    check (decayEnvelope.getStage() == glo::dsp::AmpEnvelope::Stage::decay,
           "shortening decay remaps remaining progress instead of completing instantly");

    parameters.attackMs = 0.0;
    parameters.decayMs = 0.0;
    parameters.sustain = 1.0;
    parameters.releaseMs = 1000.0;
    parameters.curve = 1.0;

    glo::dsp::AmpEnvelope releaseEnvelope;
    releaseEnvelope.prepare (1000.0);
    releaseEnvelope.noteOn (parameters, true);
    static_cast<void> (releaseEnvelope.process());
    static_cast<void> (releaseEnvelope.process());
    releaseEnvelope.noteOff();
    for (int i = 0; i < 401; ++i)
        static_cast<void> (releaseEnvelope.process());

    const auto releaseBefore = releaseEnvelope.getCurrentValue();
    parameters.releaseMs = 100.0;
    parameters.curve = 4.0;
    releaseEnvelope.updateParameters (parameters);
    const auto releaseAfter = releaseEnvelope.process();
    checkNear (releaseAfter, releaseBefore, 1.0e-12,
               "release time and curve automation preserve level continuity");
    check (releaseEnvelope.getStage() == glo::dsp::AmpEnvelope::Stage::release,
           "shortening release remaps remaining progress instead of completing instantly");
}

void testLegatoDoesNotRetriggerEnvelope()
{
    constexpr auto sampleRate = 48000.0;
    auto parameters = cleanParameters();
    parameters.oneShot = false;
    parameters.sustain = 0.7;
    parameters.attackMs = 1.0;
    parameters.decayMs = 500.0;

    glo::dsp::Voice voice;
    voice.prepare (sampleRate);
    voice.startNote (36, 1.0, parameters, false, true, 1);
    for (int i = 0; i < 2400; ++i)
        static_cast<void> (voice.renderSample());

    const auto before = voice.getEnvelopeValue();
    voice.startNote (43, 1.0, parameters, true, false, 2);
    static_cast<void> (voice.renderSample());
    const auto after = voice.getEnvelopeValue();

    check (before > 0.5, "legato test reaches audible envelope level");
    check (after > 0.5, "legato note does not zero the envelope");
    check (std::abs (after - before) < 0.02, "legato note preserves envelope continuity");
}

void testGlideCompletesAtDisplayedTime()
{
    constexpr auto sampleRate = 48000.0;
    auto parameters = cleanParameters();
    parameters.glideMs = 100.0;

    glo::dsp::Voice voice;
    voice.prepare (sampleRate);
    voice.startNote (36, 1.0, parameters, false, true, 1);
    for (int i = 0; i < 64; ++i)
        static_cast<void> (voice.renderSample());

    voice.startNote (48, 1.0, parameters, true, false, 2);
    for (int i = 0; i < 4799; ++i)
        static_cast<void> (voice.renderSample());

    check (voice.getCurrentFrequency() < glo::dsp::midiNoteToHz (48.0),
           "glide remains in flight before its final sample");
    static_cast<void> (voice.renderSample());
    checkNear (voice.getCurrentFrequency(), glo::dsp::midiNoteToHz (48.0), 1.0e-9,
               "glide lands exactly on destination");
}

void testMonoHeldNoteReturn()
{
    constexpr int length = 64;
    std::array<float, length> left {};
    std::array<float, length> right {};
    float* channels[] { left.data(), right.data() };
    auto parameters = cleanParameters();
    parameters.oneShot = false;
    parameters.sustain = 0.8;

    glo::dsp::SynthEngine engine;
    engine.prepare (48000.0);
    engine.noteOn (36, 1.0, parameters);
    engine.render (channels, 2, 0, length, parameters);
    engine.noteOn (43, 1.0, parameters);
    check (engine.getVoice (0).getNote() == 43, "last mono note has priority");
    engine.noteOff (43, parameters);
    check (engine.getVoice (0).getNote() == 36, "mono note stack returns to held note");
}

void testSeparatedMonoHitRetriggers()
{
    constexpr int length = 2400;
    std::array<float, length> left {};
    std::array<float, length> right {};
    float* channels[] { left.data(), right.data() };
    auto parameters = cleanParameters();
    parameters.legato = true;
    parameters.oneShot = true;
    parameters.decayMs = 1500.0;

    glo::dsp::SynthEngine engine;
    engine.prepare (48000.0);
    engine.noteOn (36, 1.0, parameters);
    engine.render (channels, 2, 0, length, parameters);
    engine.noteOff (36, parameters);
    check (engine.getVoice (0).getEnvelopeValue() > 0.2,
           "one-shot tail remains active after note-off");

    engine.noteOn (36, 1.0, parameters);
    checkNear (engine.getVoice (0).getEnvelopeValue(), 0.0, 1.0e-12,
               "a separated repeated hit restarts the envelope");
}

void testGateReleaseAndLiveEnvelopeUpdate()
{
    constexpr int length = 512;
    std::array<float, length> left {};
    std::array<float, length> right {};
    float* channels[] { left.data(), right.data() };
    auto parameters = cleanParameters();
    parameters.oneShot = false;
    parameters.sustain = 0.8;
    parameters.decayMs = 2000.0;
    parameters.releaseMs = 600.0;

    glo::dsp::SynthEngine engine;
    engine.prepare (48000.0);
    engine.noteOn (36, 1.0, parameters);
    engine.render (channels, 2, 0, length, parameters);
    parameters.releaseMs = 5.0;
    engine.noteOff (36, parameters);

    for (int pass = 0; pass < 4; ++pass)
    {
        left.fill (0.0f);
        right.fill (0.0f);
        engine.render (channels, 2, 0, length, parameters);
    }
    check (engine.getActiveVoiceCount() == 0,
           "gate note uses the current release setting and terminates");
}

void testFilterTailAndRetriggerContinuity()
{
    constexpr int renderLength = 1024;
    std::array<float, renderLength> left {};
    std::array<float, renderLength> right {};
    float* channels[] { left.data(), right.data() };

    auto parameters = cleanParameters();
    parameters.legato = false;
    parameters.oneShot = false;
    parameters.sustain = 1.0;
    parameters.decayMs = 2000.0;
    parameters.releaseMs = 0.0;
    parameters.toneHz = 45.0;

    glo::dsp::SynthEngine engine;
    engine.prepare (48000.0);
    engine.noteOn (36, 1.0, parameters);
    engine.render (channels, 2, 0, renderLength, parameters);
    engine.noteOff (36, parameters);

    left.fill (0.0f);
    right.fill (0.0f);
    engine.render (channels, 2, 0, 1, parameters);
    const auto releaseEdge = left[0];
    check (std::abs (releaseEdge) > 1.0e-6f,
           "abrupt release leaves a measurable filter state");
    check (engine.getActiveVoiceCount() == 0,
           "filter tail does not keep a finished note logically active");

    left.fill (0.0f);
    right.fill (0.0f);
    constexpr int tailSamples = 64;
    engine.render (channels, 2, 0, tailSamples, parameters);
    const auto tailPeak = *std::max_element (
        left.begin(), left.begin() + tailSamples,
        [] (float a, float b) { return std::abs (a) < std::abs (b); });
    check (std::abs (tailPeak) > 1.0e-6f,
           "completed envelope continues rendering the residual filter tail");

    const auto tailBeforeRetrigger = left[tailSamples - 1];
    engine.noteOn (43, 1.0, parameters);
    left.fill (0.0f);
    right.fill (0.0f);
    engine.render (channels, 2, 0, 1, parameters);
    checkNear (left[0], tailBeforeRetrigger, 1.0e-6,
               "reusing a tailing voice crossfades without a sample discontinuity");
}

void testPolyphonyAndVoiceStealing()
{
    auto parameters = cleanParameters();
    parameters.voiceMode = 1;
    parameters.oneShot = false;
    parameters.sustain = 1.0;

    glo::dsp::SynthEngine engine;
    engine.prepare (48000.0);
    for (int note = 36; note < 44; ++note)
        engine.noteOn (note, 1.0, parameters);
    check (engine.getActiveVoiceCount() == glo::dsp::SynthEngine::maxVoices,
           "poly mode fills its fixed voice pool");

    engine.noteOn (60, 1.0, parameters);
    check (engine.getActiveVoiceCount() == glo::dsp::SynthEngine::maxVoices,
           "voice stealing keeps a bounded pool");
}

std::vector<float> renderDeterministicPattern (int chunkSize)
{
    constexpr int total = 4096;
    std::vector<float> left (total, 0.0f);
    std::vector<float> right (total, 0.0f);
    float* channels[] { left.data(), right.data() };

    auto parameters = cleanParameters();
    parameters.pitchDropSemitones = 24.0;
    parameters.pitchDecayMs = 42.0;
    parameters.clickAmount = 0.25;
    parameters.harmonics = 0.18;
    parameters.bodyShape = 0.12;

    glo::dsp::SynthEngine engine;
    engine.prepare (48000.0);
    engine.noteOn (36, 0.91, parameters);

    for (int start = 0; start < total; start += chunkSize)
    {
        const auto count = std::min (chunkSize, total - start);
        engine.render (channels, 2, start, count, parameters);
    }
    return left;
}

void testDeterminismAndBlockInvariance()
{
    const auto reference = renderDeterministicPattern (4096);
    const auto smallBlocks = renderDeterministicPattern (17);
    const auto repeated = renderDeterministicPattern (4096);

    check (reference == repeated, "identical triggers render deterministically");
    check (reference == smallBlocks, "render is invariant to host block size");
}

void testStressAcrossSampleRates()
{
    constexpr std::array sampleRates { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    constexpr int samples = 2048;

    for (const auto sampleRate : sampleRates)
    {
        std::array<float, samples> left {};
        std::array<float, samples> right {};
        float* channels[] { left.data(), right.data() };
        auto parameters = cleanParameters();
        parameters.voiceMode = 1;
        parameters.bodyShape = 1.0;
        parameters.harmonics = 1.0;
        parameters.harmonicBalance = -0.75;
        parameters.pitchDropSemitones = 48.0;
        parameters.pitchDecayMs = 5.0;
        parameters.pitchCurve = 4.0;
        parameters.clickAmount = 1.0;
        parameters.clickToneHz = 16000.0;
        parameters.clickDecayMs = 30.0;
        parameters.punch = 1.0;

        glo::dsp::SynthEngine engine;
        engine.prepare (sampleRate);
        for (int note = 24; note < 32; ++note)
            engine.noteOn (note, 1.0, parameters);
        engine.render (channels, 2, 0, samples, parameters);

        for (const auto value : left)
        {
            check (std::isfinite (value), "stress render contains only finite values");
            check (std::abs (value) <= 4.0f, "voice sum remains within fixed linear headroom");
        }
    }
}
} // namespace

int main()
{
    testMath();
    testEnvelopeLifecycle();
    testEnvelopeParameterUpdateContinuity();
    testLegatoDoesNotRetriggerEnvelope();
    testGlideCompletesAtDisplayedTime();
    testMonoHeldNoteReturn();
    testSeparatedMonoHitRetriggers();
    testGateReleaseAndLiveEnvelopeUpdate();
    testFilterTailAndRetriggerContinuity();
    testPolyphonyAndVoiceStealing();
    testDeterminismAndBlockInvariance();
    testStressAcrossSampleRates();

    if (failures == 0)
    {
        std::cout << "808Glo core DSP: all tests passed\n";
        return EXIT_SUCCESS;
    }

    std::cerr << failures << " test assertion(s) failed\n";
    return EXIT_FAILURE;
}
