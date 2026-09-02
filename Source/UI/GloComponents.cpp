#include "GloComponents.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
juce::Font componentFont (float height, bool bold = false)
{
    return juce::Font { juce::FontOptions { height,
                                            bold ? juce::Font::bold : juce::Font::plain } };
}

float sanitiseLevel (float value) noexcept
{
    return std::isfinite (value) ? std::max (0.0f, value) : 0.0f;
}

float levelToProportion (float linearLevel) noexcept
{
    const auto decibels = juce::Decibels::gainToDecibels (sanitiseLevel (linearLevel), -60.0f);
    return juce::jmap (juce::jlimit (-60.0f, 0.0f, decibels), -60.0f, 0.0f, 0.0f, 1.0f);
}

int responsiveMetric (int extent, float proportion, int minimum, int maximum) noexcept
{
    if (extent <= 0)
        return 0;

    return juce::jlimit (minimum, maximum,
                         juce::roundToInt (static_cast<float> (extent) * proportion));
}
} // namespace

namespace glo::ui
{
GloCard::GloCard (juce::String title, juce::String subtitle)
    : cardTitle (std::move (title)),
      cardSubtitle (std::move (subtitle))
{
    setOpaque (false);
    setName (cardTitle);
    juce::Component::setTitle (cardTitle);
    setDescription (cardSubtitle);
}

void GloCard::paint (juce::Graphics& g)
{
    GloLookAndFeel::drawCardBackground (g, getLocalBounds().toFloat(), 16.0f, emphasised);

    if (cardTitle.isEmpty())
        return;

    const auto localBounds = getLocalBounds();
    const auto horizontalInset = responsiveMetric (localBounds.getWidth(), 0.045f, 10, 18);
    const auto verticalInset = responsiveMetric (localBounds.getHeight(), 0.075f, 8, 12);
    const auto headingHeight = responsiveMetric (localBounds.getHeight(), 0.20f, 24, 32);
    auto heading = localBounds.reduced (horizontalInset, verticalInset).removeFromTop (headingHeight);
    g.setColour (GloPalette::textPrimary());
    g.setFont (componentFont (14.0f, true));
    g.drawFittedText (cardTitle.toUpperCase(), heading.removeFromLeft (heading.getWidth() * 2 / 3),
                      juce::Justification::centredLeft, 1, 0.82f);

    if (cardSubtitle.isNotEmpty())
    {
        g.setColour (GloPalette::textMuted());
        g.setFont (componentFont (10.5f, true));
        g.drawFittedText (cardSubtitle.toUpperCase(), heading,
                          juce::Justification::centredRight, 1, 0.82f);
    }
}

void GloCard::setTitle (juce::String newTitle)
{
    if (cardTitle == newTitle)
        return;

    cardTitle = std::move (newTitle);
    setName (cardTitle);
    juce::Component::setTitle (cardTitle);
    repaint();
}

void GloCard::setSubtitle (juce::String newSubtitle)
{
    if (cardSubtitle == newSubtitle)
        return;

    cardSubtitle = std::move (newSubtitle);
    setDescription (cardSubtitle);
    repaint();
}

void GloCard::setEmphasised (bool shouldEmphasise)
{
    if (emphasised == shouldEmphasise)
        return;

    emphasised = shouldEmphasise;
    repaint();
}

juce::Rectangle<int> GloCard::getContentBounds() const
{
    const auto localBounds = getLocalBounds();
    const auto horizontalInset = responsiveMetric (localBounds.getWidth(), 0.045f, 10, 18);
    const auto verticalInset = responsiveMetric (localBounds.getHeight(), 0.075f, 8, 12);
    const auto headingHeight = responsiveMetric (localBounds.getHeight(), 0.20f, 24, 32);
    auto bounds = localBounds.reduced (horizontalInset, verticalInset);
    if (cardTitle.isNotEmpty())
        bounds.removeFromTop (headingHeight);
    return bounds;
}

GloLogo::GloLogo()
{
    setOpaque (false);
    setInterceptsMouseClicks (false, false);
    setAccessible (true);
    setName ("808Glo Pro");
    setTitle ("808Glo Pro by Diamond Loopz");
    setDescription ("Synthesized 808 instrument by Diamond Loopz");
}

void GloLogo::paint (juce::Graphics& g)
{
    GloLookAndFeel::drawBrandLogo (g, getLocalBounds().toFloat(), signalIntensity);
}

void GloLogo::setSignalIntensity (float newIntensity)
{
    const auto clamped = juce::jlimit (0.0f, 1.0f, newIntensity);
    if (std::abs (clamped - signalIntensity) < 0.002f)
        return;

    signalIntensity = clamped;
    repaint();
}

ScopeComponent::ScopeComponent()
{
    setOpaque (false);
    setAccessible (true);
    setName ("Oscilloscope");
    setTitle ("Live output waveform");
    setDescription ("Displays the recent 808 output waveform, peak level, and RMS level");
    setHelpText ("The display is visual only and does not alter the sound");
    updateTimerState();
}

ScopeComponent::~ScopeComponent()
{
    stopTimer();
    sampleProvider = {};
}

void ScopeComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.52f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 3.0f), 12.0f);

    juce::ColourGradient screenGradient (GloPalette::panelRaised().darker (0.18f),
                                         bounds.getX(), bounds.getY(),
                                         GloPalette::canvas().darker (0.32f),
                                         bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (screenGradient);
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (GloPalette::border());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

    auto content = bounds.reduced (14.0f, 9.0f);
    auto header = content.removeFromTop (24.0f);
    auto footer = content.removeFromBottom (24.0f);
    auto plot = content.reduced (0.0f, 5.0f);

    g.setColour (GloPalette::textSecondary());
    g.setFont (componentFont (10.5f, true));
    g.drawFittedText ("LIVE SIGNAL", header.toNearestInt(),
                      juce::Justification::centredLeft, 1, 0.82f);
    g.setColour (frozen ? GloPalette::warning() : GloPalette::accent());
    g.drawFittedText (frozen ? "FROZEN" : "OSCILLOSCOPE", header.toNearestInt(),
                      juce::Justification::centredRight, 1, 0.82f);

    drawGrid (g, plot);
    drawWaveform (g, plot);

    const auto peakDb = juce::Decibels::gainToDecibels (displayedPeak, -100.0f);
    const auto rmsDb = juce::Decibels::gainToDecibels (displayedRms, -100.0f);
    const auto status = "RMS " + juce::String (rmsDb, 1) + " dB    PEAK "
                      + juce::String (peakDb, 1) + " dB";
    g.setColour (GloPalette::textMuted());
    g.setFont (componentFont (10.0f, false));
    g.drawFittedText (status, footer.toNearestInt(),
                      juce::Justification::centredRight, 1, 0.78f);

    g.setColour (GloPalette::accent().withAlpha (0.60f));
    g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f)
                       .withCentre ({ footer.getX() + 3.0f, footer.getCentreY() }));
    g.setColour (GloPalette::textMuted());
    g.drawFittedText ("OUTPUT", footer.withTrimmedLeft (11.0f).toNearestInt(),
                      juce::Justification::centredLeft, 1, 0.82f);
}

