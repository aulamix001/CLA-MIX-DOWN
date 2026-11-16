cmake_minimum_required(VERSION 3.22)
project(MixDownStudy VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 29396c22c93392d6738e021b83196283d6e4d850)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(MixDownStudy COMPANY_NAME AulaMix PLUGIN_MANUFACTURER_CODE Amix
    PLUGIN_CODE Mxst FORMATS VST3 Standalone PRODUCT_NAME "MixDown Study"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(MixDownStudy PRIVATE Source/Plugin.cpp)
target_compile_definitions(MixDownStudy PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MixDownStudy PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)
enable_testing()
juce_add_console_app(MixDownSmoke PRODUCT_NAME "MixDown Smoke")
target_sources(MixDownSmoke PRIVATE Tests/AudioSmoke.cpp)
target_include_directories(MixDownSmoke PRIVATE "${juce_SOURCE_DIR}/modules")
target_compile_definitions(MixDownSmoke PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(MixDownSmoke PRIVATE MixDownStudy)
add_test(NAME AudioSmoke COMMAND MixDownSmoke)
