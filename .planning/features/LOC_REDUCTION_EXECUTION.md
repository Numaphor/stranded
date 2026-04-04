# LOC Reduction Execution Note

Last updated: 2026-04-04

## Source of Truth

- Execution plan: `.omx/plans/ralplan-reduce-loc-src-include.md`
- Context snapshot: `.omx/context/reduce-loc-src-include-20260403T215603Z.md`
- Preservation spec: `.omx/specs/deep-interview-reduce-loc-src-include.md`

## Goal

Reduce `src/` + `include/` LOC well beyond the initial formatting-heavy pass while
keeping the shipped room-viewer contract intact:

1. Boot lands in the room viewer.
2. Movement and collision still work.
3. Door transitions still work.
4. Camera recentering and auto-fit distance still work.
5. The minimap still updates correctly.
6. `BgDialog` still displays and advances correctly.

## Review Summary

The highest-yield remaining LOC targets are still concentrated in a small set of
runtime and renderer-adjacent files:

- `src/viewer/runtime/room_viewer_runtime_systems.cpp`
- `include/private/viewer/runtime/room_viewer_runtime_systems_shared.h`
- `include/private/viewer/str_room_renderer.h`
- `src/viewer/room_renderer.cpp`
- `src/viewer/room_renderer.bn_iwram.cpp`
- `src/core/dialog/str_bg_dialog.cpp`

The approved plan keeps the largest behavior risk behind staged gates: collapse
support modules first, localize shared helpers second, and shrink the renderer
API surface last. The IWRAM renderer translation unit stays physically separate.

## Gate Summary

### Gate A: Support-module collapse

- Fold runtime-state constants and dialog helpers into
  `src/viewer/runtime/room_viewer_runtime_systems.cpp`.
- Fold minimap layout helpers into `src/core/minimap/minimap.cpp`.
- Fold dialog text helpers into `src/core/dialog/str_bg_dialog.cpp`.
- Delete the now-redundant support files only after the merged build passes.

### Gate B: Shared helper localization and pruning

- Move runtime-only helpers and constants out of
  `include/private/viewer/runtime/room_viewer_runtime_systems_shared.h` when a
  narrower home is available.
- Do a move-only compile pass before pruning dead helpers.

### Gate C: Renderer-safe API compression

- Shrink `include/private/viewer/str_room_renderer.h` to the surface the room
  viewer runtime still consumes.
- Keep `src/viewer/room_renderer.cpp` and
  `src/viewer/room_renderer.bn_iwram.cpp` separate.

## Verification Expectations

At each gate, capture fresh evidence for:

- `make -j4`
- `make -B -j4`
- LOC measurement across `src/` and `include/`
- include-edge sanity for touched headers
- manual room-viewer validation for the preserved behavior contract
- native mGBA `F12` screenshot parity for any visual-affecting change

## Documentation Sync Points

Refresh the stable `.planning/codebase/` references after the code lands:

- `ARCHITECTURE.md` and `STRUCTURE.md` after Gate A removes support-module
  file boundaries.
- `CONVENTIONS.md` after the hot-spot list changes.
- `QUALITY_AND_TESTING.md` if the verification loop or screenshot workflow
  changes materially.

Do not document a deeper renderer-file merge unless a separate ADR explicitly
re-approves it after Gate C.
