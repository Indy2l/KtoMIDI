@echo off

echo ================================================
echo KtoMIDI Build and Deployment Script
echo ================================================
echo.

if not exist "CMakeLists.txt" (
    echo ERROR: CMakeLists.txt not found. Please run this script from the KtoMIDI root directory.
    pause
    exit /b 1
)



echo [1/4] Cleaning previous build...
if exist "build" (
    rmdir /s /q "build"
    echo     Previous build directory removed
) else (
    echo     No previous build found
)

echo.
echo [2/4] Configuring CMake build...
mkdir build
cd build

if not defined VCPKG_ROOT (
    echo ERROR: VCPKG_ROOT environment variable is not set.
    echo Please set VCPKG_ROOT to your vcpkg installation path.
    pause
    exit /b 1
)

set VCPKG_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=%VCPKG_TOOLCHAIN_FILE%
if errorlevel 1 (
    echo ERROR: CMake configuration failed!
    echo Please ensure:
    echo   - Qt6 and RtMidi are installed and available to CMake via vcpkg
    echo   - Visual Studio Build Tools are available
    pause
    exit /b 1
)

echo.
echo [3/4] Building Release version...
cmake --build . --config Release
if errorlevel 1 (
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [4/4] Deployment completed successfully!
echo.
echo Output files location:
echo   Executable: %CD%\Release\KtoMIDI.exe
echo   Qt6/RtMidi DLLs: %CD%\Release\
echo   Platforms:      %CD%\Release\platforms\
echo.

pause