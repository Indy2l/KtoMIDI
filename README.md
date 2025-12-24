# KtoMIDI

Windows utility for converting keycode input to MIDI messages.

## Preview:
![Preview image](/images/preview.png?raw=true)

## Features

- System-wide keycode capture
- Key press/release events to MIDI messages
- Input monitoring
- Minimize to system tray
- All settings and mappings automatically save

## Requirements

- Windows 10/11 (x64)
- MIDI output device or virtual MIDI port (loopMIDI, etc.)

## Installation

Download the latest release from the [Releases](https://github.com/Indy2l/KtoMIDI/releases) page and run `KtoMIDI.exe`.
Or [build from source](#building) if you want.

## Usage

Run the app.
Pick your MIDI output. Use loopMIDI for virtual MIDI ports.
Add some mappings and set the MIDI channel, note/CC, and velocity as needed.
The app sits in the background and saves settings to `%APPDATA%\KtoMIDI Project\KtoMIDI\`.

## Building

### Prerequisites

- [Visual Studio 2019+](https://visualstudio.microsoft.com/downloads/) with C++ development tools
- [CMake 3.20+](https://cmake.org/download/)
- [vcpkg](https://github.com/microsoft/vcpkg)

1. **Set up vcpkg**
   ```powershell
   git clone https://github.com/microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   vcpkg integrate install
   ```

2. **Install dependencies**
   ```powershell
   vcpkg install qtbase[core,gui,widgets]:x64-windows rtmidi:x64-windows
   ```

## Using build.bat

Just run build.bat. The .exe will be at "KtoMIDI\build\bin\Release\KtoMIDI.exe"

## Manual Build (Alternative)
```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

Clean rebuild: `.\build.bat clean`

## License

MIT License - see LICENSE file.

gh