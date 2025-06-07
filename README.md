# Saturn 3D Renderer Prototype

A minimal 3D renderer prototype for embedded systems, specifically targeting STM32 with SSD1306 displays.

## Features

- Simulates SSD1306 128x64 monochrome buffer
- 3D rendering of Saturn with rings
- Rotating animation
- Minimal codebase suitable for embedded systems

## Setup

### Windows (PowerShell 7.5 with Visual Studio)

1. Install dependencies (vcpkg + SDL2):
   ```powershell
   .\setup_sdl2.ps1
   ```

2. Build the project:
   ```powershell
   .\build.ps1
   ```

3. Run the prototype:
   ```powershell
   .\build\saturn_renderer.exe
   ```

### Requirements
- Visual Studio 2019+ with C++ tools
- CMake
- Ninja (comes with Visual Studio)
- Git (for vcpkg)

## Controls

- ESC: Quit
- The Saturn model rotates automatically

## Architecture

The code simulates your STM32 SSD1306 interface:
- `SSD1306_GetBuffer()` - Get the 1024-byte buffer
- `SSD1306_SetPixel()` - Set individual pixels
- `SSD1306_Clear()` - Clear the buffer

The 3D renderer uses basic projection and rotation math suitable for embedded systems. 