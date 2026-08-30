#include "WasapiMonitor.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <ksmedia.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <vector>

namespace
{
    template <typename T>
    void releaseCom(T*& p)
    {
        if (p != nullptr)
        {
            p->Release();
            p = nullptr;
        }
    }

    struct SampleCodec
    {
        enum class Kind { unsupported, f32, s16, s24, s32 } kind = Kind::unsupported;
        std::uint16_t channels = 0;
        std::uint16_t blockAlign = 0;

        static SampleCodec from(const WAVEFORMATEX* f)
        {
            SampleCodec c;
            if (f == nullptr)
                return c;

            c.channels = f->nChannels;
            c.blockAlign = f->nBlockAlign;

            bool isFloat = f->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
            bool isPcm = f->wFormatTag == WAVE_FORMAT_PCM;

            if (f->wFormatTag == WAVE_FORMAT_EXTENSIBLE && f->cbSize >= 22)
            {
                const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f);
                isFloat = ext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
                isPcm = ext->SubFormat == KSDATAFORMAT_SUBTYPE_PCM;
            }

            if (isFloat && f->wBitsPerSample == 32)
                c.kind = Kind::f32;
            else if (isPcm && f->wBitsPerSample == 16)
                c.kind = Kind::s16;
            else if (isPcm && f->wBitsPerSample == 24)
                c.kind = Kind::s24;
            else if (isPcm && f->wBitsPerSample == 32)
                c.kind = Kind::s32;

            return c;
        }

        float read(const BYTE* frame, std::uint16_t channel) const
        {
            switch (kind)
            {
                case Kind::f32:
                    return reinterpret_cast<const float*>(frame)[channel];
                case Kind::s16:
                    return static_cast<float>(reinterpret_cast<const std::int16_t*>(frame)[channel]) / 32768.0f;
                case Kind::s24:
                {
                    const BYTE* s = frame + channel * 3;
                    std::int32_t v = static_cast<std::int32_t>(s[0]) |
                                     (static_cast<std::int32_t>(s[1]) << 8) |
                                     (static_cast<std::int32_t>(s[2]) << 16);
                    if ((v & 0x00800000) != 0)
                        v |= 0xFF000000;
                    return static_cast<float>(v) / 8388608.0f;
                }
                case Kind::s32:
                    return static_cast<float>(reinterpret_cast<const std::int32_t*>(frame)[channel]) / 2147483648.0f;
                default:
                    return 0.0f;
            }
        }

        void write(BYTE* frame, std::uint16_t channel, float value) const
        {
            value = std::clamp(value, -1.0f, 1.0f);
            switch (kind)
            {
                case Kind::f32:
                    reinterpret_cast<float*>(frame)[channel] = value;
                    break;
                case Kind::s16:
                    reinterpret_cast<std::int16_t*>(frame)[channel] = static_cast<std::int16_t>(value * 32767.0f);
                    break;
                case Kind::s24:
                {
                    const std::int32_t v = static_cast<std::int32_t>(value * 8388607.0f);
                    BYTE* s = frame + channel * 3;
                    s[0] = static_cast<BYTE>(v & 0xFF);
                    s[1] = static_cast<BYTE>((v >> 8) & 0xFF);
                    s[2] = static_cast<BYTE>((v >> 16) & 0xFF);
                    break;
                }
                case Kind::s32:
                    reinterpret_cast<std::int32_t*>(frame)[channel] = static_cast<std::int32_t>(value * 2147483647.0f);
                    break;
                default:
                    break;
            }
        }
    };
}

WasapiMonitor::~WasapiMonitor()
{
    stop();
}

bool WasapiMonitor::start()
{
    if (worker.joinable())
        return false;

    stopRequested.store(false, std::memory_order_release);
    error.store(false, std::memory_order_release);
    running.store(false, std::memory_order_release);
    peak.store(0.0f, std::memory_order_relaxed);
    worker = std::thread(&WasapiMonitor::audioThread, this);
    return true;
}

