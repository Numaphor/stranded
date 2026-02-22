# Architecture

Analysis date: 2026-02-09
Last updated: 2026-02-22

## High-level Pattern

- Scene-driven application flow from `src/main.cpp`.
- Each scene exposes `execute()` and returns the next `str::Scene` value.
- Core gameplay lives in the world scene (`src/core/world.cpp`).
- Specialized 3D scenes (`room_viewer`, `model_viewer`) use the custom viewer pipeline in `src/viewer/`.

## Layered View

### Presentation Layer

- Files: `src/core/scenes.cpp`, `src/core/model_viewer.cpp`, `src/core/room_viewer.cpp`
- Responsibilities: input routing, UI text/sprites, scene-local rendering behavior.

### Gameplay Layer

- Files: `src/actors/*`, `src/core/world.cpp`, `src/core/collision.cpp`, `src/core/movement.cpp`, `src/core/quest.cpp`
- Responsibilities: movement, combat, AI, interactions, progression.

### State Layer

- Files: `src/core/world_state.cpp`, `include/str_world_state.h`
- Responsibilities: persistent world/session state between scene transitions.

### Engine Integration Layer

- Files: `src/viewer/*`, `include/fr_*.h`, Butano submodule
- Responsibilities: rendering primitives, fixed-point math, hardware abstraction.

## Scene Flow

Current scene loop in `src/main.cpp`:

1. `START`
2. `CHARACTER_SELECT`
3. `CONTROLS`
4. `MENU`
5. `WORLD`
6. Optional viewers: `MODEL_VIEWER`, `ROOM_VIEWER`

The loop creates a scene object, executes it, reads the returned scene enum, and switches again.

## Room Viewer Architecture

- Scene entrypoint: `str::RoomViewer::execute()`.
- Models: room shell + optional decor (table/chair), created as dynamic `fr::model_3d` objects.
- Player: `fr::sprite_3d` using Eris sprite sheet.
- Camera: fixed phi (`0`), adjustable distance via `L/R`.
- Navigation: six connected rooms in a 2x3 grid with door transitions.
- Corner view switching:
  - `START` triggers a smooth corner transition.
  - Transition interpolates a view angle and updates model rotation each frame.
  - Movement is paused during transition, then `_corner_index` advances and facing is remapped.

## Important Abstractions

- `str::Scene` enum for scene switching.
- `fr::model_3d` and `fr::sprite_3d` for 3D model/sprite projection.
- `fr::models_3d` as scene-local 3D object manager.
- `WorldStateManager` for shared progress/state.

## Current Architectural Risks

- `world.cpp` remains a large, multi-responsibility execution loop.
- Scene objects are mostly stack-owned, but viewers still use manual `new/delete` in `src/main.cpp`.
- Automated project-level tests are not yet wired into the workflow.
