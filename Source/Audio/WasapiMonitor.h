#pragma once

#include <atomic>
#include <cstdint>
#include <thread>

class WasapiMonitor
{
public:
    WasapiMonitor() = default;
    ~WasapiMonitor();

    WasapiMonitor(const WasapiMonitor&) = delete;
    WasapiMonitor& operator=(const WasapiMonitor&) = delete;

    bool start();
    void stop();

    void setEnabled(bool shouldEnable) noexcept { enabled.store(shouldEnable, std::memory_order_release); }
    bool isEnabled() const noexcept { return enabled.load(std::memory_order_acquire); }
    bool isRunning() const noexcept { return running.load(std::memory_order_acquire); }
    bool hasError() const noexcept { return error.load(std::memory_order_acquire); }

    float peakLevel() const noexcept { return peak.load(std::memory_order_relaxed); }
    std::uint32_t sampleRate() const noexcept { return rate.load(std::memory_order_relaxed); }
    std::uint16_t channels() const noexcept { return channelCount.load(std::memory_order_relaxed); }

private:
    void audioThread();

    std::thread worker;
    std::atomic<bool> stopRequested { false };
    std::atomic<bool> enabled { true };
    std::atomic<bool> running { false };
    std::atomic<bool> error { false };
    std::atomic<float> peak { 0.0f };
    std::atomic<std::uint32_t> rate { 0 };
    std::atomic<std::uint16_t> channelCount { 0 };
};
