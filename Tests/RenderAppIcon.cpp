#include <JuceHeader.h>

#include "808GloBinaryData.h"
#include "UI/GloLookAndFeel.h"

#include <iostream>

// Renders the standalone application icon from the embedded brand SVG. The
// Loop Diamond emblem region of the logo master is mapped into a macOS-style
// squircle drawn with the plugin palette, so the icon and UI share one source
// of truth. Rendering is vector-to-target-size; nothing is downscaled.

namespace
{
constexpr int defaultSize = 1024;
constexpr int maximumSize = 4096;

// Emblem bounds inside Resources/808GloPro_Logo.svg (viewBox 0 0 332 60).
const juce::Rectangle<float> emblemSvgBounds (1.5f, 2.5f, 53.0f, 54.0f);

juce::File resolveOutputFile (const juce::String& path)
{
    if (juce::File::isAbsolutePath (path))
        return juce::File (path);

    return juce::File::getCurrentWorkingDirectory().getChildFile (path);
}

juce::Image renderIcon (int size)
{
    using Palette = glo::ui::GloPalette;

    const auto scale = static_cast<float> (size) / static_cast<float> (defaultSize);
    juce::Image icon (juce::Image::ARGB, size, size, true);
    juce::Graphics g (icon);

    // Apple's Big Sur icon grid: an 824x824 squircle centred in a 1024 canvas.
    const auto plate = juce::Rectangle<float> (0.0f, 0.0f, 824.0f, 824.0f)
                           .withCentre ({ 512.0f, 512.0f }) * scale;
    const auto cornerRadius = plate.getWidth() * 0.225f;

    juce::Path squircle;
    squircle.addRoundedRectangle (plate, cornerRadius);

    g.saveState();
    g.reduceClipRegion (squircle);

    g.setGradientFill (juce::ColourGradient (Palette::panelRaised(),
                                             plate.getCentreX(), plate.getY(),
                                             Palette::canvas(),
                                             plate.getCentreX(), plate.getBottom(),
                                             false));
    g.fillRect (plate);

    // Faint engineering grid, echoing the editor backdrop.
    g.setColour (Palette::accent().withAlpha (0.035f));
    const auto gridStep = plate.getWidth() / 12.0f;
    for (auto x = plate.getX() + gridStep; x < plate.getRight(); x += gridStep)
        g.fillRect (juce::Rectangle<float> (x, plate.getY(), 1.0f * scale, plate.getHeight()));
    for (auto y = plate.getY() + gridStep; y < plate.getBottom(); y += gridStep)
        g.fillRect (juce::Rectangle<float> (plate.getX(), y, plate.getWidth(), 1.0f * scale));

    // Radial glow behind the emblem.
    {
        auto glow = juce::ColourGradient (Palette::accent().withAlpha (0.20f),
                                          plate.getCentreX(), plate.getCentreY() - plate.getHeight() * 0.04f,
                                          Palette::accent().withAlpha (0.0f),
                                          plate.getCentreX(), plate.getCentreY() + plate.getHeight() * 0.52f,
                                          true);
        g.setGradientFill (glow);
        g.fillRect (plate);
    }

    // Soft sheen across the upper plate.
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f),
                                             plate.getCentreX(), plate.getY(),
                                             juce::Colours::white.withAlpha (0.0f),
                                             plate.getCentreX(), plate.getCentreY(),
                                             false));
    g.fillRect (plate.withHeight (plate.getHeight() * 0.5f));

    const auto drawable = juce::Drawable::createFromSVGString (
        juce::String::fromUTF8 (EightOhEightAssets::_808GloPro_Logo_svg,
                                EightOhEightAssets::_808GloPro_Logo_svgSize));

    if (drawable != nullptr)
    {
        const auto emblemSide = plate.getHeight() * 0.56f;
        const auto emblemScale = emblemSide / emblemSvgBounds.getHeight();
        const auto transform =
            juce::AffineTransform::translation (-emblemSvgBounds.getCentreX(),
                                                -emblemSvgBounds.getCentreY())
                .scaled (emblemScale)
                .translated (plate.getCentreX(), plate.getCentreY());

        const auto emblemTarget = juce::Rectangle<float> (emblemSvgBounds.getWidth(),
                                                          emblemSvgBounds.getHeight())
                                      .transformedBy (juce::AffineTransform::scale (emblemScale))
                                      .withCentre (plate.getCentre())
                                      .expanded (10.0f * scale);

        // Paint the emblem into its own layer, clipped to its region so the
        // wordmark to its right in the logo master never enters the icon, then
        // draw an accent halo from the layer's alpha before compositing.
        juce::Image emblemLayer (juce::Image::ARGB, size, size, true);
        {
            juce::Graphics layer (emblemLayer);
            layer.reduceClipRegion (emblemTarget.getSmallestIntegerContainer());
            drawable->draw (layer, 1.0f, transform);

            // The "signal lit" diamond outline from the editor's logo state.
            juce::Path diamond;
            diamond.startNewSubPath (28.0f, 2.5f);
            diamond.lineTo (54.5f, 29.0f);
            diamond.lineTo (28.0f, 56.5f);
            diamond.lineTo (1.5f, 29.0f);
            diamond.closeSubPath();
            diamond.applyTransform (transform);

            layer.setColour (Palette::accent().withAlpha (0.35f));
            layer.strokePath (diamond, juce::PathStrokeType (5.0f * emblemScale / 8.0f,
                                                             juce::PathStrokeType::mitered,
                                                             juce::PathStrokeType::rounded));
            layer.setColour (Palette::accentHot().withAlpha (0.55f));
            layer.strokePath (diamond, juce::PathStrokeType (1.35f * emblemScale / 8.0f,
                                                             juce::PathStrokeType::mitered,
                                                             juce::PathStrokeType::rounded));
        }

        const juce::DropShadow halo (Palette::accent().withAlpha (0.40f),
                                     juce::roundToInt (30.0f * scale), {});
        halo.drawForImage (g, emblemLayer);
        g.setOpacity (1.0f); // drawForImage leaves the shadow colour's alpha active
        g.drawImageAt (emblemLayer, 0, 0);
    }

    g.restoreState();

    // Fine accent border, brighter along the top edge like the editor cards.
    g.setGradientFill (juce::ColourGradient (Palette::accent().withAlpha (0.55f),
                                             plate.getCentreX(), plate.getY(),
                                             Palette::accent().withAlpha (0.10f),
                                             plate.getCentreX(), plate.getBottom(),
                                             false));
    g.strokePath (squircle, juce::PathStrokeType (2.5f * scale));

    return icon;
}
} // namespace

