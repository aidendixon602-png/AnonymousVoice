#include "MainComponent.h"

using namespace Colors;

//==============================================================================
// Voice Card Implementation
//==============================================================================

MainComponent::VoiceCard::VoiceCard(juce::String nameToUse, juce::String iconToUse, bool selectedToUse)
    : name(std::move(nameToUse)), icon(std::move(iconToUse)), selected(selectedToUse)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void MainComponent::VoiceCard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(selected ? juce::Colour::fromRGB(26, 18, 45) : panelRaised);
    g.fillRoundedRectangle(bounds, 12.0f);
    g.setColour(selected ? purple : border);
    g.drawRoundedRectangle(bounds, 12.0f, selected ? 2.0f : 1.0f);

    g.setColour(selected ? purple : muted);
    g.setFont(juce::FontOptions(29.0f, juce::Font::bold));
    g.drawText(icon, getLocalBounds().removeFromTop(58), juce::Justification::centred);

    g.setColour(text);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawFittedText(name, getLocalBounds().withTrimmedTop(58).removeFromTop(26), juce::Justification::centred, 1);

    g.setColour(selected ? purple.brighter(0.25f) : muted);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(selected ? "Stealth" : "Preset", getLocalBounds().removeFromBottom(22), juce::Justification::centred);
}

void MainComponent::VoiceCard::mouseUp(const juce::MouseEvent&)
{
    if (onClick) onClick();
}

//==============================================================================
// Effect Card Implementation
//==============================================================================

MainComponent::EffectCard::EffectCard(juce::String nameToUse,
                                      juce::String primaryLabelToUse,
                                      double minValue,
                                      double maxValue,
                                      double defaultValue,
                                      juce::String suffixToUse)
    : effectName(std::move(nameToUse)), 
      primaryLabel(std::move(primaryLabelToUse)), 
      suffix(std::move(suffixToUse))
{
    enabled.setToggleState(true, juce::dontSendNotification);
    enabled.setButtonText({});
    addAndMakeVisible(enabled);

    amount.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amount.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 82, 20);
    amount.setRange(minValue, maxValue, 0.01);
    amount.setValue(defaultValue);
    amount.setTextValueSuffix(suffix);
    amount.setColour(juce::Slider::rotarySliderFillColourId, purple);
    amount.setColour(juce::Slider::rotarySliderOutlineColourId, border);
    amount.setColour(juce::Slider::thumbColourId, text);
    amount.setColour(juce::Slider::textBoxTextColourId, muted);
    amount.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    amount.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(amount);
}

void MainComponent::EffectCard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(panelRaised);
    g.fillRoundedRectangle(bounds, 12.0f);
    g.setColour(border);
    g.drawRoundedRectangle(bounds, 12.0f, 1.0f);

    g.setColour(text);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawFittedText(effectName, 10, 8, getWidth() - 20, 22, juce::Justification::centredLeft, 1);
    g.setColour(muted);
    g.setFont(juce::FontOptions(11.0f));
    g.drawFittedText(primaryLabel, 10, 42, getWidth() - 20, 18, juce::Justification::centred, 1);
}

void MainComponent::EffectCard::resized()
{
    enabled.setBounds(getWidth() - 30, 8, 20, 20);
    amount.setBounds(10, 58, getWidth() - 20, getHeight() - 66);
}

//==============================================================================
// Main Component Constructor
//==============================================================================

