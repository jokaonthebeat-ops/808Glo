#include "GloLookAndFeel.h"

#include "808GloBinaryData.h"

#include <algorithm>
#include <cmath>

namespace
{
juce::Font makeFont (float height, bool bold = false)
{
    return juce::Font { juce::FontOptions { height,
                                            bold ? juce::Font::bold : juce::Font::plain } };
}

juce::Point<float> polarPoint (juce::Point<float> centre, float radius, float angle)
{
    return { centre.x + std::sin (angle) * radius,
             centre.y - std::cos (angle) * radius };
}

} // namespace

namespace glo::ui
{
juce::Colour GloPalette::canvas() noexcept          { return juce::Colour::fromRGB (7, 10, 9); }
juce::Colour GloPalette::panel() noexcept           { return juce::Colour::fromRGB (13, 18, 16); }
juce::Colour GloPalette::panelRaised() noexcept     { return juce::Colour::fromRGB (18, 26, 22); }
juce::Colour GloPalette::controlWell() noexcept     { return juce::Colour::fromRGB (24, 34, 29); }
juce::Colour GloPalette::controlHover() noexcept    { return juce::Colour::fromRGB (29, 43, 36); }
juce::Colour GloPalette::border() noexcept          { return juce::Colour::fromRGB (40, 53, 46); }
juce::Colour GloPalette::borderHighlight() noexcept { return juce::Colour::fromRGB (57, 74, 64); }
juce::Colour GloPalette::accent() noexcept          { return juce::Colour::fromRGB (105, 255, 120); }
juce::Colour GloPalette::accentHot() noexcept       { return juce::Colour::fromRGB (179, 255, 103); }
juce::Colour GloPalette::textPrimary() noexcept     { return juce::Colour::fromRGB (242, 247, 243); }
juce::Colour GloPalette::textSecondary() noexcept   { return juce::Colour::fromRGB (170, 182, 175); }
juce::Colour GloPalette::textMuted() noexcept       { return juce::Colour::fromRGB (128, 144, 135); }
juce::Colour GloPalette::warning() noexcept         { return juce::Colour::fromRGB (255, 196, 77); }
juce::Colour GloPalette::clip() noexcept            { return juce::Colour::fromRGB (255, 77, 91); }

GloLookAndFeel::GloLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, GloPalette::canvas());

    setColour (juce::Slider::textBoxTextColourId, GloPalette::textPrimary());
    setColour (juce::Slider::textBoxBackgroundColourId, GloPalette::panelRaised());
    setColour (juce::Slider::textBoxOutlineColourId, GloPalette::border());
    setColour (juce::Slider::textBoxHighlightColourId, GloPalette::accent().withAlpha (0.42f));
    setColour (juce::Slider::rotarySliderFillColourId, GloPalette::accent());
    setColour (juce::Slider::rotarySliderOutlineColourId, GloPalette::border());

    setColour (juce::TextButton::buttonColourId, GloPalette::controlWell());
    setColour (juce::TextButton::buttonOnColourId, GloPalette::accent());
    setColour (juce::TextButton::textColourOffId, GloPalette::textSecondary());
    setColour (juce::TextButton::textColourOnId, GloPalette::canvas());

    setColour (juce::ToggleButton::textColourId, GloPalette::textSecondary());
    setColour (juce::ComboBox::backgroundColourId, GloPalette::controlWell());
    setColour (juce::ComboBox::outlineColourId, GloPalette::border());
    setColour (juce::ComboBox::textColourId, GloPalette::textPrimary());
    setColour (juce::ComboBox::arrowColourId, GloPalette::accent());
    setColour (juce::ComboBox::focusedOutlineColourId, GloPalette::accentHot());

    setColour (juce::Label::textColourId, GloPalette::textSecondary());
    setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textWhenEditingColourId, GloPalette::textPrimary());
    setColour (juce::Label::backgroundWhenEditingColourId, GloPalette::panelRaised());
    setColour (juce::Label::outlineWhenEditingColourId, GloPalette::accent());

    setColour (juce::PopupMenu::backgroundColourId, GloPalette::panelRaised());
    setColour (juce::PopupMenu::textColourId, GloPalette::textPrimary());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, GloPalette::accent().withAlpha (0.20f));
    setColour (juce::PopupMenu::highlightedTextColourId, GloPalette::accentHot());
}

void GloLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                       int x,
                                       int y,
                                       int width,
                                       int height,
                                       float sliderPosition,
                                       float rotaryStartAngle,
                                       float rotaryEndAngle,
                                       juce::Slider& slider)
{
    const auto alpha = slider.isEnabled() ? 1.0f : 0.42f;
    auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                          static_cast<float> (y),
                                          static_cast<float> (width),
                                          static_cast<float> (height))
                      .reduced (5.0f);
    const auto diameter = std::min (bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre (diameter, diameter);

    if (diameter <= 8.0f)
        return;

    const auto centre = bounds.getCentre();
    const auto radius = diameter * 0.5f;
    const auto angle = juce::jmap (juce::jlimit (0.0f, 1.0f, sliderPosition),
                                   rotaryStartAngle,
                                   rotaryEndAngle);
    const auto isHot = slider.isMouseOverOrDragging();
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId)
                              .withMultipliedAlpha (alpha);

    g.setColour (juce::Colours::black.withAlpha (0.52f * alpha));
    g.fillEllipse (bounds.translated (0.0f, radius * 0.09f).expanded (radius * 0.03f));

    const auto arcRadius = radius * 0.89f;
    juce::Path track;
    track.addCentredArc (centre.x, centre.y,
                         arcRadius, arcRadius,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (GloPalette::border().withMultipliedAlpha (alpha));
    g.strokePath (track, juce::PathStrokeType (std::max (2.5f, radius * 0.09f),
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    if (sliderPosition > 0.001f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (centre.x, centre.y,
                                arcRadius, arcRadius,
                                0.0f, rotaryStartAngle, angle, true);

        if (isHot)
        {
            g.setColour (accent.withAlpha (0.16f * alpha));
            g.strokePath (valueArc, juce::PathStrokeType (std::max (7.0f, radius * 0.18f),
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
        }

        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (std::max (2.5f, radius * 0.075f),
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    if (radius >= 29.0f)
    {
        for (int tick = 0; tick <= 10; ++tick)
        {
            const auto tickAngle = juce::jmap (static_cast<float> (tick) / 10.0f,
                                               rotaryStartAngle,
                                               rotaryEndAngle);
            const auto inner = polarPoint (centre, radius * 0.74f, tickAngle);
            const auto outer = polarPoint (centre, radius * 0.80f, tickAngle);
            g.setColour ((tick == 5 ? GloPalette::textSecondary() : GloPalette::borderHighlight())
                             .withAlpha (0.66f * alpha));
            g.drawLine ({ inner, outer }, tick == 5 ? 1.5f : 1.0f);
        }
    }

    const auto capBounds = bounds.withSizeKeepingCentre (diameter * 0.70f, diameter * 0.70f);
    g.setColour (juce::Colours::black.withAlpha (0.72f * alpha));
    g.fillEllipse (capBounds.expanded (2.0f));

    juce::ColourGradient capGradient (GloPalette::controlHover().brighter (0.12f),
                                      capBounds.getX() + capBounds.getWidth() * 0.24f,
                                      capBounds.getY() + capBounds.getHeight() * 0.18f,
                                      GloPalette::canvas(),
                                      capBounds.getRight(),
                                      capBounds.getBottom(),
                                      true);
    capGradient.addColour (0.58, GloPalette::controlWell());
    g.setGradientFill (capGradient);
    g.fillEllipse (capBounds);

    g.setColour (GloPalette::borderHighlight().withAlpha (0.72f * alpha));
    g.drawEllipse (capBounds.reduced (0.75f), 1.2f);

    const auto pointerStart = polarPoint (centre, radius * 0.17f, angle);
    const auto pointerEnd = polarPoint (centre, radius * 0.52f, angle);
    g.setColour (accent.brighter (0.08f));
    g.drawLine ({ pointerStart, pointerEnd }, std::max (1.8f, radius * 0.055f));
    g.setColour (GloPalette::accentHot().withAlpha (0.88f * alpha));
    g.fillEllipse (juce::Rectangle<float> (3.8f, 3.8f).withCentre (pointerEnd));

    auto capHighlight = capBounds;
    capHighlight.setHeight (capBounds.getHeight() * 0.48f);
    g.setColour (juce::Colours::white.withAlpha (0.055f * alpha));
    g.fillEllipse (capHighlight.reduced (3.0f, 1.0f));

    if (slider.hasKeyboardFocus (true))
    {
        g.setColour (GloPalette::canvas());
        g.drawEllipse (bounds.expanded (2.5f), 1.5f);
        g.setColour (GloPalette::accentHot());
        g.drawEllipse (bounds.expanded (4.5f), 2.0f);
    }
}

juce::Label* GloLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (makeFont (10.5f, true));
    label->setJustificationType (juce::Justification::centred);
    label->setMinimumHorizontalScale (0.74f);
    label->setBorderSize ({ 1, 3, 1, 3 });
    return label;
}

juce::Slider::SliderLayout GloLookAndFeel::getSliderLayout (juce::Slider& slider)
{
    if (slider.getTextBoxPosition() != juce::Slider::TextBoxBelow
        || slider.getSliderStyle() != juce::Slider::RotaryHorizontalVerticalDrag)
        return juce::LookAndFeel_V4::getSliderLayout (slider);

    auto available = slider.getLocalBounds();
    const auto textHeight = std::max (0, std::min (slider.getTextBoxHeight(), available.getHeight() - 12));
    const auto textWidth = std::max (0, std::min (slider.getTextBoxWidth(), available.getWidth() - 2));

    juce::Slider::SliderLayout layout;
    if (textHeight > 0 && textWidth > 0)
    {
        auto footer = available.removeFromBottom (textHeight);
        layout.textBoxBounds = footer.withSizeKeepingCentre (textWidth, textHeight);
    }

    const auto gap = juce::jlimit (2, 6,
                                   juce::roundToInt (static_cast<float> (
                                       std::min (available.getWidth(), available.getHeight())) * 0.045f));
    available.removeFromBottom (std::min (gap, available.getHeight()));
    layout.sliderBounds = available;
    return layout;
}

void GloLookAndFeel::drawButtonBackground (juce::Graphics& g,
                                           juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool isMouseOverButton,
                                           bool isButtonDown)
{
    const auto alpha = button.isEnabled() ? 1.0f : 0.42f;
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const auto corner = std::min (10.0f, bounds.getHeight() * 0.28f);
    const auto isActive = button.getToggleState();

    if (isButtonDown)
        bounds.translate (0.0f, 1.0f);

    g.setColour (juce::Colours::black.withAlpha (0.38f * alpha));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), corner);

    auto top = backgroundColour;
    auto bottom = GloPalette::controlWell().darker (0.22f);
    if (isActive)
    {
        top = GloPalette::accentHot();
        bottom = GloPalette::accent().darker (0.18f);
    }
    else if (isMouseOverButton)
    {
        top = GloPalette::controlHover().brighter (0.08f);
        bottom = GloPalette::controlHover().darker (0.16f);
    }

    juce::ColourGradient gradient (top.withMultipliedAlpha (alpha),
                                   bounds.getCentreX(), bounds.getY(),
                                   bottom.withMultipliedAlpha (alpha),
                                   bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (bounds, corner);

    g.setColour ((isActive ? GloPalette::accentHot() : GloPalette::borderHighlight())
                     .withMultipliedAlpha (alpha));
    g.drawRoundedRectangle (bounds.reduced (0.5f), corner, isActive ? 1.5f : 1.0f);

    g.setColour (juce::Colours::white.withAlpha ((isActive ? 0.17f : 0.055f) * alpha));
    g.drawHorizontalLine (juce::roundToInt (bounds.getY() + 2.0f),
                          bounds.getX() + corner,
                          bounds.getRight() - corner);

    if (button.hasKeyboardFocus (true))
    {
        g.setColour (GloPalette::canvas());
        g.drawRoundedRectangle (bounds.expanded (1.5f), corner + 1.5f, 1.0f);
        g.setColour (GloPalette::accentHot());
        g.drawRoundedRectangle (bounds.expanded (3.0f), corner + 3.0f, 1.5f);
    }
}

void GloLookAndFeel::drawButtonText (juce::Graphics& g,
                                     juce::TextButton& button,
                                     bool isMouseOverButton,
                                     bool isButtonDown)
{
    juce::ignoreUnused (isMouseOverButton, isButtonDown);
    const auto isActive = button.getToggleState();
    auto textColour = button.findColour (isActive ? juce::TextButton::textColourOnId
                                                  : juce::TextButton::textColourOffId);
    if (! button.isEnabled())
        textColour = textColour.withMultipliedAlpha (0.45f);

    g.setColour (textColour);
    g.setFont (getTextButtonFont (button, button.getHeight()));
    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds().reduced (10, 3),
                      juce::Justification::centred,
                      1,
                      0.80f);
}

void GloLookAndFeel::drawToggleButton (juce::Graphics& g,
                                       juce::ToggleButton& button,
                                       bool isMouseOverButton,
                                       bool isButtonDown)
{
    const auto alpha = button.isEnabled() ? 1.0f : 0.42f;
    auto area = button.getLocalBounds().toFloat();
    if (isButtonDown)
        area.translate (0.0f, 1.0f);

    const auto compact = area.getWidth() < 130.0f;
    const auto trackHeight = juce::jlimit (compact ? 17.0f : 18.0f, 24.0f,
                                           std::min (area.getHeight() * 0.40f,
                                                     area.getWidth() * 0.20f));
    const auto trackWidth = trackHeight * (compact ? 1.72f : 1.85f);
    auto track = juce::Rectangle<float> (trackWidth, trackHeight)
                     .withCentre ({ area.getX() + trackWidth * 0.5f + 2.0f, area.getCentreY() });
    const auto isOn = button.getToggleState();

    g.setColour ((isOn ? GloPalette::accent().darker (0.18f)
                       : (isMouseOverButton ? GloPalette::controlHover() : GloPalette::controlWell()))
                     .withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (track, trackHeight * 0.5f);
    g.setColour ((isOn ? GloPalette::accentHot() : GloPalette::borderHighlight())
                     .withMultipliedAlpha (alpha));
    g.drawRoundedRectangle (track.reduced (0.5f), trackHeight * 0.5f, 1.0f);

    const auto thumbDiameter = trackHeight - 5.0f;
    const auto thumbX = isOn ? track.getRight() - thumbDiameter - 2.5f
                             : track.getX() + 2.5f;
    auto thumb = juce::Rectangle<float> (thumbDiameter, thumbDiameter)
                     .withPosition (thumbX, track.getCentreY() - thumbDiameter * 0.5f);
    g.setColour ((isOn ? GloPalette::canvas() : GloPalette::textSecondary())
                     .withMultipliedAlpha (alpha));
    g.fillEllipse (thumb);

    auto textArea = area.withTrimmedLeft (trackWidth + (compact ? 7.0f : 12.0f));
    g.setColour ((isOn ? GloPalette::textPrimary() : GloPalette::textSecondary())
                     .withMultipliedAlpha (alpha));
    g.setFont (makeFont (juce::jlimit (10.0f, 13.0f,
                                       std::min (area.getHeight() * 0.30f,
                                                 area.getWidth() * 0.10f)), true));
    g.drawFittedText (button.getButtonText(), textArea.toNearestInt(),
                      juce::Justification::centredLeft, 1, 0.70f);

    if (button.hasKeyboardFocus (true))
    {
        g.setColour (GloPalette::accentHot());
        g.drawRoundedRectangle (area.reduced (1.0f), 7.0f, 1.5f);
    }
}

void GloLookAndFeel::drawComboBox (juce::Graphics& g,
                                   int width,
                                   int height,
                                   bool isButtonDown,
                                   int buttonX,
                                   int buttonY,
                                   int buttonW,
                                   int buttonH,
                                   juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                           static_cast<float> (width),
                                           static_cast<float> (height))
                      .reduced (1.0f);
    if (isButtonDown)
        bounds.translate (0.0f, 1.0f);

    juce::ColourGradient gradient (GloPalette::controlHover(), bounds.getCentreX(), bounds.getY(),
                                   GloPalette::controlWell().darker (0.20f),
                                   bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (bounds, 8.0f);

    g.setColour (box.hasKeyboardFocus (true) ? GloPalette::accentHot() : GloPalette::border());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f,
                            box.hasKeyboardFocus (true) ? 1.5f : 1.0f);

    const auto arrowArea = juce::Rectangle<float> (static_cast<float> (buttonX),
                                                    static_cast<float> (buttonY),
                                                    static_cast<float> (buttonW),
                                                    static_cast<float> (buttonH));
    const auto centre = arrowArea.getCentre();
    const auto arrowSize = std::min (7.0f, arrowArea.getHeight() * 0.18f);
    juce::Path arrow;
    arrow.startNewSubPath (centre.x - arrowSize, centre.y - arrowSize * 0.45f);
    arrow.lineTo (centre.x, centre.y + arrowSize * 0.55f);
    arrow.lineTo (centre.x + arrowSize, centre.y - arrowSize * 0.45f);
    g.setColour (box.isEnabled() ? GloPalette::accent() : GloPalette::textMuted());
    g.strokePath (arrow, juce::PathStrokeType (1.8f,
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void GloLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    const auto compact = box.getWidth() < 90;
    const auto leftInset = compact ? 7 : 12;
    const auto arrowReserve = compact ? 22 : 30;
    label.setBounds (leftInset, 1,
                     std::max (1, box.getWidth() - leftInset - arrowReserve),
                     std::max (1, box.getHeight() - 2));
    label.setFont (getComboBoxFont (box));
    label.setJustificationType (juce::Justification::centredLeft);
    label.setMinimumHorizontalScale (compact ? 0.68f : 0.78f);
}

juce::Font GloLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return makeFont (juce::jlimit (11.5f, 15.5f, static_cast<float> (buttonHeight) * 0.31f), true);
}

juce::Font GloLookAndFeel::getComboBoxFont (juce::ComboBox& box)
{
    return makeFont (juce::jlimit (9.5f, 15.0f, static_cast<float> (box.getHeight()) * 0.32f), true);
}

void GloLookAndFeel::drawCardBackground (juce::Graphics& g,
                                         juce::Rectangle<float> bounds,
                                         float cornerRadius,
                                         bool emphasised)
{
    if (bounds.isEmpty())
        return;

    bounds = bounds.reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.48f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 5.0f), cornerRadius);

    juce::ColourGradient panelGradient (emphasised ? GloPalette::panelRaised().brighter (0.035f)
                                                    : GloPalette::panelRaised(),
                                        bounds.getX(), bounds.getY(),
                                        GloPalette::panel().darker (0.12f),
                                        bounds.getRight(), bounds.getBottom(), false);
    panelGradient.addColour (0.52, GloPalette::panel());
    g.setGradientFill (panelGradient);
    g.fillRoundedRectangle (bounds, cornerRadius);

    g.setColour ((emphasised ? GloPalette::accent().withAlpha (0.30f)
                             : GloPalette::border()).withAlpha (emphasised ? 0.30f : 0.90f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), cornerRadius, emphasised ? 1.4f : 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.035f));
    g.drawRoundedRectangle (bounds.reduced (1.75f), std::max (2.0f, cornerRadius - 1.5f), 1.0f);

    if (emphasised)
    {
        juce::ColourGradient lineGradient (juce::Colours::transparentBlack,
                                           bounds.getX() + cornerRadius, bounds.getY(),
                                           GloPalette::accent().withAlpha (0.76f),
                                           bounds.getCentreX(), bounds.getY(), false);
        lineGradient.addColour (0.82, GloPalette::accent().withAlpha (0.18f));
        g.setGradientFill (lineGradient);
        g.drawHorizontalLine (juce::roundToInt (bounds.getY() + 1.0f),
                              bounds.getX() + cornerRadius,
                              bounds.getRight() - cornerRadius);
    }
}

void GloLookAndFeel::drawBrandLogo (juce::Graphics& g,
                                    juce::Rectangle<float> bounds,
                                    float signalIntensity)
{
    if (bounds.isEmpty())
        return;

    static const auto canonicalLogo = []
    {
        return juce::Drawable::createFromSVGString (
            juce::String::fromUTF8 (EightOhEightAssets::_808GloPro_Logo_svg,
                                    EightOhEightAssets::_808GloPro_Logo_svgSize));
    }();

    if (canonicalLogo == nullptr)
        return;

    canonicalLogo->drawWithin (g, bounds, juce::RectanglePlacement::centred, 1.0f);

    const auto intensity = juce::jlimit (0.0f, 1.0f, signalIntensity);
    if (intensity <= 0.005f)
        return;

    constexpr float designWidth = 332.0f;
    constexpr float designHeight = 60.0f;
    const auto scale = std::min (bounds.getWidth() / designWidth,
                                 bounds.getHeight() / designHeight);
    const auto originX = bounds.getX() + (bounds.getWidth() - designWidth * scale) * 0.5f;
    const auto originY = bounds.getY() + (bounds.getHeight() - designHeight * scale) * 0.5f;

    juce::Graphics::ScopedSaveState saveState (g);
    g.addTransform (juce::AffineTransform::scale (scale).translated (originX, originY));

    juce::Path diamond;
    diamond.startNewSubPath (28.0f, 2.5f);
    diamond.lineTo (54.5f, 29.0f);
    diamond.lineTo (28.0f, 56.5f);
    diamond.lineTo (1.5f, 29.0f);
    diamond.closeSubPath();

    g.setColour (GloPalette::accent().withAlpha (0.018f + intensity * 0.050f));
    g.strokePath (diamond, juce::PathStrokeType (5.0f, juce::PathStrokeType::mitered,
                                                 juce::PathStrokeType::rounded));
    g.setColour (GloPalette::accentHot().withAlpha (0.035f + intensity * 0.10f));
    g.strokePath (diamond, juce::PathStrokeType (1.35f, juce::PathStrokeType::mitered,
                                                 juce::PathStrokeType::rounded));

    const auto railEnd = juce::jmap (intensity, 235.5f, 327.0f);
    g.setColour (GloPalette::accent().withAlpha (0.025f + intensity * 0.055f));
    g.drawLine (143.5f, 56.2f, railEnd, 56.2f, 4.0f);
    g.setColour (GloPalette::accentHot().withAlpha (0.26f + intensity * 0.54f));
    g.drawLine (143.5f, 56.2f, railEnd, 56.2f, 1.05f);
}
} // namespace glo::ui
