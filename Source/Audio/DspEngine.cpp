#include "DspEngine.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Preparation and Reset
//==============================================================================

void DspEngine::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    currentBlockSize = std::max(64, maxBlockSize);
    currentNumChannels = std::max(1, numChannels);
    
    // Initialize state vectors
    gateEnvelopes.assign(currentNumChannels, 0.0f);
    compressorEnvelopes.assign(currentNumChannels, 0.0f);
    limiterEnvelopes.assign(currentNumChannels, 0.0f);
    
    // Initialize pitch shifting (phase vocoder with delay lines)
    const int maxPitchDelay = static_cast<int>(currentSampleRate * 0.1); // 100ms max
    pitchDelayLines.clear();
    pitchWritePositions.clear();
    pitchPhases.clear();
    
    for (int ch = 0; ch < currentNumChannels; ++ch)
    {
        pitchDelayLines.emplace_back(maxPitchDelay + 4);
        pitchWritePositions.push_back(0);
        pitchPhases.push_back(0.0f);
    }
    
    updatePitchRatio();
    
    // Initialize formant shifting filters (4 formants for voice)
    formantFilters.clear();
    for (int ch = 0; ch < currentNumChannels; ++ch)
    {
        std::vector<juce::dsp::IIR::Filter<float>> channelFilters;
        for (int f = 0; f < 4; ++f)
        {
            auto filter = juce::dsp::IIR::Filter<float>();
            // Default formant frequencies (500, 1500, 2500, 3500 Hz)
            float freq = 500.0f * (f + 1) * 1000.0f;
            filter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                currentSampleRate, freq, 1.0f, 0.5f);
            channelFilters.push_back(std::move(filter));
        }
        formantFilters.push_back(std::move(channelFilters));
    }
    
    // Initialize EQ filters
    eqLowFilters.clear();
    eqMidFilters.clear();
    eqHighFilters.clear();
    
    for (int ch = 0; ch < currentNumChannels; ++ch)
    {
        auto lowFilter = juce::dsp::IIR::Filter<float>();
        auto midFilter = juce::dsp::IIR::Filter<float>();
        auto highFilter = juce::dsp::IIR::Filter<float>();
        
        eqLowFilters.push_back(std::move(lowFilter));
        eqMidFilters.push_back(std::move(midFilter));
        eqHighFilters.push_back(std::move(highFilter));
    }
    
    updateEQFilters();
    
    // Initialize chorus
    const int chorusDelaySamples = static_cast<int>(currentSampleRate * 0.030); // 30ms
    chorusDelayBuffer.setSize(currentNumChannels, chorusDelaySamples + 4, false, true, true);
    chorusWritePosition = 0;
    chorusPhase = 0.0f;
    
    // Initialize reverb
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = params.reverbRoomSize.load();
    reverbParams.damping = params.reverbDamping.load();
    reverbParams.wetLevel = params.reverbMix.load();
    reverbParams.dryLevel = 1.0f - reverbParams.wetLevel;
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);
    
    // Initialize working buffers
    tempBuffer.setSize(currentNumChannels, maxBlockSize * 2, false, true, true);
    dryBuffer.setSize(currentNumChannels, maxBlockSize, false, true, true);
    wetBuffer.setSize(currentNumChannels, maxBlockSize, false, true, true);
    
    reset();
}

void DspEngine::reset()
{
    // Reset envelopes
    std::fill(gateEnvelopes.begin(), gateEnvelopes.end(), 0.0f);
    std::fill(compressorEnvelopes.begin(), compressorEnvelopes.end(), 0.0f);
    std::fill(limiterEnvelopes.begin(), limiterEnvelopes.end(), 0.0f);
    
    // Reset delay lines
    for (auto& delayLine : pitchDelayLines)
        std::fill(delayLine.begin(), delayLine.end(), 0.0f);
    
    for (int ch = 0; ch < currentNumChannels; ++ch)
    {
        pitchWritePositions[ch] = 0;
        pitchPhases[ch] = 0.0f;
    }
    
    // Reset chorus
    chorusDelayBuffer.clear();
    chorusWritePosition = 0;
    chorusPhase = 0.0f;
    
    // Reset reverb
    reverb.reset();
    
    // Clear working buffers
    tempBuffer.clear();
    dryBuffer.clear();
    wetBuffer.clear();
}

