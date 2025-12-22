# Building KtoMIDI

This document provides detailed instructions for building KtoMIDI from source.

## Quick Start

For experienced developers:

```batch
# One-time setup
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
setx VCPKG_ROOT "C:\vcpkg"

# Build (restart terminal after setting VCPKG_ROOT)
build.bat
```

Output: `build\bin\Release\KtoMIDI.exe`

## Prerequisites

### Required Software

1. **CMake** (version 3.20 or higher)
   - Download from: https://cmake.org/download/
   - Make sure to add CMake to your system PATH during installation

2. **Visual Studio 2019 or 2022**
   - Community Edition is sufficient
   - Install the "Desktop development with C++" workload
   - Download from: https://visualstudio.microsoft.com/downloads/

3. **vcpkg** (C++ Package Manager)
   - Install following instructions at: https://github.com/microsoft/vcpkg
   - Quick setup:
     ```batch
     git clone https://github.com/microsoft/vcpkg.git
     cd vcpkg
     bootstrap-vcpkg.bat
     vcpkg integrate install
     ```
   - Set the `VCPKG_ROOT` environment variable to your vcpkg installation directory

## Setting Up Environment

### Setting VCPKG_ROOT Environment Variable

**Option 1: System-wide (Recommended)**
1. Right-click "This PC" or "My Computer" → Properties
2. Click "Advanced system settings"
3. Click "Environment Variables"
4. Under "System variables", click "New"
5. Variable name: `VCPKG_ROOT`
6. Variable value: Path to your vcpkg directory (e.g., `C:\vcpkg`)
7. Click OK and restart any open command prompts

**Option 2: For current session only**
```batch
set VCPKG_ROOT=C:\path\to\your\vcpkg
```

## Building the Project

### Quick Build (Recommended)

Simply run the build script:
```batch
build.bat
```

The script will automatically:
- Check for CMake and vcpkg
- Install required dependencies (Qt6, RtMidi) via vcpkg manifest mode
- Configure the project with CMake
- Build the Release version
- Deploy Qt dependencies using windeployqt

**Note:** This project uses vcpkg manifest mode with `vcpkg.json`. Dependencies are automatically installed during the CMake configuration phase - no manual vcpkg installation needed!

### Clean Build

To perform a clean build (removes previous build files):
```batch
build.bat clean
```

### Manual Build

If you prefer to build manually or need more control:

1. **Dependencies are managed automatically via vcpkg.json manifest**
   - No need to manually install packages!
   - vcpkg will install dependencies during CMake configuration

2. **Configure with CMake:**
   ```batch
   cmake -B build -S . ^
       -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
       -DCMAKE_BUILD_TYPE=Release ^
       -G "Visual Studio 17 2022" ^
       -A x64
   ```
   
   *Note: If using Visual Studio 2019, change the generator to "Visual Studio 16 2019"*

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

After a successful build, you'll find:
- **Executable**: `build\bin\Release\KtoMIDI.exe`
- **Qt DLLs**: Automatically copied to the same directory
- **All required dependencies**: Included in the output directory

## Troubleshooting

### CMake not found
- Ensure CMake is installed and added to your PATH
- Restart your command prompt after installation

### VCPKG_ROOT not set
- Set the environment variable as described above
- Verify: `echo %VCPKG_ROOT%` should show your vcpkg path

### Visual Studio not found
- Install Visual Studio with C++ development tools
- If using VS 2019, edit `build.bat` and change the generator to "Visual Studio 16 2019"

### Qt6 or RtMidi installation fails
- The project uses vcpkg manifest mode - dependencies should install automatically
- Ensure you have a stable internet connection for the first build
- Run: `vcpkg update` in your vcpkg directory
- Delete the `build/` directory and try again
- Check `vcpkg.json` is present in the project root

### Build fails with missing headers
- Run: `vcpkg integrate install`
- Clean and rebuild: `build.bat clean`

### windeployqt fails
- Ensure Qt6 is properly installed via vcpkg
- The build script should handle this automatically

## Build Configurations

### Debug Build

To build a debug version:
```batch
cmake --build build --config Debug
```

### Different Architecture

The default is x64 (64-bit). To change:
1. Edit `build.bat` and change `VCPKG_ARCH` variable
2. Install packages for the desired architecture
3. Adjust CMake generator architecture flag

## Additional Options

### Custom vcpkg location

If your vcpkg is not in the default location, set VCPKG_ROOT before building:
```batch
set VCPKG_ROOT=D:\my-custom-path\vcpkg
build.bat
```

### Parallel build jobs

The build script uses all available CPU cores by default. To limit:
```batch
cmake --build build --config Release --parallel 4
```

## CI/CD Integration

For automated builds, ensure:
1. CMake is in PATH
2. VCPKG_ROOT is set as an environment variable
3. vcpkg has integrated with Visual Studio: `vcpkg integrate install`
4. Run: `build.bat` (it will handle everything automatically)

## What Gets Built

The CMake configuration:
- Generates `version.h` from `version.h.in` with project version info
- Compiles all source files in `src/`
- Processes Qt resources (`resources/KtoMIDI.qrc`)
- Includes Windows resources (`resources/KtoMIDI.rc`)
- Links against Qt6 (Core, Widgets, Gui) and RtMidi
- Creates a Windows GUI application (WIN32 executable)
- Automatically deploys Qt DLLs to output directory

## Clean Build Directory

To completely clean the build:
```batch
rmdir /s /q build
```

Then rebuild from scratch:
```batch
build.bat
```