int main (int argc, char* argv[])
{
    if (argc != 2 && argc != 3)
    {
        std::cerr << "Usage: 808GloRenderAppIcon <output.png> [size]\n";
        return 2;
    }

    const auto size = argc == 3 ? juce::String::fromUTF8 (argv[2]).getIntValue() : defaultSize;
    if (size <= 0 || size > maximumSize)
    {
        std::cerr << "size must be between 1 and " << maximumSize << " pixels\n";
        return 2;
    }

    const auto outputFile = resolveOutputFile (juce::String::fromUTF8 (argv[1]));
    if (! outputFile.hasFileExtension (".png"))
    {
        std::cerr << "Output path must have a .png extension\n";
        return 2;
    }

    const auto directoryResult = outputFile.getParentDirectory().createDirectory();
    if (directoryResult.failed())
    {
        std::cerr << "Could not create output directory: "
                  << directoryResult.getErrorMessage() << '\n';
        return 1;
    }

    juce::ScopedJuceInitialiser_GUI initialiseJuce;

    const auto icon = renderIcon (size);
    if (! icon.isValid())
    {
        std::cerr << "Failed to render the icon at " << size << 'x' << size << '\n';
        return 1;
    }

    juce::FileOutputStream stream (outputFile);
    if (! stream.openedOk())
    {
        std::cerr << "Could not open output file: " << stream.getStatus().getErrorMessage() << '\n';
        return 1;
    }

    if (! stream.setPosition (0) || stream.truncate().failed())
    {
        std::cerr << "Could not overwrite output file\n";
        return 1;
    }

    juce::PNGImageFormat png;
    if (! png.writeImageToStream (icon, stream))
    {
        std::cerr << "PNG encoding failed\n";
        return 1;
    }

    stream.flush();
    std::cout << outputFile.getFullPathName() << '\n';
    return 0;
}