//==============================================================================
// Parameter Update Helpers
//==============================================================================

void DspEngine::updatePitchRatio()
{
    const float semitones = params.pitchSemitones.load();
    pitchRatio = std::pow(2.0f, semitones / 12.0f);
}

void DspEngine::updateEQFilters()
{
    const float lowFreq = params.eqLowFreq.load();
    const float midFreq = params.eqMidFreq.load();
    const float highFreq = params.eqHighFreq.load();
    
    const float lowGain = juce::Decibels::decibelsToGain(params.eqLowGainDb.load());
    const float midGain = juce::Decibels::decibelsToGain(params.eqMidGainDb.load());
    const float highGain = juce::Decibels::decibelsToGain(params.eqHighGainDb.load());
    
    for (int ch = 0; ch < currentNumChannels; ++ch)
    {
        // Low shelf filter
        eqLowFilters[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            currentSampleRate, lowFreq, 0.5f, lowGain);
        
        // Mid peak filter
        eqMidFilters[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            currentSampleRate, midFreq, 1.0f, midGain);
        
        // High shelf filter
        eqHighFilters[ch].coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            currentSampleRate, highFreq, 0.5f, highGain);
    }
}

void DspEngine::updateFormantFilters()
{
    const float shift = params.formantShift.load();
    const float shiftFactor = std::pow(2.0f, shift / 12.0f); // Convert semitones to frequency ratio
    
    // Base formant frequencies (typical for human voice)
    const std::array<float, 4> baseFormants = { 500.0f, 1500.0f, 2500.0f, 3500.0f };
    
    for (int ch = 0; ch < currentNumChannels; ++ch)
    {
        for (int f = 0; f < 4; ++f)
        {
            const float shiftedFreq = baseFormants[f] * shiftFactor;
            // Clamp to reasonable range
            const float clampedFreq = std::clamp(shiftedFreq, 80.0f, 8000.0f);
            
            // Create peak filter at shifted formant frequency
            formantFilters[ch][f].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
                currentSampleRate, clampedFreq, 1.0f, 1.5f); // Boost formants
        }
    }
}

//==============================================================================
// Main Processing
//==============================================================================

void DspEngine::process(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (numSamples <= 0 || buffer.getNumChannels() <= 0)
        return;
    
    // Ensure we have enough channels
    const int numChannels = std::min(buffer.getNumChannels(), currentNumChannels);
    if (numChannels <= 0)
        return;
    
    // Copy input to dry buffer (for wet/dry mix)
    dryBuffer.clear();
    for (int ch = 0; ch < numChannels; ++ch)
    {
        dryBuffer.copyFrom(ch, 0, buffer, ch, startSample, numSamples);
    }
    
    // Process through DSP chain
    // Order: Gate -> Pitch -> Formant -> EQ -> Compressor -> Distortion -> Chorus -> Reverb -> Limiter
    
    tempBuffer.clear();
    tempBuffer.setSize(numChannels, numSamples, false, true, true);
    
    // Copy to temp buffer for processing
    for (int ch = 0; ch < numChannels; ++ch)
    {
        tempBuffer.copyFrom(ch, 0, buffer, ch, startSample, numSamples);
    }
    
    // 1. Gate
    if (!params.bypassGate.load())
        processGate(tempBuffer, 0, numSamples);
    
    // 2. Pitch Shift
    if (!params.bypassPitch.load())
        processPitchShift(tempBuffer, 0, numSamples);
    
    // 3. Formant Shift
    if (!params.bypassFormant.load())
        processFormantShift(tempBuffer, 0, numSamples);
    
    // 4. EQ
    if (!params.bypassEQ.load())
        processEQ(tempBuffer, 0, numSamples);
    
    // 5. Compressor
    if (!params.bypassCompressor.load())
        processCompressor(tempBuffer, 0, numSamples);
    
    // 6. Distortion
    if (!params.bypassDistortion.load())
        processDistortion(tempBuffer, 0, numSamples);
    
    // 7. Chorus
    if (!params.bypassChorus.load())
        processChorus(tempBuffer, 0, numSamples);
    
    // 8. Reverb
    if (!params.bypassReverb.load())
        processReverb(tempBuffer, 0, numSamples);
    
    // 9. Limiter
    if (!params.bypassLimiter.load())
        processLimiter(tempBuffer, 0, numSamples);
    
    // Apply wet/dry mix
    const float wetMix = params.wetDryMix.load();
    const float dryMix = 1.0f - wetMix;
    
    // Apply output gain
    const float outputGain = juce::Decibels::decibelsToGain(params.outputGainDb.load());
    
    // Mix wet and dry signals
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* temp = tempBuffer.getWritePointer(ch, 0);
        auto* dry = dryBuffer.getReadPointer(ch, 0);
        auto* out = buffer.getWritePointer(ch, startSample);
        
        for (int i = 0; i < numSamples; ++i)
        {
            out[i] = (dry[i] * dryMix + temp[i] * wetMix) * outputGain;
        }
    }
}

