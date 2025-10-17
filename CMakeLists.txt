cmake_minimum_required(VERSION 3.20)

project(KtoMIDI 
    VERSION 1.0.0
    DESCRIPTION "KtoMIDI"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose build type" FORCE)
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

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "vcpkg toolchain file")
endif()

if(DEFINED ENV{VCPKG_ROOT})
    set(VCPKG_INSTALLED_DIR "$ENV{VCPKG_ROOT}/installed/x64-windows")
else()
    set(VCPKG_INSTALLED_DIR "D:/.dev/vcpkg/installed/x64-windows")
    message(WARNING "VCPKG_ROOT environment variable not set. Using default path: ${VCPKG_INSTALLED_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

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

set(HID_SOURCES
    src/HidDevice.cpp
    src/HidMapping.cpp
    src/HidDeviceTab.cpp
)

set(SOURCES
    ${CORE_SOURCES}
    ${KEYBOARD_SOURCES}
    ${MIDI_SOURCES}
    ${HID_SOURCES}
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

set(HID_HEADERS
    src/HidDevice.h
    src/HidMapping.h
    src/HidDeviceTab.h
)

set(HEADERS
    ${CORE_HEADERS}
    ${KEYBOARD_HEADERS}
    ${MIDI_HEADERS}
    ${HID_HEADERS}
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

target_include_directories(${PROJECT_NAME} PRIVATE 
    src/
    "${VCPKG_INSTALLED_DIR}/include"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    user32          # Windows user interface API
    kernel32        # Windows kernel API
    winmm           # Windows multimedia API (MIDI support)
    setupapi        # Windows device setup API (HID support)
    hid             # Windows HID API
)

target_link_libraries(${PROJECT_NAME} PRIVATE "${VCPKG_INSTALLED_DIR}/lib/rtmidi.lib")

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
    set(QT6_BIN_DIR "${VCPKG_INSTALLED_DIR}/bin")
    set(QT6_PLUGINS_DIR "${VCPKG_INSTALLED_DIR}/Qt6/plugins")
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT6_BIN_DIR}/Qt6Core.dll"
        "${QT6_BIN_DIR}/Qt6Gui.dll" 
        "${QT6_BIN_DIR}/Qt6Widgets.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Deploying Qt6 runtime libraries")
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT6_PLUGINS_DIR}/platforms/qwindows.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/
        COMMENT "Copying Qt6 platform plugins")
endif()