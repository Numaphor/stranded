# Coding Conventions

Analysis date: 2026-02-09
Last updated: 2026-02-22

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

1. matching project header first
2. Butano `bn_*` headers
3. project/engine helpers (`fr_*`, models, shared assets)

Practical rule: keep `include/` definitions authoritative for `fr_*` types.

## Formatting and Control Flow

- 4-space indentation.
- Braces on next line for blocks.
- Keep gameplay loops explicit and branch-readable.
- Use small helper lambdas in scene functions when it reduces repetition.

## Error Handling

- Use `BN_ASSERT(...)` for invalid states and bounds.
- Avoid exception-based flow.
- Prefer deterministic fallbacks over silent failure.

## Logging and Debug

- Use Butano logging (`bn::core::log()` / related debug logs) for emulator diagnostics.
- Keep debug HUD text in scene-local code paths.

## Documentation and Comments

- Comment when math, transforms, or hardware constraints are not obvious.
- Keep comments short and behavior-focused.
- Update `.planning/` docs when constants or runtime behavior change.

## Current Hotspots to Handle Carefully

- `src/core/world.cpp`: large control loop; changes should be incremental.
- `src/core/room_viewer.cpp`: camera/view transform logic and input mapping are tightly coupled.
- `src/viewer/fr_models_3d.bn_iwram.cpp`: performance-critical path.
