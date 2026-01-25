# Technology Stack

**Domain:** GBA chunk streaming with predictive buffer management
**Researched:** 2026-01-24
**Confidence:** HIGH

## Recommended Stack

### Core Framework

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|----------------|
| Butano C++ Framework | 21.0.0 | Modern high-level GBA engine with background map management | Provides built-in background map classes, memory management, and VRAM handling without heap allocations |
| devkitARM Toolchain | r42+ | Official GBA development toolchain with GCC | Industry standard for GBA development, proven stability |
| C++17 Standard | 17 | Language standard supported by Butano | Modern C++ features with RAII and standard library compliance |

### GBA Hardware Management

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|----------------|
| DMA Transfers (GBA) | Hardware | Critical VRAM updates during VBlank/HBlank | Only way to update VRAM without tearing, 10x faster than CPU copies |
| VBlank Interrupts | Hardware | Synchronized frame updates | Prevents VRAM access during drawing, eliminates visual artifacts |
| 16KB VRAM Banks | Hardware | Background and sprite memory organization | Allows separate tile and map regions for streaming |
| 32KB IWRAM | Hardware | Fast code execution | Critical for predictive calculations, 6x faster than EWRAM |

### Buffer Management

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|----------------|
| Circular Buffer Pattern | 2.0 | Chunk streaming with wraparound | Proven GBA pattern for infinite worlds, no allocation overhead |
| Dual Buffer System | 2.0 | Predictive buffer switching | Allows background updates while displaying, eliminates wave artifacts |
| 64-tile/frame Streaming | 2.0 | Controlled VRAM bandwidth | Matches GBA DMA capabilities, maintains 60 FPS |
| Reference Counted Tiles | 2.0 | Memory-efficient tile caching | Prevents unnecessary VRAM writes, extends cache life |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| bn::bg_map_ptr | Butano 21.0.0 | Background map management | Use for all affine and regular backgrounds |
| bn::regular_bg_map_ptr | Butano 21.0.0 | Regular background handling | Standard tile-based backgrounds |
| bn::affine_bg_map_ptr | Butano 21.0.0 | Rotated/scaled backgrounds | For large worlds with rotation effects |
| bn::bg_items | Butano 21.0.0 | Background item management | Manages per-chunk tile data efficiently |
| bn::memory | Butano 21.0.0 | Memory allocation | Custom allocators for tile buffers |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| mGBA Emulator | Testing | Accurate GBA hardware emulation |
| no$gba | GDB Stub | Real-time debugging on hardware |
| VBA-M | Visual Boy Advance | Fast development cycle with save states |

## Installation

```bash
# Core setup (devkitARM + Butano)
sudo pacman -S devkitARM
sudo pacman -S butano

# Project initialization
butano new my_project
cd my_project

# Build configuration (in Makefile)
-include $(BUTANO_ROOT)/butano.mak
```

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-----------|-------------------------|
| Butano Framework | libgba | For raw hardware access without abstractions |
| devkitARM | devkitPro | Commercial GBA development with additional tools |
| DMA Transfers | CPU Manual Copy | When DMA channels are needed for audio/sound |
| Circular Buffer | Linear Streaming | For small, predictable worlds only |
| Predictive Buffer | Static Buffer | For games without scrolling or movement |

## What NOT to Use

| Avoid | Why | Use Instead |
|--------|-----|-------------|
| Heap Allocations in VBlank | Causes frame drops and tearing | Use stack allocation or pre-allocated buffers |
| Direct VRAM Writes During Display | Causes tearing and artifacts | Use DMA during VBlank only |
| 32x32 Background Maps | Exceeds GBA 128x128 tile limit | Use multiple 16x16 maps stitched together |
| Single Buffer Streaming | Causes visible wave artifacts | Use dual buffer predictive system |
| C++ Exceptions | Unpredictable behavior on constrained hardware | Use Butano's no-exception standard library |
| Floating Point Math | Too slow on ARM7TDMI without FPU | Use fixed-point or lookup tables |

## Stack Patterns by Variant

**For predictive buffer management (current project):**
- Use Butano's built-in background map classes with custom item system
- Implement dual buffer system: display buffer + compute buffer
- Reserve DMA channel 0 for critical VBlank VRAM updates
- Keep tile reference counting to minimize VRAM writes
- Pre-allocate 16x16 chunk buffer zones in IWRAM

**For large open worlds:**
- Use Butano's HUGE background map canvas (1024x1024)
- Implement chunk priority system for predictive loading
- Use 4-directional streaming with overlap zones
- Cache edge chunks in faster IWRAM, main chunks in EWRAM

**For memory-constrained projects:**
- Use 4-bit tile mode instead of 8-bit when possible
- Implement tile compression in ROM to reduce cartridge size
- Use Butano's memory pools instead of new/delete
- Limit background layers to 2 instead of 4 when possible

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|----------------|-------|
| Butano 21.0.0 | devkitARM r42+ | Current stable version, no breaking changes |
| bn::bg_* | bn::affine_bg_* | All background components work together seamlessly |
| bn::memory | bn::bg_items | Memory manager designed for background item patterns |
| DMA Channels | Butano background system | Butano reserves DMA0 for internal use, others available |

## Sources

- [Butano Docs - Background Maps](https://gvaliente.github.io/butano/group__bg__map.html) — HIGH confidence (official documentation)
- [GBATEK DMA Transfers](https://problemkaputt.de/gbatek-gba-dma-transfers.htm) — HIGH confidence (hardware reference)
- [Game Boy Advance Architecture](https://www.copetti.org/writings/consoles/game-boy-advance/) — HIGH confidence (authoritative hardware analysis)
- [GBA Resource Management](https://www.gamedeveloper.com/programming/gameboy-advance-resource-management) — MEDIUM confidence (practical algorithms, but dated 2001)
- [devkitARM Documentation](https://devkitpro.org/) — HIGH confidence (official toolchain docs)
- [GBATEK Memory Map](https://problemkaputt.de/gbatek-gba-memory-map.htm) — HIGH confidence (hardware register reference)

---
*Stack research for: GBA chunk streaming with predictive buffer management*
*Researched: 2026-01-24*