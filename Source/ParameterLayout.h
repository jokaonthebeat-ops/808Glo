#pragma once

#include <JuceHeader.h>

#include "DSP/Voice.h"

namespace glo
{
namespace ids
{
inline constexpr auto voiceMode = "voiceMode";
inline constexpr auto legato = "legato";
inline constexpr auto triggerMode = "triggerMode";
inline constexpr auto velocitySens = "velocitySens";
inline constexpr auto tune = "tune";
inline constexpr auto fine = "fine";
inline constexpr auto bendRange = "bendRange";
inline constexpr auto glide = "glide";
inline constexpr auto body = "body";
inline constexpr auto harmonics = "harmonics";
inline constexpr auto harmonicBalance = "harmonicBalance";
inline constexpr auto pitchDrop = "pitchDrop";
inline constexpr auto pitchDecay = "pitchDecay";
inline constexpr auto pitchCurve = "pitchCurve";
inline constexpr auto attack = "attack";
inline constexpr auto hold = "hold";
inline constexpr auto decay = "decay";
inline constexpr auto sustain = "sustain";
inline constexpr auto release = "release";
inline constexpr auto ampCurve = "ampCurve";
inline constexpr auto click = "click";
inline constexpr auto clickTone = "clickTone";
inline constexpr auto clickDecay = "clickDecay";
inline constexpr auto punch = "punch";
inline constexpr auto tone = "tone";
inline constexpr auto toneKeytrack = "toneKeytrack";
inline constexpr auto drive = "drive";
inline constexpr auto driveMode = "driveMode";
inline constexpr auto compressor = "compressor";
inline constexpr auto clipper = "clipper";
inline constexpr auto output = "output";
} // namespace ids

struct ParameterPointers
{
    explicit ParameterPointers (juce::AudioProcessorValueTreeState& state);

    std::atomic<float>* voiceMode {};
    std::atomic<float>* legato {};
    std::atomic<float>* triggerMode {};
    std::atomic<float>* velocitySens {};
    std::atomic<float>* tune {};
    std::atomic<float>* fine {};
    std::atomic<float>* bendRange {};
    std::atomic<float>* glide {};
    std::atomic<float>* body {};
    std::atomic<float>* harmonics {};
    std::atomic<float>* harmonicBalance {};
    std::atomic<float>* pitchDrop {};
    std::atomic<float>* pitchDecay {};
    std::atomic<float>* pitchCurve {};
    std::atomic<float>* attack {};
    std::atomic<float>* hold {};
    std::atomic<float>* decay {};
    std::atomic<float>* sustain {};
    std::atomic<float>* release {};
    std::atomic<float>* ampCurve {};
    std::atomic<float>* click {};
    std::atomic<float>* clickTone {};
    std::atomic<float>* clickDecay {};
    std::atomic<float>* punch {};
    std::atomic<float>* tone {};
    std::atomic<float>* toneKeytrack {};
    std::atomic<float>* drive {};
    std::atomic<float>* driveMode {};
    std::atomic<float>* compressor {};
    std::atomic<float>* clipper {};
    std::atomic<float>* output {};

    [[nodiscard]] dsp::VoiceParameters getVoiceParameters() const noexcept;
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
} // namespace glo

