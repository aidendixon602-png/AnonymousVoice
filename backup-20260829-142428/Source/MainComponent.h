#pragma once

#include <JuceHeader.h>

class MainComponent final : public juce::AudioAppComponent
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected,
                       double sampleRate) override;

    void getNextAudioBlock(
        const juce::AudioSourceChannelInfo& bufferToFill) override;

    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider gainSlider;
    juce::Label gainLabel;
    juce::Label statusLabel;

    std::atomic<float> gain { 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};