# Coding Conventions

Analysis date: 2026-03-22
Last updated: 2026-03-22

## Naming

- Files use snake_case, for example `room_viewer.cpp`.
- Project headers use the `str_*.h` prefix.
- 3D override headers use the `fr_*.h` prefix.
- Types use PascalCase.
- Functions and locals use snake_case.
- Private members use a leading underscore.
- Compile-time constants use `SCREAMING_SNAKE_CASE`.

## Include Style

1. Include the matching project header first.
2. Include Butano `bn_*` headers next.
3. Include project 3D helpers and generated model headers last.

## Types and Memory

- Avoid `float` and `double`; use `bn::fixed` for decimal math.
- Prefer Butano containers such as `bn::vector`, `bn::string`, and
  `bn::optional`.
- Prefer RAII and deterministic ownership.
- Avoid heap allocation in hot paths.

## Formatting

- Use 4-space indentation.
- Put braces on the next line.
- Keep control flow easy to read.
- Keep comments short and behavior-focused.

## Error Handling and Logging

- Use `BN_ASSERT(...)` for invalid states and bounds.
- Prefer deterministic fallbacks over silent failure.
- Use Butano logging for emulator diagnostics when needed.

## Documentation

- Update `.planning/` when room-viewer behavior or build assumptions change.
- Keep `include/str_constants.h` as the home for room, camera, and interaction
  constants.

## Hot Spots

- `src/room_viewer.cpp`
- `src/core/minimap.cpp`
- `src/viewer/fr_models_3d.bn_iwram.cpp`
