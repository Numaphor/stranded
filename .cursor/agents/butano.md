---
name: butano
model: inherit
description: This is a Game Boy Advance game built with the **Butano** engine (C++23, devkitARM toolchain).
---

## Environment

- **Toolchain**: devkitARM (devkitPro) — installs to `C:\devkitPro\` (Windows) or `/opt/devkitpro/` (Linux/macOS)
- **Language**: C++23, targeting ARM7TDMI (GBA CPU). No FPU, no OS, 16.78 MHz.
- **Python**: Required for asset processing. Must be on PATH.
- **Emulators** (for testing): mGBA (primary), NanoBoyAdvance, Mesen, No$gba (debug version)

## Project Structure

```
project/
├── Makefile            # Build config — LIBBUTANO path, USERFLAGS, SOURCES, etc.
├── include/            # .h / .hpp headers
├── src/                # .cpp source files
├── graphics/           # .bmp images + .json asset descriptors
├── audio/              # .mod .xm .s3m .it .wav music/sfx files
├── dmg_audio/          # .mod .s3m .vgm DMG music files
├── build/              # Generated — compiled objects, generated bn_*_items.h headers
└── *.gba              # Output ROM
```

Subfolders under `src/` or `graphics/` must be added to the Makefile manually (`SOURCES := src src/subfolder`).

## Build Commands

```sh
# Standard build (replace N with CPU core count)
make -jN

# Full rebuild (REQUIRED after any Makefile change)
make clean && make -jN

# The output is <project_name>.gba in the project root
```

There is no `cmake`, no `ninja`, no package manager. The Makefile is the single source of truth for build configuration.

## Testing

There is no test framework. Verification means:

1. Build the ROM (`make -jN`)
2. Run the `.gba` file in an emulator (mGBA recommended)
3. For crashes, enable `STACKTRACE := true` in Makefile, rebuild, reproduce crash, read emulator log window. Demangle symbols at demangler.com.
4. For hardware rendering bugs, use No$gba debug version with its exception system.

## Key Constraints (quick reference)

These are the constraints agents violate most often. Full details are in `.cursor/rules/butano-gba.mdc`.

- **No `float`/`double`** → use `bn::fixed`
- **No `std::vector`/`std::string`** → use `bn::vector<T, MaxSize>`, `bn::string<MaxSize>`
- **No `sprintf`** → use `bn::to_string<N>()` or `bn::ostringstream`
- **No heap if avoidable** → Butano containers live on the stack
- **No globals before `bn::core::init()`** → use struct + pointer pattern
- **Include headers explicitly** — "incomplete type" = missing `#include`
- **Coordinates are screen-center relative**, not top-left
- **`make clean` after every Makefile edit** — always

## Adding Assets

**Sprites/Backgrounds:**
1. Create indexed-color BMP (use Usenti for best compatibility, 15BPP palette)
2. Place in `graphics/` with a matching `.json` descriptor
3. Build — Butano generates `bn_sprite_items_<name>.h` (or `bn_regular_bg_items_<name>.h`) in `build/`
4. Use in code: `#include "bn_sprite_items_<name>.h"` → `bn::sprite_items::<name>.create_sprite(x, y)`

**Music:**
1. Place `.mod`/`.xm`/`.s3m`/`.it` file in `audio/`
2. Build — generates `bn_music_items_<name>.h`
3. Use: `bn::music_items::<name>.play()`

**Sound effects:**
1. Place `.wav` in `audio/` (must be short — Maxmod limitation)
2. Build — generates `bn_sound_items_<name>.h`
3. Use: `bn::sound_items::<name>.play()`

## GBA Hardware Budget

| Resource | Limit |
|---|---|
| IWRAM (fast RAM) | 32 KB |
| EWRAM (slow RAM) | 256 KB |
| Sprite colors | 256 total (shared across all sprites) |
| Background colors | 256 total (shared across tiled backgrounds) |
| Regular backgrounds | 4 simultaneous |
| Affine backgrounds | 2 simultaneous |
| Screen resolution | 240 × 160 px |
| Target framerate | 60 FPS |

## Code Style

- Prefer `bn::` types over `std::` equivalents everywhere
- Use `bn::optional<T>` for load/unload lifecycle of sprites and backgrounds
- Use `constexpr` for read-only data (places it in ROM, saves RAM)
- Use `BN_DATA_EWRAM` for large mutable data that doesn't need fast access
- Use `BN_CODE_IWRAM` + `.bn_iwram.cpp` files for performance-critical functions
- Keep unique sprite z-orders minimal (reduces sort layer pressure)