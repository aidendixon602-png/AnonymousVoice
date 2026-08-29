#include <JuceHeader.h>
#include "MainComponent.h"

class AnonymousVoiceApplication final
    : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override
    {
        return "Anonymous Voice";
    }

    const juce::String getApplicationVersion() override
    {
        return "0.1.0";
    }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>(
            getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : DocumentWindow(
                  name,
                  juce::Colours::black,
                  DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new MainComponent(), true);

            centreWithSize(900, 600);
            setResizable(true, true);
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()
                ->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(AnonymousVoiceApplication)