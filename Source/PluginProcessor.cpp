#include "PluginProcessor.h"

#include "PluginEditor.h"

#include <cmath>

EightOhEightGloProAudioProcessor::EightOhEightGloProAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", glo::createParameterLayout()),
      parameterPointers (parameters),
      presetManager (parameters)
{
    for (auto& level : peakLevels)
        level.store (0.0f, std::memory_order_relaxed);
    for (auto& level : rmsLevels)
        level.store (0.0f, std::memory_order_relaxed);
    for (auto& sample : scopeSamples)
        sample.store (0.0f, std::memory_order_relaxed);

    keyboardState.addListener (this);
}

EightOhEightGloProAudioProcessor::~EightOhEightGloProAudioProcessor()
{
    keyboardState.removeListener (this);
}

void EightOhEightGloProAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = std::max (1.0, sampleRate);
    maximumProcessingBlockSize = std::max (1, samplesPerBlock);
    synthEngine.prepare (currentSampleRate);
    previousVoiceMode = static_cast<int> (parameterPointers.voiceMode->load (std::memory_order_relaxed));

    oversampling.reset();
    oversampling.initProcessing (static_cast<std::size_t> (maximumProcessingBlockSize));
    setLatencySamples (static_cast<int> (std::ceil (oversampling.getLatencyInSamples())));

    constexpr auto oversamplingFactor = 4.0;
    const auto oversampledRate = currentSampleRate * oversamplingFactor;
    for (auto& blocker : outputDcBlockers)
        blocker.prepare (oversampledRate, 5.0);

    const auto smoothingSeconds = 0.02;
    driveGainSmoothed.reset (oversampledRate, smoothingSeconds);
    outputGainSmoothed.reset (oversampledRate, smoothingSeconds);
    compressionSmoothed.reset (oversampledRate, smoothingSeconds);
    clipperSmoothed.reset (oversampledRate, smoothingSeconds);

    driveGainSmoothed.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (parameterPointers.drive->load (std::memory_order_relaxed)));
    outputGainSmoothed.setCurrentAndTargetValue (
        juce::Decibels::decibelsToGain (parameterPointers.output->load (std::memory_order_relaxed)));
    compressionSmoothed.setCurrentAndTargetValue (
        parameterPointers.compressor->load (std::memory_order_relaxed));
    clipperSmoothed.setCurrentAndTargetValue (
        parameterPointers.clipper->load (std::memory_order_relaxed));

    compressorEnvelope = 0.0f;
    compressorAttack = std::exp (-1.0f / static_cast<float> (oversampledRate * 0.004));
    compressorRelease = std::exp (-1.0f / static_cast<float> (oversampledRate * 0.090));
    ceilingHit.store (false, std::memory_order_relaxed);
    scopeDecimationCounter = 0;
    for (auto& level : peakLevels)
        level.store (0.0f, std::memory_order_relaxed);
    for (auto& level : rmsLevels)
        level.store (0.0f, std::memory_order_relaxed);
}

void EightOhEightGloProAudioProcessor::releaseResources()
{
    synthEngine.reset();
    oversampling.reset();
}

bool EightOhEightGloProAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet().isDisabled();
}

void EightOhEightGloProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                     juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const auto totalSamples = buffer.getNumSamples();
    if (totalSamples <= 0)
        return;

    const auto voiceParameters = parameterPointers.getVoiceParameters();

    if (voiceParameters.voiceMode != previousVoiceMode)
    {
        synthEngine.allNotesOff (false);
        previousVoiceMode = voiceParameters.voiceMode;
    }

    // UI notes are applied at the first sample of this block. Unlike
    // MidiKeyboardState::processNextMidiBuffer(), this fixed SPSC drain neither
    // takes the keyboard state's CriticalSection nor grows the host MIDI buffer.
    drainUiNoteQueue (voiceParameters);

    std::array<float*, 2> channels { buffer.getWritePointer (0), buffer.getWritePointer (1) };
    auto renderPosition = 0;

    for (const auto metadata : midiMessages)
    {
        const auto eventPosition = juce::jlimit (0, totalSamples, metadata.samplePosition);
        if (eventPosition > renderPosition)
        {
            synthEngine.render (channels.data(), 2, renderPosition,
                                eventPosition - renderPosition, voiceParameters);
            renderPosition = eventPosition;
        }
        handleMidiMessage (metadata.getMessage(), voiceParameters);
    }

    if (renderPosition < totalSamples)
        synthEngine.render (channels.data(), 2, renderPosition,
                            totalSamples - renderPosition, voiceParameters);

    processColourAndDynamics (buffer);
    publishMetersAndScope (buffer);
}

