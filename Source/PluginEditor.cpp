#include "PluginEditor.h"

#include "ParameterLayout.h"

#include <algorithm>

namespace
{
constexpr float designWidth = 1600.0f;
constexpr float designHeight = 1000.0f;

void showPresetError (juce::Component* associatedComponent,
                      const juce::String& title,
                      const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                            title,
                                            message,
                                            "OK",
                                            associatedComponent);
}

class PresetListModel final : public juce::ListBoxModel
{
public:
    std::function<int()> getCount;
    std::function<const glo::PresetManager::Preset* (int)> getPreset;
    std::function<void (int)> selectionChangedCallback;
    std::function<void (int)> doubleClickCallback;

    int getNumRows() override
    {
        return getCount ? getCount() : 0;
    }

    void paintListBoxItem (int row,
                           juce::Graphics& graphics,
                           int width,
                           int height,
                           bool selected) override
    {
        const auto* preset = getPreset ? getPreset (row) : nullptr;
        if (preset == nullptr)
            return;

        const auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                                    static_cast<float> (width),
                                                    static_cast<float> (height));
        if (selected)
        {
            graphics.setColour (glo::ui::GloPalette::accent().withAlpha (0.13f));
            graphics.fillRoundedRectangle (bounds.reduced (4.0f, 3.0f), 8.0f);
            graphics.setColour (glo::ui::GloPalette::accent().withAlpha (0.7f));
            graphics.fillRoundedRectangle ({ 5.0f, 10.0f, 3.0f,
                                             static_cast<float> (height - 20) }, 1.5f);
        }

        graphics.setColour (selected ? glo::ui::GloPalette::textPrimary()
                                     : glo::ui::GloPalette::textSecondary());
        graphics.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
        graphics.drawText (preset->name, 18, 5, width - 28, 24,
                           juce::Justification::centredLeft, true);
        graphics.setColour (glo::ui::GloPalette::textMuted());
        graphics.setFont (juce::Font (juce::FontOptions (11.0f)));
        graphics.drawText (preset->category.toUpperCase(), 18, 28, width - 28, 18,
                           juce::Justification::centredLeft, true);
    }

    void selectedRowsChanged (int lastRowSelected) override
    {
        if (selectionChangedCallback)
            selectionChangedCallback (lastRowSelected);
    }

    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override
    {
        if (doubleClickCallback)
            doubleClickCallback (row);
    }
};
} // namespace