MainComponent::MainComponent()
{
    setSize(1600, 900);

    // Branding
    brandLabel.setText("ANONYMOUS", juce::dontSendNotification);
    brandLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    brandLabel.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(brandLabel);

    brandSubLabel.setText("VOICE CHANGER", juce::dontSendNotification);
    brandSubLabel.setFont(juce::FontOptions(12.0f));
    brandSubLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(brandSubLabel);

    // Navigation buttons
    configureNavButton(voiceboxButton, true);
    configureNavButton(soundboardButton, false);
    configureNavButton(voiceLabButton, false);
    configureNavButton(settingsButton, false);
    configureNavButton(keybindsButton, false);
    for (auto* button : { &voiceboxButton, &soundboardButton, &voiceLabButton, &keybindsButton, &settingsButton })
        addAndMakeVisible(*button);

    userLabel.setText("Anonymous\nOnline", juce::dontSendNotification);
    userLabel.setColour(juce::Label::textColourId, green);
    addAndMakeVisible(userLabel);

    // Voicebox section
    voiceboxTitle.setText("VOICE PRESETS", juce::dontSendNotification);
    voiceboxTitle.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    voiceboxTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(voiceboxTitle);

    searchEditor.setTextToShowWhenEmpty("Search voices...", muted);
    searchEditor.setColour(juce::TextEditor::backgroundColourId, background);
    searchEditor.setColour(juce::TextEditor::outlineColourId, border);
    searchEditor.setColour(juce::TextEditor::focusedOutlineColourId, purple);
    searchEditor.setColour(juce::TextEditor::textColourId, text);
    addAndMakeVisible(searchEditor);

    categoryBox.addItem("All Categories", 1);
    categoryBox.addItem("Character", 2);
    categoryBox.addItem("Utility", 3);
    categoryBox.setSelectedId(1);
    categoryBox.setColour(juce::ComboBox::backgroundColourId, background);
    categoryBox.setColour(juce::ComboBox::outlineColourId, border);
    categoryBox.setColour(juce::ComboBox::textColourId, text);
    addAndMakeVisible(categoryBox);

    // Voice cards
    const std::array<std::pair<const char*, const char*>, 10> cards {{
        {"Anonymous", "A"}, {"Deep", "D"}, {"Demon", "X"}, {"Robot", "R"}, {"Alien", "O"},
        {"Ethereal", "E"}, {"Baby", "B"}, {"Radio", "Q"}, {"Witch", "W"}, {"Custom", "+"}
    }};

    for (size_t i = 0; i < voiceCards.size(); ++i)
    {
        voiceCards[i] = std::make_unique<VoiceCard>(cards[i].first, cards[i].second, i == 0);
        voiceCards[i]->onClick = [this, i] { selectVoiceCard(static_cast<int>(i)); };
        addAndMakeVisible(*voiceCards[i]);
    }

    // Preset details section
    presetTitle.setText("PRESET DETAILS", juce::dontSendNotification);
    presetTitle.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    presetTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(presetTitle);

    presetName.setText("Anonymous  \u2022  Stealth", juce::dontSendNotification);
    presetName.setFont(juce::FontOptions(17.0f, juce::Font::bold));
    presetName.setColour(juce::Label::textColourId, purple);
    addAndMakeVisible(presetName);

    presetDescription.setText("Designed to make your voice harder to recognize while preserving clear speech.", juce::dontSendNotification);
    presetDescription.setColour(juce::Label::textColourId, muted);
    presetDescription.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(presetDescription);

    pitchLabel.setText("Pitch", juce::dontSendNotification);
    formantLabel.setText("Formant", juce::dontSendNotification);
    mixLabel.setText("Mix", juce::dontSendNotification);
    for (auto* label : { &pitchLabel, &formantLabel, &mixLabel }) 
    {
        label->setColour(juce::Label::textColourId, muted); 
        addAndMakeVisible(*label); 
    }

    configureHorizontalSlider(pitchSlider, -12.0, 12.0, -5.0, 0.1);
    configureHorizontalSlider(formantSlider, -12.0, 12.0, -3.0, 0.1);
    configureHorizontalSlider(mixSlider, 0.0, 100.0, 100.0, 1.0);
    pitchSlider.setTextValueSuffix(" st");
    mixSlider.setTextValueSuffix("%");
    addAndMakeVisible(pitchSlider); 
    addAndMakeVisible(formantSlider); 
    addAndMakeVisible(mixSlider);

    testVoiceButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(35, 22, 55));
    testVoiceButton.setColour(juce::TextButton::textColourOffId, text);
    addAndMakeVisible(testVoiceButton);

    // Voice editor section
    editorTitle.setText("VOICE EDITOR", juce::dontSendNotification);
    editorTitle.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    editorTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(editorTitle);

    // Effect cards
    effectCards[0] = std::make_unique<EffectCard>("Gate", "Threshold", -80.0, 0.0, -45.0, " dB");
    effectCards[1] = std::make_unique<EffectCard>("Pitch", "Pitch Shift", -12.0, 12.0, -5.0, " st");
    effectCards[2] = std::make_unique<EffectCard>("Formant", "Formant Shift", -12.0, 12.0, -3.0, "");
    effectCards[3] = std::make_unique<EffectCard>("EQ", "Low Shelf", -12.0, 12.0, 4.0, " dB");
    effectCards[4] = std::make_unique<EffectCard>("Compressor", "Threshold", -60.0, 0.0, -18.0, " dB");
    effectCards[5] = std::make_unique<EffectCard>("Distortion", "Drive", 0.0, 100.0, 8.0, "%");
    effectCards[6] = std::make_unique<EffectCard>("Chorus", "Depth", 0.0, 100.0, 6.0, "%");
    effectCards[7] = std::make_unique<EffectCard>("Reverb", "Room Size", 0.0, 100.0, 20.0, "%");
    effectCards[8] = std::make_unique<EffectCard>("Limiter", "Threshold", -12.0, 0.0, -1.0, " dB");
    
    for (auto& effect : effectCards) 
        addAndMakeVisible(*effect);

    // I/O section
    inputLabel.setText("AUDIO INPUT", juce::dontSendNotification);
    monitorLabel.setText("LIVE MONITOR", juce::dontSendNotification);
    outputLabel.setText("OUTPUT DEVICE (VIRTUAL)", juce::dontSendNotification);
    for (auto* label : { &inputLabel, &monitorLabel, &outputLabel }) 
    {
        label->setColour(juce::Label::textColourId, muted); 
        label->setFont(juce::FontOptions(11.0f, juce::Font::bold)); 
        addAndMakeVisible(*label); 
    }

    // Device selection
    updateDeviceList();
    
    for (auto* box : { &inputBox, &monitorBox, &outputBox }) 
    {
        box->setColour(juce::ComboBox::backgroundColourId, background);
        box->setColour(juce::ComboBox::outlineColourId, border);
        box->setColour(juce::ComboBox::textColourId, text);
        addAndMakeVisible(*box);
    }

    // Monitor controls
    monitorToggle.setToggleState(true, juce::dontSendNotification);
    monitorToggle.setColour(juce::ToggleButton::textColourId, muted);
    addAndMakeVisible(monitorToggle);
    
    monitorVolume.setSliderStyle(juce::Slider::LinearHorizontal);
    monitorVolume.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    monitorVolume.setRange(0.0, 1.0, 0.01);
    monitorVolume.setValue(0.72);
    monitorVolume.setColour(juce::Slider::trackColourId, purple);
    monitorVolume.setColour(juce::Slider::thumbColourId, purple);
    monitorVolume.setColour(juce::Slider::backgroundColourId, border);
    addAndMakeVisible(monitorVolume);

    // Status section
    statusLabel.setText("Protection Active", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, green);
    addAndMakeVisible(statusLabel);
    
    cpuLabel.setText("CPU: --", juce::dontSendNotification);
    latencyLabel.setText("Latency: --", juce::dontSendNotification);
    versionLabel.setText("v0.2.0-dev", juce::dontSendNotification);
    for (auto* label : { &cpuLabel, &latencyLabel, &versionLabel }) 
    {
        label->setColour(juce::Label::textColourId, muted); 
        addAndMakeVisible(*label); 
    }

    // Connect UI controls to DSP parameters (thread-safe via atomics)
    pitchSlider.onValueChange = [this] {
        dspEngine.parameters().pitchSemitones.store(static_cast<float>(pitchSlider.getValue()));
        if (effectCards[1]) effectCards[1]->slider().setValue(pitchSlider.getValue(), juce::dontSendNotification);
    };
    
    formantSlider.onValueChange = [this] {
        dspEngine.parameters().formantShift.store(static_cast<float>(formantSlider.getValue()));
        if (effectCards[2]) effectCards[2]->slider().setValue(formantSlider.getValue(), juce::dontSendNotification);
    };
    
    mixSlider.onValueChange = [this] {
        dspEngine.parameters().wetDryMix.store(static_cast<float>(mixSlider.getValue() / 100.0));
    };
    
    // Effect card parameter connections
    effectCards[0]->slider().onValueChange = [this] {
        dspEngine.parameters().gateThresholdDb.store(static_cast<float>(effectCards[0]->slider().getValue()));
    };
    
    effectCards[1]->slider().onValueChange = [this] {
        dspEngine.parameters().pitchSemitones.store(static_cast<float>(effectCards[1]->slider().getValue()));
        pitchSlider.setValue(effectCards[1]->slider().getValue(), juce::dontSendNotification);
    };
    
    effectCards[2]->slider().onValueChange = [this] {
        dspEngine.parameters().formantShift.store(static_cast<float>(effectCards[2]->slider().getValue()));
        formantSlider.setValue(effectCards[2]->slider().getValue(), juce::dontSendNotification);
    };
    
    effectCards[3]->slider().onValueChange = [this] {
        dspEngine.parameters().eqLowGainDb.store(static_cast<float>(effectCards[3]->slider().getValue()));
    };
    
    effectCards[4]->slider().onValueChange = [this] {
        dspEngine.parameters().compressorThresholdDb.store(static_cast<float>(effectCards[4]->slider().getValue()));
    };
    
    effectCards[5]->slider().onValueChange = [this] {
        dspEngine.parameters().distortionDrive.store(static_cast<float>(effectCards[5]->slider().getValue() / 100.0));
    };
    
    effectCards[6]->slider().onValueChange = [this] {
        dspEngine.parameters().chorusDepth.store(static_cast<float>(effectCards[6]->slider().getValue() / 100.0));
    };
    
    effectCards[7]->slider().onValueChange = [this] {
        dspEngine.parameters().reverbMix.store(static_cast<float>(effectCards[7]->slider().getValue() / 100.0));
    };
    
    effectCards[8]->slider().onValueChange = [this] {
        dspEngine.parameters().limiterThresholdDb.store(static_cast<float>(effectCards[8]->slider().getValue()));
    };
    
    // Monitor controls
    monitorToggle.onStateChange = [this] {
        monitorEnabled.store(monitorToggle.getToggleState());
    };
    
    monitorVolume.onValueChange = [this] {
        monitorVolumeValue.store(static_cast<float>(monitorVolume.getValue()));
    };

    // Initialize audio
    setAudioChannels(1, 2);
    
    // Initialize virtual mic
    virtualMicOutput.setDeviceName("Anonymous Virtual Microphone");
    virtualMicOutput.prepare(48000);
    
    // Start timer for UI updates
    startTimerHz(30);
}

