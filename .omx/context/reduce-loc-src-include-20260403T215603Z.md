# Deep Interview Context Snapshot

- Task statement: Reduce LOC in `src/` and `include/` as much as possible while preserving current room-viewer functionality.
- Desired outcome: A smaller code footprint with the shipped room-viewer behavior unchanged.
- Stated solution: Refactor and consolidate code in `src/` and `include/` to remove redundancy and unnecessary structure.
- Probable intent hypothesis: Lower maintenance cost and simplify the surviving room-viewer codebase without reopening removed systems.

## Known Facts / Evidence

- The project is a brownfield Butano-based GBA room-viewer-only runtime.
- Grounding docs require using `.planning/codebase/` plus local Butano headers as the behavior source of truth.
- Current validation is manual: build the ROM and verify boot, movement, collision, doors, camera behavior, minimap, and `BgDialog`.
- Largest source/header files under `src/` and `include/` are:
  - `src/viewer/runtime/room_viewer_runtime_systems.cpp` at 1006 lines
  - `src/viewer/room_renderer.bn_iwram.cpp` at 886 lines
  - `include/private/viewer/runtime/room_viewer_runtime_systems_shared.h` at 573 lines
  - `include/private/viewer/str_room_renderer.h` at 541 lines
  - `src/core/dialog/str_bg_dialog.cpp` at 446 lines
  - `src/viewer/room_renderer.cpp` at 417 lines
- Planning docs mark the runtime systems file, runtime state file, minimap layout, dialog, and IWram renderer path as current hotspots.
- Architecture docs explicitly keep scope limited to the room viewer and exclude world, combat, menu, and model-viewer systems.

## Constraints

- Preserve current runtime behavior.
- Prefer small, incremental edits in `src/room_viewer.cpp`, `src/core/`, and `src/viewer/`.
- Keep `.planning/` docs in sync with any runtime behavior or structure changes.
- Avoid reintroducing deleted world, combat, menu, or model-viewer systems.
- There are no automated tests; validation depends on successful build plus emulator checks.

## Unknowns / Open Questions

- Whether LOC reduction should prioritize fewer files, fewer repeated constants/helpers, smaller hot files, or all of the above.
- Whether behavior-preserving internal indirection is acceptable if it improves maintainability but does not sharply reduce total LOC.
- Whether performance-sensitive renderer code may be structurally changed if behavior stays the same.
- Whether generated or model header files are in scope if they live under `include/`.

## Decision-Boundary Unknowns

- Can implementation trade readability or subsystem boundaries for fewer lines?
- Can public/private API boundaries change if runtime behavior does not?
- Can large files be split or should the goal be net LOC reduction only?
- How much build-time or code-generation work is acceptable in pursuit of lower checked-in LOC?

## Likely Touchpoints

- `src/viewer/runtime/room_viewer_runtime_systems.cpp`
- `src/viewer/runtime/room_viewer_runtime_state.cpp`
- `include/private/viewer/runtime/room_viewer_runtime_systems_shared.h`
- `src/core/dialog/str_bg_dialog.cpp`
- `src/core/dialog/str_bg_dialog_text.cpp`
- `include/str_bg_dialog.h`
- `src/viewer/room_renderer.cpp`
- `src/viewer/room_renderer.bn_iwram.cpp`
- `include/private/viewer/str_room_renderer.h`
