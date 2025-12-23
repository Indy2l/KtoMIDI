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

:: Initialize Visual Studio environment
echo [INFO] Initializing Visual Studio environment...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe not found. Is Visual Studio installed?
    pause
    exit /b 1
)

:: Find Visual Studio installation
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
    set VS_INSTALL_PATH=%%i
)

if not defined VS_INSTALL_PATH (
    echo [ERROR] Visual Studio installation not found!
    pause
    exit /b 1
)

:: Set up Visual Studio environment
if exist "%VS_INSTALL_PATH%\VC\Auxiliary\Build\vcvarsall.bat" (
    echo [INFO] Found Visual Studio at: %VS_INSTALL_PATH%
    call "%VS_INSTALL_PATH%\VC\Auxiliary\Build\vcvarsall.bat" x64
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] Failed to initialize Visual Studio environment!
        pause
        exit /b 1
    )
    echo [INFO] Visual Studio environment initialized successfully.
) else (
    echo [ERROR] vcvarsall.bat not found in Visual Studio installation!
    pause
    exit /b 1
)
echo.

echo [INFO] Using global vcpkg packages
echo [INFO] Ensure Qt6 and RtMidi are installed:
echo        vcpkg install qtbase:x64-windows rtmidi:x64-windows
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
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

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

:: Check if KtoMIDI is running and terminate it
echo [INFO] Checking for running KtoMIDI instances...
tasklist /FI "IMAGENAME eq KtoMIDI.exe" 2>NUL | find /I /N "KtoMIDI.exe">NUL
if "%ERRORLEVEL%"=="0" (
    echo [INFO] KtoMIDI is running, terminating...
    taskkill /F /IM KtoMIDI.exe >NUL 2>&1
    timeout /t 1 /nobreak >NUL
    echo [INFO] KtoMIDI terminated.
)
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

exit /b 0
