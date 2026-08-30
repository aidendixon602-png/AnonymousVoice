#include "LightweightAudioAppComponent.h"

LightweightAudioAppComponent::~LightweightAudioAppComponent()
{
    shutdownAudio();
}

void LightweightAudioAppComponent::setAudioChannels(int numInputChannels, int numOutputChannels)
{
    juce::ignoreUnused(numInputChannels, numOutputChannels);
    juce::Logger::writeToLog("AnonymousVoice safe startup: audio device initialisation temporarily disabled.");
}

void LightweightAudioAppComponent::shutdownAudio()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
    callbackBuffer.setSize(0, 0);
}

void LightweightAudioAppComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device == nullptr)
        return;

    const auto blockSize = juce::jmax(1, device->getCurrentBufferSizeSamples());
    const auto channelCapacity = juce::jmax(2, device->getOutputChannelNames().size());
    callbackBuffer.setSize(channelCapacity, blockSize, false, true, true);
    callbackBuffer.clear();

    prepareToPlay(blockSize, device->getCurrentSampleRate());
}

void LightweightAudioAppComponent::audioDeviceStopped()
{
    releaseResources();
    callbackBuffer.clear();
}

void LightweightAudioAppComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                                      int numInputChannels,
                                                                      float* const* outputChannelData,
                                                                      int numOutputChannels,
                                                                      int numSamples,
                                                                      const juce::AudioIODeviceCallbackContext&)
{
    if (numSamples <= 0)
        return;

    if (numOutputChannels <= 0 ||
        callbackBuffer.getNumChannels() < numOutputChannels ||
        callbackBuffer.getNumSamples() < numSamples)
    {
        for (int channel = 0; channel < numOutputChannels; ++channel)
            if (outputChannelData != nullptr && outputChannelData[channel] != nullptr)
                juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);

        return;
    }

    const auto* monoInput = (inputChannelData != nullptr && numInputChannels > 0)
        ? inputChannelData[0]
        : nullptr;

    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (monoInput != nullptr)
            callbackBuffer.copyFrom(channel, 0, monoInput, numSamples);
        else
            callbackBuffer.clear(channel, 0, numSamples);
    }

    juce::AudioSourceChannelInfo info(&callbackBuffer, 0, numSamples);
    getNextAudioBlock(info);

    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData != nullptr && outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::copy(outputChannelData[channel],
                                              callbackBuffer.getReadPointer(channel),
                                              numSamples);
    }
}
