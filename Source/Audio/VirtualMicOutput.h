#pragma once

#include <JuceHeader.h>

// Forward declarations for Windows audio APIs
struct IAudioClient;
struct IAudioRenderClient;
struct IMMDevice;
struct IMMDeviceEnumerator;

/**
 * Virtual Microphone Output - Creates a Windows virtual audio endpoint.
 * 
 * This class implements a virtual microphone that other applications can use as an input device.
 * It uses Windows WASAPI to create a loopback device that captures processed audio.
 * 
 * For a production-ready solution, consider:
 * - Windows Audio Device (WDM) driver
 * - Virtual Audio Cable (VAC) SDK
 * - Steinberg Audio Stream I/O (ASIO)
 */
class VirtualMicOutput final
{
public:
    VirtualMicOutput();
    ~VirtualMicOutput();

    /** Initialize the virtual microphone */
    bool initialize(int sampleRate = 48000, int numChannels = 1);
    
    /** Shutdown the virtual microphone */
    void shutdown();
    
    /** Prepare the FIFO buffer */
    void prepare(int capacitySamples = 48000);
    
    /** Reset the FIFO buffer */
    void reset();
    
    /** Push mono audio data into the buffer */
    void pushMono(const float* samples, int numSamples) noexcept;
    
    /** Pop mono audio data from the buffer */
    int popMono(float* destination, int maxSamples) noexcept;
    
    /** Get the number of buffered samples */
    int getBufferedSamples() const noexcept;
    
    /** Get the number of dropped samples */
    int getDroppedSamples() const noexcept { return droppedSamples.load(); }
    
    /** Check if virtual mic is active */
    bool isActive() const noexcept { return active.load(); }
    
    /** Get the device ID (for UI display) */
    juce::String getDeviceId() const;
    
    /** Set the device name */
    void setDeviceName(const juce::String& name);
    
    /** Get the device name */
    juce::String getDeviceName() const { return deviceName; }

private:
    // FIFO buffer for audio data
    juce::AbstractFifo fifo { 1 };
    juce::AudioBuffer<float> storage;
    std::atomic<int> droppedSamples { 0 };
    std::atomic<bool> active { false };
    
    // Device information
    juce::String deviceName { "Anonymous Virtual Microphone" };
    juce::String deviceId;
    
    // Windows WASAPI components
#if JUCE_WINDOWS
    IMMDevice* audioDevice = nullptr;
    IAudioClient* audioClient = nullptr;
    IAudioRenderClient* renderClient = nullptr;
    WAVEFORMATEX* waveFormat = nullptr;
    UINT32 bufferFrameCount = 0;
    REFERENCE_TIME bufferDuration = 0;
    HANDLE audioThread = nullptr;
    bool threadRunning = false;
#endif
    
    // Sample rate and format
    int sampleRate = 48000;
    int numChannels = 1;
    
    // Internal methods
    bool createVirtualEndpoint();
    void destroyVirtualEndpoint();
    
#if JUCE_WINDOWS
    static DWORD WINAPI audioThreadProc(LPVOID lpParam);
    void audioThreadFunction();
#endif
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualMicOutput)
};