class EightOhEightGloProAudioProcessorEditor::PresetBrowserOverlay final
    : public juce::Component,
      private juce::TextEditor::Listener,
      private juce::ComboBox::Listener
{
public:
    explicit PresetBrowserOverlay (glo::PresetManager& managerToUse)
        : manager (managerToUse),
          list ("Factory presets", &listModel)
    {
        setWantsKeyboardFocus (true);
        setFocusContainerType (FocusContainerType::keyboardFocusContainer);

        searchBox.setTextToShowWhenEmpty ("Search name, style, tag or purpose...",
                                          glo::ui::GloPalette::textMuted());
        searchBox.setTitle ("Search factory presets");
        searchBox.setEscapeAndReturnKeysConsumed (false);
        searchBox.addListener (this);
        addAndMakeVisible (searchBox);

        categoryBox.addItem ("All categories", 1);
        auto itemID = 2;
        for (const auto& category : manager.getCategories())
            categoryBox.addItem (category, itemID++);
        categoryBox.setSelectedId (1, juce::dontSendNotification);
        categoryBox.setTitle ("Preset category");
        categoryBox.addListener (this);
        addAndMakeVisible (categoryBox);

        list.setRowHeight (54);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        list.setColour (juce::ListBox::outlineColourId, glo::ui::GloPalette::border());
        list.setOutlineThickness (1);
        addAndMakeVisible (list);

        titleLabel.setText ("FACTORY LIBRARY", juce::dontSendNotification);
        titleLabel.setColour (juce::Label::textColourId, glo::ui::GloPalette::textPrimary());
        titleLabel.setFont (juce::Font (juce::FontOptions (23.0f, juce::Font::bold)));
        addAndMakeVisible (titleLabel);

        countLabel.setColour (juce::Label::textColourId, glo::ui::GloPalette::accent());
        countLabel.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (countLabel);

        nameLabel.setColour (juce::Label::textColourId, glo::ui::GloPalette::textPrimary());
        nameLabel.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
        nameLabel.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (nameLabel);

        categoryLabel.setColour (juce::Label::textColourId, glo::ui::GloPalette::accent());
        categoryLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        addAndMakeVisible (categoryLabel);

        descriptionLabel.setColour (juce::Label::textColourId, glo::ui::GloPalette::textSecondary());
        descriptionLabel.setFont (juce::Font (juce::FontOptions (15.0f)));
        descriptionLabel.setJustificationType (juce::Justification::topLeft);
        descriptionLabel.setMinimumHorizontalScale (1.0f);
        addAndMakeVisible (descriptionLabel);

        tagsLabel.setColour (juce::Label::textColourId, glo::ui::GloPalette::textMuted());
        tagsLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
        tagsLabel.setJustificationType (juce::Justification::topLeft);
        addAndMakeVisible (tagsLabel);

        loadButton.setButtonText ("LOAD PRESET");
        loadButton.onClick = [this] { loadSelection(); };
        loadButton.setTitle ("Load selected preset");
        addAndMakeVisible (loadButton);

        closeButton.setButtonText ("CLOSE");
        closeButton.onClick = [this] { hide(); };
        addAndMakeVisible (closeButton);

        listModel.getCount = [this] { return static_cast<int> (results.size()); };
        listModel.getPreset = [this] (int row) -> const glo::PresetManager::Preset*
        {
            if (row < 0 || row >= static_cast<int> (results.size()))
                return nullptr;
            return &manager.getFactoryPreset (results[static_cast<std::size_t> (row)]);
        };
        listModel.selectionChangedCallback = [this] (int row) { showDetails (row); };
        listModel.doubleClickCallback = [this] (int) { loadSelection(); };

        refreshResults();
        setVisible (false);
    }

    void paint (juce::Graphics& graphics) override
    {
        graphics.fillAll (juce::Colours::black.withAlpha (0.78f));
        glo::ui::GloLookAndFeel::drawCardBackground (graphics, shell.toFloat(), 22.0f, true);

        graphics.setColour (glo::ui::GloPalette::border());
        graphics.drawVerticalLine (inspectorBounds.getX() - 16,
                                   static_cast<float> (inspectorBounds.getY()),
                                   static_cast<float> (inspectorBounds.getBottom()));
    }

    void resized() override
    {
        shell = getLocalBounds().reduced (juce::jmax (50, getWidth() / 9),
                                          juce::jmax (36, getHeight() / 10));
        auto inner = shell.reduced (28);
        auto header = inner.removeFromTop (54);
        titleLabel.setBounds (header.removeFromLeft (260));
        closeButton.setBounds (header.removeFromRight (100).reduced (0, 7));
        countLabel.setBounds (header.removeFromRight (170));

        inner.removeFromTop (14);
        auto filters = inner.removeFromTop (42);
        categoryBox.setBounds (filters.removeFromLeft (220));
        filters.removeFromLeft (12);
        searchBox.setBounds (filters.removeFromLeft (juce::jmin (460, filters.getWidth())));
        inner.removeFromTop (18);

        auto footer = inner.removeFromBottom (48);
        inner.removeFromBottom (16);
        auto listArea = inner.removeFromLeft (juce::roundToInt (inner.getWidth() * 0.54f));
        list.setBounds (listArea);
        inner.removeFromLeft (32);
        inspectorBounds = inner;
        auto inspector = inspectorBounds;

        nameLabel.setBounds (inspector.removeFromTop (54));
        categoryLabel.setBounds (inspector.removeFromTop (28));
        inspector.removeFromTop (14);
        descriptionLabel.setBounds (inspector.removeFromTop (110));
        inspector.removeFromTop (12);
        tagsLabel.setBounds (inspector.removeFromTop (80));
        loadButton.setBounds (footer.removeFromRight (170).reduced (0, 5));
    }

    bool keyPressed (const juce::KeyPress& key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            hide();
            return true;
        }
        if (key == juce::KeyPress::returnKey)
        {
            loadSelection();
            return true;
        }
        return false;
    }

    void open()
    {
        refreshResults();
        setVisible (true);
        toFront (true);
        searchBox.grabKeyboardFocus();
    }

    void hide()
    {
        setVisible (false);
    }

