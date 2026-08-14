cmake_minimum_required(VERSION 4.0)
set(CMAKE_CXX_STANDARD 20)
set(PROJECT_NAME "Sines")
project("${PROJECT_NAME}" VERSION 0.0.1 LANGUAGES C CXX)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(FORMATS VST3 AU Standalone)
else ()
    set(FORMATS VST3 LV2 Standalone)
endif ()

include(FetchContent)
FetchContent_Declare(
        JUCE
        GIT_REPOSITORY
        https://github.com/juce-framework/JUCE.git
        GIT_TAG e18f7f506c0b96f2c738a0bcd7fe6467a5005ad8)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin("${PROJECT_NAME}"

        COMPANY_NAME "PW"
        BUNDLE_ID "PWSines"

        # On MacOS, plugin is copied to ~/Users/yourname/Library/Audio/Plug-Ins/
        COPY_PLUGIN_AFTER_BUILD TRUE

        # A four-character plugin id
        # First character MUST be uppercase for AU formats
        PLUGIN_MANUFACTURER_CODE "Psin"

        # A unique four-character plugin id
        # Note: this must have at least one upper-case character
        PLUGIN_CODE "Psin"
        FORMATS "${FORMATS}"


        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE

        MICROPHONE_PERMISSION_ENABLED FALSE

        HARDENED_RUNTIME_ENABLED TRUE
        HARDENED_RUNTIME_OPTIONS com.apple.security.device.audio-input

        PRODUCT_NAME "Sines"

)

juce_generate_juce_header("${PROJECT_NAME}")

juce_add_binary_data("${PROJECT_NAME}Data"
        SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Resources/scales.xml")

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h")
target_sources("${PROJECT_NAME}" PUBLIC "${SourceFiles}")

target_compile_definitions("${PROJECT_NAME}"
        PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(
        "${PROJECT_NAME}"
        PUBLIC
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_cryptography
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_osc

        "${PROJECT_NAME}Data"

        juce::juce_recommended_config_flags
)