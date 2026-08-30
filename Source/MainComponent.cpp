#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize(1280, 720);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(5, 8, 14));

    auto area = getLocalBounds().reduced(32);

    g.setColour(juce::Colour::fromRGB(161, 72, 255));
    g.setFont(juce::FontOptions(34.0f, juce::Font::bold));
    g.drawText("ANONYMOUS VOICE", area.removeFromTop(56), juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(244, 246, 252));
    g.setFont(juce::FontOptions(20.0f));
    g.drawText("Stable UI baseline", area.removeFromTop(40), juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(147, 157, 178));
    g.setFont(juce::FontOptions(15.0f));
    g.drawFittedText("The application window is running. Audio, DSP, presets, and device routing are temporarily disabled while the startup crash is isolated.",
                     area.removeFromTop(90),
                     juce::Justification::topLeft,
                     3);
}

void MainComponent::resized()
{
}
