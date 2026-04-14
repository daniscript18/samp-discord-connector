# Discord connector plugin for SA-MP and open.mp

This project builds a Discord bot connector plugin for SA-MP / open.mp servers.

## What it produces

- `discord-connector.dll` on Windows
- `discord-connector.so` on Linux
- `log-core2.dll` or `log-core2.so` as the logging helper library

## Windows build

Use a 32-bit toolchain. This plugin is meant to be built as x86, not x64.

### Prerequisites

- Git
- CMake 3.15 or newer, but use CMake 3.x. CMake 4.x is not compatible with some Conan 1.x recipes used by this project.
- Visual Studio Build Tools with the C++ x86/x64 toolset
- Conan 1.x

If Conan or a CMake 3.x executable is not already available, you can install them for your current Python user:

```powershell
python -m pip install --user "conan<2" "cmake<4"
```

If pip installs the executables outside your PATH, add the Python user scripts directory for the current shell:

```powershell
$scripts = "$env:APPDATA\Python\Python312\Scripts"
$env:PATH = "$scripts;$env:PATH"
```

### Build steps

1. Clone this repository.
2. Make sure submodules are present if you cloned from a remote:
   ```bash
   git submodule update --init --recursive
   ```
3. Configure CMake for a 32-bit Visual Studio build:
   ```powershell
   cmake -S . -B build -G "Visual Studio 15 2017" -A Win32 -DCMAKE_CONFIGURATION_TYPES=Release -DCMAKE_BUILD_TYPE=Release
   ```
4. Build the project:
   ```powershell
   cmake --build build --config Release
   ```

The Windows build outputs are written to:

- `build/artifact/plugins/discord-connector.dll`
- `build/artifact/log-core2.dll`
- `build/artifact/pawno/include/discord-connector.inc`

## Linux build

The project also builds on Linux as a 32-bit plugin when the required 32-bit libraries are available.

```bash
mkdir build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS=-m32 \
  -DCMAKE_CXX_FLAGS=-m32
cmake --build . -j"$(nproc)"
```

## Installation

For SA-MP, copy the plugin into your server's `plugins` directory and add it to `server.cfg`.

- Windows: `plugins discord-connector`
- Linux: `plugins discord-connector.so`

For open.mp, place the plugin in the `components` directory if you want it loaded as a component.

## Configuration

Set your Discord bot token in the server configuration or environment:

- `DCC_BOT_TOKEN`

Never share your bot token publicly.
