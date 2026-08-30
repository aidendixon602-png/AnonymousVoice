#include <JuceHeader.h>
#include "MainComponent.h"

#if JUCE_WINDOWS
#include <windows.h>
#endif

namespace
{
    void startupCheckpoint(const wchar_t* message)
    {
       #if JUCE_WINDOWS
        ::MessageBoxW(nullptr, message, L"AnonymousVoice startup diagnostic", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
       #else
        juce::ignoreUnused(message);
       #endif
    }
}

class AnonymousVoiceApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override       { return "Anonymous Voice"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String&) override
    {
        startupCheckpoint(L"Checkpoint 1: JUCE application initialise() entered.");
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
        startupCheckpoint(L"Checkpoint 5: Main window construction completed.");
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(const juce::String& name)
            : juce::DocumentWindow(name,
                                   juce::Colour::fromRGB(7, 11, 18),
                                   juce::DocumentWindow::allButtons)
        {
            startupCheckpoint(L"Checkpoint 2: DocumentWindow constructed. About to construct MainComponent.");

            setUsingNativeTitleBar(true);
            setResizable(true, true);
            setResizeLimits(1200, 720, 2400, 1500);

            auto* content = new MainComponent();
            startupCheckpoint(L"Checkpoint 3: MainComponent constructor completed.");

            setContentOwned(content, true);
            centreWithSize(1600, 900);
            setVisible(true);

            startupCheckpoint(L"Checkpoint 4: Window made visible.");
        }

        void closeButtonPressed() override
        {
            if (auto* app = juce::JUCEApplication::getInstance())
                app->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(AnonymousVoiceApplication)
