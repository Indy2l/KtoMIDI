# KtoMIDI

Windows utility for converting keyboard input to MIDI messages.

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

### Download Release (Recommended)

1. Download the latest release from the [Releases](https://github.com/Indy2l/KtoMIDI/releases) page
2. Extract the ZIP file
3. Run `KtoMIDI.exe`

All dependencies are included.

### Build from Source

See the [Building](#building) section below.

## Usage

1. Run `KtoMIDI.exe`
2. Select your MIDI output port from the dropdown
3. Click "Add Mapping" to create keycode-to-MIDI mappings
4. Configure MIDI message type, channel, note/controller, and velocity

The application runs in the background. Settings are saved automatically to `%APPDATA%\KtoMIDI Project\KtoMIDI\`.

## Building

### Prerequisites

1. Install [vcpkg](https://github.com/microsoft/vcpkg) and set `VCPKG_ROOT` environment variable
2. Install dependencies:
   ```
   vcpkg install qtbase:x64-windows rtmidi:x64-windows
   ```
3. Install [CMake](https://cmake.org/download/) and [Visual Studio 2019+](https://visualstudio.microsoft.com/downloads/)
4. Run `build.bat`

Output: `build\bin\Release\KtoMIDI.exe`

### Detailed Instructions

For detailed build instructions, troubleshooting, and advanced options, see [BUILD.md](BUILD.md).

## License

MIT License - see LICENSE file.