//==============================================================================
// Destructor
//==============================================================================

MainComponent::~MainComponent()
{
    stopTimer();
    shutdownAudio();
    virtualMicOutput.shutdown();
}

//==============================================================================
// Audio Callbacks
//==============================================================================

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    estimatedLatencyMs.store(sampleRate > 0.0
        ? (1000.0 * static_cast<double>(samplesPerBlockExpected) / sampleRate)
        : 0.0);

    dspEngine.prepare(sampleRate, samplesPerBlockExpected, 2);
    virtualMicOutput.prepare(static_cast<int>(std::max(48000.0, sampleRate)));
    
    juce::Logger::writeToLog("Audio prepared: " + 
        juce::String(samplesPerBlockExpected) + " samples, " + 
        juce::String(sampleRate, 1) + " Hz");
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;
    if (buffer == nullptr || buffer->getNumChannels() <= 0 || bufferToFill.numSamples <= 0)
        return;

    const auto start = bufferToFill.startSample;
    const auto count = bufferToFill.numSamples;
    
    // Measure input level
    const auto inputLevel = buffer->getMagnitude(0, start, count);
    inputPeak.store(std::max(inputPeak.load() * 0.90f, inputLevel));

    // Copy input to all channels (mono to stereo)
    for (int channel = 1; channel < buffer->getNumChannels(); ++channel)
        buffer->copyFrom(channel, start, *buffer, 0, start, count);

    // Process through DSP chain
    dspEngine.process(*buffer, start, count);

    // Measure output level
    const auto processedPeak = buffer->getMagnitude(0, start, count);
    outputPeak.store(std::max(outputPeak.load() * 0.90f, processedPeak));

    // Send to virtual microphone
    virtualMicOutput.pushMono(buffer->getReadPointer(0, start), count);

    // Apply monitor output
    const float monitorGain = monitorEnabled.load() ? monitorVolumeValue.load() : 0.0f;
    for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
        buffer->applyGain(channel, start, count, monitorGain * masterGain.load());
}

