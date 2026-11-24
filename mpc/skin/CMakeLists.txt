# The MPC skin's renderer: SOLARIS-23's own sections (src/ui/), drawn on the MPC's pages, every control cut out at
# every value it can take, with a manifest for make_skin.py; and a picture of the whole VST panel (--panel).
#   cmake -S mpc/skin -B build-skin -DS23_JUCE_DIR=/path/to/JUCE && cmake --build build-skin
#   build-skin/s23_skin_render mpc/build/skin-render
#   python3 mpc/skin/make_skin.py mpc/build/skin-render mpc/plugin/skins
# (Only when the panel changes: the result, mpc/plugin/skins/, is kept in the repository.)
cmake_minimum_required(VERSION 3.22)
project(S23SkinRender VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(S23_JUCE_DIR "" CACHE PATH "a JUCE checkout (empty: from GitHub)")
if(S23_JUCE_DIR)
    add_subdirectory(${S23_JUCE_DIR} juce)
else()
    include(FetchContent)
    FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.9 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)
endif()
set(R ${CMAKE_CURRENT_SOURCE_DIR}/../..)
juce_add_console_app(s23_skin_render PRODUCT_NAME "s23_skin_render")
juce_add_binary_data(S23SkinData SOURCES
    ${R}/fonts/FiraSans-SemiBold.ttf
    ${R}/fonts/FiraSans-Bold.ttf
    ${R}/fonts/FiraSans-ExtraBold.ttf
    ${R}/fonts/AnonymousPro-Bold.ttf
    ${R}/res/swtdr34mz-tag.png)
target_sources(s23_skin_render PRIVATE
    render_skin.cpp
    ${R}/src/PluginProcessor.cpp
    ${R}/src/PluginEditor.cpp
    ${R}/src/ui/Look.cpp
    ${R}/src/ui/Controls.cpp
    ${R}/src/ui/Sections.cpp
    ${R}/dsp/engine.cpp
    ${R}/dsp/fx.cpp
    ${R}/dsp/params.cpp
    ${R}/dsp/presets.cpp)
target_include_directories(s23_skin_render PRIVATE ${R}/dsp ${R}/src ${R}/src/ui)
target_compile_definitions(s23_skin_render PRIVATE
    JucePlugin_Name="SOLARIS-23" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0
    JucePlugin_IsMidiEffect=0 JucePlugin_EditorRequiresKeyboardFocus=0
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ALSA=0 JUCE_JACK=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0
    JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XSHM=0 JUCE_USE_XRENDER=0 JUCE_USE_XCURSOR=0
    JUCE_MODAL_LOOPS_PERMITTED=0)
target_link_libraries(s23_skin_render PRIVATE S23SkinData juce::juce_audio_utils juce::juce_recommended_config_flags)