//==============================================================================
// Individual Effect Processing
//==============================================================================

void DspEngine::processGate(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float thresholdDb = params.gateThresholdDb.load();
    const float threshold = juce::Decibels::decibelsToGain(thresholdDb);
    const float attackMs = params.gateAttackMs.load();
    const float releaseMs = params.gateReleaseMs.load();
    
    const float attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * currentSampleRate));
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * currentSampleRate));
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer(ch, startSample);
        float& envelope = gateEnvelopes[ch];
        
        for (int i = 0; i < numSamples; ++i)
        {
            const float magnitude = std::abs(samples[i]);
            
            // Update envelope
            if (magnitude > envelope)
                envelope = attackCoeff * envelope + (1.0f - attackCoeff) * magnitude;
            else
                envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * magnitude;
            
            // Apply gate
            if (envelope < threshold)
            {
                samples[i] *= 0.01f; // Very quiet, but not zero to avoid clicks
            }
        }
    }
}

void DspEngine::processPitchShift(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    // Check if pitch shift is needed
    const float semitones = params.pitchSemitones.load();
    if (std::abs(semitones) < 0.01f)
        return;
    
    // Update pitch ratio
    updatePitchRatio();
    
    // Simple pitch shifting using delay line modulation (phase vocoder would be better)
    // This is a simplified approach - for production, consider Rubber Band Library
    
    const float delayMs = 10.0f; // Base delay in ms
    const float delaySamples = delayMs * 0.001f * currentSampleRate;
    const float maxDelaySamples = static_cast<float>(pitchDelayLines[0].size() - 4);
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer(ch, startSample);
        auto& delayLine = pitchDelayLines[ch];
        int& writePos = pitchWritePositions[ch];
        float& phase = pitchPhases[ch];
        
        for (int i = 0; i < numSamples; ++i)
        {
            // Store current sample in delay line
            delayLine[writePos] = samples[i];
            
            // Calculate read position with pitch shift
            // For pitch down (ratio > 1), we read slower (further back)
            // For pitch up (ratio < 1), we read faster (closer)
            float readPos = writePos - delaySamples * (pitchRatio - 1.0f);
            
            // Wrap around
            while (readPos < 0.0f)
                readPos += maxDelaySamples;
            while (readPos >= maxDelaySamples)
                readPos -= maxDelaySamples;
            
            // Linear interpolation
            int readPosInt = static_cast<int>(readPos);
            float frac = readPos - readPosInt;
            int nextPos = (readPosInt + 1) % static_cast<int>(maxDelaySamples);
            
            float delayed = delayLine[readPosInt % static_cast<int>(maxDelayLines[ch].size())];
            float next = delayLine[nextPos % static_cast<int>(maxDelayLines[ch].size())];
            float output = delayed + frac * (next - delayed);
            
            // Mix with original (for smoother transitions)
            samples[i] = output * 0.7f + samples[i] * 0.3f;
            
            // Update positions
            writePos = (writePos + 1) % static_cast<int>(delayLine.size());
        }
    }
}