void ScopeComponent::resized()
{
    repaint();
}

void ScopeComponent::visibilityChanged()
{
    updateTimerState();
}

void ScopeComponent::parentHierarchyChanged()
{
    updateTimerState();
}

void ScopeComponent::setSampleProvider (SampleProvider newProvider)
{
    sampleProvider = std::move (newProvider);
    clear();
}

void ScopeComponent::setRefreshRateHz (int newRateHz)
{
    refreshRateHz = juce::jlimit (10, 60, newRateHz);
    updateTimerState();
}

void ScopeComponent::setReducedMotion (bool shouldReduceMotion)
{
    reducedMotion = shouldReduceMotion;
    updateTimerState();
}

void ScopeComponent::setFrozen (bool shouldFreeze)
{
    if (frozen == shouldFreeze)
        return;

    frozen = shouldFreeze;
    repaint();
}

void ScopeComponent::setTraceGain (float newGain)
{
    traceGain = juce::jlimit (0.1f, 8.0f, newGain);
    repaint();
}

void ScopeComponent::clear()
{
    sampleBuffer.fill (0.0f);
    validSampleCount = 0;
    displayedPeak = 0.0f;
    displayedRms = 0.0f;
    repaint();
}

void ScopeComponent::timerCallback()
{
    if (! isShowing())
    {
        stopTimer();
        return;
    }

    if (frozen || ! sampleProvider)
        return;

    const auto copied = sampleProvider (sampleBuffer.data(), sampleCapacity);
    validSampleCount = juce::jlimit (0, sampleCapacity, copied);

    if (validSampleCount == 0)
    {
        displayedPeak *= 0.82f;
        displayedRms *= 0.88f;
        repaint();
        return;
    }

    auto peak = 0.0f;
    auto sumOfSquares = 0.0;
    for (int index = 0; index < validSampleCount; ++index)
    {
        const auto sample = sanitiseLevel (std::abs (sampleBuffer[static_cast<std::size_t> (index)]));
        peak = std::max (peak, sample);
        sumOfSquares += static_cast<double> (sample) * static_cast<double> (sample);
    }

    const auto rms = static_cast<float> (std::sqrt (sumOfSquares
                                  / static_cast<double> (std::max (1, validSampleCount))));
    displayedPeak = peak >= displayedPeak ? peak : displayedPeak * 0.86f + peak * 0.14f;
    displayedRms += (rms - displayedRms) * (rms >= displayedRms ? 0.72f : 0.16f);
    repaint();
}