juce::AudioProcessorEditor* EightOhEightGloProAudioProcessor::createEditor()
{
    return new EightOhEightGloProAudioProcessorEditor (*this);
}

double EightOhEightGloProAudioProcessor::getTailLengthSeconds() const
{
    const auto attack = parameterPointers.attack->load (std::memory_order_relaxed);
    const auto hold = parameterPointers.hold->load (std::memory_order_relaxed);
    const auto decay = parameterPointers.decay->load (std::memory_order_relaxed);
    const auto release = parameterPointers.release->load (std::memory_order_relaxed);
    constexpr auto residualAllowanceSeconds = 0.25;
    return juce::jlimit (0.0, 12.0,
                         static_cast<double> (attack + hold + decay + release) * 0.001
                             + residualAllowanceSeconds);
}

int EightOhEightGloProAudioProcessor::getNumPrograms()
{
    // Program zero is the deterministic Init state. Factory programs follow it,
    // so a user preset or untouched Init state is never misreported as factory 0.
    return 1 + presetManager.getNumberOfFactoryPresets();
}

int EightOhEightGloProAudioProcessor::getCurrentProgram()
{
    const auto factoryIndex = presetManager.getCurrentFactoryPresetIndex();
    return factoryIndex >= 0 && factoryIndex < presetManager.getNumberOfFactoryPresets()
             ? factoryIndex + 1
             : 0;
}

void EightOhEightGloProAudioProcessor::setCurrentProgram (int index)
{
    if (index == 0)
        presetManager.loadInitPreset();
    else if (index > 0)
        presetManager.loadFactoryPreset (index - 1);
}

const juce::String EightOhEightGloProAudioProcessor::getProgramName (int index)
{
    if (index == 0)
        return "Init 808";

    const auto factoryIndex = index - 1;
    if (factoryIndex >= 0 && factoryIndex < presetManager.getNumberOfFactoryPresets())
        return presetManager.getFactoryPreset (factoryIndex).name;
    return {};
}

void EightOhEightGloProAudioProcessor::getStateInformation (juce::MemoryBlock& destinationData)
{
    auto stateSnapshot = parameters.copyState();
    presetManager.writeMetadataToState (stateSnapshot);
    if (const auto xml = stateSnapshot.createXml())
        copyXmlToBinary (*xml, destinationData);
}

void EightOhEightGloProAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (parameters.state.getType()))
        return;

    const auto restoredState = juce::ValueTree::fromXml (*xml);
    if (! restoredState.isValid() || restoredState.getType() != parameters.state.getType())
        return;

    parameters.replaceState (restoredState);
    presetManager.restoreMetadataFromState (restoredState);
}

float EightOhEightGloProAudioProcessor::getPeakLevel (int channel) const noexcept
{
    return peakLevels[static_cast<std::size_t> (juce::jlimit (0, 1, channel))]
        .load (std::memory_order_relaxed);
}

float EightOhEightGloProAudioProcessor::getRmsLevel (int channel) const noexcept
{
    return rmsLevels[static_cast<std::size_t> (juce::jlimit (0, 1, channel))]
        .load (std::memory_order_relaxed);
}

bool EightOhEightGloProAudioProcessor::consumeCeilingHit() noexcept
{
    return ceilingHit.exchange (false, std::memory_order_acq_rel);
}

