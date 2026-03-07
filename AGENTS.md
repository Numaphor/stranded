# AGENTS.md

## Project Overview

**Stranded** is a GBA homebrew game built with the **Butano** C++ engine. The build produces a `.gba` ROM file. There are no automated tests -- validation is done by building the ROM and running it in an emulator.

## Grounding (mandatory)

Before modifying any code you **must** ground yourself using these three sources:

1. **`.planning/`** -- read `.planning/codebase/` files (ARCHITECTURE.md, STRUCTURE.md, CONVENTIONS.md, QUALITY_AND_TESTING.md as relevant). This is the authoritative project-level context.
2. **`/butano`** -- browse `butano/butano/include/` headers for the authoritative Butano API surface. The submodule is the single source of truth for types, signatures, and constraints. Do not guess API behaviour.
3. **Web** -- consult the [Butano docs](https://gvaliente.github.io/butano/) or GitHub issues when the local headers are insufficient (migration notes, known bugs, etc.).

Detailed rules live in `.agents/rules/`:

| File | Purpose |
|------|---------|
| `.agents/rules/butano-gba.md` | Full Butano GBA development guide |
| `.agents/rules/codebase-documentation.md` | How to use `.planning/` and project references |

---

## Build Commands

### Standard Build

```bash
export WONDERFUL_TOOLCHAIN=/opt/wonderful
export PATH="/opt/wonderful/bin:/opt/wonderful/toolchain/gcc-arm-none-eabi/bin:$PATH"
make -j$(nproc)
```

The output ROM is named after the workspace directory (e.g., `workspace.gba` in Cloud, `stranded.gba` locally).

### Clean Build

```bash
make clean && make -j$(nproc)
```

### Lint / Static Analysis

There is no linter or static analysis configured. The compiler warnings from `make` serve as the primary code quality check.

---

## Testing

There are **no automated tests**. Validation requires building the ROM and running it in an emulator.

### Manual Testing Checklist

After gameplay or rendering changes, manually verify:

1. **Core flow**: Start -> character select -> menu -> world -> return
2. **Movement and combat**: Movement, attacks, dodges, weapon switching
3. **NPC interaction**: Merchant trades, dialogue progression
4. **Room viewer**: Navigation, collision, door transitions
5. **Model viewer**: Open/close, camera controls

### Running the ROM (Canonical AI E2E Workflow)

- **Use mGBA Qt** at `tools/mGBA-0.10.5-win64/mGBA.exe` for local AI bugfix validation.
- **Always capture screenshots with mGBA native `F12`** (never browser/desktop screenshot tooling).
- Canonical one-shot command (build + launch + F12 capture):

```powershell
powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1
```

- Fast repeat capture without rebuilding:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1 -SkipBuild
```

- The script prints the absolute path of the new screenshot (`stranded-<n>.png`).

### Emulator Key Bindings (mGBA Qt defaults)

| Key | GBA Button |
|-----|-----------|
| X | A |
| Z | B |
| Arrow keys | D-Pad |
| Enter | Start |
| Backspace | Select |
| A | L shoulder |
| S | R shoulder |

### Screenshot Evidence Rules (Mandatory)

- Visual bugfix claims must be grounded in actual screenshot inspection.
- Use direct screenshot vision as the only analysis method.
- Do not infer visual correctness from file size, byte count, PNG metadata, or filename patterns.
- Do not use browser screenshots, Playwright screenshots, or desktop capture substitutes when validating game rendering.
- Use `stranded-<n>.png` output from mGBA `F12` as the source of truth.

### GDB Debugging (mGBA)

```bash
# Terminal 1: launch mGBA with GDB server on port 2345
bash scripts/launch_debug.sh gdb

# Terminal 2: connect debugger with GBA-specific commands
gdb-multiarch -x gba-debug.gdb stranded.elf
```

Or use VS Code: run the **"Attach debugger (mGBA GDB server)"** launch configuration, which builds, starts mGBA with GDB, and attaches the C/C++ debugger with breakpoints/stepping/watch.

Custom GDB commands (type `help-gba`): `oam-visible`, `affine-all`, `gba-status`, `oam-entry N`, `affine-entry N`, and more.

- ELF with debug symbols: `stranded.elf`
- For best debugging, build with: `make -j8 USERFLAGS=-Og USERLDFLAGS=`
- **mGBA GDB uses port 2345** (configurable in `.vscode/settings.json` as `stranded.gdbServerAddress`)

### mGBA Advanced Debugging

- **CLI debugger**: `bash scripts/launch_debug.sh cli` - breakpoints, watchpoints, memory inspect, event trace
- **Verbose logging**: `bash scripts/launch_debug.sh log` - all log levels including SWI (`BN_LOG` output)
- **Lua scripting**: Load `scripts/gba_debug.lua` via mGBA > Tools > Scripting for live OAM/affine/IO inspection
- **Built-in viewers**: mGBA > Tools menu provides Sprite, Tile, Map, Palette, I/O, and Memory viewers

See `.planning/features/MGBA_DEBUG_GUIDE.md` for the full reference.

---

## Code Style Guidelines

### Naming Conventions

- **Files**: snake_case (`room_viewer.cpp`, `world_state.cpp`)
- **Game headers**: `str_*.h` prefix
- **3D override headers**: `fr_*.h` prefix
- **Classes/types**: PascalCase (`RoomViewer`, `WorldStateManager`)
- **Functions/locals**: snake_case
- **Private members**: leading underscore (`_corner_index`)
- **Constants**: SCREAMING_SNAKE_CASE for compile-time constants

### Include Style

Order includes:
1. Matching project header first
2. Butano `bn_*` headers
3. Project/engine helpers (`fr_*`, models, shared assets)

Example:
```cpp
#include "str_player.h"
#include <bn_sprite_ptr.h>
#include <bn_camera.h>
#include "fr_models.h"
```

### Formatting

- 4-space indentation
- Braces on next line for blocks
- Keep gameplay loops explicit and branch-readable
- Use small helper lambdas in scene functions when it reduces repetition

### Types

- **No `float` or `double`**: GBA has no FPU. Use `bn::fixed` for decimal math.
- **Use Butano containers**: `bn::vector`, `bn::string`, `bn::optional`, `bn::deque`, `bn::unordered_map` instead of STL equivalents. They live on the stack, not the heap.
- **No `sprintf` / `std::ostringstream`**: Use `bn::to_string<N>(value)` or `bn::ostringstream`.

### Error Handling

- Use `BN_ASSERT(...)` for invalid states and bounds
- Avoid exception-based flow
- Prefer deterministic fallbacks over silent failure
- No heap allocation in hot paths

### Memory Management

- **IWRAM** (32KB, fast) -- default for data and stack
- **EWRAM** (256KB, slow) -- use for large data with `BN_DATA_EWRAM`
- **No raw `new`/`delete`** -- use RAII patterns
- **No global Butano objects before `bn::core::init()`**

### Logging and Debug

- Use `bn::core::log()` for emulator diagnostics
- Keep debug HUD text in scene-local code paths
- Enable `STACKTRACE := true` in Makefile for stack traces

### Documentation

- Comment when math, transforms, or hardware constraints are not obvious
- Keep comments short and behavior-focused
- Update `.planning/` docs when constants or runtime behavior change

---

## Hotspots (Handle Carefully)

- `src/core/world.cpp`: Large control loop (500+ lines); changes should be incremental
- `src/core/room_viewer.cpp`: Camera/view transform logic tightly coupled
- `src/viewer/fr_models_3d.bn_iwram.cpp`: Performance-critical path

---

## Toolchain Notes

- Wonderful Toolchain: `/opt/wonderful` with env var `WONDERFUL_TOOLCHAIN`
- `wf-pacman` packages: `wf-tools`, `target-gba`, `blocksds-toolchain`
- `butano/` is a git submodule -- run `git submodule update --init --recursive` if empty
- Python required (provided by `python-is-python3`)

---

## Local E2E Testing

See `.agents/skills/stranded-vision-e2e-testing/SKILL.md` for detailed local testing instructions on Windows ARM64.

Default rule: use full screenshot-grounded E2E validation before claiming visual fixes.