void ScopeComponent::updateTimerState()
{
    if (! isShowing())
    {
        stopTimer();
        return;
    }

    startTimerHz (reducedMotion ? std::min (20, refreshRateHz) : refreshRateHz);
}

void ScopeComponent::drawGrid (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    g.saveState();
    g.reduceClipRegion (bounds.toNearestInt());

    for (int division = 0; division <= 8; ++division)
    {
        const auto x = juce::jmap (static_cast<float> (division) / 8.0f,
                                   bounds.getX(), bounds.getRight());
        g.setColour (GloPalette::accent().withAlpha (division == 4 ? 0.095f : 0.040f));
        g.drawVerticalLine (juce::roundToInt (x), bounds.getY(), bounds.getBottom());
    }

    for (int division = 0; division <= 4; ++division)
    {
        const auto y = juce::jmap (static_cast<float> (division) / 4.0f,
                                   bounds.getY(), bounds.getBottom());
        g.setColour (GloPalette::accent().withAlpha (division == 2 ? 0.13f : 0.048f));
        g.drawHorizontalLine (juce::roundToInt (y), bounds.getX(), bounds.getRight());
    }

    g.restoreState();
}

void ScopeComponent::drawWaveform (juce::Graphics& g, juce::Rectangle<float> bounds) const
{
    const auto centreY = bounds.getCentreY();
    if (validSampleCount <= 1)
    {
        g.setColour (GloPalette::accent().withAlpha (0.34f));
        g.drawHorizontalLine (juce::roundToInt (centreY), bounds.getX(), bounds.getRight());
        return;
    }

    const auto columns = std::max (2, juce::roundToInt (bounds.getWidth()));
    juce::Path envelopePath;
    juce::Path tracePath;

    for (int column = 0; column < columns; ++column)
    {
        const auto first = column * validSampleCount / columns;
        const auto last = std::max (first + 1, (column + 1) * validSampleCount / columns);
        auto minimum = 1.0f;
        auto maximum = -1.0f;
        auto sum = 0.0f;

        for (int sampleIndex = first; sampleIndex < std::min (last, validSampleCount); ++sampleIndex)
        {
            const auto value = juce::jlimit (-1.0f, 1.0f,
                                             sampleBuffer[static_cast<std::size_t> (sampleIndex)]
                                                 * traceGain);
            minimum = std::min (minimum, value);
            maximum = std::max (maximum, value);
            sum += value;
        }

        const auto count = std::max (1, std::min (last, validSampleCount) - first);
        const auto average = sum / static_cast<float> (count);
        const auto x = juce::jmap (static_cast<float> (column), 0.0f,
                                   static_cast<float> (columns - 1), bounds.getX(), bounds.getRight());
        const auto minimumY = centreY - minimum * bounds.getHeight() * 0.44f;
        const auto maximumY = centreY - maximum * bounds.getHeight() * 0.44f;
        const auto averageY = centreY - average * bounds.getHeight() * 0.44f;

        envelopePath.startNewSubPath (x, minimumY);
        envelopePath.lineTo (x, maximumY);

        if (column == 0)
            tracePath.startNewSubPath (x, averageY);
        else
            tracePath.lineTo (x, averageY);
    }

    g.saveState();
    g.reduceClipRegion (bounds.toNearestInt());
    g.setColour (GloPalette::accent().withAlpha (0.16f));
    g.strokePath (envelopePath, juce::PathStrokeType (1.0f));

    if (! reducedMotion)
    {
        g.setColour (GloPalette::accent().withAlpha (0.10f));
        g.strokePath (tracePath, juce::PathStrokeType (6.0f,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    }

    g.setColour (GloPalette::accentHot().withAlpha (0.92f));
    g.strokePath (tracePath, juce::PathStrokeType (1.65f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    g.restoreState();
}

StereoMeter::StereoMeter()
{
    setOpaque (false);
    setAccessible (true);
    setName ("Stereo output meter");
    setTitle ("Stereo output level");
    setDescription ("Left and right output peak meters. Click the clip indicator to reset it");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    updateTimerState();
}

StereoMeter::~StereoMeter()
{
    stopTimer();
    leftLevelSource = nullptr;
    rightLevelSource = nullptr;
}

void StereoMeter::paint (juce::Graphics& g)
{
    GloLookAndFeel::drawCardBackground (g, getLocalBounds().toFloat(), 10.0f, false);
    auto content = getLocalBounds().toFloat().reduced (8.0f, 7.0f);
    auto clipArea = content.removeFromTop (20.0f);
    auto meterArea = content.reduced (1.0f, 5.0f);
    const auto laneGap = std::max (4.0f, meterArea.getWidth() * 0.08f);
    const auto laneWidth = (meterArea.getWidth() - laneGap) * 0.5f;
    auto leftLane = meterArea.removeFromLeft (laneWidth);
    meterArea.removeFromLeft (laneGap);
    auto rightLane = meterArea.removeFromLeft (laneWidth);

    g.setColour (clipLatched ? GloPalette::clip() : GloPalette::border());
    g.fillRoundedRectangle (clipArea.reduced (2.0f, 3.0f), 4.0f);
    g.setFont (componentFont (9.0f, true));
    g.setColour (clipLatched ? GloPalette::canvas() : GloPalette::textMuted());
    g.drawFittedText (clipLatched ? "CLIP" : "PEAK", clipArea.toNearestInt(),
                      juce::Justification::centred, 1, 0.82f);

    drawMeterLane (g, leftLane, displayedLeft, heldLeft, "L");
    drawMeterLane (g, rightLane, displayedRight, heldRight, "R");
}

void StereoMeter::mouseDown (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    clearClip();
    repaint();
}

void StereoMeter::visibilityChanged()
{
    updateTimerState();
}

void StereoMeter::parentHierarchyChanged()
{
    updateTimerState();
}

void StereoMeter::setLevelSources (const std::atomic<float>* leftSource,
                                   const std::atomic<float>* rightSource) noexcept
{
    JUCE_ASSERT_MESSAGE_MANAGER_IS_LOCKED
    leftLevelSource = leftSource != nullptr ? leftSource : &ownedLeft;
    rightLevelSource = rightSource != nullptr ? rightSource : &ownedRight;
}

void StereoMeter::setLevels (float leftLinear, float rightLinear, bool ceilingWasHit) noexcept
{
    ownedLeft.store (sanitiseLevel (leftLinear), std::memory_order_relaxed);
    ownedRight.store (sanitiseLevel (rightLinear), std::memory_order_relaxed);
    if (ceilingWasHit)
        ownedCeilingHit.store (true, std::memory_order_release);
}

void StereoMeter::setRefreshRateHz (int newRateHz)
{
    refreshRateHz = juce::jlimit (10, 60, newRateHz);
    updateTimerState();
}

void StereoMeter::clearClip() noexcept
{
    clipLatched = false;
    ownedCeilingHit.store (false, std::memory_order_release);
}

void StereoMeter::timerCallback()
{
    if (! isShowing())
    {
        stopTimer();
        return;
    }

    clipLatched = clipLatched || ownedCeilingHit.exchange (false, std::memory_order_acq_rel);

    const auto targetLeft = leftLevelSource != nullptr
                          ? sanitiseLevel (leftLevelSource->load (std::memory_order_relaxed)) : 0.0f;
    const auto targetRight = rightLevelSource != nullptr
                           ? sanitiseLevel (rightLevelSource->load (std::memory_order_relaxed)) : 0.0f;

    const auto follow = [] (float current, float target)
    {
        return target >= current ? target : current * 0.86f + target * 0.14f;
    };
    displayedLeft = follow (displayedLeft, targetLeft);
    displayedRight = follow (displayedRight, targetRight);

    const auto updateHold = [this] (float target, float& held, int& frames)
    {
        if (target >= held)
        {
            held = target;
            frames = refreshRateHz;
        }
        else if (frames > 0)
        {
            --frames;
        }
        else
        {
            held *= 0.94f;
        }
    };
    updateHold (targetLeft, heldLeft, leftHoldFrames);
    updateHold (targetRight, heldRight, rightHoldFrames);

    // The processor applies a final -0.8 dBFS safety ceiling, so 1.0 can never
    // reach this meter. Latch when that ceiling is reached instead.
    constexpr auto outputCeiling = 0.91201085f;
    clipLatched = clipLatched || targetLeft >= outputCeiling || targetRight >= outputCeiling;
    repaint();
}

void StereoMeter::updateTimerState()
{
    if (! isShowing())
    {
        stopTimer();
        return;
    }

    startTimerHz (refreshRateHz);
}

void StereoMeter::drawMeterLane (juce::Graphics& g,
                                 juce::Rectangle<float> bounds,
                                 float linearLevel,
                                 float heldLevel,
                                 juce::StringRef label) const
{
    auto labelArea = bounds.removeFromBottom (17.0f);
    auto lane = bounds.reduced (1.0f, 0.0f);
    g.setColour (GloPalette::canvas().darker (0.25f));
    g.fillRoundedRectangle (lane, 4.0f);
    g.setColour (GloPalette::border());
    g.drawRoundedRectangle (lane.reduced (0.5f), 4.0f, 1.0f);

    const auto proportion = levelToProportion (linearLevel);
    auto fill = lane.withTop (lane.getBottom() - lane.getHeight() * proportion).reduced (2.0f);
    if (! fill.isEmpty())
    {
        juce::ColourGradient meterGradient (GloPalette::clip(),
                                            lane.getCentreX(), lane.getY(),
                                            GloPalette::accent().darker (0.15f),
                                            lane.getCentreX(), lane.getBottom(), false);
        meterGradient.addColour (0.11, GloPalette::warning());
        meterGradient.addColour (0.30, GloPalette::accentHot());
        g.setGradientFill (meterGradient);
        g.fillRoundedRectangle (fill, 2.5f);
    }

    g.setColour (GloPalette::canvas().withAlpha (0.78f));
    constexpr int segments = 18;
    for (int segment = 1; segment < segments; ++segment)
    {
        const auto y = juce::jmap (static_cast<float> (segment) / static_cast<float> (segments),
                                   lane.getY(), lane.getBottom());
        g.drawHorizontalLine (juce::roundToInt (y), lane.getX() + 1.5f, lane.getRight() - 1.5f);
    }

    const auto holdY = lane.getBottom() - lane.getHeight() * levelToProportion (heldLevel);
    g.setColour (heldLevel >= 1.0f ? GloPalette::clip() : GloPalette::textPrimary());
    g.drawHorizontalLine (juce::roundToInt (holdY), lane.getX() + 1.0f, lane.getRight() - 1.0f);

    g.setColour (GloPalette::textSecondary());
    g.setFont (componentFont (10.0f, true));
    g.drawFittedText (label.text, labelArea.toNearestInt(),
                      juce::Justification::centred, 1, 0.82f);
}

ParameterKnob::ParameterKnob (juce::AudioProcessorValueTreeState& state,
                              const juce::String& parameterID,
                              juce::String displayName)
{
    nameLabel.setText (displayName, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setFont (componentFont (11.0f, true));
    nameLabel.setColour (juce::Label::textColourId, GloPalette::textSecondary());
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.20f,
                                juce::MathConstants<float>::pi * 2.80f,
                                true);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 88, 22);
    slider.setMouseDragSensitivity (240);
    slider.setWantsKeyboardFocus (true);
    slider.setPopupDisplayEnabled (true, false, this);
    slider.setTitle (displayName);
    slider.setDescription ("Adjust " + displayName);
    slider.setName (displayName);
    addAndMakeVisible (slider);

    if (auto* parameter = state.getParameter (parameterID))
    {
        const auto range = parameter->getNormalisableRange();
        slider.setRange (static_cast<double> (range.start),
                         static_cast<double> (range.end),
                         static_cast<double> (range.interval));
        slider.setSkewFactor (static_cast<double> (range.skew), range.symmetricSkew);
        slider.setDoubleClickReturnValue (true,
                                          static_cast<double> (parameter->convertFrom0to1 (
                                              parameter->getDefaultValue())));

        slider.textFromValueFunction = [parameter] (double value)
        {
            return parameter->getText (parameter->convertTo0to1 (static_cast<float> (value)), 64);
        };
        slider.valueFromTextFunction = [parameter] (const juce::String& text)
        {
            return static_cast<double> (parameter->convertFrom0to1 (parameter->getValueForText (text)));
        };

        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            state, parameterID, slider);
    }
    else
    {
        jassertfalse;
        slider.setEnabled (false);
    }
}

void ParameterKnob::resized()
{
    auto bounds = getLocalBounds();
    const auto labelHeight = std::min (bounds.getHeight(),
                                       responsiveMetric (bounds.getHeight(),
                                                         featured ? 0.12f : 0.18f,
                                                         featured ? 16 : 12,
                                                         featured ? 21 : 18));
    nameLabel.setFont (componentFont (featured
                                         ? static_cast<float> (responsiveMetric (getWidth(), 0.075f, 10, 13))
                                         : static_cast<float> (responsiveMetric (getWidth(), 0.10f, 9, 11)),
                                      true));
    nameLabel.setBounds (bounds.removeFromTop (labelHeight));

    const auto titleGap = responsiveMetric (bounds.getHeight(), featured ? 0.04f : 0.025f,
                                             featured ? 4 : 2, featured ? 7 : 4);
    bounds.removeFromTop (std::min (titleGap, bounds.getHeight()));

    const auto horizontalInset = featured ? responsiveMetric (bounds.getWidth(), 0.025f, 2, 5) : 1;
    auto sliderBounds = bounds.reduced (horizontalInset, 0);
    if (! sliderBounds.isEmpty())
    {
        const auto textBoxWidth = std::max (1, std::min (88, sliderBounds.getWidth() - 2));
        const auto textBoxHeight = responsiveMetric (sliderBounds.getHeight(), 0.28f, 12, 22);

        // Tall master cells need a compact title/dial/readout stack. Without this cap,
        // JUCE centres the rotary stage in all remaining height and creates dead space
        // between the dial and its readout even though the zones do not technically overlap.
        if (! featured)
        {
            const auto readoutGap = responsiveMetric (sliderBounds.getHeight(), 0.035f, 2, 6);
            const auto compactHeight = sliderBounds.getWidth() + textBoxHeight + readoutGap;
            if (sliderBounds.getHeight() > compactHeight)
                sliderBounds.setHeight (compactHeight);
        }

        slider.setBounds (sliderBounds);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false,
                                textBoxWidth, textBoxHeight);
    }
    else
    {
        slider.setBounds (sliderBounds);
    }
}

void ParameterKnob::paint (juce::Graphics& g)
{
    if (! featured)
        return;

    const auto width = static_cast<float> (getWidth());
    const auto railWidth = std::min (42.0f, width * 0.30f);
    const auto centreX = width * 0.5f;
    const auto y = static_cast<float> (nameLabel.getBottom()) - 1.0f;

    juce::ColourGradient rail (juce::Colours::transparentBlack, centreX - railWidth, y,
                               juce::Colours::transparentBlack, centreX + railWidth, y, false);
    rail.addColour (0.40, GloPalette::accent().withAlpha (0.18f));
    rail.addColour (0.50, GloPalette::accent().withAlpha (0.46f));
    rail.addColour (0.60, GloPalette::accent().withAlpha (0.18f));
    g.setGradientFill (rail);
    g.drawHorizontalLine (juce::roundToInt (y), centreX - railWidth, centreX + railWidth);

    juce::Path notch;
    notch.startNewSubPath (centreX - 2.5f, y - 2.5f);
    notch.lineTo (centreX + 2.5f, y);
    notch.lineTo (centreX - 2.5f, y + 2.5f);
    notch.closeSubPath();
    g.setColour (GloPalette::accentHot().withAlpha (0.78f));
    g.fillPath (notch);
}

void ParameterKnob::setFeatured (bool shouldBeFeatured)
{
    if (featured == shouldBeFeatured)
        return;

    featured = shouldBeFeatured;
    nameLabel.setColour (juce::Label::textColourId,
                         featured ? GloPalette::accentHot() : GloPalette::textSecondary());
    resized();
    repaint();
}

ParameterToggle::ParameterToggle (juce::AudioProcessorValueTreeState& state,
                                  const juce::String& parameterID,
                                  juce::String displayName)
{
    button.setButtonText (displayName);
    button.setClickingTogglesState (true);
    button.setWantsKeyboardFocus (true);
    button.setTitle (displayName);
    button.setDescription ("Toggle " + displayName);
    addAndMakeVisible (button);

    if (state.getParameter (parameterID) != nullptr)
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            state, parameterID, button);
    else
    {
        jassertfalse;
        button.setEnabled (false);
    }
}

