cmake_minimum_required(VERSION 3.22)

project(AnonymousVoice VERSION 0.2.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN)
    # Enable Windows audio APIs
    find_package(WindowsSDK REQUIRED)
endif()

# JUCE configuration - use FetchContent to download JUCE
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.8
    GIT_SHALLOW    TRUE
)

# Set JUCE options
set(JUCE_ENABLE_MODULE_STATIC_LIBRARY ON CACHE BOOL "Build JUCE as static library")
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "Don't build JUCE extras")

FetchContent_MakeAvailable(JUCE)

# Create the application
juce_add_gui_app(AnonymousVoice
    PRODUCT_NAME "Anonymous Voice"
    COMPANY_NAME "Anonymous Voice"
    BUNDLE_ID "com.anonymousvoice.app"
    VERSION ${PROJECT_VERSION}
    ICON_FILE "Assets/icon.png"
)

# Add source files
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

# Include directories
target_include_directories(AnonymousVoice
    PRIVATE
        Source
        Source/Audio
)

# Compiler definitions
target_compile_definitions(AnonymousVoice
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DSP_AVAILABLE=1
        JUCE_AUDIO_FORMATS_AVAILABLE=1
)

# Windows-specific audio settings
if(WIN32)
    target_compile_definitions(AnonymousVoice PRIVATE JUCE_WINDOWS=1)
    target_link_libraries(AnonymousVoice PRIVATE ole32 oleaut32 winmm)
    
    # Enable WASAPI for virtual microphone
    target_compile_definitions(AnonymousVoice PRIVATE JUCE_WASAPI=1)
endif()

# Link JUCE modules
target_link_libraries(AnonymousVoice
    PRIVATE
        juce::juce_audio_devices
        juce::juce_audio_basics
        juce::juce_audio_dsp
        juce::juce_audio_formats
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_data_structures
        juce::juce_events
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# Multi-threaded compilation for MSVC
if(MSVC)
    target_compile_options(AnonymousVoice PRIVATE /MP)
endif()

# Output directory configuration
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Install target (optional)
install(TARGETS AnonymousVoice
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
