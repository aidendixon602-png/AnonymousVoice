#include "LightweightAudioAppComponent.h"
#include <atomic>

LightweightAudioAppComponent::~LightweightAudioAppComponent()
{
    shutdownAudio();
}

void LightweightAudioAppComponent::setAudioChannels(int numInputChannels, int numOutputChannels)
{
    shutdownAudio();

    // Store requested configuration
    const int requestedInputChannels = numInputChannels;
    const int requestedOutputChannels = numOutputChannels;

    // Find the best audio device
    auto deviceList = deviceManager.getAvailableDevices();
    juce::AudioIODevice* bestDevice = nullptr;
    
    for (auto* device : deviceList)
    {
        if (device->getType() == juce::AudioIODeviceType::input ||
            device->getType() == juce::AudioIODeviceType::duplex)
        {
            if (device->getInputChannelNames().size() >= static_cast<size_t>(requestedInputChannels) &&
                device->getOutputChannelNames().size() >= static_cast<size_t>(requestedOutputChannels))
            {
                bestDevice = device;
                break;
            }
        }
    }

    if (bestDevice == nullptr)
    {
        juce::Logger::writeToLog("No suitable audio device found!");
        return;
    }

    // Set up audio device with error handling
    juce::String error;
    
    if (!deviceManager.setAudioDeviceEnabled(bestDevice->getName(), true, error))
    {
        juce::Logger::writeToLog("Failed to enable audio device: " + error);
        return;
    }

    // Configure device
    if (!deviceManager.setAudioDeviceSetup(
            {bestDevice->getName(), 
             bestDevice->getName()},
            requestedInputChannels,
            requestedOutputChannels,
            bestDevice->getDefaultSampleRate(),
             bestDevice->getDefaultBufferSize(),
             error))
    {
        juce::Logger::writeToLog("Failed to configure audio device: " + error);
        return;
    }

    // Add callback and start
    deviceManager.addAudioCallback(this);
    
    currentDevice = bestDevice;
    audioActive.store(true);
    
    juce::Logger::writeToLog("Audio device initialized: " + bestDevice->getName());
}

void LightweightAudioAppComponent::shutdownAudio()
{
    if (audioActive.load())
    {
        deviceManager.removeAudioCallback(this);
        deviceManager.closeAudioDevice();
        currentDevice = nullptr;
        audioActive.store(false);
        currentSampleRate.store(48000);
        currentBufferSize.store(512);
    }
}

void LightweightAudioAppComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device == nullptr)
        return;

    currentDevice = device;
    currentSampleRate.store(static_cast<int>(device->getCurrentSampleRate()));
    currentBufferSize.store(device->getCurrentBufferSizeSamples());
    
    prepareToPlay(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());
}

void LightweightAudioAppComponent::audioDeviceStopped()
{
    releaseResources();
    currentDevice = nullptr;
    audioActive.store(false);
}

void LightweightAudioAppComponent::audioDeviceIOCallbackError(const juce::String& errorMessage)
{
    juce::Logger::writeToLog("Audio device error: " + errorMessage);
}

void LightweightAudioAppComponent::audioDeviceIOCallbackWithContext(
    const float* const* inputChannelData,
    int numInputChannels,
    float* const* outputChannelData,
    int numOutputChannels,
    int numSamples,
    const juce::AudioIODeviceCallbackContext& context)
{
    // Check for XRUN or other errors
    if (context.isInputActive == false || context.isOutputActive == false)
    {
        juce::Logger::writeToLog("Audio stream glitch detected!");
        return;
    }

    if (numOutputChannels <= 0 || numSamples <= 0)
        return;

    // Create output buffer
    juce::AudioBuffer<float> buffer(const_cast<float**>(outputChannelData), numOutputChannels, numSamples);
    
    // Clear output buffer
    buffer.clear();

    // Copy input to output if available (passthrough mode)
    if (numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            if (channel < numInputChannels && inputChannelData[channel] != nullptr)
            {
                buffer.copyFrom(channel, 0, inputChannelData[channel], numSamples);
            }
        }
    }

    // Process audio through user callback
    juce::AudioSourceChannelInfo info(&buffer, 0, numSamples);
    getNextAudioBlock(info);
}
