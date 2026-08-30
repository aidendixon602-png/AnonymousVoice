#include "DspEngine.h"

#include <cmath>

void DspEngine::prepare(double sampleRate, int, int numChannels)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;

    const auto channels = juce::jmax(1, numChannels);
    const auto maxDelaySamples = juce::jmax(64, static_cast<int>(currentSampleRate * 0.030));
    chorusDelayBuffer.setSize(channels, maxDelaySamples + 2, false, true, true);

    reset();
}

void DspEngine::reset()
{
    compressorEnvelope = 0.0f;
    chorusWritePosition = 0;
    chorusPhase = 0.0f;
    chorusDelayBuffer.clear();
    reverb.reset();
}

void DspEngine::process(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (numSamples <= 0 || buffer.getNumChannels() <= 0)
        return;

    const auto gateThreshold = juce::Decibels::decibelsToGain(params.gateThresholdDb.load());
    const auto compressorThreshold = juce::Decibels::decibelsToGain(params.compressorThresholdDb.load());
    const auto compressorRatio = juce::jmax(1.0f, params.compressorRatio.load());
    const auto drive = juce::jlimit(0.0f, 1.0f, params.distortionDrive.load());
    const auto limiter = juce::Decibels::decibelsToGain(params.limiterThresholdDb.load());
    const auto outputGain = params.outputGain.load();

    const auto attackCoeff = std::exp(-1.0f / static_cast<float>(0.005 * currentSampleRate));
    const auto releaseCoeff = std::exp(-1.0f / static_cast<float>(0.080 * currentSampleRate));

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* samples = buffer.getWritePointer(channel, startSample);

        for (int i = 0; i < numSamples; ++i)
        {
            auto x = samples[i];
            const auto magnitude = std::abs(x);

            if (magnitude < gateThreshold)
                x *= 0.18f;

            if (magnitude > compressorEnvelope)
                compressorEnvelope = attackCoeff * compressorEnvelope + (1.0f - attackCoeff) * magnitude;
            else
                compressorEnvelope = releaseCoeff * compressorEnvelope + (1.0f - releaseCoeff) * magnitude;

            if (compressorEnvelope > compressorThreshold && compressorEnvelope > 0.0f)
            {
                const auto overDb = juce::Decibels::gainToDecibels(compressorEnvelope / compressorThreshold);
                const auto gainReductionDb = overDb - (overDb / compressorRatio);
                x *= juce::Decibels::decibelsToGain(-gainReductionDb);
            }

            const auto driveAmount = 1.0f + drive * 8.0f;
            x = std::tanh(x * driveAmount) / std::tanh(driveAmount);

            x *= outputGain;
            samples[i] = juce::jlimit(-limiter, limiter, x);
        }
    }

    const auto depth = juce::jlimit(0.0f, 1.0f, params.chorusDepth.load());
    if (depth > 0.0001f && chorusDelayBuffer.getNumSamples() > 2)
    {
        const auto delayBufferSize = chorusDelayBuffer.getNumSamples();
        const auto chorusChannels = juce::jmin(buffer.getNumChannels(), chorusDelayBuffer.getNumChannels());
        const auto phaseIncrement = static_cast<float>(juce::MathConstants<double>::twoPi * 0.25 / currentSampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            const auto mod = std::sin(chorusPhase);
            const auto delayMs = 7.0f + depth * 5.0f * mod;
            const auto delaySamples = juce::jlimit(1.0f, static_cast<float>(delayBufferSize - 2),
                                                  delayMs * static_cast<float>(currentSampleRate) / 1000.0f);

            auto readPosition = static_cast<float>(chorusWritePosition) - delaySamples;
            while (readPosition < 0.0f)
                readPosition += static_cast<float>(delayBufferSize);

            const auto indexA = static_cast<int>(readPosition) % delayBufferSize;
            const auto indexB = (indexA + 1) % delayBufferSize;
            const auto frac = readPosition - std::floor(readPosition);

            for (int channel = 0; channel < chorusChannels; ++channel)
            {
                auto* output = buffer.getWritePointer(channel, startSample);
                auto* delay = chorusDelayBuffer.getWritePointer(channel);
                const auto dry = output[i];

                delay[chorusWritePosition] = dry;
                const auto delayed = delay[indexA] + frac * (delay[indexB] - delay[indexA]);
                output[i] = dry * 0.70f + delayed * 0.30f;
            }

            chorusWritePosition = (chorusWritePosition + 1) % delayBufferSize;
            chorusPhase += phaseIncrement;
            if (chorusPhase >= juce::MathConstants<float>::twoPi)
                chorusPhase -= juce::MathConstants<float>::twoPi;
        }
    }

    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.22f;
    reverbParams.damping = 0.55f;
    reverbParams.wetLevel = juce::jlimit(0.0f, 0.5f, params.reverbMix.load());
    reverbParams.dryLevel = 1.0f - reverbParams.wetLevel;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    if (buffer.getNumChannels() >= 2)
        reverb.processStereo(buffer.getWritePointer(0, startSample), buffer.getWritePointer(1, startSample), numSamples);
    else
        reverb.processMono(buffer.getWritePointer(0, startSample), numSamples);
}
