#include "LightweightAudioAppComponent.h"

LightweightAudioAppComponent::~LightweightAudioAppComponent()
{
    shutdownAudio();
}

void LightweightAudioAppComponent::setAudioChannels(int numInputChannels, int numOutputChannels)
{
    const auto error = deviceManager.initialise(numInputChannels,
                                                numOutputChannels,
                                                nullptr,
                                                true);

    if (error.isNotEmpty())
        juce::Logger::writeToLog("Audio device initialisation failed: " + error);

    deviceManager.addAudioCallback(this);
}

void LightweightAudioAppComponent::shutdownAudio()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void LightweightAudioAppComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device == nullptr)
        return;

    prepareToPlay(device->getCurrentBufferSizeSamples(), device->getCurrentSampleRate());
}

void LightweightAudioAppComponent::audioDeviceStopped()
{
    releaseResources();
}

void LightweightAudioAppComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                                      int numInputChannels,
                                                                      float* const* outputChannelData,
                                                                      int numOutputChannels,
                                                                      int numSamples,
                                                                      const juce::AudioIODeviceCallbackContext&)
{
    if (numOutputChannels <= 0 || numSamples <= 0)
        return;

    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        auto* output = outputChannelData[channel];
        if (output == nullptr)
            continue;

        if (numInputChannels > 0 && inputChannelData[0] != nullptr)
            juce::FloatVectorOperations::copy(output, inputChannelData[0], numSamples);
        else
            juce::FloatVectorOperations::clear(output, numSamples);
    }

    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    juce::AudioSourceChannelInfo info(&buffer, 0, numSamples);
    getNextAudioBlock(info);
}