void MainComponent::releaseResources()
{
    dspEngine.reset();
    virtualMicOutput.reset();
    juce::Logger::writeToLog("Audio resources released");
}

//==============================================================================
// UI Methods
//==============================================================================

void MainComponent::timerCallback()
{
    // Sync DSP parameters from UI
    syncDspParameters();
    
    // Update CPU and latency displays
    cpuLabel.setText("CPU: " + juce::String(deviceManager.getCpuUsage() * 100.0, 1) + "%", juce::dontSendNotification);
    latencyLabel.setText("Latency: " + juce::String(estimatedLatencyMs.load(), 1) + " ms", juce::dontSendNotification);
    
    // Trigger repaint for meters
    repaint();
}

void MainComponent::configureNavButton(juce::TextButton& button, bool selected)
{
    button.setColour(juce::TextButton::buttonColourId, selected ? juce::Colour::fromRGB(31, 23, 49) : juce::Colours::transparentBlack);
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(31, 23, 49));
    button.setColour(juce::TextButton::textColourOffId, selected ? purple : muted);
    button.setColour(juce::TextButton::textColourOnId, purple);
}

void MainComponent::configureHorizontalSlider(juce::Slider& slider, double min, double max, double value, double interval)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 76, 22);
    slider.setRange(min, max, interval);
    slider.setValue(value);
    slider.setColour(juce::Slider::trackColourId, purple);
    slider.setColour(juce::Slider::backgroundColourId, border);
    slider.setColour(juce::Slider::thumbColourId, purple);
    slider.setColour(juce::Slider::textBoxTextColourId, text);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void MainComponent::drawPanel(juce::Graphics& g, juce::Rectangle<float> area, float radius) const
{
    g.setColour(panel); 
    g.fillRoundedRectangle(area, radius);
    g.setColour(border); 
    g.drawRoundedRectangle(area, radius, 1.0f);
}

