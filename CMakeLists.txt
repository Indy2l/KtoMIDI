cmake_minimum_required(VERSION 3.20)

# Project definition
project(KtoMIDI
    VERSION 1.0.0
    DESCRIPTION "Keycode to MIDI converter"
    LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Qt settings
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Configure version header
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h
    @ONLY
)

# Find dependencies
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)
find_package(RtMidi REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/MidiEngine.cpp
    src/MidiEngine.h
    src/KeyHook.cpp
    src/KeyHook.h
    src/KeyMapping.cpp
    src/KeyMapping.h
    src/KeyUtils.h
    src/InputMonitor.cpp
    src/InputMonitor.h
    src/MappingDialog.cpp
    src/MappingDialog.h
)

# Resource files
set(RESOURCES
    resources/KtoMIDI.qrc
)

# Windows-specific resources
if(WIN32)
    list(APPEND SOURCES resources/KtoMIDI.rc)
    # Ensure the .rc file can find version.h
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I\"${CMAKE_CURRENT_BINARY_DIR}/generated\"")
endif()

# Create executable
add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${RESOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    RtMidi::rtmidi
)

# Windows-specific settings
if(WIN32)
    # Ensure we link against Windows libraries needed for keyboard hooks
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32
        gdi32
        winmm
    )
    
    # Set application properties
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_NAME "KtoMIDI"
    )
endif()
# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Qt deployment (Windows only)
if(WIN32)
    # Get Qt6 bin directory for windeployqt
    get_target_property(QT6_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT6_BIN_DIR "${QT6_QMAKE_EXECUTABLE}" DIRECTORY)
    
    # Add custom command to run windeployqt after build
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT6_BIN_DIR}/windeployqt.exe"
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Running windeployqt to copy Qt dependencies..."
    )
    
    # Copy additional Qt dependencies that windeployqt may miss
    set(ADDITIONAL_DLLS
        "pcre2-16.dll"
        "double-conversion.dll"
    )
    
    foreach(DLL_NAME ${ADDITIONAL_DLLS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT6_BIN_DIR}/${DLL_NAME}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
            COMMENT "Copying ${DLL_NAME}..."
            VERBATIM
        )
    endforeach()
endif()

# Print configuration
message(STATUS "")
message(STATUS "======================================")
message(STATUS "  ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "======================================")
message(STATUS "Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler:         ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Qt6 version:      ${Qt6_VERSION}")
message(STATUS "Install prefix:   ${CMAKE_INSTALL_PREFIX}")
message(STATUS "======================================")
message(STATUS "")
