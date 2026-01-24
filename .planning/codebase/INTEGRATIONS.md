# External Integrations

**Analysis Date:** 2026-01-24

## APIs & External Services

**Game Engine Integration:**
- Butano Engine - Core GBA hardware abstraction
  - SDK/Client: `butano/butano/include/bn_*.h` headers
  - Auth: None (local library)
  - Integration points: Sprites, backgrounds, audio, input, camera

**Build System Integration:**
- devkitARM Toolchain - ARM cross-compilation
  - SDK/Client: Command-line toolchain
  - Auth: DEVKITARM environment variable
  - Integration points: Compilation, linking, ROM generation

## Data Storage

**Game Data:**
- SRAM (Save RAM) - Game save state persistence
  - Connection: GBA hardware SRAM (64KB typical)
  - Client: Butano's sram API
  - Implementation: `WorldStateManager` class

**Asset Storage:**
- ROM embedded assets - Graphics and audio data
  - Connection: Compiled into ROM binary
  - Client: grit/mmutil conversion tools
  - Implementation: `graphics/` and `audio/` directories

**File Storage:**
- Local filesystem only - Development assets
  - Source files: `src/`, `include/`
  - Assets: `graphics/`, `audio/`
  - Build artifacts: `build/` directory

## Caching & Performance

**Graphics Caching:**
- Sprite tiles caching - GBA VRAM management
  - Implementation: Butano sprite tile system
  - Location: Hardware VRAM (32KB tiles + 32KB palettes)

**Audio Caching:**
- Sound effect caching - GBA audio RAM
  - Implementation: Butano audio system
  - Location: Hardware audio channels

## Authentication & Identity

**Development Environment:**
- Docker Container - Development environment isolation
  - Implementation: `.devcontainer/devcontainer.json`
  - Image: `ghcr.io/numaphor/stranded/stranded-dev:latest`
  - Purpose: Consistent development environment

**Version Control:**
- Git Submodules - External dependency management
  - Butano engine: `butano/` submodule
  - LLM Council Skill: `llm_council_skill/` submodule
  - Get-Shit-Done: `get-shit-done/` submodule

## Monitoring & Observability

**Error Tracking:**
- Butano Logging System - Runtime debugging
  - Implementation: `bn::log` functions
  - Output: Emulator console or hardware debug interface

**Performance Profiling:**
- Butano Profiler - Frame timing and performance analysis
  - Implementation: `BN_PROFILER_*` macros
  - Purpose: Optimize GBA performance constraints

**Stack Tracing:**
- Enabled in build - `STACKTRACE := true` in Makefile
  - Purpose: Debug crash locations
  - Output: Console error messages

## CI/CD & Deployment

**Container Registry:**
- GitHub Container Registry - Development image hosting
  - Repository: `ghcr.io/numaphor/stranded/stranded-dev`
  - Purpose: Development environment distribution

**Build Pipeline:**
- Make-based build - Local compilation and ROM generation
  - Input: Source files, assets
  - Output: `stranded.gba` ROM file
  - Tools: devkitARM, Butano build tools

## Environment Configuration

**Required env vars:**
- `DEVKITARM` - Primary toolchain path
- `WONDERFUL_TOOLCHAIN` - Alternative toolchain path (optional)

**Build Configuration:**
- `Makefile` variables:
  - `TARGET := stranded` - Output ROM name
  - `SOURCES := src src/core src/actors butano/common/src` - Source directories
  - `INCLUDES := include butano/common/include` - Header directories
  - `GRAPHICS := graphics butano/common/graphics` - Asset directories
  - `AUDIO := audio butano/common/audio` - Audio directories
  - `STACKTRACE := true` - Enable debug features

## Asset Pipeline Integrations

**Graphics Processing:**
- grit integration - Bitmap to GBA format conversion
  - Input: `.bmp`, `.png` files with `.json` metadata
  - Output: GBA tile/map data
  - Tool: Butano's `butano_graphics_tool.py`

**Audio Processing:**
- mmutil integration - Audio to GBA format conversion
  - Input: `.wav`, `.it` (Impulse Tracker) files
  - Output: GBA audio data
  - Tool: Butano's `butano_audio_tool.py`

**Font Generation:**
- PIL/Pillow integration - Custom font creation
  - Input: Python script with system fonts
  - Output: `graphics/fonts/fixed_8x8_font.png`
  - Tool: `tools/generate_font.py`

## Platform-Specific Integrations

**GBA Hardware Integration:**
- Memory Management - GBA memory map adherence
  - VRAM: 32KB tiles + 32KB palettes
  - WRAM: 256KB work RAM
  - ROM: 2MB cartridge space

**Display Integration:**
- GBA Display Controller - 240x160 pixel rendering
  - Background layers: 4 BG layers
  - Sprite layer: 128 OBJ sprites
  - Implementation: Butano background/sprite systems

**Input Integration:**
- GBA Input Hardware - Button state reading
  - D-Pad, A, B, L, R, Start, Select buttons
  - Implementation: Butano input system
  - Polling: `bn::keypad` API

**Audio Integration:**
- GBA Sound Hardware - 6-channel audio output
  - 4 PCM channels + 2 PSG channels
  - Implementation: Butano audio mixer
  - Sample rate: 32768 Hz hardware

## Development Tool Integrations

**Emulator Integration:**
- mGBA - Development and testing
  - Location: `tools/mGBA.exe`
  - Purpose: Real hardware emulation
  - Features: Debug logging, save states

**Asset Creation Tools:**
- Aseprite - Sprite animation creation
  - Files: `graphics/sprite/player/hero32.aseprite`, `hero64.aseprite`
  - Integration: Export to BMP/JSON for Butano

**Version Control Integration:**
- Git Submodules - External dependency tracking
  - Automatic initialization: `git submodule update --init --recursive`
  - Build dependency: Butano engine availability

---

*Integration audit: 2026-01-24*