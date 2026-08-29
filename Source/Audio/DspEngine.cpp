#include "DspEngine.h"

#include <cmath>

void DspEngine::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, maxBlockSize));
    spec.numChannels = static_cast<juce::uint32>(juce::jmax(1, numChannels));

    chorus.prepare(spec);
    chorus.setRate(0.25f);
    chorus.setCentreDelay(7.0f);
    chorus.setFeedback(0.0f);
    chorus.setMix(0.30f);

    reset();
}

void DspEngine::reset()
{
    compressorEnvelope = 0.0f;
    chorus.reset();
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

    juce::dsp::AudioBlock<float> block(buffer);
    auto subBlock = block.getSubBlock(static_cast<size_t>(startSample), static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextReplacing<float> context(subBlock);

    chorus.setDepth(juce::jlimit(0.0f, 1.0f, params.chorusDepth.load()));
    chorus.process(context);

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
