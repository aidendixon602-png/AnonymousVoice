#include "WasapiCapture.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>

#include <algorithm>
#include <cmath>

namespace
{
    template <typename T>
    void releaseCom(T*& pointer)
    {
        if (pointer != nullptr)
        {
            pointer->Release();
            pointer = nullptr;
        }
    }

    float samplePeak(const BYTE* data, UINT32 frames, const WAVEFORMATEX* format)
    {
        if (data == nullptr || format == nullptr || frames == 0)
            return 0.0f;

        const UINT32 channels = std::max<UINT32>(1, format->nChannels);
        const UINT32 samples = frames * channels;
        float peak = 0.0f;

        bool isFloat = false;
        bool isPcm = false;

        if (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
            isFloat = true;
        else if (format->wFormatTag == WAVE_FORMAT_PCM)
            isPcm = true;
        else if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE && format->cbSize >= 22)
        {
            const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(format);
            if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
                isFloat = true;
            else if (ext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)
                isPcm = true;
        }

        if (isFloat && format->wBitsPerSample == 32)
        {
            const auto* values = reinterpret_cast<const float*>(data);
            for (UINT32 i = 0; i < samples; ++i)
                peak = std::max(peak, std::abs(values[i]));
        }
        else if (isPcm && format->wBitsPerSample == 16)
        {
            const auto* values = reinterpret_cast<const std::int16_t*>(data);
            for (UINT32 i = 0; i < samples; ++i)
                peak = std::max(peak, std::abs(static_cast<float>(values[i]) / 32768.0f));
        }
        else if (isPcm && format->wBitsPerSample == 32)
        {
            const auto* values = reinterpret_cast<const std::int32_t*>(data);
            for (UINT32 i = 0; i < samples; ++i)
                peak = std::max(peak, std::abs(static_cast<float>(values[i]) / 2147483648.0f));
        }
        else if (isPcm && format->wBitsPerSample == 24)
        {
            for (UINT32 i = 0; i < samples; ++i)
            {
                const BYTE* s = data + i * 3;
                std::int32_t value = static_cast<std::int32_t>(s[0]) |
                                     (static_cast<std::int32_t>(s[1]) << 8) |
                                     (static_cast<std::int32_t>(s[2]) << 16);
                if ((value & 0x00800000) != 0)
                    value |= 0xFF000000;
                peak = std::max(peak, std::abs(static_cast<float>(value) / 8388608.0f));
            }
        }

        return std::clamp(peak, 0.0f, 1.0f);
    }
}

WasapiCapture::~WasapiCapture()
{
    stop();
}

bool WasapiCapture::start()
{
    if (worker.joinable())
        return false;

    stopRequested.store(false, std::memory_order_release);
    error.store(false, std::memory_order_release);
    running.store(false, std::memory_order_release);
    peak.store(0.0f, std::memory_order_relaxed);
    latency.store(0.0, std::memory_order_relaxed);
    rate.store(0, std::memory_order_relaxed);
    channelCount.store(0, std::memory_order_relaxed);

    worker = std::thread(&WasapiCapture::captureThread, this);
    return true;
}

void WasapiCapture::stop()
{
    stopRequested.store(true, std::memory_order_release);
    if (worker.joinable())
        worker.join();

    running.store(false, std::memory_order_release);
    peak.store(0.0f, std::memory_order_relaxed);
}

void WasapiCapture::captureThread()
{
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool comReady = SUCCEEDED(initHr) || initHr == RPC_E_CHANGED_MODE;
    if (!comReady)
    {
        error.store(true, std::memory_order_release);
        return;
    }

    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioCaptureClient* captureClient = nullptr;
    WAVEFORMATEX* mixFormat = nullptr;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr))
        goto cleanup;

    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &device);
    if (FAILED(hr))
        hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &device);
    if (FAILED(hr))
        goto cleanup;

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient));
    if (FAILED(hr))
        goto cleanup;

    hr = audioClient->GetMixFormat(&mixFormat);
    if (FAILED(hr) || mixFormat == nullptr)
        goto cleanup;

    rate.store(mixFormat->nSamplesPerSec, std::memory_order_relaxed);
    channelCount.store(mixFormat->nChannels, std::memory_order_relaxed);

    {
        REFERENCE_TIME defaultPeriod = 0;
        REFERENCE_TIME minimumPeriod = 0;
        if (SUCCEEDED(audioClient->GetDevicePeriod(&defaultPeriod, &minimumPeriod)))
            latency.store(static_cast<double>(defaultPeriod) / 10000.0, std::memory_order_relaxed);
    }

    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                 AUDCLNT_STREAMFLAGS_NOPERSIST,
                                 0,
                                 0,
                                 mixFormat,
                                 nullptr);
    if (FAILED(hr))
        goto cleanup;

    hr = audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&captureClient));
    if (FAILED(hr))
        goto cleanup;

    hr = audioClient->Start();
    if (FAILED(hr))
        goto cleanup;

    running.store(true, std::memory_order_release);

    while (!stopRequested.load(std::memory_order_acquire))
    {
        UINT32 packetFrames = 0;
        hr = captureClient->GetNextPacketSize(&packetFrames);
        if (FAILED(hr))
            break;

        float packetPeak = 0.0f;

        while (packetFrames > 0)
        {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            UINT64 devicePosition = 0;
            UINT64 qpcPosition = 0;

            hr = captureClient->GetBuffer(&data, &frames, &flags, &devicePosition, &qpcPosition);
            if (FAILED(hr))
                break;

            if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) == 0)
                packetPeak = std::max(packetPeak, samplePeak(data, frames, mixFormat));

            captureClient->ReleaseBuffer(frames);

            hr = captureClient->GetNextPacketSize(&packetFrames);
            if (FAILED(hr))
                break;
        }

        const float previous = peak.load(std::memory_order_relaxed);
        const float smoothed = packetPeak >= previous ? packetPeak : previous * 0.88f;
        peak.store(std::clamp(smoothed, 0.0f, 1.0f), std::memory_order_relaxed);

        Sleep(5);
    }

    audioClient->Stop();

cleanup:
    running.store(false, std::memory_order_release);
    if (FAILED(hr) && !stopRequested.load(std::memory_order_acquire))
        error.store(true, std::memory_order_release);

    if (mixFormat != nullptr)
        CoTaskMemFree(mixFormat);

    releaseCom(captureClient);
    releaseCom(audioClient);
    releaseCom(device);
    releaseCom(enumerator);

    if (SUCCEEDED(initHr))
        CoUninitialize();
}