void WasapiMonitor::stop()
{
    stopRequested.store(true, std::memory_order_release);
    if (worker.joinable())
        worker.join();
    running.store(false, std::memory_order_release);
}

void WasapiMonitor::audioThread()
{
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE)
    {
        error.store(true, std::memory_order_release);
        return;
    }

    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* captureDevice = nullptr;
    IMMDevice* renderDevice = nullptr;
    IAudioClient* captureClient = nullptr;
    IAudioClient* renderClient = nullptr;
    IAudioCaptureClient* captureService = nullptr;
    IAudioRenderClient* renderService = nullptr;
    WAVEFORMATEX* format = nullptr;

    HRESULT hr = S_OK;
    UINT32 renderBufferFrames = 0;
    SampleCodec codec {};
    std::size_t fifoCapacityFrames = 0;
    std::vector<float> fifo;
    std::vector<BYTE> silentFrame;
    std::size_t readFrame = 0;
    std::size_t writeFrame = 0;
    std::size_t queuedFrames = 0;
    float lowpassState = 0.0f;
    double phase = 0.0;
    double phaseStep = 0.0;
    std::function<float(const BYTE*)> pushFrame;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
    if (FAILED(hr)) goto cleanup;

    hr = enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &captureDevice);
    if (FAILED(hr)) hr = enumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &captureDevice);
    if (FAILED(hr)) goto cleanup;

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &renderDevice);
    if (FAILED(hr)) goto cleanup;

    hr = captureDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                 reinterpret_cast<void**>(&captureClient));
    if (FAILED(hr)) goto cleanup;

    hr = renderDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                reinterpret_cast<void**>(&renderClient));
    if (FAILED(hr)) goto cleanup;

    hr = captureClient->GetMixFormat(&format);
    if (FAILED(hr) || format == nullptr) goto cleanup;

    codec = SampleCodec::from(format);
    if (codec.kind == SampleCodec::Kind::unsupported || codec.channels == 0 || codec.blockAlign == 0)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    rate.store(format->nSamplesPerSec, std::memory_order_relaxed);
    channelCount.store(format->nChannels, std::memory_order_relaxed);

    hr = captureClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                   AUDCLNT_STREAMFLAGS_NOPERSIST,
                                   0, 0, format, nullptr);
    if (FAILED(hr)) goto cleanup;

    hr = renderClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM |
                                  AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY |
                                  AUDCLNT_STREAMFLAGS_NOPERSIST,
                                  0, 0, format, nullptr);
    if (FAILED(hr)) goto cleanup;

    hr = captureClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&captureService));
    if (FAILED(hr)) goto cleanup;

    hr = renderClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&renderService));
    if (FAILED(hr)) goto cleanup;

    hr = renderClient->GetBufferSize(&renderBufferFrames);
    if (FAILED(hr)) goto cleanup;

    fifoCapacityFrames = std::max<std::size_t>(static_cast<std::size_t>(format->nSamplesPerSec) * 2u,
                                                static_cast<std::size_t>(renderBufferFrames) * 8u);
    fifo.assign(fifoCapacityFrames * codec.channels, 0.0f);
    silentFrame.assign(codec.blockAlign, 0);
    phaseStep = 2.0 * 3.14159265358979323846 * 72.0 / static_cast<double>(format->nSamplesPerSec);

    pushFrame = [&](const BYTE* frame) -> float
    {
        if (queuedFrames >= fifoCapacityFrames)
        {
            readFrame = (readFrame + 1) % fifoCapacityFrames;
            --queuedFrames;
        }

        float mono = 0.0f;
        for (std::uint16_t ch = 0; ch < codec.channels; ++ch)
            mono += codec.read(frame, ch);
        mono /= static_cast<float>(codec.channels);

        const float driven = std::tanh(mono * 2.25f);
        lowpassState += 0.18f * (driven - lowpassState);

        const float ring = static_cast<float>(std::sin(phase));
        phase += phaseStep;
        if (phase >= 2.0 * 3.14159265358979323846)
            phase -= 2.0 * 3.14159265358979323846;

        const float processed = std::clamp(lowpassState * 0.78f + (lowpassState * ring) * 0.22f,
                                           -1.0f, 1.0f);
        const float output = enabled.load(std::memory_order_acquire) ? processed * 0.65f : 0.0f;

        const std::size_t base = writeFrame * codec.channels;
        for (std::uint16_t ch = 0; ch < codec.channels; ++ch)
            fifo[base + ch] = output;

        writeFrame = (writeFrame + 1) % fifoCapacityFrames;
        ++queuedFrames;
        return std::abs(mono);
    };

    hr = renderClient->Start();
    if (FAILED(hr)) goto cleanup;

    hr = captureClient->Start();
    if (FAILED(hr)) goto cleanup;

    running.store(true, std::memory_order_release);

    while (!stopRequested.load(std::memory_order_acquire))
    {
        UINT32 packetFrames = 0;
        hr = captureService->GetNextPacketSize(&packetFrames);
        if (FAILED(hr)) break;

        float packetPeak = 0.0f;
        while (packetFrames > 0)
        {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            UINT64 devPos = 0;
            UINT64 qpcPos = 0;

            hr = captureService->GetBuffer(&data, &frames, &flags, &devPos, &qpcPos);
            if (FAILED(hr)) break;

            if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) == 0 && data != nullptr)
            {
                for (UINT32 f = 0; f < frames; ++f)
                    packetPeak = std::max(packetPeak,
                                          pushFrame(data + static_cast<std::size_t>(f) * codec.blockAlign));
            }
            else
            {
                for (UINT32 f = 0; f < frames; ++f)
                    pushFrame(silentFrame.data());
            }

            captureService->ReleaseBuffer(frames);

            hr = captureService->GetNextPacketSize(&packetFrames);
            if (FAILED(hr)) break;
        }

        const float oldPeak = peak.load(std::memory_order_relaxed);
        peak.store(std::clamp(packetPeak >= oldPeak ? packetPeak : oldPeak * 0.88f, 0.0f, 1.0f),
                   std::memory_order_relaxed);

        UINT32 padding = 0;
        hr = renderClient->GetCurrentPadding(&padding);
        if (FAILED(hr)) break;

        const UINT32 writable = renderBufferFrames > padding ? renderBufferFrames - padding : 0;
        if (writable > 0)
        {
            BYTE* out = nullptr;
            hr = renderService->GetBuffer(writable, &out);
            if (FAILED(hr)) break;

            for (UINT32 f = 0; f < writable; ++f)
            {
                BYTE* frame = out + static_cast<std::size_t>(f) * codec.blockAlign;
                if (queuedFrames > 0)
                {
                    const std::size_t base = readFrame * codec.channels;
                    for (std::uint16_t ch = 0; ch < codec.channels; ++ch)
                        codec.write(frame, ch, fifo[base + ch]);

                    readFrame = (readFrame + 1) % fifoCapacityFrames;
                    --queuedFrames;
                }
                else
                {
                    for (std::uint16_t ch = 0; ch < codec.channels; ++ch)
                        codec.write(frame, ch, 0.0f);
                }
            }

            hr = renderService->ReleaseBuffer(writable, 0);
            if (FAILED(hr)) break;
        }

        Sleep(2);
    }

cleanup:
    running.store(false, std::memory_order_release);

    if (captureClient != nullptr)
        captureClient->Stop();
    if (renderClient != nullptr)
        renderClient->Stop();

    if (FAILED(hr) && !stopRequested.load(std::memory_order_acquire))
        error.store(true, std::memory_order_release);

    if (format != nullptr)
        CoTaskMemFree(format);

    releaseCom(renderService);
    releaseCom(captureService);
    releaseCom(renderClient);
    releaseCom(captureClient);
    releaseCom(renderDevice);
    releaseCom(captureDevice);
    releaseCom(enumerator);

    if (SUCCEEDED(initHr))
        CoUninitialize();
}
