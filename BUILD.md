# Building KtoMIDI

## Quick Start

```batch
# One-time setup
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg integrate install
setx VCPKG_ROOT "C:\vcpkg"

# Install dependencies
vcpkg install qtbase:x64-windows rtmidi:x64-windows

# Build (restart terminal after setting VCPKG_ROOT)
build.bat
```

Output: `build\bin\Release\KtoMIDI.exe`

## Prerequisites

### Required Software

1. **CMake** (3.20 or higher)
   - Download: https://cmake.org/download/
   - Add to PATH during installation

2. **Visual Studio 2019+**
   - Install "Desktop development with C++" workload
   - Download: https://visualstudio.microsoft.com/downloads/

3. **vcpkg** (C++ package manager)
   - Instructions: https://github.com/microsoft/vcpkg
   - Quick setup:
     ```batch
     git clone https://github.com/microsoft/vcpkg.git
     cd vcpkg
     bootstrap-vcpkg.bat
     vcpkg integrate install
     ```
   - Set `VCPKG_ROOT` environment variable
   - Install packages:
     ```batch
     vcpkg install qtbase:x64-windows rtmidi:x64-windows
     ```

## Setting Up Environment

### Setting VCPKG_ROOT

**System-wide (recommended):**
1. Right-click "This PC" or "My Computer" → Properties
2. Click "Advanced system settings"
3. Click "Environment Variables"
4. Under "System variables", click "New"
5. Variable name: `VCPKG_ROOT`
6. Variable value: Path to your vcpkg directory (e.g., `C:\vcpkg`)
7. Click OK and restart any open command prompts

**Current session only:**
```batch
set VCPKG_ROOT=C:\path\to\your\vcpkg
```

## Building the Project

### Quick Build

Run the build script:
```batch
build.bat
```

The script will:
- Check for CMake and vcpkg
- Detect your Visual Studio version
- Configure the project with CMake
- Build the Release version
- Deploy Qt dependencies

**Important:** Install Qt6 and RtMidi via vcpkg before building.

### Clean Build

To remove previous build files and rebuild:
```batch
build.bat clean
```

### Manual Build

If you prefer to build manually:

1. **Install dependencies:**
   ```batch
   vcpkg install qtbase:x64-windows rtmidi:x64-windows
   ```

2. **Configure:**
   ```batch
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build:**
   ```batch
   cmake --build build --config Release --parallel
   ```

4. **Deploy Qt dependencies:**
   ```batch
   cd build\bin\Release
   windeployqt KtoMIDI.exe
   ```

## Build Output

After building:
- Executable: `build\bin\Release\KtoMIDI.exe`
- Qt DLLs are copied to the same directory
- All dependencies are included

## Troubleshooting

### CMake not found
- Ensure CMake is installed and added to your PATH
- Restart your command prompt after installation

### VCPKG_ROOT not set
- Set the environment variable as described above
- Verify with: `echo %VCPKG_ROOT%`

### Visual Studio not found
- Install Visual Studio 2019 or 2022 with C++ development tools
- The build script will detect your version automatically

### Qt6 or RtMidi installation fails
- Make sure you ran: `vcpkg install qtbase:x64-windows rtmidi:x64-windows`
- Check your internet connection
- Run `vcpkg update` in your vcpkg directory
- Try a clean build: `build.bat clean`

### Generator mismatch error
- Run: `build.bat clean`
- This removes the old CMake cache

### Build fails with missing headers
- Run: `vcpkg integrate install`
- Then try: `build.bat clean`

### windeployqt fails
- Make sure Qt6 is installed: `vcpkg list | findstr qt`
- The build script handles deployment automatically

## Build Configurations

### Debug Build

```batch
cmake --build build --config Debug
```

### Different Architecture

The default is x64. To change:
1. Edit `build.bat` and modify `VCPKG_ARCH`
2. Install packages for your target architecture
3. Adjust the CMake generator flags

## Additional Options

### Custom vcpkg location

```batch
set VCPKG_ROOT=D:\my-custom-path\vcpkg
build.bat
```

### Parallel build jobs

The build uses all CPU cores by default. To limit jobs:
```batch
cmake --build build --config Release --parallel 4
```

## CI/CD

For automated builds:
1. Add CMake to PATH
2. Set VCPKG_ROOT environment variable
3. Run `vcpkg integrate install`
4. Run `build.bat`

## What Gets Built

The CMake configuration:
- Generates `version.h` from `version.h.in`
- Compiles all source files in `src/`
- Processes Qt resources (`resources/KtoMIDI.qrc`)
- Includes Windows resources (`resources/KtoMIDI.rc`)
- Links Qt6 (Core, Widgets, Gui) and RtMidi
- Creates a Windows GUI executable
- Deploys Qt DLLs to output directory

## Clean Build Directory

To completely remove the build directory:
```batch
rmdir /s /q build
```

Then rebuild:
```batch
build.bat
```
