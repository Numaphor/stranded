# Architecture

Analysis date: 2026-03-22
Last updated: 2026-03-22

## Scope

The current runtime is a single room-viewer flow booted from `src/main.cpp`.
The old gameplay, menu, and model-viewer stacks are no longer part of the
documented baseline.

## Runtime Layers

### Boot and Scene Entry

- `src/main.cpp` sets up the game and enters the room viewer.
- `include/str_scene.h` and `include/str_scene_room_viewer.h` define the
  surviving scene surface.

### Room Viewer Presentation

- `src/room_viewer.cpp` owns the room-viewer loop, camera behavior, room
  transitions, and rendering coordination.
- `include/str_bg_dialog.h` provides the room dialog flow used for NPC text.
- `src/core/minimap.cpp` and `include/str_minimap.h` provide the room minimap.
- `include/str_constants.h` holds the room, camera, and interaction constants.

### 3D Runtime

- `src/viewer/` contains the shared 3D runtime adapted from Butano's
  varooom-3d example.
- `include/fr_*.h` contains project overrides for the 3D runtime types.
- `include/models/` contains generated model headers used by the room viewer.

## Key Behavior

- The room viewer uses a fixed isometric presentation with committed-heading
  camera follow.
- `START` recenters the camera toward the current heading.
- `L` and `R` adjust camera distance within the supported range.
- Door transitions are time-based and block movement while the transition runs.
- NPC interaction uses `BgDialog` so the room viewer can keep the sprite budget
  under control.
- The minimap and room decorations are updated from the same room-viewer
  state.

## Intentional Non-Scope

- No world, combat, quest, or model-viewer systems are documented here.
- No multi-scene menu flow remains in the current baseline.