private:
    void textEditorTextChanged (juce::TextEditor&) override { refreshResults(); }
    void comboBoxChanged (juce::ComboBox*) override { refreshResults(); }

    void refreshResults()
    {
        const auto category = categoryBox.getSelectedId() <= 1 ? juce::String()
                                                               : categoryBox.getText();
        results = manager.search (searchBox.getText(), category);
        countLabel.setText (juce::String (results.size()) + " PRESETS", juce::dontSendNotification);
        list.updateContent();
        if (! results.empty())
        {
            list.selectRow (0);
            showDetails (0);
        }
        else
            showDetails (-1);
        repaint();
    }

    void showDetails (int row)
    {
        const auto* preset = listModel.getPreset ? listModel.getPreset (row) : nullptr;
        if (preset == nullptr)
        {
            nameLabel.setText ("NO MATCHES", juce::dontSendNotification);
            categoryLabel.setText ({}, juce::dontSendNotification);
            descriptionLabel.setText ("Try a different name, tag, or category.",
                                      juce::dontSendNotification);
            tagsLabel.setText ({}, juce::dontSendNotification);
            return;
        }

        nameLabel.setText (preset->name, juce::dontSendNotification);
        categoryLabel.setText (preset->category.toUpperCase(), juce::dontSendNotification);
        descriptionLabel.setText (preset->description, juce::dontSendNotification);
        tagsLabel.setText ("TAGS  /  "
                               + preset->tags.joinIntoString (juce::String::fromUTF8 ("  •  ")),
                           juce::dontSendNotification);
    }

    void loadSelection()
    {
        const auto row = list.getSelectedRow();
        if (row < 0 || row >= static_cast<int> (results.size()))
            return;
        if (manager.loadFactoryPreset (results[static_cast<std::size_t> (row)]))
        {
            hide();
            return;
        }

        showPresetError (this,
                         "Unable to Load Preset",
                         "The selected factory preset is incomplete or could not be loaded.");
    }

    glo::PresetManager& manager;
    PresetListModel listModel;
    juce::ListBox list;
    juce::TextEditor searchBox;
    juce::ComboBox categoryBox;
    juce::Label titleLabel;
    juce::Label countLabel;
    juce::Label nameLabel;
    juce::Label categoryLabel;
    juce::Label descriptionLabel;
    juce::Label tagsLabel;
    juce::TextButton loadButton;
    juce::TextButton closeButton;
    std::vector<int> results;
    juce::Rectangle<int> shell;
    juce::Rectangle<int> inspectorBounds;
};

