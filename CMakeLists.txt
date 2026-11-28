# SOLARIUM-23A: the plugin (VST3, AU on the Mac) and the standalone app, with JUCE 8.
#   cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --config Release
# JUCE comes from GitHub (or -DS23_JUCE_DIR=/path/to/JUCE).
cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    endif()
endif()
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(SOLARIS23 VERSION 1.3.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(S23_JUCE_DIR "" CACHE PATH "a JUCE checkout (empty: from GitHub)")
if(S23_JUCE_DIR)
    add_subdirectory(${S23_JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()

set(S23_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND S23_FORMATS AU)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND S23_FORMATS LV2)
endif()

juce_add_plugin(Solaris23
    PRODUCT_NAME "SOLARIUM-23A"
    COMPANY_NAME "swtdr34mz"
    BUNDLE_ID "com.swtdr34mz.solaris23"
    LV2URI "https://github.com/swtdr34mz-afk/SOLARIUM-23A"
    PLUGIN_MANUFACTURER_CODE Swtd
    PLUGIN_CODE So23                     # (kept from SOLARIS-23, its name before v1.3: hosts find their projects)
    FORMATS ${S23_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    MICROPHONE_PERMISSION_ENABLED FALSE)

juce_add_binary_data(S23Data SOURCES
    fonts/FiraSans-SemiBold.ttf
    fonts/FiraSans-Bold.ttf
    fonts/FiraSans-ExtraBold.ttf
    fonts/AnonymousPro-Bold.ttf
    res/swtdr34mz-tag.png)

target_sources(Solaris23 PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/ui/Look.cpp
    src/ui/Controls.cpp
    src/ui/Sections.cpp
    dsp/engine.cpp
    dsp/fx.cpp
    dsp/params.cpp
    dsp/presets.cpp)

target_include_directories(Solaris23 PRIVATE dsp src src/ui)

target_compile_definitions(Solaris23 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(Solaris23
    PRIVATE
        S23Data
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
