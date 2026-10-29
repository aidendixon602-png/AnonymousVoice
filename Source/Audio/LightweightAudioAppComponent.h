#pragma once

#include <JuceHeader.h>

/**
 * Lightweight audio component that manages audio device I/O.
 * 
 * This component provides a simplified interface for audio input/output
 * with proper thread safety between the audio thread and UI thread.
 */
class LightweightAudioAppComponent : public juce::Component,
                                     private juce::AudioIODeviceCallback,
                                     private juce::AudioDeviceManager::AudioDeviceIOCallback
{
public:
    LightweightAudioAppComponent() = default;
    ~LightweightAudioAppComponent() override;

    /** Initialize audio with specified input/output channel counts */
    void setAudioChannels(int numInputChannels, int numOutputChannels);
    
    /** Shutdown audio devices */
    void shutdownAudio();
    
    /** Get the audio device manager */
    juce::AudioDeviceManager& getDeviceManager() noexcept { return deviceManager; }
    
    /** Get the current audio device */
    juce::AudioIODevice* getCurrentAudioDevice() const noexcept { return currentDevice; }
    
    /** Check if audio is active */
    bool isAudioActive() const noexcept { return audioActive; }

    /** Virtual methods that subclasses must implement */
    virtual void prepareToPlay(int samplesPerBlockExpected, double sampleRate) = 0;
    virtual void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) = 0;
    virtual void releaseResources() = 0;

protected:
    juce::AudioDeviceManager deviceManager;

private:
    // Audio device state
    juce::AudioIODevice* currentDevice = nullptr;
    std::atomic<bool> audioActive{false};
    std::atomic<int> currentSampleRate{48000};
    std::atomic<int> currentBufferSize{512};

    // Device change handling
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceIOCallbackError(const juce::String& errorMessage) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LightweightAudioAppComponent)
};
