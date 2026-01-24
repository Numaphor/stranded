# Technology Stack

**Analysis Date:** 2026-01-24

## Languages

**Primary:**
- C++17 - Core game logic, entity systems, and all gameplay code
- Used in: `src/*.cpp`, `include/*.h`

**Secondary:**
- Python 3.x - Build tools, asset processing, and development utilities
- Used in: `tools/generate_font.py`, Butano asset pipeline
- Makefile - Build system configuration and dependency management

## Runtime

**Environment:**
- Game Boy Advance hardware (ARM7TDMI, 16.78 MHz)
- 240x160 pixel display, 32KB VRAM, 256KB WRAM, 2MB ROM

**Package Manager:**
- devkitARM toolchain (ARM cross-compiler)
- GNU Make build system
- Lockfile: Not applicable (Makefile-based)

## Frameworks

**Core:**
- Butano Engine (Latest) - Modern C++ high-level GBA engine
  - Purpose: Hardware abstraction, sprite/background management, audio system
  - Location: `butano/butano/` (git submodule)
- ETL (Embedded Template Library) - Standard library replacement
  - Purpose: Heap-free containers, data structures, algorithms
  - Integrated via Butano

**Build/Dev:**
- grit - Graphics conversion tool (bitmap → GBA format)
- mmutil - Audio conversion tool (WAV/IT → GBA format)
- mod2gbt/s3m2gbt - Module tracker conversion for chiptune audio

## Key Dependencies

**Critical:**
- devkitARM - ARM cross-compiler toolchain
  - Purpose: Compile C++ for GBA ARM architecture
  - Required env: DEVKITARM environment variable
- Butano Engine - Game framework
  - Purpose: Hardware abstraction, rendering, input, audio
  - Location: `butano/butano/` submodule

**Infrastructure:**
- Python 3.x with PIL/Pillow - Asset processing
  - Purpose: Font generation, image manipulation
  - Used in: `tools/generate_font.py`
- Wonderful Toolchain (optional) - Alternative to devkitARM
  - Purpose: Modern GBA development toolchain
  - Required env: WONDERFUL_TOOLCHAIN environment variable

## Configuration

**Environment:**
- Primary: devkitARM toolchain
- Alternative: Wonderful Toolchain
- Key configs required: DEVKITARM or WONDERFUL_TOOLCHAIN environment variable

**Build:**
- `Makefile` - Main build configuration
- `butano/butano/butano.mak` - Butano integration
- Auto-detection between devkitARM and Wonderful Toolchain

## Platform Requirements

**Development:**
- Windows/macOS/Linux (devkitARM supported platforms)
- Python 3.x with Pillow library
- GNU Make
- Git with submodule support

**Production:**
- Game Boy Advance hardware
- GBA-compatible flash cart or emulator
- 2MB ROM size limit (standard GBA cartridge)

---

*Stack analysis: 2026-01-24*