cmake_minimum_required(VERSION 3.22)

project(AnonymousVoice VERSION 0.1.0 LANGUAGES CXX)

add_executable(AnonymousVoice WIN32
    Source/Main.cpp
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
)
