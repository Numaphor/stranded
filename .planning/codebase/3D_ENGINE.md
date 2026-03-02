# 3D Engine Reference

Last updated: 2026-03-01

## Scope

Stranded uses the varooom-3d pipeline through project-controlled code paths:

- Runtime sources: `src/viewer/`
- Project header overrides: `include/fr_*.h`
- Upstream baseline: `butano/games/varooom-3d/`

The Butano submodule stays clean; project behavior changes are done in project files.

## Coordinate System

```text
Engine X -> screen horizontal axis
Engine Y -> depth axis (camera looks toward -Y)
Engine Z -> screen vertical axis
```

Camera defaults:

- Position: `(0, 256, 0)`
- Phi: configurable; room viewer uses `0`
- Screen center used by projection: `(120, 80)`

## Transform Pipeline

For a model vertex:

1. Rotate by model matrix (`fr::model_3d` internal matrix).
2. Scale by model scale.
3. Translate by model position.
4. Convert to camera space.
5. Perspective project to screen.
6. Depth-sort and rasterize.

Order is rotate -> scale -> translate.

## Runtime Limits

- Max vertices: `100`
- Max faces: `300`
- `RoomViewer` applies a runtime budget check and skips optional dynamic models when adding them would exceed `100` vertices.

## Project Extensions (Important)

### `include/fr_model_3d.h`

- Supports direct matrix assignment via `set_rotation_matrix(...)`.
- Adds `depth_bias()` and `set_depth_bias(int)`.
- Adds layering mode support used by room shell rendering (`layering_mode::room_perspective`).

### `include/fr_sprite_3d.h`

- Adds horizontal flip controls to avoid negative-scale flip hacks.

### `include/fr_sprite_3d_item.h`

- Extends sprite sizing metadata for 8/16/32/64 pixel workflows.

### `src/viewer/fr_models_3d*.cpp`

- Contains camera-space transforms, projection, depth sorting, and sprite_3d render submission.
- IWram variant handles hot loops.

## Room Viewer Isometric Setup

`src/room_viewer.cpp` uses fixed angles:

- `iso_phi = 6400`
- `iso_theta = 59904`
- `iso_psi = 6400`

A base corner matrix is derived from these angles. The view angle follows committed movement heading plus a behind-offset and is quantized into quarter turns for `_corner_index`.

## Corner Handling and Camera Follow (Current)

- Quarter turn angle: `QUARTER_TURN_ANGLE = 16384`.
- Render refresh threshold: `RENDER_UPDATE_ANGLE_STEP = 64`; orientations/paintings update when the view angle moves by this step.
- View angle source: committed movement heading plus `CAMERA_BEHIND_OFFSET_ANGLE = 24576` with easing gains and max step clamps; `_corner_index` derives from quantized view angle instead of a discrete transition coroutine.
- Camera distance: adjustable via `L/R`, clamped `100–500`; `START` (without `SELECT`) recenters toward committed heading with a short boost (`CAMERA_START_BOOST_FRAMES = 10`).
- Movement continues while turning corners; only door transitions pause movement.

## Door Transition (Current)

- Duration: `DOOR_TRANSITION_DURATION_FRAMES = 16` with smoothstep easing.
- Interpolates player/global position and anchor; preloads decor/models for the target room; movement input blocked during the transition.
- Depth bias applied to transition decor to prevent Z-fighting; furniture reloaded after swap.

See `.planning/features/ROOM_VIEWER_CORNER_TRANSITIONS.md` for corner/turn behavior and tuning notes.

## Build Include Order

`Makefile` include order keeps project overrides first:

- `include`
- `butano/common/include`
- `butano/games/varooom-3d/include`

This ensures `#include "fr_model_3d.h"` resolves to the project override.
