#include "PluginProcessor.h"

#include <JuceHeader.h>

#include "808GloBinaryData.h"
#include "UI/GloLookAndFeel.h"

#include <iostream>
#include <memory>
#include <vector>

// Renders the 808Glo Pro demo video frames and their exactly-synchronised
// audio by driving the real processor with MIDI and snapshotting the real
// editor. Demo rules: audition format (one section per factory category, not
// a song), everything in F minor, logo opener, category progress rail, live
// moving knobs/keys/scope. The message loop is pumped between frames so the
// editor's 30 Hz timer, parameter attachments, and the host-note keyboard
// mirror all behave exactly as they do live.
//
// Usage: 808GloRenderDemo <output-dir> [reel]
//        808GloRenderDemo still "<preset name>" <output.png>
//
// Video mode writes frame_00000.png ... plus demo.wav (48 kHz stereo).
// Mux with: mkvideo out.mp4 <output-dir>/frame 30 1600 audio=<dir>/demo.wav

namespace
{
constexpr double sampleRate = 48000.0;
constexpr int frameRate = 30;
constexpr int samplesPerFrame = 1600; // 48000 / 30 exactly
constexpr int videoWidth = 1600;
constexpr int videoHeight = 1000;
constexpr double bpm = 142.0;
constexpr double samplesPerBeat = sampleRate * 60.0 / bpm;

struct NoteEvent
{
    double startBeat;
    double lengthBeats;
    int note;
    float velocity;
};

struct Section
{
    const char* category;
    const char* presetName;
    double beats; // section length in beats (2 bars = 8, finale = 16)
    std::vector<NoteEvent> notes;
};

// F minor throughout. F1 = 29.
const std::vector<Section> fullShow = {
    { "Clean & Sub", "Night Foundation", 8.0,
      { { 0.0, 1.9, 29, 0.92f }, { 2.0, 0.9, 29, 0.78f }, { 3.0, 0.9, 32, 0.84f },
        { 4.0, 1.9, 29, 0.95f }, { 6.0, 1.9, 27, 0.88f } } },
    { "Trap", "Redline Bounce", 8.0,
      { { 0.0, 0.45, 29, 1.0f }, { 0.75, 0.2, 29, 0.7f }, { 1.0, 0.45, 29, 0.9f },
        { 1.5, 0.45, 32, 0.85f }, { 2.0, 0.9, 29, 1.0f }, { 3.0, 0.45, 36, 0.8f },
        { 3.5, 0.45, 34, 0.8f }, { 4.0, 0.45, 29, 1.0f }, { 4.75, 0.2, 29, 0.7f },
        { 5.0, 0.45, 29, 0.9f }, { 5.5, 0.45, 32, 0.85f }, { 6.0, 0.9, 29, 1.0f },
        { 7.0, 0.9, 39, 0.9f } } },
    { "Distorted", "Rabid Circuit", 8.0,
      { { 0.0, 0.9, 29, 1.0f }, { 1.5, 0.4, 29, 0.8f }, { 2.0, 0.9, 31, 0.9f },
        { 3.0, 0.9, 32, 0.95f }, { 4.0, 1.4, 29, 1.0f }, { 5.5, 0.4, 29, 0.8f },
        { 6.0, 1.9, 24, 1.0f } } },
    { "Drill", "Sliding Shadow", 8.0,
      { { 0.0, 1.2, 29, 0.95f }, { 1.1, 0.9, 34, 0.85f }, { 2.5, 1.4, 29, 0.95f },
        { 4.0, 1.2, 32, 0.9f }, { 5.1, 0.9, 29, 0.85f }, { 6.0, 1.9, 24, 1.0f } } },
    { "Detroit", "Buffed Up", 8.0,
      { { 0.0, 0.4, 29, 1.0f }, { 0.5, 0.4, 29, 0.75f }, { 1.25, 0.4, 32, 0.85f },
        { 2.0, 0.4, 29, 0.95f }, { 2.75, 0.4, 36, 0.8f }, { 3.5, 0.4, 34, 0.8f },
        { 4.0, 0.4, 29, 1.0f }, { 4.5, 0.4, 29, 0.75f }, { 5.25, 0.4, 32, 0.85f },
        { 6.0, 0.4, 36, 0.9f }, { 6.75, 0.9, 29, 1.0f } } },
    { "West Coast", "Palm Bounce", 8.0,
      { { 0.0, 0.9, 29, 0.95f }, { 1.0, 0.9, 29, 0.8f }, { 2.0, 0.65, 36, 0.85f },
        { 2.75, 1.1, 34, 0.85f }, { 4.0, 0.9, 29, 0.95f }, { 5.0, 0.9, 32, 0.85f },
        { 6.0, 1.8, 29, 0.95f } } },
    { "Long Glide", "Lunar Portamento", 8.0,
      { { 0.0, 1.9, 41, 0.9f }, { 1.8, 6.0, 29, 0.95f } } },
    { "Short Punch", "Quick Knock", 8.0,
      { { 0.0, 0.2, 29, 1.0f }, { 0.5, 0.2, 29, 0.8f }, { 1.0, 0.2, 29, 0.9f },
        { 1.5, 0.2, 32, 0.85f }, { 2.0, 0.2, 29, 1.0f }, { 2.5, 0.2, 29, 0.8f },
        { 3.0, 0.2, 36, 0.9f }, { 3.5, 0.2, 34, 0.85f }, { 4.0, 0.2, 29, 1.0f },
        { 4.5, 0.2, 29, 0.8f }, { 5.0, 0.2, 29, 0.9f }, { 5.5, 0.2, 32, 0.85f },
        { 6.0, 0.2, 29, 1.0f }, { 6.5, 0.2, 34, 0.85f }, { 7.0, 0.4, 29, 1.0f } } },
    { "Experimental", "Phase Beast", 8.0,
      { { 0.0, 1.9, 29, 0.9f }, { 1.8, 2.1, 39, 0.85f }, { 3.8, 4.1, 24, 0.95f } } },
    { "Mix Ready", "Club Translation", 8.0,
      { { 0.0, 0.9, 29, 0.95f }, { 1.0, 0.45, 29, 0.8f }, { 1.5, 0.45, 32, 0.85f },
        { 2.0, 0.9, 34, 0.9f }, { 3.0, 0.9, 36, 0.9f }, { 4.0, 0.9, 29, 0.95f },
        { 5.0, 0.45, 32, 0.85f }, { 5.5, 0.45, 34, 0.85f }, { 6.0, 1.9, 29, 0.95f } } },
    { "Finale", "Crown Pressure", 16.0,
      { { 0.0, 0.9, 29, 1.0f }, { 1.0, 0.45, 29, 0.8f }, { 1.5, 0.45, 29, 0.8f },
        { 2.0, 0.9, 32, 0.95f }, { 3.0, 0.9, 34, 0.9f }, { 4.0, 0.9, 36, 1.0f },
        { 5.0, 0.45, 34, 0.85f }, { 5.5, 0.45, 32, 0.85f }, { 6.0, 0.9, 29, 1.0f },
        { 7.0, 0.9, 31, 0.9f }, { 8.0, 1.9, 32, 1.0f }, { 10.0, 0.9, 34, 0.9f },
        { 11.0, 0.9, 36, 0.95f }, { 12.0, 0.9, 39, 1.0f }, { 13.0, 0.45, 36, 0.85f },
        { 13.5, 0.45, 34, 0.85f }, { 14.0, 1.9, 29, 1.0f } } },
};

// The reel keeps the opener/finale and the four most contrasting categories.
const std::vector<int> reelSections = { 1, 3, 5, 6, 10 };

std::unique_ptr<juce::Drawable> loadLogo()
{
    return juce::Drawable::createFromSVGString (
        juce::String::fromUTF8 (EightOhEightAssets::_808GloPro_Logo_svg,
                                EightOhEightAssets::_808GloPro_Logo_svgSize));
}

void drawTitleCard (juce::Image& target, const juce::Drawable* logo,
                    const juce::String& subline, float alpha)
{
    using Palette = glo::ui::GloPalette;
    juce::Graphics g (target);
    g.fillAll (Palette::canvas());

    g.setGradientFill (juce::ColourGradient (Palette::accent().withAlpha (0.10f * alpha),
                                             videoWidth * 0.5f, videoHeight * 0.38f,
                                             Palette::accent().withAlpha (0.0f),
                                             videoWidth * 0.5f, videoHeight * 0.95f,
                                             true));
    g.fillAll();

    if (logo != nullptr)
    {
        const auto logoArea = juce::Rectangle<float> (videoWidth * 0.22f, videoHeight * 0.34f,
                                                      videoWidth * 0.56f, videoHeight * 0.20f);
        logo->drawWithin (g, logoArea, juce::RectanglePlacement::centred, alpha);
    }

    g.setColour (Palette::textSecondary().withAlpha (alpha));
    g.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    g.drawText (subline, juce::Rectangle<int> (0, juce::roundToInt (videoHeight * 0.60f),
                                               videoWidth, 40),
                juce::Justification::centred);
}

void drawSectionOverlay (juce::Image& frame, const juce::String& category,
                         const juce::String& presetName, int sectionIndex, int sectionCount)
{
    using Palette = glo::ui::GloPalette;
    juce::Graphics g (frame);

    // Category + preset label, top right, clear of the header controls.
    const auto label = category.toUpperCase() + "  \xe2\x80\x94  " + presetName.toUpperCase();
    g.setFont (juce::Font (juce::FontOptions (21.0f, juce::Font::bold)));
    const auto textWidth = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), label);
    const auto labelBox = juce::Rectangle<int> (videoWidth - textWidth - 56, 108, textWidth + 28, 34);
    g.setColour (Palette::canvas().withAlpha (0.82f));
    g.fillRoundedRectangle (labelBox.toFloat(), 8.0f);
    g.setColour (Palette::accent().withAlpha (0.85f));
    g.drawRoundedRectangle (labelBox.toFloat().reduced (0.75f), 8.0f, 1.5f);
    g.drawText (label, labelBox, juce::Justification::centred);

    // Category progress rail along the very bottom edge.
    const float railY = videoHeight - 8.0f;
    const float railMargin = 36.0f;
    const float railWidth = videoWidth - railMargin * 2.0f;
    const float gap = 6.0f;
    const float segment = (railWidth - gap * (float) (sectionCount - 1)) / (float) sectionCount;
    for (int i = 0; i < sectionCount; ++i)
    {
        const auto x = railMargin + (segment + gap) * (float) i;
        g.setColour (i < sectionIndex ? Palette::accent().withAlpha (0.45f)
                     : i == sectionIndex ? Palette::accentHot()
                                         : Palette::border());
        g.fillRoundedRectangle (x, railY, segment, 4.0f, 2.0f);
    }
}

