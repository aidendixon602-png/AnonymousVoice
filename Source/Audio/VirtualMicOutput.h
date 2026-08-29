#pragma once

#include <JuceHeader.h>

class VirtualMicOutput final
{
public:
    void prepare(int capacitySamples = 48000);
    void reset();

    void pushMono(const float* samples, int numSamples) noexcept;
    int popMono(float* destination, int maxSamples) noexcept;

    int getBufferedSamples() const noexcept;
    int getDroppedSamples() const noexcept { return droppedSamples.load(); }

private:
    juce::AbstractFifo fifo { 1 };
    juce::AudioBuffer<float> storage;
    std::atomic<int> droppedSamples { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualMicOutput)
};