void DspEngine::processFormantShift(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float shift = params.formantShift.load();
    if (std::abs(shift) < 0.01f)
        return;
    
    updateFormantFilters();
    
    // Apply formant filters in series
    for (int ch = 0; ch < buffer.getNumChannels() && ch < static_cast<int>(formantFilters.size()); ++ch)
    {
        auto* samples = buffer.getWritePointer(ch, startSample);
        auto& filters = formantFilters[ch];
        
        for (int i = 0; i < numSamples; ++i)
        {
            float x = samples[i];
            
            // Apply each formant filter
            for (auto& filter : filters)
            {
                x = filter.processSingleSample(x);
            }
            
            samples[i] = x;
        }
    }
}

void DspEngine::processEQ(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    // Update EQ filters if parameters changed
    static float lastLowGain = 0.0f;
    static float lastMidGain = 0.0f;
    static float lastHighGain = 0.0f;
    
    if (std::abs(params.eqLowGainDb.load() - lastLowGain) > 0.1f ||
        std::abs(params.eqMidGainDb.load() - lastMidGain) > 0.1f ||
        std::abs(params.eqHighGainDb.load() - lastHighGain) > 0.1f)
    {
        updateEQFilters();
        lastLowGain = params.eqLowGainDb.load();
        lastMidGain = params.eqMidGainDb.load();
        lastHighGain = params.eqHighGainDb.load();
    }
    
    for (int ch = 0; ch < buffer.getNumChannels() && ch < static_cast<int>(eqLowFilters.size()); ++ch)
    {
        auto* samples = buffer.getWritePointer(ch, startSample);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float x = samples[i];
            
            // Apply low shelf
            x = eqLowFilters[ch].processSingleSample(x);
            
            // Apply mid peak
            x = eqMidFilters[ch].processSingleSample(x);
            
            // Apply high shelf
            x = eqHighFilters[ch].processSingleSample(x);
            
            samples[i] = x;
        }
    }
}

void DspEngine::processCompressor(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float thresholdDb = params.compressorThresholdDb.load();
    const float threshold = juce::Decibels::decibelsToGain(thresholdDb);
    const float ratio = std::max(1.0f, params.compressorRatio.load());
    const float attackMs = params.compressorAttackMs.load();
    const float releaseMs = params.compressorReleaseMs.load();
    
    const float attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * currentSampleRate));
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * currentSampleRate));
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer(ch, startSample);
        float& envelope = compressorEnvelopes[ch];
        
        for (int i = 0; i < numSamples; ++i)
        {
            const float magnitude = std::abs(samples[i]);
            
            // Update envelope
            if (magnitude > envelope)
                envelope = attackCoeff * envelope + (1.0f - attackCoeff) * magnitude;
            else
                envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * magnitude;
            
            // Apply compression
            if (envelope > threshold && envelope > 0.0f)
            {
                const float overDb = juce::Decibels::gainToDecibels(envelope / threshold);
                const float gainReductionDb = overDb - (overDb / ratio);
                const float gainReduction = juce::Decibels::decibelsToGain(-gainReductionDb);
                samples[i] *= gainReduction;
            }
        }
    }
}

void DspEngine::processDistortion(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float drive = std::clamp(params.distortionDrive.load(), 0.0f, 1.0f);
    const float mix = params.distortionMix.load();
    const float dry = 1.0f - mix;
    
    // Pre-calculate drive amount
    const float driveAmount = 1.0f + drive * 8.0f;
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer(ch, startSample);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float x = samples[i];
            
            // Soft clipping using tanh
            const float distorted = std::tanh(x * driveAmount) / std::tanh(driveAmount);
            
            // Mix dry and wet
            samples[i] = x * dry + distorted * mix;
        }
    }
}

