#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>

class WasapiCapture
{
public:
    WasapiCapture() = default;
    ~WasapiCapture();

    WasapiCapture(const WasapiCapture&) = delete;
    WasapiCapture& operator=(const WasapiCapture&) = delete;

    bool start();
    void stop();

    bool isRunning() const noexcept { return running.load(std::memory_order_acquire); }
    bool hasError() const noexcept { return error.load(std::memory_order_acquire); }
    float peakLevel() const noexcept { return peak.load(std::memory_order_relaxed); }
    double latencyMs() const noexcept { return latency.load(std::memory_order_relaxed); }
    std::uint32_t sampleRate() const noexcept { return rate.load(std::memory_order_relaxed); }
    std::uint16_t channels() const noexcept { return channelCount.load(std::memory_order_relaxed); }

private:
    void captureThread();

    std::thread worker;
    std::atomic<bool> stopRequested { false };
    std::atomic<bool> running { false };
    std::atomic<bool> error { false };
    std::atomic<float> peak { 0.0f };
    std::atomic<double> latency { 0.0 };
    std::atomic<std::uint32_t> rate { 0 };
    std::atomic<std::uint16_t> channelCount { 0 };
};
