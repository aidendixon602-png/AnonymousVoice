#include "MainComponent.h"

#if JUCE_WINDOWS
 #include <windows.h>
#endif

namespace
{
    const auto background = juce::Colour::fromRGB(5, 8, 14);
    const auto panel = juce::Colour::fromRGB(10, 15, 24);
    const auto panelRaised = juce::Colour::fromRGB(15, 21, 33);
    const auto border = juce::Colour::fromRGB(37, 45, 63);
    const auto text = juce::Colour::fromRGB(244, 246, 252);
    const auto muted = juce::Colour::fromRGB(147, 157, 178);
    const auto purple = juce::Colour::fromRGB(161, 72, 255);
    const auto teal = juce::Colour::fromRGB(40, 209, 204);
    const auto green = juce::Colour::fromRGB(45, 214, 119);

    void startupCheckpoint(const wchar_t* message)
    {
       #if JUCE_WINDOWS
        ::MessageBoxW(nullptr, message, L"AnonymousVoice UI diagnostic", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
       #else
        juce::ignoreUnused(message);
       #endif
    }
}

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

MainComponent::EffectCard::EffectCard(juce::String nameToUse,
                                      juce::String primaryLabelToUse,
                                      double minValue,
                                      double maxValue,
                                      double defaultValue,
                                      juce::String suffixToUse)
    : effectName(std::move(nameToUse)), primaryLabel(std::move(primaryLabelToUse)), suffix(std::move(suffixToUse))
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

MainComponent::MainComponent()
{
    setSize(1600, 900);
    startupCheckpoint(L"UI Checkpoint A: MainComponent entered and setSize completed.");

    brandLabel.setText("ANONYMOUS", juce::dontSendNotification);
    brandLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    brandLabel.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(brandLabel);

    brandSubLabel.setText("VOICE CHANGER", juce::dontSendNotification);
    brandSubLabel.setFont(juce::FontOptions(12.0f));
    brandSubLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(brandSubLabel);

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
    startupCheckpoint(L"UI Checkpoint B: Sidebar, search, and category controls completed.");

    const std::array<std::pair<const char*, const char*>, 10> cards {{
        {"Anonymous", "A"}, {"Deep", "D"}, {"Demon", "X"}, {"Robot", "R"}, {"Alien", "O"},
        {"Ethereal", "E"}, {"Baby", "B"}, {"Radio", "Q"}, {"Witch", "W"}, {"Custom", "+"}
    }};

    for (size_t i = 0; i < voiceCards.size(); ++i)
    {
        voiceCards[i] = std::make_unique<VoiceCard>(cards[i].first, cards[i].second, i == 0);
        addAndMakeVisible(*voiceCards[i]);
    }
    startupCheckpoint(L"UI Checkpoint C: Voice preset cards completed.");

    presetTitle.setText("PRESET DETAILS", juce::dontSendNotification);
    presetTitle.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    presetTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(presetTitle);

    presetName.setText("Anonymous  •  Stealth", juce::dontSendNotification);
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
    for (auto* label : { &pitchLabel, &formantLabel, &mixLabel }) { label->setColour(juce::Label::textColourId, muted); addAndMakeVisible(*label); }

    configureHorizontalSlider(pitchSlider, -12.0, 12.0, -5.0, 0.1);
    configureHorizontalSlider(formantSlider, -12.0, 12.0, -3.0, 0.1);
    configureHorizontalSlider(mixSlider, 0.0, 100.0, 100.0, 1.0);
    pitchSlider.setTextValueSuffix(" st");
    mixSlider.setTextValueSuffix("%");
    addAndMakeVisible(pitchSlider); addAndMakeVisible(formantSlider); addAndMakeVisible(mixSlider);

    testVoiceButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(35, 22, 55));
    testVoiceButton.setColour(juce::TextButton::textColourOffId, text);
    addAndMakeVisible(testVoiceButton);

    editorTitle.setText("VOICE EDITOR", juce::dontSendNotification);
    editorTitle.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    editorTitle.setColour(juce::Label::textColourId, text);
    addAndMakeVisible(editorTitle);
    startupCheckpoint(L"UI Checkpoint D: Preset details and top sliders completed.");