EightOhEightGloProAudioProcessorEditor::EightOhEightGloProAudioProcessorEditor (
    EightOhEightGloProAudioProcessor& processorToUse)
    : AudioProcessorEditor (&processorToUse),
      processor (processorToUse),
      voicePanel ("VOICE", processor.getParameterState()),
      punchHero (processor.getParameterState(), glo::ids::punch, "PUNCH"),
      gloHero (processor.getParameterState(), glo::ids::drive, "GLO"),
      tailHero (processor.getParameterState(), glo::ids::decay, "TAIL"),
      masterPanel ("MASTER", processor.getParameterState()),
      pitchPanel ("PITCH", processor.getParameterState()),
      envelopePanel ("ENVELOPE", processor.getParameterState()),
      tonePanel ("BODY + TONE", processor.getParameterState()),
      transientPanel ("TRANSIENT", processor.getParameterState()),
      keyboard (processor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&gloLookAndFeel);
    setOpaque (true);
    setWantsKeyboardFocus (true);

    headerCard.setEmphasised (true);
    addAndMakeVisible (headerCard);
    addAndMakeVisible (logo);
    addAndMakeVisible (previousPresetButton);
    addAndMakeVisible (presetNameButton);
    addAndMakeVisible (nextPresetButton);
    addAndMakeVisible (browserButton);
    addAndMakeVisible (saveButton);
    addAndMakeVisible (loadButton);
    addAndMakeVisible (presetCategoryLabel);

    presetCategoryLabel.setColour (juce::Label::textColourId, glo::ui::GloPalette::accent());
    presetCategoryLabel.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    presetCategoryLabel.setJustificationType (juce::Justification::centredLeft);

    previousPresetButton.setTooltip ("Previous factory preset");
    nextPresetButton.setTooltip ("Next factory preset");
    presetNameButton.setTooltip ("Open the searchable 128-preset browser");
    browserButton.setTooltip ("Browse factory presets (Ctrl/Cmd+F)");
    saveButton.setTooltip ("Save the current sound as a user preset");
    loadButton.setTooltip ("Load a .808glo user preset");
    previousPresetButton.setTitle ("Previous preset");
    previousPresetButton.setDescription ("Load the previous factory preset");
    nextPresetButton.setTitle ("Next preset");
    nextPresetButton.setDescription ("Load the next factory preset");

    previousPresetButton.onClick = [this]
    {
        processor.getPresetManager().loadNextFactoryPreset (-1);
        updatePresetDisplay();
    };
    nextPresetButton.onClick = [this]
    {
        processor.getPresetManager().loadNextFactoryPreset (1);
        updatePresetDisplay();
    };
    presetNameButton.onClick = [this] { showPresetBrowser(); };
    browserButton.onClick = [this] { showPresetBrowser(); };
    saveButton.onClick = [this] { saveUserPreset(); };
    loadButton.onClick = [this] { loadUserPreset(); };

    voicePanel.setColumns (2);
    voicePanel.addChoice (glo::ids::voiceMode, "MODE", { "MONO", "POLY" });
    voicePanel.addChoice (glo::ids::triggerMode, "TRIGGER", { "ONE SHOT", "GATE" });
    voicePanel.addToggle (glo::ids::legato, "LEGATO");
    voicePanel.addKnob (glo::ids::tune, "TUNE");
    voicePanel.addKnob (glo::ids::fine, "FINE");
    voicePanel.addKnob (glo::ids::bendRange, "BEND");
    voicePanel.addKnob (glo::ids::velocitySens, "VELOCITY");
    addAndMakeVisible (voicePanel);

    scopeCard.setEmphasised (true);
    addAndMakeVisible (scopeCard);
    addAndMakeVisible (scope);
    addAndMakeVisible (punchHero);
    addAndMakeVisible (gloHero);
    addAndMakeVisible (tailHero);
    punchHero.setFeatured (true);
    gloHero.setFeatured (true);
    tailHero.setFeatured (true);
    scope.setSampleProvider ([this] (float* destination, int maximum)
    {
        return processor.copyLatestScopeSamples (destination, maximum);
    });
    scope.setRefreshRateHz (60);

    masterPanel.setColumns (2);
    masterPanel.addKnob (glo::ids::output, "OUTPUT");
    masterPanel.addKnob (glo::ids::compressor, "GLUE");
    masterPanel.addKnob (glo::ids::clipper, "CLIP");
    masterPanel.addChoice (glo::ids::driveMode, "COLOR", { "CLEAN", "WARM", "HARD", "FOLD" });
    addAndMakeVisible (masterPanel);
    addAndMakeVisible (outputMeter);

    pitchPanel.setColumns (4);
    pitchPanel.addKnob (glo::ids::pitchDrop, "DROP");
    pitchPanel.addKnob (glo::ids::pitchDecay, "TIME");
    pitchPanel.addKnob (glo::ids::pitchCurve, "CURVE");
    pitchPanel.addKnob (glo::ids::glide, "GLIDE");

    envelopePanel.setColumns (5);
    envelopePanel.addKnob (glo::ids::attack, "ATTACK");
    envelopePanel.addKnob (glo::ids::hold, "HOLD");
    envelopePanel.addKnob (glo::ids::sustain, "SUSTAIN");
    envelopePanel.addKnob (glo::ids::release, "RELEASE");
    envelopePanel.addKnob (glo::ids::ampCurve, "CURVE");

    tonePanel.setColumns (4);
    tonePanel.addKnob (glo::ids::body, "BODY");
    tonePanel.addKnob (glo::ids::harmonics, "HARMONICS");
    tonePanel.addKnob (glo::ids::harmonicBalance, "BALANCE");
    tonePanel.addKnob (glo::ids::tone, "TONE");

    transientPanel.setColumns (4);
    transientPanel.addKnob (glo::ids::click, "CLICK");
    transientPanel.addKnob (glo::ids::clickTone, "COLOR");
    transientPanel.addKnob (glo::ids::clickDecay, "LENGTH");
    transientPanel.addKnob (glo::ids::toneKeytrack, "KEYTRACK");

    addAndMakeVisible (pitchPanel);
    addAndMakeVisible (envelopePanel);
    addAndMakeVisible (tonePanel);
    addAndMakeVisible (transientPanel);

    addAndMakeVisible (keyboardCard);
    addAndMakeVisible (keyboard);
    keyboard.setAvailableRange (24, 84);
    keyboard.setLowestVisibleKey (24);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setKeyWidth (31.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId,
                        glo::ui::GloPalette::canvas());
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId,
                        glo::ui::GloPalette::accent());
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId,
                        glo::ui::GloPalette::accent().withAlpha (0.30f));
    keyboard.setColour (juce::MidiKeyboardComponent::textLabelColourId,
                        glo::ui::GloPalette::accent().withAlpha (0.85f));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId,
                        glo::ui::GloPalette::accentHot());
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                        glo::ui::GloPalette::accent().withAlpha (0.24f));

    statusLabel.setText (juce::String::fromUTF8 (
                             "4× OVERSAMPLED DRIVE  •  TUNED MIDI  •  SUB-SAFE STEREO  •  v")
                             + JucePlugin_VersionString,
                         juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, glo::ui::GloPalette::textMuted());
    statusLabel.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    statusLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (statusLabel);

    presetBrowser = std::make_unique<PresetBrowserOverlay> (processor.getPresetManager());
    addChildComponent (*presetBrowser);

    setResizable (true, true);
    setResizeLimits (1152, 720, 1920, 1200);
    if (auto* boundsConstrainer = getConstrainer())
        boundsConstrainer->setFixedAspectRatio (static_cast<double> (designWidth / designHeight));
    setSize (1440, 900);
    updatePresetDisplay();
    startTimerHz (30);
}

