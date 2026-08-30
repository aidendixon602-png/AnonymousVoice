cmake_minimum_required(VERSION 3.22)

project(AnonymousVoice VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
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
)

target_include_directories(AnonymousVoice
    PRIVATE
        Source
)

target_compile_definitions(AnonymousVoice
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

if (MSVC)
    # Low-memory Windows build hotfixes:
    # - one compiler worker only
    # - compact legacy diagnostics instead of verbose caret output
    # - embedded debug information avoids a separate compiler PDB writer
    # - suppress warnings on this temporary baseline to minimise MSBuild/SARIF traffic
    target_compile_options(AnonymousVoice PRIVATE
        /MP1
        /diagnostics:classic
        /W0
        $<$<CONFIG:Debug>:/Z7>
    )
endif()

target_link_libraries(AnonymousVoice
    PRIVATE
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
)