void ParameterToggle::resized()
{
    const auto bounds = getLocalBounds();
    const auto inset = responsiveMetric (std::min (bounds.getWidth(), bounds.getHeight()),
                                         0.055f, 2, 4);
    button.setBounds (bounds.reduced (inset));
}

ParameterChoice::ParameterChoice (juce::AudioProcessorValueTreeState& state,
                                  const juce::String& parameterID,
                                  juce::String displayName,
                                  const juce::StringArray& choices)
{
    nameLabel.setText (displayName, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centredLeft);
    nameLabel.setFont (componentFont (11.0f, true));
    nameLabel.setColour (juce::Label::textColourId, GloPalette::textSecondary());
    nameLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (nameLabel);

    for (int index = 0; index < choices.size(); ++index)
        comboBox.addItem (choices[index], index + 1);

    comboBox.setTextWhenNothingSelected ("SELECT");
    comboBox.setWantsKeyboardFocus (true);
    comboBox.setTitle (displayName);
    comboBox.setDescription ("Choose " + displayName);
    addAndMakeVisible (comboBox);

    if (state.getParameter (parameterID) != nullptr)
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            state, parameterID, comboBox);
    else
    {
        jassertfalse;
        comboBox.setEnabled (false);
    }
}

void ParameterChoice::resized()
{
    const auto localBounds = getLocalBounds();
    const auto inset = responsiveMetric (std::min (localBounds.getWidth(), localBounds.getHeight()),
                                         0.055f, 2, 4);
    auto bounds = localBounds.reduced (inset);
    const auto labelHeight = std::min (bounds.getHeight(),
                                       responsiveMetric (bounds.getHeight(), 0.30f, 14, 20));
    const auto controlHeight = std::min (36, std::max (0, bounds.getHeight() - labelHeight));
    const auto groupHeight = std::min (bounds.getHeight(), labelHeight + controlHeight + 3);
    bounds = bounds.withSizeKeepingCentre (bounds.getWidth(), groupHeight);
    nameLabel.setBounds (bounds.removeFromTop (labelHeight));
    bounds.removeFromTop (std::min (3, bounds.getHeight()));
    comboBox.setBounds (bounds.removeFromTop (std::min (controlHeight, bounds.getHeight())));
}