void MainComponent::syncDspParameters()
{
    // This is called from the timer (UI thread), so we just update any
    // parameters that might have changed from the UI
    // The actual parameter updates happen in the onValueChange callbacks
}

void MainComponent::updateDeviceList()
{
    inputBox.clear();
    monitorBox.clear();
    outputBox.clear();
    
    auto& deviceManager = getDeviceManager();
    auto deviceList = deviceManager.getAvailableDevices();
    
    // Add input devices
    inputBox.addItem("Default Microphone", 1);
    for (auto* device : deviceList)
    {
        if (device->getType() == juce::AudioIODeviceType::input ||
            device->getType() == juce::AudioIODeviceType::duplex)
        {
            inputBox.addItem(device->getName(), inputBox.getNumItems() + 1);
        }
    }
    inputBox.setSelectedId(1);
    
    // Add output devices (for monitoring)
    monitorBox.addItem("Default Headphones", 1);
    for (auto* device : deviceList)
    {
        if (device->getType() == juce::AudioIODeviceType::output ||
            device->getType() == juce::AudioIODeviceType::duplex)
        {
            monitorBox.addItem(device->getName(), monitorBox.getNumItems() + 1);
        }
    }
    monitorBox.setSelectedId(1);
    
    // Output device (virtual mic)
    outputBox.addItem("Anonymous Virtual Microphone", 1);
    outputBox.setSelectedId(1);
}

