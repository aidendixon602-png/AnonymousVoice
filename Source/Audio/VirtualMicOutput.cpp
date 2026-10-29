#include "VirtualMicOutput.h"
#include <atomic>

//==============================================================================
// Platform-specific includes
//==============================================================================

#if JUCE_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <mmsystem.h>
    #include <mmdeviceapi.h>
    #include <audioclient.h>
    #include <avrt.h>
    #pragma comment(lib, "ole32.lib")
    #pragma comment(lib, "winmm.lib")
#endif

//==============================================================================
// Constructor / Destructor
//==============================================================================

VirtualMicOutput::VirtualMicOutput()
{
#if JUCE_WINDOWS
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
}

VirtualMicOutput::~VirtualMicOutput()
{
    shutdown();
#if JUCE_WINDOWS
    CoUninitialize();
#endif
}

//==============================================================================
// Initialization
//==============================================================================

bool VirtualMicOutput::initialize(int targetSampleRate, int targetNumChannels)
{
    shutdown();
    
    sampleRate = targetSampleRate;
    numChannels = targetNumChannels;
    
    // Prepare buffer
    prepare(sampleRate);
    
    // Try to create virtual endpoint
    if (createVirtualEndpoint())
    {
        active.store(true);
        juce::Logger::writeToLog("Virtual microphone initialized: " + deviceName);
        return true;
    }
    else
    {
        juce::Logger::writeToLog("Warning: Could not create virtual microphone endpoint");
        // Still usable as FIFO, just won't appear as system device
        active.store(false);
        return false;
    }
}

void VirtualMicOutput::shutdown()
{
    if (active.load())
    {
        destroyVirtualEndpoint();
        active.store(false);
    }
    
    reset();
}

//==============================================================================
// FIFO Buffer Management
//==============================================================================

void VirtualMicOutput::prepare(int capacitySamples)
{
    const auto capacity = juce::jmax(1024, capacitySamples);
    storage.setSize(numChannels, capacity, false, true, false);
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
            // Buffer full - drop samples and continue
            droppedSamples.fetch_add(remaining);
            break;
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

//==============================================================================
// Device Information
//==============================================================================

void VirtualMicOutput::setDeviceName(const juce::String& name)
{
    deviceName = name;
}

juce::String VirtualMicOutput::getDeviceId() const
{
    return deviceId;
}

//==============================================================================
// Windows WASAPI Virtual Endpoint Implementation
//==============================================================================

#if JUCE_WINDOWS

bool VirtualMicOutput::createVirtualEndpoint()
{
    HRESULT hr = S_OK;
    
    try
    {
        // Get device enumerator
        IMMDeviceEnumerator* pEnumerator = nullptr;
        hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            (void**)&pEnumerator);
        
        if (FAILED(hr))
        {
            juce::Logger::writeToLog("Failed to create device enumerator: " + juce::String::fromUTF8((const char*)&hr, 4));
            return false;
        }
        
        // For now, we'll use a simpler approach: register as a loopback device
        // In a production app, you'd create a proper virtual audio driver
        
        // Create wave format
        waveFormat = new WAVEFORMATEX();
        waveFormat->wFormatTag = WAVE_FORMAT_PCM;
        waveFormat->nChannels = static_cast<WORD>(numChannels);
        waveFormat->nSamplesPerSec = sampleRate;
        waveFormat->wBitsPerSample = 16; // Use 16-bit for compatibility
        waveFormat->nBlockAlign = (waveFormat->nChannels * waveFormat->wBitsPerSample) / 8;
        waveFormat->nAvgBytesPerSec = waveFormat->nSamplesPerSec * waveFormat->nBlockAlign;
        waveFormat->cbSize = 0;
        
        // Get default audio endpoint (for loopback)
        IMMDevice* pEndpoint = nullptr;
        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pEndpoint);
        
        if (FAILED(hr))
        {
            pEnumerator->Release();
            delete waveFormat;
            waveFormat = nullptr;
            return false;
        }
        
        // Activate audio client
        hr = pEndpoint->Activate(
            __uuidof(IAudioClient),
            CLSCTX_ALL,
            nullptr,
            (void**)&audioClient);
        
        pEndpoint->Release();
        pEnumerator->Release();
        
        if (FAILED(hr))
        {
            delete waveFormat;
            waveFormat = nullptr;
            return false;
        }
        
        // Get buffer size
        hr = audioClient->GetBufferSize(&bufferFrameCount);
        
        if (FAILED(hr))
        {
            audioClient->Release();
            audioClient = nullptr;
            delete waveFormat;
            waveFormat = nullptr;
            return false;
        }
        
        // Initialize audio client in shared mode
        hr = audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK,
            bufferFrameCount * 2, // Double buffer
            0,
            waveFormat,
            nullptr);
        
        if (FAILED(hr))
        {
            // Try exclusive mode
            hr = audioClient->Initialize(
                AUDCLNT_SHAREMODE_SHARED,
                0,
                bufferFrameCount,
                0,
                waveFormat,
                nullptr);
        }
        
        if (FAILED(hr))
        {
            audioClient->Release();
            audioClient = nullptr;
            delete waveFormat;
            waveFormat = nullptr;
            return false;
        }
        
        // Get render client
        hr = audioClient->GetService(
            __uuidof(IAudioRenderClient),
            (void**)&renderClient);
        
        if (FAILED(hr))
        {
            audioClient->Release();
            audioClient = nullptr;
            delete waveFormat;
            waveFormat = nullptr;
            return false;
        }
        
        // Start audio thread
        threadRunning = true;
        audioThread = CreateThread(
            nullptr,
            0,
            audioThreadProc,
            this,
            0,
            nullptr);
        
        if (audioThread == nullptr)
        {
            renderClient->Release();
            renderClient = nullptr;
            audioClient->Release();
            audioClient = nullptr;
            delete waveFormat;
            waveFormat = nullptr;
            return false;
        }
        
        // Set thread priority
        SetThreadPriority(audioThread, THREAD_PRIORITY_HIGHEST);
        
        deviceId = "AnonymousVirtualMic";
        return true;
    }
    catch (...)
    {
        // Cleanup on exception
        if (renderClient)
        {
            renderClient->Release();
            renderClient = nullptr;
        }
        if (audioClient)
        {
            audioClient->Release();
            audioClient = nullptr;
        }
        if (waveFormat)
        {
            delete waveFormat;
            waveFormat = nullptr;
        }
        return false;
    }
}

