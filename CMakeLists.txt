cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build universal macOS binary")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
endif()

project(Grainveil VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# Override on the command line if needed, e.g. -DGRAINVEIL_FORMATS=VST3
if(APPLE)
    set(GRAINVEIL_DEFAULT_FORMATS VST3 AU)
else()
    set(GRAINVEIL_DEFAULT_FORMATS VST3)
endif()
set(GRAINVEIL_FORMATS "${GRAINVEIL_DEFAULT_FORMATS}" CACHE STRING "Plugin formats to build (VST3, AU)")

juce_add_plugin(Grainveil
    COMPANY_NAME                "Grainveil Audio"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    GvLb
    PLUGIN_CODE                 Grvl
    FORMATS                     ${GRAINVEIL_FORMATS}
    PRODUCT_NAME                "Grainveil"
)

target_sources(Grainveil
    PRIVATE
        source/PluginProcessor.cpp
        source/PluginEditor.cpp
        source/ui/GrainveilLookAndFeel.cpp
        source/ui/OledDisplay.cpp
)

target_include_directories(Grainveil
    PRIVATE
        source
)

target_compile_definitions(Grainveil
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Grainveil
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
