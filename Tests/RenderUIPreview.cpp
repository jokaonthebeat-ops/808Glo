#include "PluginProcessor.h"

#include <JuceHeader.h>

#include <iostream>
#include <memory>

namespace
{
constexpr int defaultWidth = 1600;
constexpr int defaultHeight = 1000;
constexpr int maximumDimension = 8192;

int parseDimension (const char* text, const char* name)
{
    const auto value = juce::String::fromUTF8 (text).getIntValue();
    if (value <= 0 || value > maximumDimension)
    {
        std::cerr << name << " must be between 1 and " << maximumDimension << " pixels\n";
        return 0;
    }

    return value;
}

juce::File resolveOutputFile (const juce::String& path)
{
    if (juce::File::isAbsolutePath (path))
        return juce::File (path);

    return juce::File::getCurrentWorkingDirectory().getChildFile (path);
}
} // namespace

int main (int argc, char* argv[])
{
    if (argc != 2 && argc != 4)
    {
        std::cerr << "Usage: 808GloRenderUIPreview <output.png> [width height]\n";
        return 2;
    }

    const auto width = argc == 4 ? parseDimension (argv[2], "width") : defaultWidth;
    const auto height = argc == 4 ? parseDimension (argv[3], "height") : defaultHeight;
    if (width == 0 || height == 0)
        return 2;

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
    EightOhEightGloProAudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    if (editor == nullptr)
    {
        std::cerr << "The processor did not create an editor\n";
        return 1;
    }

    editor->setSize (width, height);
    editor->setVisible (true);
    const auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    if (! snapshot.isValid() || snapshot.getWidth() != width || snapshot.getHeight() != height)
    {
        std::cerr << "Failed to render the editor at " << width << 'x' << height << '\n';
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
    if (! png.writeImageToStream (snapshot, stream))
    {
        std::cerr << "PNG encoding failed\n";
        return 1;
    }

    stream.flush();
    std::cout << outputFile.getFullPathName() << '\n';
    return 0;
}
