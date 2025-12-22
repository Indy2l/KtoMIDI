
cmake_minimum_required(VERSION 3.20)

# Prevent in-source builds (keep build artifacts out of root)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Please create a separate build directory and run CMake from there (e.g., mkdir build && cd build && cmake ..)")
endif()

project(KtoMIDI 
    VERSION 1.0.2
    DESCRIPTION "KtoMIDI"
    LANGUAGES CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "KtoMIDI is a Windows-only application. This platform is not supported.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Set a single generated output directory inside build/
set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

# Generate version.h from template into generated/
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${GENERATED_DIR}/version.h"
    @ONLY
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
endif()


# Set output directory for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
endif()

if(MSVC)
    add_compile_options(/W4 /WX- /permissive- /Zc:__cplusplus)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /Ob2 /DNDEBUG)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "$ENV{VCPKG_ROOT}/installed/x64-windows" CACHE STRING "vcpkg installed path" FORCE)
endif()


# Use CMake's find_package for dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
find_package(RtMidi REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CORE_SOURCES
    src/main.cpp
    src/MainWindow.cpp
)

set(KEYBOARD_SOURCES
    src/KeyHook.cpp
    src/KeyMapping.cpp
    src/InputMonitor.cpp
    src/MappingDialog.cpp
)

set(MIDI_SOURCES
    src/MidiEngine.cpp
)

set(SOURCES
    ${CORE_SOURCES}
    ${KEYBOARD_SOURCES}
    ${MIDI_SOURCES}
)

set(CORE_HEADERS
    src/MainWindow.h
)

set(KEYBOARD_HEADERS
    src/KeyHook.h
    src/KeyMapping.h
    src/InputMonitor.h
    src/MappingDialog.h
)

set(MIDI_HEADERS
    src/MidiEngine.h
)

set(HEADERS
    ${CORE_HEADERS}
    ${KEYBOARD_HEADERS}
    ${MIDI_HEADERS}
)

set(RESOURCES
    resources/KtoMIDI.qrc
)

if(WIN32)
    set(WIN32_RESOURCES
        resources/KtoMIDI.rc
    )
endif()

if(WIN32)
    add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES} ${WIN32_RESOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
endif()


set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME "KtoMIDI"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    WIN32_EXECUTABLE TRUE
)

# Always include the generated directory for headers
target_include_directories(${PROJECT_NAME} PRIVATE 
    src/
    "${GENERATED_DIR}"
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    user32
    kernel32
    winmm
    setupapi
    hid
    RtMidi::rtmidi
)

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set_target_properties(${PROJECT_NAME} PROPERTIES
            LINK_FLAGS "/SUBSYSTEM:WINDOWS /LTCG"
        )
    endif()
endif()

if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(RELEASE_DIR "${CMAKE_BINARY_DIR}/Release")

    # Copy Qt DLLs using target properties
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE:RtMidi::rtmidi>
            "${RELEASE_DIR}"
        COMMENT "Deploying Qt6 and rtmidi runtime libraries to Release folder")

    # Copy Qt platform plugin (qwindows)
    get_target_property(QT6_PLUGINS_DIR Qt6::Core LOCATION)
    get_filename_component(QT6_PLUGINS_DIR "${QT6_PLUGINS_DIR}/../plugins" ABSOLUTE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RELEASE_DIR}/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_PLUGINS_DIR}/platforms/qwindows.dll"
            "${RELEASE_DIR}/platforms/"
        COMMENT "Copying Qt6 platform plugins to Release folder")
endif()