    effectCards[0] = std::make_unique<EffectCard>("Gate", "Threshold", -80.0, 0.0, -45.0, " dB");
    effectCards[1] = std::make_unique<EffectCard>("Pitch", "Pitch Shift", -12.0, 12.0, -5.0, " st");
    effectCards[2] = std::make_unique<EffectCard>("Formant", "Formant Shift", -12.0, 12.0, -3.0, "");
    effectCards[3] = std::make_unique<EffectCard>("EQ", "Low Shelf", -12.0, 12.0, 4.0, " dB");
    effectCards[4] = std::make_unique<EffectCard>("Compressor", "Threshold", -60.0, 0.0, -18.0, " dB");
    effectCards[5] = std::make_unique<EffectCard>("Distortion", "Drive", 0.0, 100.0, 8.0, "%");
    effectCards[6] = std::make_unique<EffectCard>("Chorus", "Depth", 0.0, 100.0, 6.0, "%");
    effectCards[7] = std::make_unique<EffectCard>("Reverb", "Room Size", 0.0, 100.0, 20.0, "%");
    effectCards[8] = std::make_unique<EffectCard>("Limiter", "Threshold", -12.0, 0.0, -1.0, " dB");
    for (auto& effect : effectCards) addAndMakeVisible(*effect);
    startupCheckpoint(L"UI Checkpoint E: Effect cards completed.");

    inputLabel.setText("AUDIO INPUT", juce::dontSendNotification);
    monitorLabel.setText("LIVE MONITOR", juce::dontSendNotification);
    outputLabel.setText("OUTPUT DEVICE (VIRTUAL)", juce::dontSendNotification);
    for (auto* label : { &inputLabel, &monitorLabel, &outputLabel }) { label->setColour(juce::Label::textColourId, muted); label->setFont(juce::FontOptions(11.0f, juce::Font::bold)); addAndMakeVisible(*label); }

    inputBox.addItem("Default Microphone", 1); inputBox.setSelectedId(1);
    monitorBox.addItem("Default Headphones", 1); monitorBox.setSelectedId(1);
    outputBox.addItem("Anonymous Virtual Microphone", 1); outputBox.setSelectedId(1);
    for (auto* box : { &inputBox, &monitorBox, &outputBox }) { box->setColour(juce::ComboBox::backgroundColourId, background); box->setColour(juce::ComboBox::outlineColourId, border); box->setColour(juce::ComboBox::textColourId, text); addAndMakeVisible(*box); }

    monitorToggle.setToggleState(true, juce::dontSendNotification); monitorToggle.setColour(juce::ToggleButton::textColourId, muted); addAndMakeVisible(monitorToggle);
    monitorVolume.setSliderStyle(juce::Slider::LinearHorizontal); monitorVolume.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0); monitorVolume.setRange(0.0, 1.0, 0.01); monitorVolume.setValue(0.72); monitorVolume.setColour(juce::Slider::trackColourId, purple); monitorVolume.setColour(juce::Slider::thumbColourId, purple); monitorVolume.setColour(juce::Slider::backgroundColourId, border); addAndMakeVisible(monitorVolume);

    statusLabel.setText("Protection Active", juce::dontSendNotification); statusLabel.setColour(juce::Label::textColourId, green); addAndMakeVisible(statusLabel);
    cpuLabel.setText("CPU: --", juce::dontSendNotification); latencyLabel.setText("Latency: --", juce::dontSendNotification); versionLabel.setText("v0.2.0-dev", juce::dontSendNotification);
    for (auto* label : { &cpuLabel, &latencyLabel, &versionLabel }) { label->setColour(juce::Label::textColourId, muted); addAndMakeVisible(*label); }
    startupCheckpoint(L"UI Checkpoint F: I/O and footer controls completed.");

    pitchSlider.onValueChange = [this] { if (effectCards[1]) effectCards[1]->slider().setValue(pitchSlider.getValue(), juce::dontSendNotification); };
    formantSlider.onValueChange = [this] { if (effectCards[2]) effectCards[2]->slider().setValue(formantSlider.getValue(), juce::dontSendNotification); };

    setAudioChannels(1, 2);
    startTimerHz(30);
    startupCheckpoint(L"UI Checkpoint G: MainComponent constructor completed.");
}

