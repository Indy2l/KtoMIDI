@echo off
setlocal enabledelayedexpansion

:: ============================================
:: KtoMIDI Build Script
:: ============================================
:: This script builds the KtoMIDI project using CMake and vcpkg
:: Prerequisites:
::   - CMake installed and in PATH
::   - Visual Studio 2019 or later
::   - vcpkg installed (VCPKG_ROOT environment variable set)
:: ============================================

echo ========================================
echo   KtoMIDI Build Script
echo ========================================
echo.

:: Check if CMake is available
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake not found in PATH!
    echo Please install CMake and add it to your PATH.
    echo Download from: https://cmake.org/download/
    pause
    exit /b 1
)

:: Check if vcpkg is set up
if not defined VCPKG_ROOT (
    echo [ERROR] VCPKG_ROOT environment variable is not set!
    echo.
    echo Please set VCPKG_ROOT to your vcpkg installation directory.
    echo Example: set VCPKG_ROOT=C:\vcpkg
    echo.
    echo Or add it to your system environment variables.
    pause
    exit /b 1
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo [ERROR] vcpkg.cmake not found at: %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
    echo Please ensure VCPKG_ROOT points to a valid vcpkg installation.
    pause
    exit /b 1
)

echo [INFO] Using vcpkg from: %VCPKG_ROOT%
echo.

:: Detect architecture
set VCPKG_ARCH=x64-windows
echo [INFO] Target architecture: %VCPKG_ARCH%
echo.

:: Note: Dependencies are now managed via vcpkg.json manifest
:: vcpkg will automatically install required packages during CMake configuration
echo [INFO] Using vcpkg manifest mode (vcpkg.json)
echo [INFO] Dependencies will be installed automatically during configuration
echo.

:: Set build directory
set BUILD_DIR=build
set BUILD_TYPE=Release

:: Detect Visual Studio version
set VS_GENERATOR=
set VS_VERSION=

:: Check for VS 2026 (version 18)
if exist "%ProgramFiles%\Microsoft Visual Studio\2026" (
    set VS_GENERATOR=Visual Studio 18 2026
    set VS_VERSION=2026
    goto :vs_found
)

:: Check for VS 2022 (version 17)
if exist "%ProgramFiles%\Microsoft Visual Studio\2022" (
    set VS_GENERATOR=Visual Studio 17 2022
    set VS_VERSION=2022
    goto :vs_found
)

:: Check for VS 2019 (version 16)
if exist "%ProgramFiles%\Microsoft Visual Studio\2019" (
    set VS_GENERATOR=Visual Studio 16 2019
    set VS_VERSION=2019
    goto :vs_found
)

:: No Visual Studio found
echo [ERROR] Visual Studio not found!
echo Please install Visual Studio 2019, 2022, or 2026 with C++ development tools.
pause
exit /b 1

:vs_found
echo [INFO] Detected Visual Studio %VS_VERSION%
echo [INFO] Using generator: %VS_GENERATOR%
echo.

echo ========================================
echo   Configuring CMake
echo ========================================
echo.

:: Clean build directory if requested
if "%1"=="clean" (
    echo [INFO] Cleaning build directory...
    if exist "%BUILD_DIR%" (
        rmdir /s /q "%BUILD_DIR%"
    )
    echo [INFO] Build directory cleaned.
    echo.
)

:: Check for generator mismatch
if exist "%BUILD_DIR%\CMakeCache.txt" (
    findstr /C:"CMAKE_GENERATOR:INTERNAL=%VS_GENERATOR%" "%BUILD_DIR%\CMakeCache.txt" >nul 2>&1
    if !ERRORLEVEL! neq 0 (
        echo [WARNING] Build directory was configured with a different generator.
        echo [INFO] Cleaning build directory to avoid conflicts...
        rmdir /s /q "%BUILD_DIR%"
        echo [INFO] Build directory cleaned.
        echo.
    )
)

:: Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Configure with CMake
echo [INFO] Running CMake configuration...
cmake -B "%BUILD_DIR%" -S . ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -G "%VS_GENERATOR%" ^
    -A x64

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo.
    echo Troubleshooting tips:
    echo 1. Ensure Visual Studio is properly installed with C++ development tools
    echo 2. Check that all dependencies are properly installed via vcpkg
    echo 3. Try running: vcpkg integrate install
    echo 4. Try a clean build: build.bat clean
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Building Project
echo ========================================
echo.

:: Build with CMake
echo [INFO] Building %BUILD_TYPE% configuration...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --parallel

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   Build Complete!
echo ========================================
echo.
echo [SUCCESS] KtoMIDI has been built successfully!
echo.
echo Executable location:
echo   %BUILD_DIR%\bin\%BUILD_TYPE%\KtoMIDI.exe
echo.
echo The executable has been automatically deployed with Qt dependencies.
echo You can run it directly from the build directory.
echo.

:: Ask if user wants to run the application
set /p RUN_APP="Would you like to run KtoMIDI now? (y/n): "
if /i "!RUN_APP!"=="y" (
    echo.
    echo [INFO] Launching KtoMIDI...
    start "" "%BUILD_DIR%\bin\%BUILD_TYPE%\KtoMIDI.exe"
)

echo.
echo Press any key to exit...
pause >nul
exit /b 0