ParameterPanel::ParameterPanel (juce::String title,
                                juce::AudioProcessorValueTreeState& parameterState)
    : GloCard (std::move (title)),
      state (parameterState)
{
}

ParameterKnob& ParameterPanel::addKnob (const juce::String& parameterID,
                                        juce::String displayName)
{
    return addControl<ParameterKnob> (state, parameterID, std::move (displayName));
}

ParameterToggle& ParameterPanel::addToggle (const juce::String& parameterID,
                                            juce::String displayName)
{
    return addControl<ParameterToggle> (state, parameterID, std::move (displayName));
}

ParameterChoice& ParameterPanel::addChoice (const juce::String& parameterID,
                                            juce::String displayName,
                                            const juce::StringArray& choices)
{
    return addControl<ParameterChoice> (state, parameterID, std::move (displayName), choices);
}

void ParameterPanel::setColumns (int newColumnCount)
{
    columns = std::max (1, newColumnCount);
    resized();
}

void ParameterPanel::resized()
{
    if (controls.empty())
        return;

    auto area = getContentBounds().reduced (1);
    const auto columnCount = juce::jlimit (1, static_cast<int> (controls.size()), columns);
    const auto nominalColumnWidth = area.getWidth() / columnCount;
    const auto gap = nominalColumnWidth < 56 ? 3 : nominalColumnWidth < 76 ? 5 : 8;
    const auto rowCount = (static_cast<int> (controls.size()) + columnCount - 1) / columnCount;
    const auto cellWidth = std::max (1, (area.getWidth() - gap * (columnCount - 1)) / columnCount);
    const auto cellHeight = std::max (1, (area.getHeight() - gap * (rowCount - 1)) / rowCount);

    for (std::size_t index = 0; index < controls.size(); ++index)
    {
        const auto column = static_cast<int> (index) % columnCount;
        const auto row = static_cast<int> (index) / columnCount;
        controls[index]->setBounds (area.getX() + column * (cellWidth + gap),
                                    area.getY() + row * (cellHeight + gap),
                                    cellWidth,
                                    cellHeight);
    }
}
} // namespace glo::ui
