cmake_minimum_required(VERSION 3.22)

project(AnonymousVoice VERSION 0.4.1 LANGUAGES CXX)

add_executable(AnonymousVoice WIN32
    Source/Main.cpp
    Source/Audio/AudioRuntime.cpp
    Source/Audio/WasapiCapture.cpp
    Source/Audio/WasapiCapture.h
)

set_target_properties(AnonymousVoice PROPERTIES
    OUTPUT_NAME "Anonymous Voice"
)

target_compile_features(AnonymousVoice PRIVATE cxx_std_20)

target_compile_definitions(AnonymousVoice PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if (MSVC)
    target_compile_options(AnonymousVoice PRIVATE
        /MP1
        /W0
        /diagnostics:classic
        /Z7
        /EHsc
    )
endif()

target_link_libraries(AnonymousVoice PRIVATE
    user32
    gdi32
    dwmapi
    ole32
    uuid
)

add_custom_command(TARGET AnonymousVoice POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:AnonymousVoice>"
            "${CMAKE_SOURCE_DIR}/Anonymous Voice.exe"
    COMMENT "Copying Anonymous Voice.exe to the project root for easy launching"
)
