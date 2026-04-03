# Architecture

Analysis date: 2026-03-22
Last updated: 2026-04-03

## Scope

The current baseline is a single room-viewer runtime. The old world, combat,
menu, and model-viewer flows are not part of the shipped behavior.

## Runtime Layers

### Boot

- `src/main.cpp` initializes Butano and jumps straight into
  `str::run_room_viewer()`.
- `include/str_scene_room_viewer.h` exposes that single room-viewer entrypoint.

### Room Viewer

- `src/room_viewer.cpp` owns the gameplay loop, movement, collision, doors,
  camera control, NPC interaction, paintings, and minimap coordination.
- `src/core/str_bg_dialog.cpp` and `include/str_bg_dialog.h` provide the fixed
  room-viewer dialog UI.
- `src/core/minimap.cpp` and `include/str_minimap.h` provide the fixed two-room
  minimap.

### 3D Rendering

- `src/viewer/room_renderer.h`
- `src/viewer/room_renderer.cpp`
- `src/viewer/room_renderer.bn_iwram.cpp`

These files are the private room-viewer renderer. They replace the old public
project-local `fr_*` surface and keep only the features the room viewer still
uses.

### Models and Generation

- `scripts/generate_room_shell_header.py` writes the room-shell header to
  `build/generated/include/models/str_model_3d_items_room.h`.
- `include/models/str_model_3d_items_books.h` and
  `include/models/str_model_3d_items_potted_plant.h` stay tracked so decor work
  can be restored or extended later without rebuilding that pipeline first.

## Key Behavior

- The runtime is a two-room interior joined by one doorway.
- Only the current room shell is loaded during normal play; the destination
  shell is also loaded during a door transition.
- The camera is constrained to 8 directions, recenters behind the player after
  a short idle delay, and also recenters on `START`.
- Camera distance auto-fits the active room shell to the viewport.
- Door transitions block movement while active and interpolate the player and
  camera smoothly.
- NPC interaction uses `BgDialog`.
- Paintings are projected as textured quads on the active room walls.
- The minimap mirrors the same two-room layout as the runtime.
- There is no in-game debug overlay or profiler menu in the current baseline.

## Intentional Non-Scope

- No world traversal, combat, quests, or inventory systems.
- No multi-scene boot flow.
- No public project-local 3D renderer API under `include/`.
