#pragma once

#include <JuceHeader.h>
#include "Audio/LightweightAudioAppComponent.h"
#include "Audio/DspEngine.h"
#include "Audio/VirtualMicOutput.h"

/**
 * Main application component.
 * 
 * This is the root component that contains all UI elements and manages
 * the audio processing pipeline.
 */
class MainComponent final : public LightweightAudioAppComponent,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    // Audio callbacks
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    // UI callbacks
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    //=======================================================================
    // UI Components
    //=======================================================================
    
    // Branding
    juce::Label brandLabel, brandSubLabel, userLabel;
    
    // Navigation buttons
    juce::TextButton voiceboxButton { "Voicebox" };
    juce::TextButton soundboardButton { "Soundboard" };
    juce::TextButton voiceLabButton { "Voice Lab" };
    juce::TextButton settingsButton { "Settings" };
    juce::TextButton keybindsButton { "Keybinds" };
    
    // Voicebox section
    juce::Label voiceboxTitle;
    juce::TextEditor searchEditor;
    juce::ComboBox categoryBox;
    
    class VoiceCard final : public juce::Component
    {
    public:
        VoiceCard(juce::String nameToUse, juce::String iconToUse, bool selectedToUse = false);
        void paint(juce::Graphics& g) override;
        void mouseUp(const juce::MouseEvent&) override;
        
        std::function<void()> onClick;
        void setSelected(bool shouldBeSelected) { selected = shouldBeSelected; repaint(); }
        bool isSelected() const noexcept { return selected; }

    private:
        juce::String name;
        juce::String icon;
        bool selected = false;
    };
    
    std::array<std::unique_ptr<VoiceCard>, 10> voiceCards;
    
    // Preset details section
    juce::Label presetTitle, presetName, presetDescription;
    juce::Label pitchLabel, formantLabel, mixLabel;
    juce::Slider pitchSlider, formantSlider, mixSlider;
    juce::TextButton testVoiceButton { "TEST VOICE" };
    
    // Voice editor section
    juce::Label editorTitle;
    
    class EffectCard final : public juce::Component
    {
    public:
        EffectCard(juce::String nameToUse,
                   juce::String primaryLabelToUse,
                   double minValue,
                   double maxValue,
                   double defaultValue,
                   juce::String suffixToUse);

        void paint(juce::Graphics& g) override;
        void resized() override;
        juce::Slider& slider() noexcept { return amount; }
        juce::ToggleButton& toggle() noexcept { return enabled; }
        void setName(const juce::String& newName) { effectName = newName; repaint(); }

    private:
        juce::String effectName;
        juce::String primaryLabel;
        juce::String suffix;
        juce::ToggleButton enabled;
        juce::Slider amount;
    };
    
    std::array<std::unique_ptr<EffectCard>, 9> effectCards;
    
    // I/O section
    juce::Label inputLabel, monitorLabel, outputLabel;
    juce::ComboBox inputBox, monitorBox, outputBox;
    juce::ToggleButton monitorToggle { "Live Monitor" };
    juce::Slider monitorVolume;
    
    // Status section
    juce::Label statusLabel, cpuLabel, latencyLabel, versionLabel;
    
    //=======================================================================
    // Audio Processing
    //=======================================================================
    
    DspEngine dspEngine;
    VirtualMicOutput virtualMicOutput;
    
    // Thread-safe parameters (accessed from both UI and audio threads)
    std::atomic<float> masterGain { 1.0f };
    std::atomic<bool> monitorEnabled { true };
    std::atomic<float> monitorVolumeValue { 0.72f };
    
    // Metering (updated from audio thread, read from UI thread)
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<double> estimatedLatencyMs { 0.0 };
    
    //=======================================================================
    // Layout Areas
    //=======================================================================
    
    juce::Rectangle<int> sidebarArea, voiceboxArea, presetArea, editorArea, ioArea;
    
    //=======================================================================
    // Private Methods
    //=======================================================================
    
    void timerCallback() override;
    void configureNavButton(juce::TextButton& button, bool selected);
    void configureHorizontalSlider(juce::Slider& slider, double min, double max, double value, double interval);
    void drawPanel(juce::Graphics& g, juce::Rectangle<float> area, float radius = 16.0f) const;
    void syncDspParameters();
    void updateDeviceList();
    void loadPreset(int presetIndex);
    void selectVoiceCard(int index);
    
    //=======================================================================
    // Color Scheme
    //=======================================================================
    
    namespace Colors
    {
        constexpr auto background = juce::Colour::fromRGB(5, 8, 14);
        constexpr auto panel = juce::Colour::fromRGB(10, 15, 24);
        constexpr auto panelRaised = juce::Colour::fromRGB(15, 21, 33);
        constexpr auto border = juce::Colour::fromRGB(37, 45, 63);
        constexpr auto text = juce::Colour::fromRGB(244, 246, 252);
        constexpr auto muted = juce::Colour::fromRGB(147, 157, 178);
        constexpr auto purple = juce::Colour::fromRGB(161, 72, 255);
        constexpr auto teal = juce::Colour::fromRGB(40, 209, 204);
        constexpr auto green = juce::Colour::fromRGB(45, 214, 119);
        constexpr auto orange = juce::Colour::fromRGB(255, 140, 0);
        constexpr auto red = juce::Colour::fromRGB(255, 69, 58);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
