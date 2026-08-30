#include "WasapiMonitor.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cwchar>
#include <thread>

namespace
{
    class AudioRuntime
    {
    public:
        AudioRuntime()
        {
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

            while (!stopRequested.load(std::memory_order_acquire))
            {
                const bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
                if (f8Down && !f8WasDown)
                    monitor.setEnabled(!monitor.isEnabled());
                f8WasDown = f8Down;

                if (window == nullptr || !IsWindow(window))
                    window = FindWindowW(L"AnonymousVoiceMainWindow", nullptr);

                if (window != nullptr)
                {
                    wchar_t title[256] {};

                    if (monitor.isRunning())
                    {
                        const int levelPercent = std::clamp(static_cast<int>(std::lround(monitor.peakLevel() * 100.0f)), 0, 100);
                        swprintf_s(title,
                                   L"Anonymous Voice  |  DSP MONITOR %s  |  %u Hz / %u ch  |  Input %d%%  |  F8 toggles monitor",
                                   monitor.isEnabled() ? L"ON" : L"OFF",
                                   monitor.sampleRate(),
                                   static_cast<unsigned>(monitor.channels()),
                                   levelPercent);
                    }
                    else if (monitor.hasError())
                    {
                        swprintf_s(title,
                                   L"Anonymous Voice  |  AUDIO ERROR - check mic/speaker device and Windows permissions");
                    }
                    else
                    {
                        swprintf_s(title, L"Anonymous Voice  |  Starting processed audio path...");
                    }

                    SetWindowTextW(window, title);
                }

                Sleep(50);
            }
        }

        WasapiMonitor monitor;
        std::thread updater;
        std::atomic<bool> stopRequested { false };
    };

    AudioRuntime audioRuntime;
}