EightOhEightGloProAudioProcessorEditor::~EightOhEightGloProAudioProcessorEditor()
{
    stopTimer();
    fileChooser.reset();
    presetBrowser.reset();
    setLookAndFeel (nullptr);
}

void EightOhEightGloProAudioProcessorEditor::paint (juce::Graphics& graphics)
{
    juce::ColourGradient background (glo::ui::GloPalette::canvas(), 0.0f, 0.0f,
                                     juce::Colour::fromRGB (5, 12, 8),
                                     0.0f, static_cast<float> (getHeight()), false);
    graphics.setGradientFill (background);
    graphics.fillAll();

    graphics.setColour (glo::ui::GloPalette::accent().withAlpha (0.018f));
    const auto spacing = juce::jmax (12, juce::roundToInt (20.0f *
        std::min (getWidth() / designWidth, getHeight() / designHeight)));
    for (int x = 0; x < getWidth(); x += spacing)
        graphics.drawVerticalLine (x, 0.0f, static_cast<float> (getHeight()));
    for (int y = 0; y < getHeight(); y += spacing)
        graphics.drawHorizontalLine (y, 0.0f, static_cast<float> (getWidth()));
}

void EightOhEightGloProAudioProcessorEditor::resized()
{
    headerCard.setBounds (designRect (32, 24, 1536, 80));
    logo.setBounds (designRect (48, 34, 332, 60));

    presetCategoryLabel.setBounds (designRect (420, 36, 140, 19));
    previousPresetButton.setBounds (designRect (404, 57, 46, 35));
    presetNameButton.setBounds (designRect (458, 52, 470, 44));
    nextPresetButton.setBounds (designRect (936, 57, 46, 35));
    browserButton.setBounds (designRect (997, 52, 112, 44));
    loadButton.setBounds (designRect (1210, 52, 96, 44));
    saveButton.setBounds (designRect (1318, 52, 118, 44));

    voicePanel.setBounds (designRect (32, 120, 342, 506));
    scopeCard.setBounds (designRect (394, 120, 812, 506));
    scope.setBounds (designRect (423, 171, 754, 250));
    punchHero.setBounds (designRect (468, 430, 154, 190));
    gloHero.setBounds (designRect (704, 422, 192, 200));
    tailHero.setBounds (designRect (978, 430, 154, 190));
    masterPanel.setBounds (designRect (1226, 120, 240, 506));
    outputMeter.setBounds (designRect (1480, 157, 68, 434));

    pitchPanel.setBounds (designRect (32, 644, 369, 164));
    envelopePanel.setBounds (designRect (421, 644, 369, 164));
    tonePanel.setBounds (designRect (810, 644, 369, 164));
    transientPanel.setBounds (designRect (1199, 644, 369, 164));

    keyboardCard.setBounds (designRect (32, 824, 1536, 134));
    keyboard.setBounds (designRect (57, 868, 1486, 72));
    keyboard.setKeyWidth (std::max (18.0f, static_cast<float> (keyboard.getWidth()) / 36.0f));
    statusLabel.setBounds (designRect (32, 970, 1536, 18));

    if (presetBrowser != nullptr)
        presetBrowser->setBounds (getLocalBounds());
}