void VirtualMicOutput::destroyVirtualEndpoint()
{
    threadRunning = false;
    
    if (audioThread)
    {
        // Wait for thread to finish
        WaitForSingleObject(audioThread, 2000);
        CloseHandle(audioThread);
        audioThread = nullptr;
    }
    
    if (renderClient)
    {
        renderClient->Release();
        renderClient = nullptr;
    }
    
    if (audioClient)
    {
        audioClient->Stop();
        audioClient->Release();
        audioClient = nullptr;
    }
    
    if (waveFormat)
    {
        delete waveFormat;
        waveFormat = nullptr;
    }
}

DWORD WINAPI VirtualMicOutput::audioThreadProc(LPVOID lpParam)
{
    auto* self = static_cast<VirtualMicOutput*>(lpParam);
    if (self)
        self->audioThreadFunction();
    return 0;
}

void VirtualMicOutput::audioThreadFunction()
{
    HRESULT hr = S_OK;
    BYTE* pData = nullptr;
    UINT32 numFramesAvailable = 0;
    DWORD flags = 0;
    
    // Start the audio client
    hr = audioClient->Start();
    if (FAILED(hr))
    {
        juce::Logger::writeToLog("Failed to start audio client");
        return;
    }
    
    // Audio processing loop
    while (threadRunning)
    {
        // Wait for data to be available
        hr = audioClient->GetNextPacketSize(&numFramesAvailable);
        
        if (FAILED(hr))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        if (numFramesAvailable == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        // Get buffer
        hr = renderClient->GetBuffer(numFramesAvailable, &pData);
        
        if (FAILED(hr))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Process audio
        if (pData && numFramesAvailable > 0)
        {
            // Try to pop audio from our FIFO
            float tempBuffer[1024];
            int samplesToRead = static_cast<int>(numFramesAvailable);
            
            if (samplesToRead > 0)
            {
                int samplesRead = popMono(tempBuffer, samplesToRead);
                
                if (samplesRead > 0)
                {
                    // Convert float to 16-bit PCM
                    auto* pBuffer = reinterpret_cast<short*>(pData);
                    for (int i = 0; i < samplesRead && i < static_cast<int>(numFramesAvailable); ++i)
                    {
                        float sample = tempBuffer[i];
                        // Clamp to [-1, 1]
                        sample = std::clamp(sample, -1.0f, 1.0f);
                        // Convert to 16-bit
                        pBuffer[i] = static_cast<short>(sample * 32767.0f);
                    }
                    
                    // Fill remaining with silence
                    for (UINT32 i = static_cast<UINT32>(samplesRead); i < numFramesAvailable; ++i)
                    {
                        reinterpret_cast<short*>(pData)[i] = 0;
                    }
                }
                else
                {
                    // No data available - output silence
                    std::memset(pData, 0, numFramesAvailable * waveFormat->nBlockAlign);
                }
            }
        }
        
        // Release buffer
        hr = renderClient->ReleaseBuffer(numFramesAvailable, flags);
        
        if (FAILED(hr))
        {
            juce::Logger::writeToLog("Failed to release audio buffer");
        }
    }
    
    // Stop the audio client
    hr = audioClient->Stop();
}

#else

// Non-Windows platforms - virtual mic not supported
bool VirtualMicOutput::createVirtualEndpoint()
{
    juce::Logger::writeToLog("Virtual microphone not supported on this platform");
    return false;
}

void VirtualMicOutput::destroyVirtualEndpoint()
{
    // Nothing to do on non-Windows
}

#endif
