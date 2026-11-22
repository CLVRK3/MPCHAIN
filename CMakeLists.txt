cmake_minimum_required(VERSION 3.22)

project(MPCHAIN VERSION 1.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal macOS architectures")
    set(MPCHAIN_FORMATS VST3 AU Standalone)
else()
    set(MPCHAIN_FORMATS VST3 Standalone)
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MPCHAIN
    COMPANY_NAME                "MIXCHAIN"
    PRODUCT_NAME                "MPCHAIN"
    BUNDLE_ID                   "com.mixchain.mpchain"
    PLUGIN_MANUFACTURER_CODE    Mxcn
    PLUGIN_CODE                 MZpz
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             "Fx" "Mastering" "Dynamics" "EQ"
    AU_MAIN_TYPE                kAudioUnitType_Effect
    FORMATS                     ${MPCHAIN_FORMATS}
)

target_sources(MPCHAIN PRIVATE
    src/MPChainDSP.h
    src/PluginProcessor.h
    src/PluginProcessor.cpp
    src/PluginEditor.h
    src/PluginEditor.cpp
)

target_compile_definitions(MPCHAIN PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MPCHAIN PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