int EightOhEightGloProAudioProcessor::copyLatestScopeSamples (float* destination,
                                                              int maximumSamples) const noexcept
{
    if (destination == nullptr || maximumSamples <= 0)
        return 0;

    const auto count = std::min (maximumSamples, static_cast<int> (scopeCapacity));
    const auto end = scopeWriteCounter.load (std::memory_order_acquire);
    const auto available = static_cast<int> (std::min<std::uint64_t> (end, scopeCapacity));
    const auto toCopy = std::min (count, available);
    const auto start = end - static_cast<std::uint64_t> (toCopy);

    for (int i = 0; i < toCopy; ++i)
    {
        const auto index = static_cast<std::size_t> ((start + static_cast<std::uint64_t> (i)) % scopeCapacity);
        destination[i] = scopeSamples[index].load (std::memory_order_relaxed);
    }
    return toCopy;
}

void EightOhEightGloProAudioProcessor::handleMidiMessage (
    const juce::MidiMessage& message,
    const glo::dsp::VoiceParameters& voiceParameters) noexcept
{
    if (message.isNoteOn())
    {
        synthEngine.noteOn (message.getNoteNumber(), message.getFloatVelocity(), voiceParameters);
    }
    else if (message.isNoteOff())
    {
        synthEngine.noteOff (message.getNoteNumber(), voiceParameters);
    }
    else if (message.isPitchWheel())
    {
        const auto value = message.getPitchWheelValue();
        const auto bend = value >= 8192 ? static_cast<double> (value - 8192) / 8191.0
                                        : static_cast<double> (value - 8192) / 8192.0;
        synthEngine.setPitchBend (bend);
    }
    else if (message.isController() && message.getControllerNumber() == 64)
    {
        synthEngine.setSustainPedal (message.getControllerValue() >= 64, voiceParameters);
    }
    else if (message.isAllNotesOff())
    {
        synthEngine.allNotesOff (true);
    }
    else if (message.isAllSoundOff())
    {
        synthEngine.allNotesOff (false);
    }
}

void EightOhEightGloProAudioProcessor::handleNoteOn (juce::MidiKeyboardState* source,
                                                      int midiChannel,
                                                      int midiNoteNumber,
                                                      float velocity) noexcept
{
    juce::ignoreUnused (source, midiChannel);
    enqueueUiNote (midiNoteNumber, velocity, true);
}

void EightOhEightGloProAudioProcessor::handleNoteOff (juce::MidiKeyboardState* source,
                                                       int midiChannel,
                                                       int midiNoteNumber,
                                                       float velocity) noexcept
{
    juce::ignoreUnused (source, midiChannel);
    enqueueUiNote (midiNoteNumber, velocity, false);
}

void EightOhEightGloProAudioProcessor::enqueueUiNote (int midiNoteNumber,
                                                       float velocity,
                                                       bool isNoteOn) noexcept
{
    const auto write = uiNoteWriteIndex.load (std::memory_order_relaxed);
    const auto read = uiNoteReadIndex.load (std::memory_order_acquire);

    if (write - read >= uiNoteQueueCapacity)
    {
        // Dropping an unmatched note-off could leave a voice held forever.
        // Request an audio-thread panic instead; it clears every held/sustained
        // note and discards the compromised batch without taking a lock.
        uiNoteOverflowPanic.store (true, std::memory_order_release);
        return;
    }

    auto& event = uiNoteQueue[write & (uiNoteQueueCapacity - 1u)];
    event.velocity = juce::jlimit (0.0f, 1.0f, velocity);
    event.note = static_cast<std::uint8_t> (juce::jlimit (0, 127, midiNoteNumber));
    event.noteOn = isNoteOn;
    uiNoteWriteIndex.store (write + 1u, std::memory_order_release);
}

void EightOhEightGloProAudioProcessor::drainUiNoteQueue (
    const glo::dsp::VoiceParameters& voiceParameters) noexcept
{
    if (uiNoteOverflowPanic.exchange (false, std::memory_order_acq_rel))
    {
        discardUiNoteQueueAndPanic();
        return;
    }

    auto read = uiNoteReadIndex.load (std::memory_order_relaxed);
    for (;;)
    {
        const auto write = uiNoteWriteIndex.load (std::memory_order_acquire);
        if (read == write)
            break;

        const auto event = uiNoteQueue[read & (uiNoteQueueCapacity - 1u)];
        ++read;
        uiNoteReadIndex.store (read, std::memory_order_release);

        if (event.noteOn)
            synthEngine.noteOn (event.note, event.velocity, voiceParameters);
        else
            synthEngine.noteOff (event.note, voiceParameters);

        if (uiNoteOverflowPanic.load (std::memory_order_acquire))
            break;
    }

    // The producer may have observed a full queue while this drain was in
    // progress. In that case even events already rendered above are cancelled,
    // guaranteeing that a dropped note-off cannot produce a stuck voice.
    if (uiNoteOverflowPanic.exchange (false, std::memory_order_acq_rel))
        discardUiNoteQueueAndPanic();
}