void MainComponent::loadPreset(int presetIndex)
{
    // Define presets based on README targets
    struct Preset
    {
        juce::String name;
        juce::String description;
        float pitch;
        float formant;
        float eqLow;
        float eqMid;
        float eqHigh;
        float distortion;
        float chorus;
        float reverb;
    };
    
    static const std::array<Preset, 9> presets = {{
        {"Anonymous", "Designed to make your voice harder to recognize while preserving clear speech.", -5.0f, -3.0f, 4.0f, 1.0f, -5.0f, 8.0f, 6.0f, 20.0f},
        {"Deep", "Deep, resonant voice with lower pitch.", -8.0f, -2.0f, 6.0f, 2.0f, -3.0f, 10.0f, 4.0f, 15.0f},
        {"Demon", "Dark, growling demon voice.", -7.0f, -4.0f, 8.0f, 3.0f, -4.0f, 20.0f, 8.0f, 25.0f},
        {"Robot", "Mechanical, synthetic voice.", 0.0f, -6.0f, -2.0f, -1.0f, 5.0f, 0.0f, 15.0f, 30.0f},
        {"Alien", "Otherworldly, strange voice.", 3.0f, 5.0f, -3.0f, 4.0f, 6.0f, 15.0f, 20.0f, 25.0f},
        {"Ethereal", "Light, airy, ethereal voice.", 4.0f, 3.0f, -4.0f, -2.0f, 8.0f, 10.0f, 15.0f, 30.0f},
        {"Baby", "High-pitched, child-like voice.", 7.0f, 4.0f, -5.0f, -3.0f, 5.0f, 5.0f, 10.0f, 20.0f},
        {"Radio", "AM radio style voice.", -2.0f, -1.0f, 3.0f, 0.0f, -6.0f, 5.0f, 5.0f, 10.0f},
        {"Witch", "Mystical, enchanting voice.", -3.0f, -5.0f, 5.0f, 2.0f, -2.0f, 12.0f, 10.0f, 25.0f}
    }};
    
    if (presetIndex >= 0 && presetIndex < static_cast<int>(presets.size()))
    {
        const auto& preset = presets[presetIndex];
        
        // Update preset display
        presetName.setText(preset.name + "  \u2022  Stealth", juce::dontSendNotification);
        presetDescription.setText(preset.description, juce::dontSendNotification);
        
        // Update sliders (these will trigger parameter updates via callbacks)
        pitchSlider.setValue(preset.pitch, juce::dontSendNotification);
        formantSlider.setValue(preset.formant, juce::dontSendNotification);
        
        // Update effect cards
        if (effectCards[0]) effectCards[0]->slider().setValue(-45.0, juce::dontSendNotification);
        if (effectCards[3]) effectCards[3]->slider().setValue(preset.eqLow, juce::dontSendNotification);
        if (effectCards[5]) effectCards[5]->slider().setValue(preset.distortion, juce::dontSendNotification);
        if (effectCards[6]) effectCards[6]->slider().setValue(preset.chorus, juce::dontSendNotification);
        if (effectCards[7]) effectCards[7]->slider().setValue(preset.reverb, juce::dontSendNotification);
        
        // Update DSP parameters directly
        dspEngine.parameters().pitchSemitones.store(preset.pitch);
        dspEngine.parameters().formantShift.store(preset.formant);
        dspEngine.parameters().eqLowGainDb.store(preset.eqLow);
        dspEngine.parameters().distortionDrive.store(preset.distortion / 100.0f);
        dspEngine.parameters().chorusDepth.store(preset.chorus / 100.0f);
        dspEngine.parameters().reverbMix.store(preset.reverb / 100.0f);
    }
}