int findProgramForPreset (EightOhEightGloProAudioProcessor& processor, const juce::String& name)
{
    for (int i = 0; i < processor.getNumPrograms(); ++i)
        if (processor.getProgramName (i).equalsIgnoreCase (name))
            return i;
    return -1;
}

bool writePng (const juce::Image& image, const juce::File& file)
{
    juce::FileOutputStream stream (file);
    if (! stream.openedOk() || ! stream.setPosition (0) || stream.truncate().failed())
        return false;
    juce::PNGImageFormat png;
    return png.writeImageToStream (image, stream);
}

// Advance the editor's timer-driven UI (meters, scope, preset display, the
// host-note keyboard mirror drain) without running a dispatch loop: sleep so
// wall-clock timers become due, then fire them synchronously. Parameter
// attachments already update synchronously on this thread, and snapshots
// repaint everything directly, so this is all the pumping the tool needs.
// (JUCE 9 removed runDispatchLoopUntil, and stopDispatchLoop poisons any
// later runDispatchLoop, so the modal-pump pattern is not an option here.)
void pumpMessageLoop (int milliseconds)
{
    juce::Thread::sleep (milliseconds);
    juce::Timer::callPendingTimersSynchronously();
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI initialiseJuce;

    EightOhEightGloProAudioProcessor processor;
    processor.prepareToPlay (sampleRate, samplesPerFrame);

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    if (editor == nullptr)
    {
        std::cerr << "no editor\n";
        return 1;
    }
    editor->setSize (videoWidth, videoHeight);
    // The scope and meters stop their timers when the editor is not showing,
    // and isShowing() needs a window peer. Park a real window far offscreen.
    editor->addToDesktop (juce::ComponentPeer::windowIsTemporary);
    editor->setVisible (true);
    editor->setTopLeftPosition (-videoWidth - 400, 300);
    pumpMessageLoop (60);

    // ---- still mode: load a preset, run signal so the scope/meters are live,
    // snapshot once.
    if (argc == 4 && juce::String (argv[1]) == "still")
    {
        const auto presetName = juce::String::fromUTF8 (argv[2]);
        const auto program = findProgramForPreset (processor, presetName);
        if (program < 0)
        {
            std::cerr << "preset not found: " << argv[2] << '\n';
            return 1;
        }
        processor.setCurrentProgram (program);
        pumpMessageLoop (80);

        juce::AudioBuffer<float> audio (2, samplesPerFrame);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, 29, (juce::uint8) 116), 0);
        auto stillPeak = 0.0f;
        for (int block = 0; block < 18; ++block)
        {
            processor.processBlock (audio, midi);
            midi.clear();
            stillPeak = juce::jmax (stillPeak, audio.getMagnitude (0, 0, audio.getNumSamples()));
            processor.applyPendingHostNotesToKeyboard();
            pumpMessageLoop (25);
        }
        std::cout << "still audio peak: " << stillPeak
                  << "  meter peak: " << processor.getPeakLevel (0) << '\n';

        auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        const auto out = juce::File::getCurrentWorkingDirectory()
                             .getChildFile (juce::String::fromUTF8 (argv[3]));
        out.getParentDirectory().createDirectory();
        if (! writePng (snapshot, out))
        {
            std::cerr << "png write failed\n";
            return 1;
        }
        std::cout << out.getFullPathName() << '\n';
        return 0;
    }

    if (argc != 2 && argc != 3)
    {
        std::cerr << "Usage: 808GloRenderDemo <output-dir> [reel]\n"
                     "       808GloRenderDemo still \"<preset name>\" <output.png>\n";
        return 2;
    }

    const bool reel = argc == 3 && juce::String (argv[2]) == "reel";
    const auto outputDir = juce::File::getCurrentWorkingDirectory()
                               .getChildFile (juce::String::fromUTF8 (argv[1]));
    outputDir.createDirectory();

    std::vector<Section> show;
    if (reel)
        for (auto index : reelSections)
            show.push_back (fullShow[(size_t) index]);
    else
        show = fullShow;

    const auto logo = loadLogo();
    std::vector<float> audioLeft, audioRight;
    int frameNumber = 0;

    const auto writeFrame = [&] (const juce::Image& image)
    {
        const auto file = outputDir.getChildFile (
            juce::String::formatted ("frame_%05d.png", frameNumber++));
        if (! writePng (image, file))
        {
            std::cerr << "frame write failed: " << file.getFullPathName() << '\n';
            std::exit (1);
        }
    };

    const auto renderSilentFrames = [&] (int frames)
    {
        audioLeft.insert (audioLeft.end(), (size_t) (frames * samplesPerFrame), 0.0f);
        audioRight.insert (audioRight.end(), (size_t) (frames * samplesPerFrame), 0.0f);
    };

    // ---- opener: fade the brand card in and hold. 2.4 s.
    {
        constexpr int openerFrames = 72;
        for (int f = 0; f < openerFrames; ++f)
        {
            const auto fade = juce::jlimit (0.0f, 1.0f, (float) f / 14.0f);
            juce::Image card (juce::Image::ARGB, videoWidth, videoHeight, true);
            drawTitleCard (card, logo.get(),
                           "SYNTHESIZED 808 INSTRUMENT  \xe2\x80\xa2  128 FACTORY PRESETS", fade);
            writeFrame (card);
        }
        renderSilentFrames (openerFrames);
    }

    // ---- sections
    juce::AudioBuffer<float> audio (2, samplesPerFrame);
    const auto sectionCount = (int) show.size();

    for (int s = 0; s < sectionCount; ++s)
    {
        const auto& section = show[(size_t) s];
        const auto program = findProgramForPreset (processor, section.presetName);
        if (program < 0)
        {
            std::cerr << "preset not found: " << section.presetName << '\n';
            return 1;
        }
        processor.setCurrentProgram (program);
        pumpMessageLoop (80); // let attachments move the knobs before the cut

        const auto sectionSamples = (juce::int64) (section.beats * samplesPerBeat);
        const auto tailSamples = (juce::int64) (samplesPerBeat * (s == sectionCount - 1 ? 4.0 : 0.5));
        const auto totalSamples = sectionSamples + tailSamples;
        const auto frames = (int) ((totalSamples + samplesPerFrame - 1) / samplesPerFrame);

        juce::int64 samplePosition = 0;
        for (int f = 0; f < frames; ++f)
        {
            juce::MidiBuffer midi;
            const auto windowStart = samplePosition;
            const auto windowEnd = samplePosition + samplesPerFrame;

            for (const auto& note : section.notes)
            {
                const auto onSample = (juce::int64) (note.startBeat * samplesPerBeat);
                const auto offSample = (juce::int64) ((note.startBeat + note.lengthBeats) * samplesPerBeat);
                if (onSample >= windowStart && onSample < windowEnd)
                    midi.addEvent (juce::MidiMessage::noteOn (1, note.note, note.velocity),
                                   (int) (onSample - windowStart));
                if (offSample >= windowStart && offSample < windowEnd)
                    midi.addEvent (juce::MidiMessage::noteOff (1, note.note),
                                   (int) (offSample - windowStart));
            }

            audio.clear();
            processor.processBlock (audio, midi);
            for (int i = 0; i < samplesPerFrame; ++i)
            {
                audioLeft.push_back (audio.getSample (0, i));
                audioRight.push_back (audio.getSample (1, i));
            }

            processor.applyPendingHostNotesToKeyboard();
            pumpMessageLoop (5); // snapshot time supplies the rest of the wall clock

            auto frame = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
            drawSectionOverlay (frame, section.category, section.presetName, s, sectionCount);
            writeFrame (frame);
            samplePosition = windowEnd;
        }

        std::cout << "section done: " << section.category << " (" << frames << " frames)\n";
    }

    // ---- end card while the finale tail rings out of the buffer above. 1.8 s.
    {
        constexpr int endFrames = 54;
        for (int f = 0; f < endFrames; ++f)
        {
            const auto fade = juce::jlimit (0.0f, 1.0f, (float) (endFrames - f) / 20.0f);
            juce::Image card (juce::Image::ARGB, videoWidth, videoHeight, true);
            drawTitleCard (card, logo.get(),
                           "VST3  \xe2\x80\xa2  AU  \xe2\x80\xa2  STANDALONE  \xe2\x80\xa2  DIAMOND LOOPZ", fade);
            writeFrame (card);
        }
        renderSilentFrames (endFrames);
    }

    // ---- write the synchronised audio
    {
        const auto wavFile = outputDir.getChildFile ("demo.wav");
        wavFile.deleteFile();
        juce::WavAudioFormat wav;
        auto stream = wavFile.createOutputStream();
        if (stream == nullptr)
        {
            std::cerr << "cannot open demo.wav\n";
            return 1;
        }
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wav.createWriterFor (stream.get(), sampleRate, 2, 16, {}, 0));
        if (writer == nullptr)
        {
            std::cerr << "cannot create wav writer\n";
            return 1;
        }
        stream.release(); // the writer owns it now

        juce::AudioBuffer<float> all (2, (int) audioLeft.size());
        all.copyFrom (0, 0, audioLeft.data(), (int) audioLeft.size());
        all.copyFrom (1, 0, audioRight.data(), (int) audioRight.size());
        writer->writeFromAudioSampleBuffer (all, 0, all.getNumSamples());
    }

    std::cout << "frames: " << frameNumber << "  audio samples: " << audioLeft.size()
              << "  seconds: " << (double) audioLeft.size() / sampleRate << '\n';
    return 0;
}
