# Technology Stack

**Analysis Date:** 2026-02-09

## Languages

**Primary:**
- C++23 - Core game logic and systems (src/, include/)
- C++23 - Butano engine integration (butano/)

**Secondary:**
- Python - Asset processing and tooling (tools/, build system integration)

## Runtime

**Environment:**
- Game Boy Advance (ARM7TDMI CPU)
- 60 FPS target with frame management
- Hardware constraints: 32KB IWRAM, 256KB EWRAM, no FPU

**Package Manager / Build System:**
- Make - Primary build system
- Lockfile: Not applicable (embedded target)
- Build configuration: Makefile with Butano integration

## Frameworks

**Core:**
- Butano C++23 Engine - Game Boy Advance development framework
- devkitARM / Wonderful Toolchain - GBA toolchain
- Maxmod - Audio playback (Direct Sound)

**Testing:**
- Manual testing via mGBA emulator
- No automated test framework detected

**Build/Dev:**
- Make - Build orchestration
- Python - Asset processing scripts
- Grit - Graphics conversion tool
- mmutil - Audio conversion tool

## Key Dependencies

**Critical:**
- Butano Library (butano/butano/) - Core GBA engine providing sprites, backgrounds, input, audio, memory management
- bn::fixed-point arithmetic - Essential for GBA (no FPU)
- bn::optional - Resource management pattern

**Infrastructure:**
- mGBA emulator - Development testing environment
- Grit - Converts BMP images to GBA format
- Maxmod - MOD/XM/S3M/IT music playback

## Configuration

**Environment:**
- Makefile-based configuration with USERFLAGS for compiler options
- Build targets: ROM file generation for GBA
- Asset pipeline: JSON descriptors for graphics/audio

**Build:**
- Main Makefile with LIBBUTANO path configuration
- SOURCES += src src/core src/actors butano/common/src
- GRAPHICS += graphics/ butano/common/graphics/
- AUDIO += audio/ butano/common/audio/

## Platform Requirements

**Development:**
- Windows environment (based on file paths)
- devkitARM or Wonderful Toolchain
- Python for asset processing
- mGBA emulator for testing

**Target:**
- Game Boy Advance hardware
- GBA-compatible flash carts
- mGBA/NanoBoyAdvance/Mesen for emulation

---
*Stack analysis: 2026-02-09*