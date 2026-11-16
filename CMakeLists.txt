cmake_minimum_required(VERSION 3.22)
project(MixDownStudy VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 29396c22c93392d6738e021b83196283d6e4d850)
FetchContent_MakeAvailable(JUCE)
set(PANEL_ARTWORK "${CMAKE_CURRENT_SOURCE_DIR}/assets/exec-a5bf11cb-f48d-464e-862a-d64669108d61.png")
file(SHA256 "${PANEL_ARTWORK}" PANEL_HASH)
if(NOT PANEL_HASH STREQUAL "915abf671b0a41b716282ad2fd4eeed8b47011dcdb38f0c00430b02cea562a86")
    message(FATAL_ERROR "The original panel artwork is immutable. Restore the original bytes.")
endif()
juce_add_binary_data(MixDownArtwork NAMESPACE MixDownArtwork HEADER_NAME Artwork.h
    SOURCES "${PANEL_ARTWORK}")
juce_add_plugin(MixDownStudy COMPANY_NAME AulaMix PLUGIN_MANUFACTURER_CODE Amix
    PLUGIN_CODE Mxst FORMATS VST3 Standalone PRODUCT_NAME "MixDown Study"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(MixDownStudy PRIVATE Source/Plugin.cpp)
target_compile_definitions(MixDownStudy PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
set_property(SOURCE Source/Plugin.cpp APPEND PROPERTY COMPILE_OPTIONS
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
target_link_libraries(MixDownStudy PRIVATE MixDownArtwork juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)
enable_testing()
juce_add_console_app(MixDownSmoke PRODUCT_NAME "MixDown Smoke")
target_sources(MixDownSmoke PRIVATE Tests/AudioSmoke.cpp)
target_include_directories(MixDownSmoke PRIVATE "${juce_SOURCE_DIR}/modules")
target_compile_definitions(MixDownSmoke PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(MixDownSmoke PRIVATE MixDownStudy)
add_test(NAME AudioSmoke COMMAND MixDownSmoke)
# This test runs JUCEApplication and must use the same Windows GUI entry point
# as the linked JUCE event-loop code, rather than the console main(argc, argv).
juce_add_gui_app(MixDownUiSmoke PRODUCT_NAME "MixDown UI Smoke")
target_sources(MixDownUiSmoke PRIVATE Tests/UiSmoke.cpp)
target_include_directories(MixDownUiSmoke PRIVATE "${juce_SOURCE_DIR}/modules")
target_compile_definitions(MixDownUiSmoke PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(MixDownUiSmoke PRIVATE MixDownStudy)
add_test(NAME UiSmoke COMMAND MixDownUiSmoke "${CMAKE_CURRENT_BINARY_DIR}/ui-snapshots")
set_tests_properties(UiSmoke PROPERTIES TIMEOUT 30)