void EightOhEightGloProAudioProcessor::discardUiNoteQueueAndPanic() noexcept
{
    uiNoteReadIndex.store (uiNoteWriteIndex.load (std::memory_order_acquire),
                           std::memory_order_release);
    synthEngine.allNotesOff (false);
}

void EightOhEightGloProAudioProcessor::processColourAndDynamics (juce::AudioBuffer<float>& buffer) noexcept
{
    const auto driveDb = parameterPointers.drive->load (std::memory_order_relaxed);
    const auto outputDb = parameterPointers.output->load (std::memory_order_relaxed);
    const auto driveMode = static_cast<int> (parameterPointers.driveMode->load (std::memory_order_relaxed));
    driveGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (driveDb));
    outputGainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (outputDb));
    compressionSmoothed.setTargetValue (parameterPointers.compressor->load (std::memory_order_relaxed));
    clipperSmoothed.setTargetValue (parameterPointers.clipper->load (std::memory_order_relaxed));

    constexpr auto oversampledCeiling = 0.89125094f; // -1.0 dB at the 4x stage
    constexpr auto emergencySampleCeiling = 0.91201085f; // -0.8 dBFS after reconstruction
    juce::dsp::AudioBlock<float> completeBlock (buffer);
    const auto totalSamples = buffer.getNumSamples();
    auto limiterWasHit = false;

    // Hosts may deliver an offline block larger than the size supplied to
    // prepareToPlay(). JUCE's Oversampling scratch buffer must never receive a
    // larger slice than initProcessing() reserved.
    for (int blockStart = 0; blockStart < totalSamples; blockStart += maximumProcessingBlockSize)
    {
        const auto blockLength = std::min (maximumProcessingBlockSize, totalSamples - blockStart);
        auto block = completeBlock.getSubBlock (static_cast<std::size_t> (blockStart),
                                                static_cast<std::size_t> (blockLength));
        auto upsampled = oversampling.processSamplesUp (block);
        const auto upsampledSamples = static_cast<int> (upsampled.getNumSamples());

        for (int sample = 0; sample < upsampledSamples; ++sample)
        {
            const auto driveGain = driveGainSmoothed.getNextValue();
            const auto compression = compressionSmoothed.getNextValue();
            const auto outputGain = outputGainSmoothed.getNextValue();
            const auto clip = clipperSmoothed.getNextValue();
            auto detector = 0.0f;

            for (std::size_t channel = 0; channel < upsampled.getNumChannels(); ++channel)
            {
                auto* data = upsampled.getChannelPointer (channel);
                data[sample] = waveshape (data[sample], driveGain, driveMode);
                detector = std::max (detector, std::abs (data[sample]));
            }

            const auto coefficient = detector > compressorEnvelope ? compressorAttack
                                                                    : compressorRelease;
            compressorEnvelope = coefficient * compressorEnvelope
                               + (1.0f - coefficient) * detector;

            const auto thresholdDb = -5.0f - 19.0f * compression;
            const auto ratio = 1.0f + 5.0f * compression;
            const auto envelopeDb = juce::Decibels::gainToDecibels (compressorEnvelope, -120.0f);
            const auto reductionDb = envelopeDb > thresholdDb
                ? (thresholdDb + (envelopeDb - thresholdDb) / ratio) - envelopeDb
                : 0.0f;
            const auto compressorGain = juce::Decibels::decibelsToGain (
                reductionDb + 2.5f * compression);

            for (std::size_t channel = 0; channel < upsampled.getNumChannels(); ++channel)
            {
                auto* data = upsampled.getChannelPointer (channel);
                auto value = data[sample] * compressorGain * outputGain;
                value = static_cast<float> (outputDcBlockers[channel].process (value));

                if (clip > 0.0001f)
                {
                    const auto amount = 1.0f + 7.0f * clip;
                    const auto threshold = 1.0f - 0.26f * clip;
                    value = threshold * std::tanh (value * amount / threshold) / std::tanh (amount);
                }

                if (std::abs (value) >= oversampledCeiling)
                    limiterWasHit = true;
                data[sample] = juce::jlimit (-oversampledCeiling, oversampledCeiling, value);
            }
        }

        oversampling.processSamplesDown (block);
    }

    // Reconstruction should remain below this guard because the nonlinear
    // ceiling ran at 4x. This is only an emergency sample-domain bound.
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* data = buffer.getWritePointer (channel);
        for (int sample = 0; sample < totalSamples; ++sample)
        {
            if (std::abs (data[sample]) >= emergencySampleCeiling)
                limiterWasHit = true;
            data[sample] = juce::jlimit (-emergencySampleCeiling, emergencySampleCeiling, data[sample]);
        }
    }

    if (limiterWasHit)
        ceilingHit.store (true, std::memory_order_release);
}