MainComponent::~MainComponent()
{
    stopTimer();
    shutdownAudio();
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

void MainComponent::syncDspParameters()
{
    if (!effectCards[0] || !effectCards[4] || !effectCards[5] || !effectCards[6] || !effectCards[7] || !effectCards[8])
        return;

    auto& p = dspEngine.parameters();
    p.gateThresholdDb.store(static_cast<float>(effectCards[0]->slider().getValue()));
    p.compressorThresholdDb.store(static_cast<float>(effectCards[4]->slider().getValue()));
    p.distortionDrive.store(static_cast<float>(effectCards[5]->slider().getValue() / 100.0));
    p.chorusDepth.store(static_cast<float>(effectCards[6]->slider().getValue() / 100.0));
    p.reverbMix.store(static_cast<float>(effectCards[7]->slider().getValue() / 100.0));
    p.limiterThresholdDb.store(static_cast<float>(effectCards[8]->slider().getValue()));
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    estimatedLatencyMs.store(sampleRate > 0.0
        ? (1000.0 * static_cast<double>(samplesPerBlockExpected) / sampleRate)
        : 0.0);

    dspEngine.prepare(sampleRate, samplesPerBlockExpected, 2);
    virtualMicOutput.prepare(static_cast<int>(juce::jmax(48000.0, sampleRate)));
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;
    if (buffer == nullptr || buffer->getNumChannels() <= 0 || bufferToFill.numSamples <= 0)
        return;

    const auto start = bufferToFill.startSample;
    const auto count = bufferToFill.numSamples;
    const auto inputLevel = buffer->getMagnitude(0, start, count);
    inputPeak.store(juce::jmax(inputPeak.load() * 0.90f, inputLevel));

    for (int channel = 1; channel < buffer->getNumChannels(); ++channel)
        buffer->copyFrom(channel, start, *buffer, 0, start, count);

    dspEngine.process(*buffer, start, count);

    const auto processedPeak = buffer->getMagnitude(0, start, count);
    outputPeak.store(juce::jmax(outputPeak.load() * 0.90f, processedPeak));

    virtualMicOutput.pushMono(buffer->getReadPointer(0, start), count);

    const auto monitorGain = monitorToggle.getToggleState() ? static_cast<float>(monitorVolume.getValue()) : 0.0f;
    for (int channel = 0; channel < buffer->getNumChannels(); ++channel)
        buffer->applyGain(channel, start, count, monitorGain * masterGain.load());
}

void MainComponent::releaseResources()
{
    dspEngine.reset();
    virtualMicOutput.reset();
}

void MainComponent::timerCallback()
{
    syncDspParameters();
    repaint();
    cpuLabel.setText("CPU: " + juce::String(deviceManager.getCpuUsage() * 100.0, 1) + "%", juce::dontSendNotification);
    latencyLabel.setText("Latency: " + juce::String(estimatedLatencyMs.load(), 1) + " ms", juce::dontSendNotification);
}

void MainComponent::drawPanel(juce::Graphics& g, juce::Rectangle<float> area, float radius) const
{
    g.setColour(panel); g.fillRoundedRectangle(area, radius);
    g.setColour(border); g.drawRoundedRectangle(area, radius, 1.0f);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(background);
    drawPanel(g, sidebarArea.toFloat(), 18.0f);
    drawPanel(g, voiceboxArea.toFloat(), 14.0f);
    drawPanel(g, presetArea.toFloat(), 14.0f);
    drawPanel(g, editorArea.toFloat(), 14.0f);
    drawPanel(g, ioArea.toFloat(), 14.0f);

    auto visualArea = presetArea;
    auto visual = visualArea.removeFromRight(260).reduced(45, 60).toFloat();
    auto circle = visual.withSizeKeepingCentre(160.0f, 160.0f);
    g.setColour(purple.withAlpha(0.13f)); g.fillEllipse(circle.expanded(16.0f));
    g.setColour(purple.withAlpha(0.90f)); g.drawEllipse(circle, 2.0f);
    g.setColour(panelRaised); g.fillEllipse(circle.reduced(12.0f));
    g.setColour(text); g.setFont(juce::FontOptions(42.0f, juce::Font::bold)); g.drawText("A", circle.toNearestInt(), juce::Justification::centred);

    auto drawMeter = [&g](juce::Rectangle<float> b, float level)
    {
        g.setColour(border); g.fillRoundedRectangle(b, 3.0f);
        auto filled = b; filled.setWidth(b.getWidth() * juce::jlimit(0.0f, 1.0f, level));
        g.setColour(level > 0.85f ? juce::Colours::orange : teal); g.fillRoundedRectangle(filled, 3.0f);
    };

    drawMeter({ (float) ioArea.getX() + 55.0f, (float) ioArea.getBottom() - 25.0f, 250.0f, 6.0f }, inputPeak.load());
    drawMeter({ (float) ioArea.getRight() - 315.0f, (float) ioArea.getBottom() - 25.0f, 250.0f, 6.0f }, outputPeak.load());
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(12);
    auto footer = bounds.removeFromBottom(24);
    statusLabel.setBounds(footer.removeFromLeft(260)); cpuLabel.setBounds(footer.removeFromLeft(120)); latencyLabel.setBounds(footer.removeFromLeft(180)); versionLabel.setBounds(footer.removeFromRight(110));

    auto content = bounds;
    sidebarArea = content.removeFromLeft(215); content.removeFromLeft(12);
    ioArea = content.removeFromBottom(105); content.removeFromBottom(10);
    editorArea = content.removeFromBottom(340); content.removeFromBottom(10);
    auto top = content; voiceboxArea = top.removeFromLeft((int) (top.getWidth() * 0.52f)); top.removeFromLeft(10); presetArea = top;

    auto side = sidebarArea.reduced(16);
    brandLabel.setBounds(side.removeFromTop(32)); brandSubLabel.setBounds(side.removeFromTop(22)); side.removeFromTop(28);
    for (auto* button : { &voiceboxButton, &soundboardButton, &voiceLabButton, &keybindsButton, &settingsButton }) { button->setBounds(side.removeFromTop(48)); side.removeFromTop(10); }
    userLabel.setBounds(side.removeFromBottom(64));

    auto vb = voiceboxArea.reduced(18);
    voiceboxTitle.setBounds(vb.removeFromTop(28)); vb.removeFromTop(8);
    auto searchRow = vb.removeFromTop(38); searchEditor.setBounds(searchRow.removeFromLeft(searchRow.getWidth() - 175)); searchRow.removeFromLeft(10); categoryBox.setBounds(searchRow); vb.removeFromTop(12);
    const int gap = 10, cols = 5, rows = 2;
    const int cardWidth = (vb.getWidth() - gap * (cols - 1)) / cols;
    const int cardHeight = (vb.getHeight() - gap * (rows - 1)) / rows;
    for (int i = 0; i < (int) voiceCards.size(); ++i) { const int row = i / cols, col = i % cols; voiceCards[(size_t) i]->setBounds(vb.getX() + col * (cardWidth + gap), vb.getY() + row * (cardHeight + gap), cardWidth, cardHeight); }

    auto pr = presetArea.reduced(18);
    presetTitle.setBounds(pr.removeFromTop(28)); presetName.setBounds(pr.removeFromTop(28)); presetDescription.setBounds(pr.removeFromTop(46));
    auto controls = pr.removeFromLeft(300); controls.removeFromTop(8);
    auto placeSlider = [&controls](juce::Label& label, juce::Slider& slider) { label.setBounds(controls.removeFromTop(20)); slider.setBounds(controls.removeFromTop(38)); controls.removeFromTop(8); };
    placeSlider(pitchLabel, pitchSlider); placeSlider(formantLabel, formantSlider); placeSlider(mixLabel, mixSlider); testVoiceButton.setBounds(controls.removeFromTop(38).removeFromLeft(180));

    auto editor = editorArea.reduced(16); editorTitle.setBounds(editor.removeFromTop(28)); editor.removeFromTop(10);
    const int fxGap = 8, fxCount = (int) effectCards.size();
    const int fxWidth = (editor.getWidth() - fxGap * (fxCount - 1)) / fxCount;
    for (int i = 0; i < fxCount; ++i) effectCards[(size_t) i]->setBounds(editor.getX() + i * (fxWidth + fxGap), editor.getY(), fxWidth, editor.getHeight());

    auto io = ioArea.reduced(18); const int third = io.getWidth() / 3;
    auto inputColumn = io.removeFromLeft(third).reduced(10, 0); auto monitorColumn = io.removeFromLeft(third).reduced(10, 0); auto outputColumn = io.reduced(10, 0);
    inputLabel.setBounds(inputColumn.removeFromTop(20)); inputBox.setBounds(inputColumn.removeFromTop(36));
    monitorLabel.setBounds(monitorColumn.removeFromTop(20)); monitorBox.setBounds(monitorColumn.removeFromTop(36)); monitorToggle.setBounds(monitorColumn.removeFromLeft(115).removeFromTop(28)); monitorVolume.setBounds(monitorColumn.removeFromTop(28));
    outputLabel.setBounds(outputColumn.removeFromTop(20)); outputBox.setBounds(outputColumn.removeFromTop(36));
}
