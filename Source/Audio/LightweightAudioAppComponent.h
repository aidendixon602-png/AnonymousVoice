#pragma once

#include <JuceHeader.h>

class LightweightAudioAppComponent : public juce::Component,
                                     private juce::AudioIODeviceCallback
{
public:
    LightweightAudioAppComponent() = default;
    ~LightweightAudioAppComponent() override;

    void setAudioChannels(int numInputChannels, int numOutputChannels);
    void shutdownAudio();

    virtual void prepareToPlay(int samplesPerBlockExpected, double sampleRate) = 0;
    virtual void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) = 0;
    virtual void releaseResources() = 0;

protected:
    juce::AudioDeviceManager deviceManager;

private:
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext&) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LightweightAudioAppComponent)
};