bool EightOhEightGloProAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if ((key.getModifiers().isCommandDown() || key.getModifiers().isCtrlDown())
        && (key.getKeyCode() == 'f' || key.getKeyCode() == 'F'))
    {
        showPresetBrowser();
        return true;
    }
    return false;
}

void EightOhEightGloProAudioProcessorEditor::timerCallback()
{
    processor.applyPendingHostNotesToKeyboard();

    const auto leftPeak = processor.getPeakLevel (0);
    const auto rightPeak = processor.getPeakLevel (1);
    logo.setSignalIntensity (juce::jlimit (0.0f, 1.0f, std::max (leftPeak, rightPeak)));
    outputMeter.setLevels (leftPeak, rightPeak, processor.consumeCeilingHit());
    updatePresetDisplay();
}

void EightOhEightGloProAudioProcessorEditor::updatePresetDisplay()
{
    auto& manager = processor.getPresetManager();
    auto displayName = manager.getCurrentPresetName();
    if (manager.isDirty())
        displayName += juce::String::fromUTF8 ("  •");
    presetNameButton.setButtonText (displayName.toUpperCase());
    presetCategoryLabel.setText (manager.getCurrentPresetCategory().toUpperCase(),
                                 juce::dontSendNotification);
}

void EightOhEightGloProAudioProcessorEditor::showPresetBrowser()
{
    if (presetBrowser != nullptr)
        presetBrowser->open();
}

void EightOhEightGloProAudioProcessorEditor::saveUserPreset()
{
    const auto initial = processor.getPresetManager().getDefaultUserPresetDirectory()
        .getChildFile (processor.getPresetManager().getCurrentPresetName() + ".808glo");
    fileChooser = std::make_unique<juce::FileChooser> ("Save 808Glo Pro preset", initial,
                                                       "*.808glo", true);
    const auto chooserFlags = juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::canSelectFiles
                            | juce::FileBrowserComponent::warnAboutOverwriting;
    fileChooser->launchAsync (chooserFlags, [safe = juce::Component::SafePointer (this)] (const juce::FileChooser& chooser)
    {
        if (safe == nullptr)
            return;

        const auto selected = chooser.getResult();
        const auto wasCancelled = selected == juce::File();
        const auto saved = ! wasCancelled
                        && safe->processor.getPresetManager().saveUserPreset (
                               selected, selected.getFileNameWithoutExtension());

        safe->fileChooser.reset();

        if (saved)
            safe->updatePresetDisplay();
        else if (! wasCancelled)
            showPresetError (safe.getComponent(),
                             "Unable to Save Preset",
                             "The preset file could not be written. Check the folder permissions and available disk space.");
    });
}

void EightOhEightGloProAudioProcessorEditor::loadUserPreset()
{
    fileChooser = std::make_unique<juce::FileChooser> (
        "Load 808Glo Pro preset", processor.getPresetManager().getDefaultUserPresetDirectory(),
        "*.808glo", true);
    const auto chooserFlags = juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles;
    fileChooser->launchAsync (chooserFlags, [safe = juce::Component::SafePointer (this)] (const juce::FileChooser& chooser)
    {
        if (safe == nullptr)
            return;

        const auto selected = chooser.getResult();
        const auto wasCancelled = selected == juce::File();
        const auto loaded = ! wasCancelled
                         && selected.existsAsFile()
                         && safe->processor.getPresetManager().loadUserPreset (selected);

        safe->fileChooser.reset();

        if (loaded)
            safe->updatePresetDisplay();
        else if (! wasCancelled)
            showPresetError (safe.getComponent(),
                             "Unable to Load Preset",
                             "The selected file is not a valid 808Glo Pro preset or could not be read.");
    });
}

juce::Rectangle<int> EightOhEightGloProAudioProcessorEditor::designRect (
    float x, float y, float width, float height) const
{
    const auto scale = std::min (static_cast<float> (getWidth()) / designWidth,
                                 static_cast<float> (getHeight()) / designHeight);
    const auto offsetX = (static_cast<float> (getWidth()) - designWidth * scale) * 0.5f;
    const auto offsetY = (static_cast<float> (getHeight()) - designHeight * scale) * 0.5f;
    return juce::Rectangle<float> (offsetX + x * scale, offsetY + y * scale,
                                   width * scale, height * scale).toNearestInt();
}