void EightOhEightGloProAudioProcessor::publishMetersAndScope (
    const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto samples = buffer.getNumSamples();
    if (samples <= 0)
        return;

    // Audio blocks usually arrive several times between UI refreshes. Hold a
    // transient peak in the publisher and release it by elapsed audio time so a
    // following quiet block cannot overwrite it before the UI observes it.
    constexpr auto peakReleaseSeconds = 0.12;
    const auto peakRelease = static_cast<float> (std::exp (
        -static_cast<double> (samples) / (currentSampleRate * peakReleaseSeconds)));

    for (int channel = 0; channel < 2; ++channel)
    {
        const auto peak = buffer.getMagnitude (channel, 0, samples);
        const auto rms = buffer.getRMSLevel (channel, 0, samples);
        auto& publishedPeak = peakLevels[static_cast<std::size_t> (channel)];
        const auto heldPeak = publishedPeak.load (std::memory_order_relaxed) * peakRelease;
        publishedPeak.store (std::max (peak, heldPeak), std::memory_order_relaxed);
        rmsLevels[static_cast<std::size_t> (channel)].store (rms, std::memory_order_relaxed);
    }

    const auto decimation = std::max (1, static_cast<int> (currentSampleRate / 12000.0));
    auto counter = scopeWriteCounter.load (std::memory_order_relaxed);
    for (int sample = 0; sample < samples; ++sample)
    {
        if (++scopeDecimationCounter < decimation)
            continue;

        scopeDecimationCounter = 0;
        const auto mono = 0.5f * (buffer.getSample (0, sample) + buffer.getSample (1, sample));
        scopeSamples[static_cast<std::size_t> (counter % scopeCapacity)].store (mono, std::memory_order_relaxed);
        ++counter;
    }
    scopeWriteCounter.store (counter, std::memory_order_release);
}

float EightOhEightGloProAudioProcessor::waveshape (float sample, float driveGain, int mode) noexcept
{
    const auto driven = sample * driveGain;
    const auto compensation = 1.0f / std::sqrt (std::max (1.0f, driveGain));

    switch (mode)
    {
        case 0: // clean gain with only an emergency soft boundary
            return std::tanh (driven * 0.5f) * 2.0f * compensation;

        case 1: // warm asymmetric body
        {
            constexpr auto bias = 0.075f;
            const auto shaped = std::tanh (driven + bias) - std::tanh (bias);
            return shaped * compensation;
        }

        case 2: // hard but anti-aliased by the fixed 4x path
            return juce::jlimit (-1.0f, 1.0f, driven) * compensation;

        case 3: // controlled wavefold for aggressive presets
        {
            auto folded = std::fmod (driven + 3.0f, 4.0f);
            if (folded < 0.0f)
                folded += 4.0f;
            folded = std::abs (folded - 2.0f) - 1.0f;
            return folded * compensation;
        }

        default:
            return std::tanh (driven) * compensation;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EightOhEightGloProAudioProcessor();
}
