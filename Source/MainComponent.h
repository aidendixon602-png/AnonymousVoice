#pragma once

#include <JuceHeader.h>
#include "Audio/DspEngine.h"
#include "Audio/VirtualMicOutput.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate);
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill);
    void releaseResources();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    class VoiceCard final : public juce::Component
    {
    public:
        VoiceCard(juce::String nameToUse, juce::String iconToUse, bool selectedToUse = false);
        void paint(juce::Graphics& g) override;
        void mouseUp(const juce::MouseEvent&) override;
        std::function<void()> onClick;

    private:
        juce::String name;
        juce::String icon;
        bool selected = false;
    };

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

    private:
        juce::String effectName;
        juce::String primaryLabel;
        juce::String suffix;
        juce::ToggleButton enabled;
        juce::Slider amount;
    };

    void timerCallback() override;
    void configureNavButton(juce::TextButton& button, bool selected);
    void configureHorizontalSlider(juce::Slider& slider, double min, double max, double value, double interval);
    void drawPanel(juce::Graphics& g, juce::Rectangle<float> area, float radius = 16.0f) const;
    void syncDspParameters();

    juce::Label brandLabel, brandSubLabel, userLabel;
    juce::TextButton voiceboxButton { "Voicebox" }, soundboardButton { "Soundboard" },
                     voiceLabButton { "Voice Lab" }, settingsButton { "Settings" }, keybindsButton { "Keybinds" };

    juce::Label voiceboxTitle;
    juce::TextEditor searchEditor;
    juce::ComboBox categoryBox;
    std::array<std::unique_ptr<VoiceCard>, 10> voiceCards;

    juce::Label presetTitle, presetName, presetDescription, pitchLabel, formantLabel, mixLabel;
    juce::Slider pitchSlider, formantSlider, mixSlider;
    juce::TextButton testVoiceButton { "TEST VOICE" };

    juce::Label editorTitle;
    std::array<std::unique_ptr<EffectCard>, 9> effectCards;

    juce::Label inputLabel, monitorLabel, outputLabel;
    juce::ComboBox inputBox, monitorBox, outputBox;
    juce::ToggleButton monitorToggle { "Live Monitor" };
    juce::Slider monitorVolume;

    juce::Label statusLabel, cpuLabel, latencyLabel, versionLabel;

    juce::Rectangle<int> sidebarArea, voiceboxArea, presetArea, editorArea, ioArea;

    DspEngine dspEngine;
    VirtualMicOutput virtualMicOutput;

    std::atomic<float> masterGain { 1.0f };
    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<double> estimatedLatencyMs { 0.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
