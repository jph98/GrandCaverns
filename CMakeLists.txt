cmake_minimum_required(VERSION 3.22)
project(GrandCaverns VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(GrandCavernsRenderTest
    tests/render_test.cpp
    Source/DSP/GrandCavernsEngine.cpp)
target_include_directories(GrandCavernsRenderTest PRIVATE Source)
target_compile_options(GrandCavernsRenderTest PRIVATE -Wall -Wextra)

include(FetchContent)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GrandCaverns
    COMPANY_NAME "GrandCaverns"
    BUNDLE_ID com.grandcaverns.grandcaverns
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE GrCv
    PLUGIN_CODE GdCv
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "GrandCaverns"
    VST3_CATEGORIES "Fx Reverb Delay"
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(GrandCaverns PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/PresetLibrary.cpp
    Source/DSP/GrandCavernsEngine.cpp)

target_compile_definitions(GrandCaverns PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GrandCaverns
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

target_compile_features(GrandCaverns PUBLIC cxx_std_20)