void DspEngine::processChorus(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float rate = params.chorusRate.load();
    const float depth = std::clamp(params.chorusDepth.load(), 0.0f, 1.0f);
    const float mix = params.chorusMix.load();
    const float dry = 1.0f - mix;
    
    if (depth < 0.001f || chorusDelayBuffer.getNumSamples() <= 0)
        return;
    
    const float phaseIncrement = 2.0f * juce::MathConstants<float>::pi * rate / currentSampleRate;
    const int delayBufferSize = chorusDelayBuffer.getNumSamples();
    const int chorusChannels = std::min(buffer.getNumChannels(), chorusDelayBuffer.getNumChannels());
    
    for (int i = 0; i < numSamples; ++i)
    {
        const float mod = std::sin(chorusPhase);
        const float delayMs = 7.0f + depth * 5.0f * (mod + 1.0f) * 0.5f; // 7-12ms
        const float delaySamples = std::clamp(
            delayMs * currentSampleRate * 0.001f, 
            1.0f, 
            static_cast<float>(delayBufferSize - 2));
        
        float readPosition = static_cast<float>(chorusWritePosition) - delaySamples;
        while (readPosition < 0.0f)
            readPosition += static_cast<float>(delayBufferSize);
        
        const int indexA = static_cast<int>(readPosition) % delayBufferSize;
        const int indexB = (indexA + 1) % delayBufferSize;
        const float frac = readPosition - std::floor(readPosition);
        
        for (int channel = 0; channel < chorusChannels; ++channel)
        {
            auto* output = buffer.getWritePointer(channel, startSample + i);
            auto* delay = chorusDelayBuffer.getWritePointer(channel);
            
            // Store current sample
            delay[chorusWritePosition] = *output;
            
            // Read delayed sample with interpolation
            const float delayed = delay[indexA] + frac * (delay[indexB] - delay[indexA]);
            
            // Mix dry and wet
            *output = *output * dry + delayed * mix;
        }
        
        chorusWritePosition = (chorusWritePosition + 1) % delayBufferSize;
        chorusPhase += phaseIncrement;
        if (chorusPhase >= juce::MathConstants<float>::twoPi)
            chorusPhase -= juce::MathConstants<float>::twoPi;
    }
}

void DspEngine::processReverb(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float roomSize = std::clamp(params.reverbRoomSize.load(), 0.0f, 1.0f);
    const float damping = std::clamp(params.reverbDamping.load(), 0.0f, 1.0f);
    const float wetMix = std::clamp(params.reverbMix.load(), 0.0f, 1.0f);
    const float dryMix = 1.0f - wetMix;
    
    // Update reverb parameters
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = roomSize;
    reverbParams.damping = damping;
    reverbParams.wetLevel = wetMix;
    reverbParams.dryLevel = dryMix;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);
    
    // Process based on number of channels
    if (buffer.getNumChannels() >= 2)
    {
        reverb.processStereo(
            buffer.getWritePointer(0, startSample),
            buffer.getWritePointer(1, startSample),
            numSamples);
    }
    else
    {
        reverb.processMono(buffer.getWritePointer(0, startSample), numSamples);
    }
}

void DspEngine::processLimiter(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    const float thresholdDb = params.limiterThresholdDb.load();
    const float threshold = juce::Decibels::decibelsToGain(thresholdDb);
    const float releaseMs = params.limiterReleaseMs.load();
    
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * currentSampleRate));
    
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* samples = buffer.getWritePointer(ch, startSample);
        float& envelope = limiterEnvelopes[ch];
        
        for (int i = 0; i < numSamples; ++i)
        {
            const float magnitude = std::abs(samples[i]);
            
            // Update envelope (only release, instant attack)
            if (magnitude > envelope)
                envelope = magnitude;
            else
                envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * magnitude;
            
            // Apply limiting
            if (envelope > threshold && envelope > 0.0f)
            {
                const float gain = threshold / envelope;
                samples[i] *= gain;
            }
        }
    }
}
