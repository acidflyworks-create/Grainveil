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

# UI assets are embedded, not loaded from the user's system. Keep the budget
# explicit so an accidentally oversized future image cannot enter a release.
set(GRAINVEIL_UI_ASSETS
    assets/fonts/Lato-Regular.ttf
    assets/fonts/Lato-SemiBold.ttf
)
# Count every file under assets, including files that are not yet embedded.
file(GLOB_RECURSE GRAINVEIL_ALL_ASSETS CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
set(GRAINVEIL_UI_ASSET_BYTES 0)
foreach(asset IN LISTS GRAINVEIL_ALL_ASSETS)
    if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${asset}")
        file(SIZE "${CMAKE_CURRENT_SOURCE_DIR}/${asset}" asset_bytes)
        math(EXPR GRAINVEIL_UI_ASSET_BYTES "${GRAINVEIL_UI_ASSET_BYTES} + ${asset_bytes}")
        message(STATUS "Grainveil UI asset: ${asset} (${asset_bytes} bytes)")
    endif()
endforeach()
set(GRAINVEIL_UI_ASSET_LIMIT 60000000) # 60 MB (decimal)
message(STATUS "Grainveil total UI assets (including non-embedded files): ${GRAINVEIL_UI_ASSET_BYTES} / ${GRAINVEIL_UI_ASSET_LIMIT} bytes")
if(GRAINVEIL_UI_ASSET_BYTES GREATER GRAINVEIL_UI_ASSET_LIMIT)
    message(FATAL_ERROR "Grainveil UI assets exceed the 60 MB budget (${GRAINVEIL_UI_ASSET_BYTES} bytes).")
endif()
juce_add_binary_data(GrainveilAssets SOURCES ${GRAINVEIL_UI_ASSETS})

option(GRAINVEIL_BUILD_UI_TESTS "Build the UI layout and text test harness" OFF)
if(GRAINVEIL_BUILD_UI_TESTS)
    enable_testing()
    juce_add_console_app(GrainveilUiTests PRODUCT_NAME "Grainveil UI Tests")
    target_sources(GrainveilUiTests PRIVATE tests/UiLayoutTests.cpp source/ui/UiTypography.cpp)
    target_include_directories(GrainveilUiTests PRIVATE source)
    target_compile_definitions(GrainveilUiTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(GrainveilUiTests PRIVATE GrainveilAssets juce::juce_gui_basics)
    add_test(NAME GrainveilUiLayout COMMAND GrainveilUiTests)
endif()

# Override on the command line if needed, e.g. -DGRAINVEIL_FORMATS=VST3
if(APPLE)
    set(GRAINVEIL_DEFAULT_FORMATS VST3 AU)
else()
    set(GRAINVEIL_DEFAULT_FORMATS VST3)
endif()
set(GRAINVEIL_FORMATS "${GRAINVEIL_DEFAULT_FORMATS}" CACHE STRING "Plugin formats to build (VST3, AU)")

juce_add_plugin(Grainveil
    COMPANY_NAME                "Grainveil Audio"
    BUNDLE_ID                   "com.grainveilaudio.grainveil"
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
        source/PresetManager.cpp
        source/PluginEditor.cpp
        source/ui/GrainveilLookAndFeel.cpp
        source/ui/OledDisplay.cpp
        source/ui/PresetBar.cpp
        source/ui/UiTypography.cpp
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
        GrainveilAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(GRAINVEIL_BUILD_UI_TESTS)
    juce_add_console_app(GrainveilUiPreview PRODUCT_NAME "Grainveil UI Preview")
    target_sources(GrainveilUiPreview PRIVATE tests/UiPreview.cpp)
    target_include_directories(GrainveilUiPreview PRIVATE source)
    target_link_libraries(GrainveilUiPreview PRIVATE Grainveil juce::juce_audio_utils juce::juce_dsp)
    add_test(NAME GrainveilUiPreview COMMAND GrainveilUiPreview "${CMAKE_BINARY_DIR}/ui-previews")

    juce_add_console_app(GrainveilPresetTests PRODUCT_NAME "Grainveil Preset Tests")
    target_sources(GrainveilPresetTests PRIVATE tests/PresetTests.cpp)
    target_include_directories(GrainveilPresetTests PRIVATE source)
    target_link_libraries(GrainveilPresetTests PRIVATE Grainveil juce::juce_audio_utils juce::juce_dsp)
    add_test(NAME GrainveilPresetsAndTelemetry COMMAND GrainveilPresetTests)
endif()
