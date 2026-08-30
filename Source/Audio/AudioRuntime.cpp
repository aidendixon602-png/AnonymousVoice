#include "WasapiCapture.h"

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
            capture.start();
            updater = std::thread([this] { updateLoop(); });
        }

        ~AudioRuntime()
        {
            stopRequested.store(true, std::memory_order_release);
            if (updater.joinable())
                updater.join();
            capture.stop();
        }

    private:
        void updateLoop()
        {
            HWND window = nullptr;

            while (!stopRequested.load(std::memory_order_acquire))
            {
                if (window == nullptr || !IsWindow(window))
                    window = FindWindowW(L"AnonymousVoiceMainWindow", nullptr);

                if (window != nullptr)
                {
                    wchar_t title[256] {};

                    if (capture.isRunning())
                    {
                        const int levelPercent = std::clamp(static_cast<int>(std::lround(capture.peakLevel() * 100.0f)), 0, 100);
                        const auto rate = capture.sampleRate();
                        const auto channels = capture.channels();
                        const double latency = capture.latencyMs();

                        swprintf_s(title,
                                   L"Anonymous Voice  |  MIC ONLINE  |  %u Hz / %u ch  |  Level %d%%  |  %.1f ms",
                                   rate,
                                   static_cast<unsigned>(channels),
                                   levelPercent,
                                   latency);
                    }
                    else if (capture.hasError())
                    {
                        swprintf_s(title, L"Anonymous Voice  |  MIC ERROR - check Windows microphone permissions/device");
                    }
                    else
                    {
                        swprintf_s(title, L"Anonymous Voice  |  Starting microphone...");
                    }

                    SetWindowTextW(window, title);
                }

                Sleep(50);
            }
        }

        WasapiCapture capture;
        std::thread updater;
        std::atomic<bool> stopRequested { false };
    };

    AudioRuntime audioRuntime;
}
