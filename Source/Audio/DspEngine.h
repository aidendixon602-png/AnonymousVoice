#pragma once

#include <JuceHeader.h>

/**
 * DSP Engine for voice processing.
 * 
 * Implements a complete voice transformation chain:
 * Gate -> Pitch Shift -> Formant Shift -> EQ -> Compressor -> Distortion -> Chorus -> Reverb -> Limiter
 * 
 * All parameters are atomic for thread-safe access from UI and audio threads.
 */
class DspEngine final
{
public:
    DspEngine() = default;
    ~DspEngine() = default;

    /** DSP Parameters - all atomic for thread safety */
    struct Parameters
    {
        // Gate
        std::atomic<float> gateThresholdDb { -45.0f };
        std::atomic<float> gateAttackMs { 1.0f };
        std::atomic<float> gateReleaseMs { 10.0f };
        
        // Pitch Shift (in semitones, negative = lower, positive = higher)
        std::atomic<float> pitchSemitones { -5.0f };
        
        // Formant Shift (preserves character while changing pitch)
        std::atomic<float> formantShift { -3.0f };
        
        // 3-band EQ (in dB)
        std::atomic<float> eqLowGainDb { 4.0f };
        std::atomic<float> eqMidGainDb { 1.0f };
        std::atomic<float> eqHighGainDb { -5.0f };
        std::atomic<float> eqLowFreq { 250.0f };
        std::atomic<float> eqMidFreq { 1500.0f };
        std::atomic<float> eqHighFreq { 4000.0f };
        
        // Compressor
        std::atomic<float> compressorThresholdDb { -18.0f };
        std::atomic<float> compressorRatio { 4.0f };
        std::atomic<float> compressorAttackMs { 5.0f };
        std::atomic<float> compressorReleaseMs { 80.0f };
        
        // Distortion
        std::atomic<float> distortionDrive { 0.08f };  // 0-1
        std::atomic<float> distortionMix { 0.5f };    // 0-1, wet/dry
        
        // Chorus
        std::atomic<float> chorusRate { 0.25f };      // Hz
        std::atomic<float> chorusDepth { 0.06f };     // 0-1
        std::atomic<float> chorusMix { 0.3f };        // 0-1
        
        // Reverb
        std::atomic<float> reverbRoomSize { 0.22f }; // 0-1
        std::atomic<float> reverbDamping { 0.55f };   // 0-1
        std::atomic<float> reverbMix { 0.20f };      // 0-1, wet/dry
        
        // Limiter
        std::atomic<float> limiterThresholdDb { -1.0f };
        std::atomic<float> limiterReleaseMs { 50.0f };
        
        // Global
        std::atomic<float> outputGainDb { 0.0f };
        std::atomic<float> wetDryMix { 1.0f };        // 0 = full dry, 1 = full wet
        
        // Bypass flags
        std::atomic<bool> bypassGate { false };
        std::atomic<bool> bypassPitch { false };
        std::atomic<bool> bypassFormant { false };
        std::atomic<bool> bypassEQ { false };
        std::atomic<bool> bypassCompressor { false };
        std::atomic<bool> bypassDistortion { false };
        std::atomic<bool> bypassChorus { false };
        std::atomic<bool> bypassReverb { false };
        std::atomic<bool> bypassLimiter { false };
    };

    /** Prepare the DSP engine */
    void prepare(double sampleRate, int maxBlockSize, int numChannels);
    
    /** Reset all internal state */
    void reset();
    
    /** Process audio buffer */
    void process(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    /** Get parameters (non-const) */
    Parameters& parameters() noexcept { return params; }
    
    /** Get parameters (const) */
    const Parameters& parameters() const noexcept { return params; }
    
    /** Get current sample rate */
    double getSampleRate() const noexcept { return currentSampleRate; }
    
    /** Get current block size */
    int getBlockSize() const noexcept { return currentBlockSize; }

private:
    Parameters params;
    
    // State
    double currentSampleRate = 48000.0;
    int currentBlockSize = 512;
    int currentNumChannels = 2;
    
    // Gate state
    std::vector<float> gateEnvelopes;
    
    // Compressor state
    std::vector<float> compressorEnvelopes;
    
    // Pitch shifting state (using phase vocoder approach)
    std::vector<std::vector<float>> pitchDelayLines;
    std::vector<int> pitchWritePositions;
    std::vector<float> pitchPhases;
    float pitchRatio = 1.0f;
    
    // Formant shifting state (using filter bank)
    std::vector<std::vector<juce::dsp::IIR::Filter<float>>> formantFilters;
    
    // EQ filters (3-band: low, mid, high)
    std::vector<juce::dsp::IIR::Filter<float>> eqLowFilters;
    std::vector<juce::dsp::IIR::Filter<float>> eqMidFilters;
    std::vector<juce::dsp::IIR::Filter<float>> eqHighFilters;
    
    // Chorus state
    juce::AudioBuffer<float> chorusDelayBuffer;
    int chorusWritePosition = 0;
    float chorusPhase = 0.0f;
    
    // Reverb
    juce::Reverb reverb;
    
    // Limiter state
    std::vector<float> limiterEnvelopes;
    
    // Working buffers
    juce::AudioBuffer<float> tempBuffer;
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> wetBuffer;
    
    // Internal processing methods
    void processGate(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void processPitchShift(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void processFormantShift(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void processEQ(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void processCompressor(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void processDistortion(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void processChorus(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void processReverb(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    void processLimiter(juce::AudioBuffer<float>& buffer, int startSample, int numSamples);
    
    // Utility methods
    void updatePitchRatio();
    void updateEQFilters();
    void updateFormantFilters();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspEngine)
};
