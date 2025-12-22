# KtoMIDI

Windows utility for converting keycode input to MIDI messages!

## Preview:
![Preview image](/images/preview.png?raw=true)

## Features

- System-wide keyboard capture
- Key press and release events to MIDI messages
- Real-time input monitoring
- Minimize to system tray
- Persistent mappings and settings

## Requirements

- Windows 10/11 (x64)
- MIDI output device or virtual MIDI port (loopMIDI, etc.)

## Installation

### Option 1: Download Release (Recommended)

1. Download the latest release from the [Releases](https://github.com/Indy2l/KtoMIDI/releases) page
2. Extract the ZIP file
3. Run `KtoMIDI.exe`

All dependencies are included.

### Option 2: Build from Source

See the [Building](#building) section below.

## Usage

1. Run `KtoMIDI.exe`
2. Select your MIDI output port from the dropdown
3. Click "Add Mapping" to create keycode-to-MIDI mappings
4. Configure MIDI message type, channel, note/controller, and velocity

The application runs in the background. Settings are saved automatically to `%APPDATA%\KtoMIDI Project\KtoMIDI\`.

## Building

### Prerequisites

- Visual Studio 2019+ or Build Tools
- CMake 3.20+
- vcpkg package manager
- Qt6 and RtMidi (installed via vcpkg)

### Install Dependencies

```bash
vcpkg install qt6[core,widgets,gui]:x64-windows
vcpkg install rtmidi:x64-windows
```

### Build with deploy.bat (Easiest)

1. Set the `VCPKG_ROOT` environment variable to your vcpkg installation path
2. Make sure you have installed the dependencies above with vcpkg.
3. Run `deploy.bat` from the project root directory.

The built executable and all required DLLs will be in `build\Release\KtoMIDI.exe`.

### Manual Build (If deploy.bat fails)

```bat
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

- This method uses the vcpkg toolchain file for dependency resolution.
- The executable and dependencies will be in `build\Release\KtoMIDI.exe`.

## License

MIT License - see LICENSE file.