void MainComponent::selectVoiceCard(int index)
{
    // Update selection
    for (size_t i = 0; i < voiceCards.size(); ++i)
    {
        if (voiceCards[i])
            voiceCards[i]->setSelected(i == static_cast<size_t>(index));
    }
    
    // Load corresponding preset
    loadPreset(index);
}

//==============================================================================
// Painting
//==============================================================================

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(background);
    drawPanel(g, sidebarArea.toFloat(), 18.0f);
    drawPanel(g, voiceboxArea.toFloat(), 14.0f);
    drawPanel(g, presetArea.toFloat(), 14.0f);
    drawPanel(g, editorArea.toFloat(), 14.0f);
    drawPanel(g, ioArea.toFloat(), 14.0f);

    // Draw visual element in preset area
    auto visualArea = presetArea;
    auto visual = visualArea.removeFromRight(260).reduced(45, 60).toFloat();
    auto circle = visual.withSizeKeepingCentre(160.0f, 160.0f);
    g.setColour(purple.withAlpha(0.13f)); 
    g.fillEllipse(circle.expanded(16.0f));
    g.setColour(purple.withAlpha(0.90f)); 
    g.drawEllipse(circle, 2.0f);
    g.setColour(panelRaised); 
    g.fillEllipse(circle.reduced(12.0f));
    g.setColour(text); 
    g.setFont(juce::FontOptions(42.0f, juce::Font::bold)); 
    g.drawText("A", circle.toNearestInt(), juce::Justification::centred);

    // Draw meters
    auto drawMeter = [&g](juce::Rectangle<float> b, float level)
    {
        g.setColour(border); 
        g.fillRoundedRectangle(b, 3.0f);
        auto filled = b; 
        filled.setWidth(b.getWidth() * std::min(1.0f, level));
        g.setColour(level > 0.85f ? orange : teal); 
        g.fillRoundedRectangle(filled, 3.0f);
    };

    drawMeter({ (float) ioArea.getX() + 55.0f, (float) ioArea.getBottom() - 25.0f, 250.0f, 6.0f }, inputPeak.load());
    drawMeter({ (float) ioArea.getRight() - 315.0f, (float) ioArea.getBottom() - 25.0f, 250.0f, 6.0f }, outputPeak.load());
}

