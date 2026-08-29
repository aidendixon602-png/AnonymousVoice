#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(900, 600);

    gainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    gainSlider.setRange(0.0, 2.0, 0.01);
    gainSlider.setValue(1.0);
    gainSlider.setTextValueSuffix(" x");

    gainSlider.onValueChange = [this]
    {
        gain.store(static_cast<float>(gainSlider.getValue()));
    };

    addAndMakeVisible(gainSlider);

    gainLabel.setText(
        "Microphone Gain",
        juce::dontSendNotification);

    addAndMakeVisible(gainLabel);

    statusLabel.setText(
        "Audio engine running",
        juce::dontSendNotification);

    addAndMakeVisible(statusLabel);

    setAudioChannels(1, 2);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay(
    int samplesPerBlockExpected,
    double sampleRate)
{
    juce::ignoreUnused(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;

    if (buffer == nullptr)
        return;

    const auto currentGain = gain.load();

    for (int channel = 0;
         channel < buffer->getNumChannels();
         ++channel)
    {
        buffer->applyGain(
            channel,
            bufferToFill.startSample,
            bufferToFill.numSamples,
            currentGain);
    }
}

void MainComponent::releaseResources()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(12, 12, 18));

    g.setColour(juce::Colour::fromRGB(170, 80, 255));
    g.setFont(32.0f);

    g.drawText(
        "Anonymous Voice",
        30,
        30,
        getWidth() - 60,
        50,
        juce::Justification::centredLeft);
}

void MainComponent::resized()
{
    const int left = 40;
    const int width = getWidth() - 80;

    gainLabel.setBounds(left, 120, width, 30);
    gainSlider.setBounds(left, 160, width, 50);
    statusLabel.setBounds(left, 240, width, 30);
}