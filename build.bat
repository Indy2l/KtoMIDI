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

:: Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Configure with CMake
echo [INFO] Running CMake configuration...
cmake -B "%BUILD_DIR%" -S . ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -G "Visual Studio 17 2022" ^
    -A x64

if %ERRORLEVEL% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo.
    echo Troubleshooting tips:
    echo 1. Ensure Visual Studio 2022 is installed
    echo    If you have VS 2019, change the generator to "Visual Studio 16 2019"
    echo 2. Check that all dependencies are properly installed via vcpkg
    echo 3. Try running: vcpkg integrate install
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
