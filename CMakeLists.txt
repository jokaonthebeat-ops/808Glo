cmake_minimum_required(VERSION 3.22)

project(808GloPro VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(GLO_BUILD_PLUGIN "Build the JUCE plug-in and standalone app" ON)
option(GLO_BUILD_TESTS "Build the dependency-free DSP and preset tests" ON)
option(GLO_BUILD_TOOLS "Build dependency-free developer utilities" ON)
option(GLO_FETCH_JUCE "Fetch JUCE when no local JUCE installation is found" OFF)

add_library(808GloCore INTERFACE)
target_include_directories(808GloCore INTERFACE Source)
target_compile_features(808GloCore INTERFACE cxx_std_20)

if(GLO_BUILD_TESTS)
    include(CTest)
    enable_testing()

    add_executable(808GloCoreTests
        Tests/CoreDSPTests.cpp
    )
    target_link_libraries(808GloCoreTests PRIVATE 808GloCore)
    target_compile_definitions(808GloCoreTests PRIVATE GLO_TESTING=1)

    if(MSVC)
        target_compile_options(808GloCoreTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(808GloCoreTests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    endif()

    add_test(NAME 808GloCoreTests COMMAND 808GloCoreTests)

    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME 808GloPresetValidation
            COMMAND Python3::Interpreter
                    "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_presets.py"
                    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/FactoryPresets.json"
        )
    endif()
endif()

if(GLO_BUILD_TOOLS)
    add_executable(808GloRenderPreview Tests/RenderCorePreview.cpp)
    target_link_libraries(808GloRenderPreview PRIVATE 808GloCore)
endif()

if(NOT GLO_BUILD_PLUGIN)
    return()
endif()

if(DEFINED JUCE_DIR AND EXISTS "${JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    find_package(JUCE 9 CONFIG QUIET)
endif()

if(NOT TARGET juce::juce_audio_utils AND GLO_FETCH_JUCE)
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

if(NOT TARGET juce::juce_audio_utils)
    message(FATAL_ERROR
        "JUCE 9 was not found. Pass -DJUCE_DIR=/absolute/path/to/JUCE, install JUCE's "
        "CMake package, use -DGLO_FETCH_JUCE=ON, or configure core tests only with "
        "-DGLO_BUILD_PLUGIN=OFF.")
endif()

set(GLO_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND GLO_FORMATS AU)
endif()

juce_add_binary_data(808GloAssets
    HEADER_NAME 808GloBinaryData.h
    NAMESPACE EightOhEightAssets
    SOURCES
        Resources/808GloPro_Logo.svg
        Resources/FactoryPresets.json
)

juce_add_plugin(808GloPro
    COMPANY_NAME "Diamond Loopz"
    BUNDLE_ID "com.diamondloopz.808glopro"
    PLUGIN_MANUFACTURER_CODE Dlpz
    PLUGIN_CODE Eglo
    PRODUCT_NAME "808Glo Pro"
    DESCRIPTION "Elite synthesized 808 instrument"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/AppIcon_1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/AppIcon_512.png"
    VERSION ${PROJECT_VERSION}
    FORMATS ${GLO_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    HARDENED_RUNTIME_ENABLED TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
)

juce_generate_juce_header(808GloPro)

target_sources(808GloPro PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/ParameterLayout.cpp
    Source/ParameterLayout.h
    Source/PresetManager.cpp
    Source/PresetManager.h
    Source/DSP/Envelope.h
    Source/DSP/Math.h
    Source/DSP/OnePoleFilter.h
    Source/DSP/SynthEngine.h
    Source/DSP/Voice.h
    Source/UI/GloLookAndFeel.cpp
    Source/UI/GloLookAndFeel.h
    Source/UI/GloComponents.cpp
    Source/UI/GloComponents.h
)

target_include_directories(808GloPro PRIVATE Source)

target_compile_definitions(808GloPro PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0
)

if(MSVC)
    target_compile_options(808GloPro PRIVATE /W4 /permissive-)
else()
    target_compile_options(808GloPro PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(808GloPro
    PRIVATE
        808GloAssets
        808GloCore
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(GLO_BUILD_TOOLS)
    add_executable(808GloRenderAppIcon
        Tests/RenderAppIcon.cpp
    )
    target_include_directories(808GloRenderAppIcon PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "$<TARGET_PROPERTY:808GloPro,JUCE_GENERATED_SOURCES_DIRECTORY>"
        "$<TARGET_PROPERTY:808GloPro,INCLUDE_DIRECTORIES>"
    )
    target_link_libraries(808GloRenderAppIcon PRIVATE 808GloPro)
    add_dependencies(808GloRenderAppIcon 808GloPro)

    if(MSVC)
        target_compile_options(808GloRenderAppIcon PRIVATE /W4 /permissive-)
    else()
        target_compile_options(808GloRenderAppIcon PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(808GloPresetEval
        Tests/PresetEval.cpp
    )
    target_include_directories(808GloPresetEval PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "$<TARGET_PROPERTY:808GloPro,JUCE_GENERATED_SOURCES_DIRECTORY>"
        "$<TARGET_PROPERTY:808GloPro,INCLUDE_DIRECTORIES>"
    )
    target_link_libraries(808GloPresetEval PRIVATE 808GloPro)
    add_dependencies(808GloPresetEval 808GloPro)

    if(MSVC)
        target_compile_options(808GloPresetEval PRIVATE /W4 /permissive-)
    else()
        target_compile_options(808GloPresetEval PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(808GloPresetProbe
        Tests/PresetProbe.cpp
    )
    target_include_directories(808GloPresetProbe PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "$<TARGET_PROPERTY:808GloPro,JUCE_GENERATED_SOURCES_DIRECTORY>"
        "$<TARGET_PROPERTY:808GloPro,INCLUDE_DIRECTORIES>"
    )
    target_link_libraries(808GloPresetProbe PRIVATE 808GloPro)
    add_dependencies(808GloPresetProbe 808GloPro)

    if(MSVC)
        target_compile_options(808GloPresetProbe PRIVATE /W4 /permissive-)
    else()
        target_compile_options(808GloPresetProbe PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(808GloRenderDemo
        Tests/RenderDemoVideo.cpp
    )
    target_include_directories(808GloRenderDemo PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "$<TARGET_PROPERTY:808GloPro,JUCE_GENERATED_SOURCES_DIRECTORY>"
        "$<TARGET_PROPERTY:808GloPro,INCLUDE_DIRECTORIES>"
    )
    target_link_libraries(808GloRenderDemo PRIVATE 808GloPro)
    add_dependencies(808GloRenderDemo 808GloPro)

    if(MSVC)
        target_compile_options(808GloRenderDemo PRIVATE /W4 /permissive-)
    else()
        target_compile_options(808GloRenderDemo PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_executable(808GloRenderUIPreview
        Tests/RenderUIPreview.cpp
    )
    target_include_directories(808GloRenderUIPreview PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "$<TARGET_PROPERTY:808GloPro,JUCE_GENERATED_SOURCES_DIRECTORY>"
        "$<TARGET_PROPERTY:808GloPro,INCLUDE_DIRECTORIES>"
    )
    target_link_libraries(808GloRenderUIPreview PRIVATE 808GloPro)
    add_dependencies(808GloRenderUIPreview 808GloPro)

    if(MSVC)
        target_compile_options(808GloRenderUIPreview PRIVATE /W4 /permissive-)
    else()
        target_compile_options(808GloRenderUIPreview PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()

if(GLO_BUILD_TESTS)
    add_executable(808GloJuceIntegrationTests
        Tests/JuceIntegrationTests.cpp
    )
    target_include_directories(808GloJuceIntegrationTests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source"
        "$<TARGET_PROPERTY:808GloPro,JUCE_GENERATED_SOURCES_DIRECTORY>"
        "$<TARGET_PROPERTY:808GloPro,INCLUDE_DIRECTORIES>"
    )
    target_link_libraries(808GloJuceIntegrationTests PRIVATE 808GloPro)
    add_dependencies(808GloJuceIntegrationTests 808GloPro)

    if(MSVC)
        target_compile_options(808GloJuceIntegrationTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(808GloJuceIntegrationTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(NAME 808GloJuceIntegrationTests COMMAND 808GloJuceIntegrationTests)
endif()
