# Coding Conventions

Analysis date: 2026-03-22
Last updated: 2026-04-03

## Naming

- Files use snake_case, for example `room_viewer.cpp`.
- Project headers use the `str_*.h` prefix.
- Types use PascalCase.
- Functions and locals use snake_case.
- Private members use a leading underscore.
- Compile-time constants use `SCREAMING_SNAKE_CASE`.

## Include Style

1. Include the matching project header first.
2. Include Butano `bn_*` headers next.
3. Include room-viewer support headers, generated model headers, and private
   renderer headers last.

## Types and Memory

- Avoid `float` and `double`; use `bn::fixed` for decimal math.
- Prefer Butano containers and value types.
- Prefer deterministic ownership.
- Avoid heap allocation in hot paths.

## Documentation

- Update `.planning/` when runtime behavior or build assumptions change.
- Keep the room-shell generator path and the generated include path documented
  accurately.

## Hot Spots

- `src/viewer/runtime/room_viewer_runtime_systems.cpp`
- `src/viewer/runtime/room_viewer_runtime_state.cpp`
- `src/core/minimap/minimap_layout.cpp`
- `src/core/dialog/str_bg_dialog.cpp`
- `src/viewer/room_renderer.bn_iwram.cpp`
