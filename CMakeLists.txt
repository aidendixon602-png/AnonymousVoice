cmake_minimum_required(VERSION 3.22)

project(AnonymousVoice VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("../JUCE" JUCE)

juce_add_gui_app(AnonymousVoice
    PRODUCT_NAME "Anonymous Voice"
    COMPANY_NAME "Anonymous Voice"
    BUNDLE_ID "com.anonymousvoice.app"
)

juce_generate_juce_header(AnonymousVoice)

target_sources(AnonymousVoice
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
        Source/Audio/LightweightAudioAppComponent.cpp
        Source/Audio/LightweightAudioAppComponent.h
        Source/Audio/DspEngine.cpp
        Source/Audio/DspEngine.h
        Source/Audio/VirtualMicOutput.cpp
        Source/Audio/VirtualMicOutput.h
)

target_include_directories(AnonymousVoice
    PRIVATE
        Source
        Source/Audio
)

target_compile_definitions(AnonymousVoice
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

if (MSVC)
    target_compile_options(AnonymousVoice PRIVATE /MP1)
endif()

target_link_libraries(AnonymousVoice
    PRIVATE
        juce::juce_audio_devices
        juce::juce_audio_basics
        juce::juce_gui_extra
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
