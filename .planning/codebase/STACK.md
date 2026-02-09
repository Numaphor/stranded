# Technology Stack

**Analysis Date:** 2026-02-09

## Languages

**Primary:**
- C++23 - Core game logic and engine integration (Game Boy Advance development)

**Secondary:**
- Python 3 - Asset processing, build tools, font generation
- JavaScript - Development workflows and emulator launching

## Runtime

**Environment:**
- Game Boy Advance (ARM7TDMI CPU, 16.78MHz)
- Cross-compilation target via Wonderful Toolchain/devkitARM

**Package Manager:**
- wf-pacman (Wonderful Toolchain package manager)
- Lockfile: Not applicable (embedded target)

## Frameworks

**Core:**
- Butano C++ Engine - Modern high-level GBA development engine
- Wonderful Toolchain - ARM cross-compilation toolchain

**Testing:**
- Not detected (embedded system - manual/emulator testing)

**Build/Dev:**
- Make - Build system with Butano integration
- mGBA emulator - Primary testing/development emulator
- Docker - Containerized development environment

## Key Dependencies

**Critical:**
- Butano Engine (git submodule) - Game engine framework
- Wonderful Toolchain - ARM cross-compiler and toolchain
- ETL (Embedded Template Library) - Standard library replacement used by Butano

**Infrastructure:**
- PIL (Python Imaging Library) - Font and asset generation
- Node.js - Development workflow scripts

## Configuration

**Environment:**
- Wonderful Toolchain environment variables
- Cross-compilation GCC toolchain path
- Makefile-based configuration

**Build:**
- `Makefile` - Primary build configuration
- Butano toolchain integration via `butano.mak`
- Asset processing via Butano graphics/audio tools

## Platform Requirements

**Development:**
- Windows/macOS/Linux (host OS)
- Python 3 with PIL
- Node.js (for development scripts)
- Docker (optional for containerized environment)

**Production:**
- Game Boy Advance hardware or compatible emulator
- 32KB IWRAM + 256KB EWRAM memory constraints
- ROM size limitations

---

*Stack analysis: 2026-02-09*