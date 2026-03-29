# Architecture

Analysis date: 2026-03-22
Last updated: 2026-03-30

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
- `scripts/generate_room_shell_header.py` regenerates the simplified room shell
  models used by the viewer.
- `scripts/render_player_preview_assets.py` and
  `scripts/build_voxel_player_assets.py` generate the player sprite sheets
  offline, with the voxel export path available as an optional alternative
  using a grounded multi-mesh bake and a preview-like pitched camera target.
- `scripts/blender_render_static_prop_frames.py` and
  `scripts/build_interior_prop_assets.py` bake static Interior-pack room props
  into the same sharp sprite-sheet format used by the player.

### 3D Runtime

- `src/viewer/` contains the shared 3D runtime adapted from Butano's
  varooom-3d example.
- `include/fr_*.h` contains project overrides for the 3D runtime types.
- `include/models/` contains generated model headers used by the room viewer.

## Key Behavior

- The room viewer uses a fixed 60-degree top-down presentation with an
  eight-direction camera that turns quickly through each intermediate heading
  toward the player's facing direction.
- Player walking and door transitions use capped carried catch-up budgets so
  the 60 FPS feel stays stable while brief missed-frame bursts do not turn into
  sticky locomotion or visibly chunky room-transition jumps.
- Only the active room is rendered outside active door transitions, and
  camera-facing shell surfaces are culled so the interior stays visible.
- Floors and walls remain true room-shell `model_3d` geometry rendered through
  the existing world path, but their generated shell shapes and colors now aim
  at a simplified Maria-style interior look.
- Room decor still uses the current small hard-coded 3D model set at runtime,
  while the new Interior-pack bake scripts prepare sprite assets for a later
  room-prop integration pass.
- After 1 second of no input, the camera recenters behind the player's
  current facing direction, except while the player is near the room center.
- `START` recenters the camera behind the player's current facing direction.
- `L` and `R` adjust camera distance within the supported range.
- Door transitions block movement while active and use a smoothed carried
  frame budget instead of consuming every missed frame in one visible jump.
- NPC interaction uses `BgDialog` so the room viewer can keep the sprite budget
  under control.
- The minimap and room decorations are updated from the same room-viewer
  state.

## Intentional Non-Scope

- No world, combat, quest, or model-viewer systems are documented here.
- No multi-scene menu flow remains in the current baseline.
