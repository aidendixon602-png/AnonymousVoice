#include "WasapiMonitor.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <thread>

namespace
{
    std::uint64_t fileTimeValue(const FILETIME& value)
    {
        ULARGE_INTEGER u {};
        u.LowPart = value.dwLowDateTime;
        u.HighPart = value.dwHighDateTime;
        return u.QuadPart;
    }

    const wchar_t* presetName(int index)
    {
        static constexpr const wchar_t* names[10] = {
            L"Anonymous", L"Deep", L"Demon", L"Robot", L"Alien",
            L"Ethereal", L"Baby", L"Radio", L"Witch", L"Custom"
        };
        return names[std::clamp(index, 0, 9)];
    }

    class AudioRuntime
    {
    public:
        AudioRuntime()
        {
            monitor.setPreset(0);
            monitor.setEnabled(true);
            monitor.start();
            updater = std::thread([this] { updateLoop(); });
        }

        ~AudioRuntime()
        {
            stopRequested.store(true, std::memory_order_release);
            if (updater.joinable())
                updater.join();
            monitor.stop();
        }

    private:
        void updateLoop()
        {
            HWND window = nullptr;
            bool f8WasDown = false;
            bool presetWasDown[10] {};

            FILETIME createTime {}, exitTime {}, kernelTime {}, userTime {};
            GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime);
            std::uint64_t lastProcessTime = fileTimeValue(kernelTime) + fileTimeValue(userTime);
            ULONGLONG lastCpuTick = GetTickCount64();
            double cpuPercent = 0.0;
            const DWORD logicalProcessors = std::max<DWORD>(1, GetActiveProcessorCount(ALL_PROCESSOR_GROUPS));

            while (!stopRequested.load(std::memory_order_acquire))
            {
                const bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
                if (f8Down && !f8WasDown)
                    monitor.setEnabled(!monitor.isEnabled());
                f8WasDown = f8Down;

                for (int i = 0; i < 10; ++i)
                {
                    const int key = i == 9 ? VK_F10 : (VK_F1 + i);
                    if (key == VK_F8)
                        continue;

                    const bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
                    if (down && !presetWasDown[i])
                        monitor.setPreset(i);
                    presetWasDown[i] = down;
                }

                const ULONGLONG nowTick = GetTickCount64();
                if (nowTick - lastCpuTick >= 500)
                {
                    if (GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime))
                    {
                        const std::uint64_t processTime = fileTimeValue(kernelTime) + fileTimeValue(userTime);
                        const std::uint64_t processDelta = processTime - lastProcessTime;
                        const ULONGLONG wallDeltaMs = nowTick - lastCpuTick;
                        if (wallDeltaMs > 0)
                        {
                            cpuPercent = (static_cast<double>(processDelta) /
                                          (static_cast<double>(wallDeltaMs) * 10000.0 * logicalProcessors)) * 100.0;
                            cpuPercent = std::clamp(cpuPercent, 0.0, 100.0);
                        }
                        lastProcessTime = processTime;
                        lastCpuTick = nowTick;
                    }
                }

                if (window == nullptr || !IsWindow(window))
                    window = FindWindowW(L"AnonymousVoiceMainWindow", nullptr);

                if (window != nullptr)
                {
                    wchar_t title[320] {};

                    if (monitor.isRunning())
                    {
                        const int levelPercent = std::clamp(static_cast<int>(std::lround(monitor.peakLevel() * 100.0f)), 0, 100);
                        swprintf_s(title,
                                   L"Anonymous Voice | DSP %s | %s | %u Hz/%u ch | Input %d%% | CPU %.1f%% | Audio %.1f ms | F1-F10 presets / F8 monitor",
                                   monitor.isEnabled() ? L"ON" : L"OFF",
                                   presetName(monitor.preset()),
                                   monitor.sampleRate(),
                                   static_cast<unsigned>(monitor.channels()),
                                   levelPercent,
                                   cpuPercent,
                                   monitor.latencyMs());
                    }
                    else if (monitor.hasError())
                    {
                        swprintf_s(title,
                                   L"Anonymous Voice | AUDIO ERROR - check mic/speaker device and Windows permissions");
                    }
                    else
                    {
                        swprintf_s(title, L"Anonymous Voice | Starting processed audio path...");
                    }

                    SetWindowTextW(window, title);
                }

                Sleep(25);
            }
        }

        WasapiMonitor monitor;
        std::thread updater;
        std::atomic<bool> stopRequested { false };
    };

    AudioRuntime audioRuntime;
}
