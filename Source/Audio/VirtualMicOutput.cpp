#include "VirtualMicOutput.h"

void VirtualMicOutput::prepare(int capacitySamples)
{
    const auto capacity = juce::jmax(1024, capacitySamples);
    storage.setSize(1, capacity, false, true, false);
    storage.clear();
    fifo.setTotalSize(capacity);
    fifo.reset();
    droppedSamples.store(0);
}

void VirtualMicOutput::reset()
{
    fifo.reset();
    storage.clear();
    droppedSamples.store(0);
}

void VirtualMicOutput::pushMono(const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0 || storage.getNumSamples() <= 0)
        return;

    auto remaining = numSamples;
    auto sourceOffset = 0;

    while (remaining > 0)
    {
        const auto writable = fifo.getFreeSpace();
        if (writable <= 0)
        {
            droppedSamples.fetch_add(remaining);
            return;
        }

        const auto amount = juce::jmin(remaining, writable);
        auto scope = fifo.write(amount);

        if (scope.blockSize1 > 0)
            storage.copyFrom(0, scope.startIndex1, samples + sourceOffset, scope.blockSize1);

        if (scope.blockSize2 > 0)
            storage.copyFrom(0, scope.startIndex2, samples + sourceOffset + scope.blockSize1, scope.blockSize2);

        sourceOffset += amount;
        remaining -= amount;
    }
}

int VirtualMicOutput::popMono(float* destination, int maxSamples) noexcept
{
    if (destination == nullptr || maxSamples <= 0)
        return 0;

    const auto amount = juce::jmin(maxSamples, fifo.getNumReady());
    if (amount <= 0)
        return 0;

    auto scope = fifo.read(amount);

    if (scope.blockSize1 > 0)
        std::copy_n(storage.getReadPointer(0, scope.startIndex1), scope.blockSize1, destination);

    if (scope.blockSize2 > 0)
        std::copy_n(storage.getReadPointer(0, scope.startIndex2), scope.blockSize2, destination + scope.blockSize1);

    return amount;
}

int VirtualMicOutput::getBufferedSamples() const noexcept
{
    return fifo.getNumReady();
}
