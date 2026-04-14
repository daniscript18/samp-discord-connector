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
- CMake 3.15 or newer
- A C++ compiler for Windows
- Conan 1.x

### Build steps

1. Clone this repository.
2. Make sure submodules are present if you cloned from a remote:
   ```bash
   git submodule update --init --recursive
   ```
3. Create a build directory:
   ```bash
   mkdir build
   cd build
   ```
4. Configure CMake for a 32-bit build.
   - With Visual Studio:
     ```bash
     cmake .. -A Win32 -DCMAKE_BUILD_TYPE=Release
     ```
   - With MinGW or another single-config generator, use the equivalent x86 toolchain setup.
5. Build the project:
   ```bash
   cmake --build . --config Release
   ```

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
