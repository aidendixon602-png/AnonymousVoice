#pragma once

#include <JuceHeader.h>

class DspEngine final
{
public:
    DspEngine() = default;
    ~DspEngine() = default;

    struct Parameters
    {
        std::atomic<float> gateThresholdDb { -45.0f };
        std::atomic<float> compressorThresholdDb { -18.0f };
        std::atomic<float> compressorRatio { 4.0f };
        std::atomic<float> distortionDrive { 0.08f };
        std::atomic<float> chorusDepth { 0.06f };
        std::atomic<float> reverbMix { 0.20f };
        std::atomic<float> limiterThresholdDb { -1.0f };
        std::atomic<float> outputGain { 1.0f };
    };

    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);

    Parameters& parameters() noexcept { return params; }
    const Parameters& parameters() const noexcept { return params; }

private:
    Parameters params;

    double currentSampleRate = 48000.0;
    float compressorEnvelope = 0.0f;

    juce::dsp::Chorus<float> chorus;
    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspEngine)
};
