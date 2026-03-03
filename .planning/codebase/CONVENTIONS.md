# Coding Conventions

Analysis date: 2026-02-09
Last updated: 2026-03-03

## Naming

- Files: snake_case (`room_viewer.cpp`, `world_state.cpp`).
- Game headers: `str_*.h` prefix.
- 3D override headers: `fr_*.h` prefix.
- Classes/types: PascalCase (`RoomViewer`, `WorldStateManager`).
- Functions/locals: snake_case.
- Private members: leading underscore (`_corner_index`).
- Constants: `SCREAMING_SNAKE_CASE` for compile-time constants.

## Include Style

Common pattern:

1. Matching project header first.
2. Butano `bn_*` headers.
3. Project/engine helpers (`fr_*`, models, shared assets).

Practical rule: keep `include/` definitions authoritative for `fr_*` types.

## Types

- **No `float` or `double`**: GBA has no FPU. Use `bn::fixed` for decimal math.
- **Use Butano containers**: `bn::vector`, `bn::string`, `bn::optional`, `bn::deque`, `bn::unordered_map` instead of STL equivalents. They live on the stack, not the heap.
- **No `sprintf` / `std::ostringstream`**: Use `bn::to_string<N>(value)` or `bn::ostringstream`.

## Memory Management

- **IWRAM** (32KB, fast): default for data and stack. Performance-critical code uses `.bn_iwram.cpp` suffix.
- **EWRAM** (256KB, slow): use for large data with `BN_DATA_EWRAM`. Currently used by `MinimapCanvas` cell storage and `BgDialog` cell storage.
- **No raw `new`/`delete`**: prefer RAII patterns. Exception: viewer scenes (MODEL_VIEWER, ROOM_VIEWER) still use manual heap allocation.
- **No global Butano objects before `bn::core::init()`**.

## Formatting and Control Flow

- 4-space indentation.
- Braces on next line for blocks.
- Keep gameplay loops explicit and branch-readable.
- Use small helper lambdas in scene functions when it reduces repetition.

## Error Handling

- Use `BN_ASSERT(...)` for invalid states and bounds.
- Avoid exception-based flow.
- Prefer deterministic fallbacks over silent failure.
- No heap allocation in hot paths.

## Logging and Debug

- Use Butano logging (`bn::core::log()` / related debug logs) for emulator diagnostics.
- Keep debug HUD text in scene-local code paths.
- Enable `STACKTRACE := true` in Makefile for stack traces (currently enabled).

## Documentation and Comments

- Comment when math, transforms, or hardware constraints are not obvious.
- Keep comments short and behavior-focused.
- Update `.planning/` docs when constants or runtime behavior change.

## Game Constants

All gameplay constants are centralized in `include/str_constants.h`. Key groups:

| Group | Examples |
|-------|---------|
| Map | `TILE_SIZE=8`, `MAP_COLUMNS/ROWS=128` |
| Player | Hitbox 32x16, roll speed 3.75, roll duration 64, attack durations |
| Combat | Bullet speed 4, lifetime 60, cooldown 15, knockback 3.5 |
| Companion | Idle dist 12, revival duration 300 frames |
| Camera | Deadzone 16x10, follow speed 0.06, lookahead 36x24 |
| HUD | Max HP 3, max ammo 10, max energy 3, buff cooldown 600 |
| Minimap | 6 rooms, 2x3 grid, room size 16px, gap 2px |
| Zoom | Normal 1.0, out 0.6, transition speed 0.1 |
| Characters | Hero (id=0, balanced), Soldier (id=1, ranged) |

## Current Hotspots to Handle Carefully

- `src/core/world.cpp`: large control loop; changes should be incremental.
- `src/room_viewer.cpp`: camera/view transform logic and input mapping are tightly coupled.
- `src/viewer/fr_models_3d.bn_iwram.cpp`: performance-critical path.
