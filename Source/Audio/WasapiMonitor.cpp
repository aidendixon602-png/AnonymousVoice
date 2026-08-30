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
    constexpr double pi = 3.14159265358979323846;

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

    struct DspPreset
    {
        float semitones;
        float drive;
        float body;
        float lowpassHz;
        float compressor;
        float mix;
        float gain;
        float ringMix;
        float ringHz;
    };

    DspPreset paramsForPreset(int preset)
    {
        switch (std::clamp(preset, 0, 9))
        {
            case 0: return { -5.0f, 1.55f, 0.46f, 6200.0f, 0.72f, 1.00f, 0.92f, 0.00f, 0.0f }; // Anonymous
            case 1: return { -7.0f, 1.35f, 0.55f, 5600.0f, 0.62f, 1.00f, 0.92f, 0.00f, 0.0f }; // Deep
            case 2: return { -8.0f, 2.20f, 0.62f, 4700.0f, 0.78f, 1.00f, 0.82f, 0.08f, 46.0f }; // Demon
            case 3: return { -1.0f, 1.65f, 0.16f, 7200.0f, 0.55f, 0.92f, 0.82f, 0.58f, 94.0f }; // Robot
            case 4: return {  3.0f, 1.45f, 0.05f, 8200.0f, 0.52f, 0.95f, 0.86f, 0.22f, 126.0f }; // Alien
            case 5: return {  4.0f, 1.10f, 0.02f, 9800.0f, 0.38f, 0.82f, 0.90f, 0.00f, 0.0f }; // Ethereal
            case 6: return {  7.0f, 1.05f, 0.00f, 11000.0f, 0.42f, 1.00f, 0.88f, 0.00f, 0.0f }; // Baby
            case 7: return { -2.0f, 2.35f, 0.28f, 3900.0f, 0.82f, 0.86f, 0.72f, 0.00f, 0.0f }; // Radio
            case 8: return { -3.0f, 1.65f, 0.30f, 7000.0f, 0.58f, 0.92f, 0.84f, 0.24f, 61.0f }; // Witch
            default:return {  0.0f, 1.00f, 0.00f, 12000.0f, 0.20f, 1.00f, 0.92f, 0.00f, 0.0f }; // Custom
        }
    }

    class GranularPitch
    {
    public:
        void initialise(std::uint32_t sampleRate)
        {
            rate = std::max<std::uint32_t>(sampleRate, 8000u);
            grainSamples = std::max<std::size_t>(128u, static_cast<std::size_t>(rate * 0.034));
            minimumDelay = std::max<std::size_t>(32u, static_cast<std::size_t>(rate * 0.006));
            const std::size_t bufferSize = std::max<std::size_t>(grainSamples * 4u + minimumDelay + 8u,
                                                                  static_cast<std::size_t>(rate / 4u));
            buffer.assign(bufferSize, 0.0f);
            writePos = 0;
            phase = 0.0;
            setSemitones(0.0f);
        }

        void setSemitones(float semitones)
        {
            ratio = std::pow(2.0, static_cast<double>(semitones) / 12.0);
        }

        float process(float input)
        {
            if (buffer.empty())
                return input;

            buffer[writePos] = input;

            if (std::abs(ratio - 1.0) < 0.0001)
            {
                writePos = (writePos + 1) % buffer.size();
                return input;
            }

            const double p1 = phase;
            double p2 = phase + 0.5;
            if (p2 >= 1.0)
                p2 -= 1.0;

            const float a = readGrain(p1);
            const float b = readGrain(p2);
            const double w1 = 0.5 - 0.5 * std::cos(2.0 * pi * p1);
            const double w2 = 0.5 - 0.5 * std::cos(2.0 * pi * p2);
            const double weight = std::max(0.001, w1 + w2);
            const float output = static_cast<float>((a * w1 + b * w2) / weight);

            phase += 1.0 / static_cast<double>(grainSamples);
            if (phase >= 1.0)
                phase -= 1.0;

            writePos = (writePos + 1) % buffer.size();
            return output;
        }

        double algorithmicLatencyMs() const noexcept
        {
            return rate > 0 ? static_cast<double>(minimumDelay) * 1000.0 / static_cast<double>(rate) : 0.0;
        }

    private:
        float readGrain(double grainPhase) const
        {
            const double sweep = std::abs(1.0 - ratio) * static_cast<double>(grainSamples);
            const double delay = ratio < 1.0
                ? static_cast<double>(minimumDelay) + grainPhase * sweep
                : static_cast<double>(minimumDelay) + (1.0 - grainPhase) * sweep;

            double read = static_cast<double>(writePos) - delay;
            const double size = static_cast<double>(buffer.size());
            while (read < 0.0)
                read += size;
            while (read >= size)
                read -= size;

            const std::size_t i0 = static_cast<std::size_t>(read);
            const std::size_t i1 = (i0 + 1) % buffer.size();
            const float frac = static_cast<float>(read - static_cast<double>(i0));
            return buffer[i0] + (buffer[i1] - buffer[i0]) * frac;
        }

        std::vector<float> buffer;
        std::size_t writePos = 0;
        std::size_t grainSamples = 128;
        std::size_t minimumDelay = 32;
        std::uint32_t rate = 0;
        double phase = 0.0;
        double ratio = 1.0;
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
    latency.store(0.0, std::memory_order_relaxed);
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
    std::size_t maxQueuedFrames = 0;
    std::vector<float> fifo;
    std::vector<BYTE> silentFrame;
    std::size_t readFrame = 0;
    std::size_t writeFrame = 0;
    std::size_t queuedFrames = 0;
    GranularPitch pitchShifter;
    float hpLowState = 0.0f;
    float bodyState = 0.0f;
    float lowpassState = 0.0f;
    float compressorEnvelope = 0.0f;
    double ringPhase = 0.0;
    int cachedPreset = -1;
    DspPreset cachedParams {};
    float hpCoeff = 0.0f;
    float bodyCoeff = 0.0f;
    float lpCoeff = 0.0f;
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
    pitchShifter.initialise(format->nSamplesPerSec);

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

    fifoCapacityFrames = std::max<std::size_t>(static_cast<std::size_t>(format->nSamplesPerSec) / 5u,
                                                static_cast<std::size_t>(renderBufferFrames) * 2u);
    maxQueuedFrames = std::max<std::size_t>(static_cast<std::size_t>(format->nSamplesPerSec) / 100u,
                                             static_cast<std::size_t>(renderBufferFrames) / 6u);
    fifo.assign(fifoCapacityFrames * codec.channels, 0.0f);
    silentFrame.assign(codec.blockAlign, 0);

    pushFrame = [&](const BYTE* frame) -> float
    {
        while (queuedFrames >= maxQueuedFrames && queuedFrames > 0)
        {
            readFrame = (readFrame + 1) % fifoCapacityFrames;
            --queuedFrames;
        }

        float mono = 0.0f;
        for (std::uint16_t ch = 0; ch < codec.channels; ++ch)
            mono += codec.read(frame, ch);
        mono /= static_cast<float>(codec.channels);

        const int presetNow = std::clamp(presetIndex.load(std::memory_order_acquire), 0, 9);
        if (presetNow != cachedPreset)
        {
            cachedPreset = presetNow;
            cachedParams = paramsForPreset(cachedPreset);
            pitchShifter.setSemitones(cachedParams.semitones);

            const float sr = static_cast<float>(format->nSamplesPerSec);
            hpCoeff = 1.0f - std::exp(-2.0f * static_cast<float>(pi) * 82.0f / sr);
            bodyCoeff = 1.0f - std::exp(-2.0f * static_cast<float>(pi) * 620.0f / sr);
            lpCoeff = 1.0f - std::exp(-2.0f * static_cast<float>(pi) * cachedParams.lowpassHz / sr);
        }

        // Remove sub-bass/rumble before the voice effect.
        hpLowState += hpCoeff * (mono - hpLowState);
        const float highPassed = mono - hpLowState;

        // Time-domain dual-grain pitch shifter. This changes pitch without changing stream duration.
        const float pitched = pitchShifter.process(highPassed);

        // Formant-colour approximation: reinforce low vocal body independently from pitch.
        bodyState += bodyCoeff * (pitched - bodyState);
        float coloured = pitched * (1.0f - cachedParams.body) + bodyState * (1.0f + cachedParams.body * 0.70f);

        // Mild saturation followed by bandwidth shaping.
        coloured = std::tanh(coloured * cachedParams.drive);
        lowpassState += lpCoeff * (coloured - lowpassState);
        float processed = lowpassState;

        // Optional modulation remains only on synthetic presets, not Anonymous/Deep.
        if (cachedParams.ringMix > 0.001f)
        {
            const float ring = static_cast<float>(std::sin(ringPhase));
            ringPhase += 2.0 * pi * static_cast<double>(cachedParams.ringHz) /
                         static_cast<double>(format->nSamplesPerSec);
            if (ringPhase >= 2.0 * pi)
                ringPhase -= 2.0 * pi;
            processed = processed * (1.0f - cachedParams.ringMix) + (processed * ring) * cachedParams.ringMix;
        }

        // Fast voice compressor. Keeps quiet speech present without crushing transients completely.
        const float magnitude = std::abs(processed);
        const float envCoeff = magnitude > compressorEnvelope ? 0.16f : 0.008f;
        compressorEnvelope += envCoeff * (magnitude - compressorEnvelope);
        const float threshold = 0.30f;
        float compressorGain = 1.0f;
        if (compressorEnvelope > threshold)
        {
            const float over = compressorEnvelope / threshold;
            compressorGain = std::pow(over, -cachedParams.compressor * 0.72f);
        }
        processed *= compressorGain;

        // Dry/wet mix and safety limiter.
        processed = mono * (1.0f - cachedParams.mix) + processed * cachedParams.mix;
        processed = std::tanh(processed * cachedParams.gain * 1.30f) * 0.92f;
        const float output = enabled.load(std::memory_order_acquire) ? processed : 0.0f;

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

        if (format->nSamplesPerSec > 0)
        {
            const double bufferedFrames = static_cast<double>(queuedFrames) + static_cast<double>(padding);
            const double deviceAndQueueMs = bufferedFrames * 1000.0 / static_cast<double>(format->nSamplesPerSec);
            latency.store(deviceAndQueueMs + pitchShifter.algorithmicLatencyMs(), std::memory_order_relaxed);
        }

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

        Sleep(1);
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