//==============================================================================
// Layout
//==============================================================================

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    auto footer = bounds.removeFromBottom(24);
    statusLabel.setBounds(footer.removeFromLeft(260));
    cpuLabel.setBounds(footer.removeFromLeft(120));
    latencyLabel.setBounds(footer.removeFromLeft(180));
    versionLabel.setBounds(footer.removeFromRight(110));

    auto content = bounds;
    sidebarArea = content.removeFromLeft(215); 
    content.removeFromLeft(12);
    ioArea = content.removeFromBottom(105); 
    content.removeFromBottom(10);
    editorArea = content.removeFromBottom(340); 
    content.removeFromBottom(10);
    auto top = content; 
    voiceboxArea = top.removeFromLeft(static_cast<int>(top.getWidth() * 0.52f)); 
    top.removeFromLeft(10); 
    presetArea = top;

    // Sidebar layout
    auto side = sidebarArea.reduced(16);
    brandLabel.setBounds(side.removeFromTop(32));
    brandSubLabel.setBounds(side.removeFromTop(22));
    side.removeFromTop(28);
    for (auto* button : { &voiceboxButton, &soundboardButton, &voiceLabButton, &keybindsButton, &settingsButton }) 
    {
        button->setBounds(side.removeFromTop(48));
        side.removeFromTop(10); 
    }
    userLabel.setBounds(side.removeFromBottom(64));

    // Voicebox layout
    auto vb = voiceboxArea.reduced(18);
    voiceboxTitle.setBounds(vb.removeFromTop(28));
    vb.removeFromTop(8);
    auto searchRow = vb.removeFromTop(38);
    searchEditor.setBounds(searchRow.removeFromLeft(searchRow.getWidth() - 175));
    searchRow.removeFromLeft(10);
    categoryBox.setBounds(searchRow);
    vb.removeFromTop(12);
    
    const int gap = 10, cols = 5, rows = 2;
    const int cardWidth = (vb.getWidth() - gap * (cols - 1)) / cols;
    const int cardHeight = (vb.getHeight() - gap * (rows - 1)) / rows;
    for (int i = 0; i < static_cast<int>(voiceCards.size()); ++i) 
    {
        const int row = i / cols, col = i % cols;
        voiceCards[static_cast<size_t>(i)]->setBounds(
            vb.getX() + col * (cardWidth + gap),
            vb.getY() + row * (cardHeight + gap),
            cardWidth, cardHeight);
    }

    // Preset area layout
    auto pr = presetArea.reduced(18);
    presetTitle.setBounds(pr.removeFromTop(28));
    presetName.setBounds(pr.removeFromTop(28));
    presetDescription.setBounds(pr.removeFromTop(46));
    auto controls = pr.removeFromLeft(300);
    controls.removeFromTop(8);
    
    auto placeSlider = [&controls](juce::Label& label, juce::Slider& slider) 
    {
        label.setBounds(controls.removeFromTop(20));
        slider.setBounds(controls.removeFromTop(38));
        controls.removeFromTop(8);
    };
    placeSlider(pitchLabel, pitchSlider);
    placeSlider(formantLabel, formantSlider);
    placeSlider(mixLabel, mixSlider);
    testVoiceButton.setBounds(controls.removeFromTop(38).removeFromLeft(180));

    // Editor area layout
    auto editor = editorArea.reduced(16);
    editorTitle.setBounds(editor.removeFromTop(28));
    editor.removeFromTop(10);
    const int fxGap = 8, fxCount = static_cast<int>(effectCards.size());
    const int fxWidth = (editor.getWidth() - fxGap * (fxCount - 1)) / fxCount;
    for (int i = 0; i < fxCount; ++i)
    {
        effectCards[static_cast<size_t>(i)]->setBounds(
            editor.getX() + i * (fxWidth + fxGap),
            editor.getY(),
            fxWidth, editor.getHeight());
    }

    // I/O area layout
    auto io = ioArea.reduced(18);
    const int third = io.getWidth() / 3;
    auto inputColumn = io.removeFromLeft(third).reduced(10, 0);
    auto monitorColumn = io.removeFromLeft(third).reduced(10, 0);
    auto outputColumn = io.reduced(10, 0);
    
    inputLabel.setBounds(inputColumn.removeFromTop(20));
    inputBox.setBounds(inputColumn.removeFromTop(36));
    
    monitorLabel.setBounds(monitorColumn.removeFromTop(20));
    monitorBox.setBounds(monitorColumn.removeFromTop(36));
    monitorToggle.setBounds(monitorColumn.removeFromLeft(115).removeFromTop(28));
    monitorVolume.setBounds(monitorColumn.removeFromTop(28));
    
    outputLabel.setBounds(outputColumn.removeFromTop(20));
    outputBox.setBounds(outputColumn.removeFromTop(36));
}
