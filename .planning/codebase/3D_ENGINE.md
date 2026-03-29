# 3D Engine Reference

Last updated: 2026-03-29

## Scope

This note covers the shared 3D runtime used by the room viewer:

- `src/viewer/`
- `include/fr_*.h`
- `include/models/`

The Butano submodule remains the upstream source of truth for the base runtime
behavior.

## Coordinate System

```text
Engine X -> screen horizontal axis
Engine Y -> depth axis
Engine Z -> screen vertical axis
```

## Transform Pipeline

The runtime follows the usual model path:

1. Rotate by the model matrix.
2. Scale by the model scale.
3. Translate by the model position.
4. Convert to camera space.
5. Project to screen space.
6. Depth-sort and render.

## Runtime Limits

- Max vertices: `256`
- Max faces: `300`

The room viewer keeps within these limits by loading only the models and
decorations needed for the current room state.

## Project Overrides

- `include/fr_model_3d.h` adds direct matrix control and depth bias support.
- `include/fr_sprite_3d.h` adds explicit horizontal flip controls.
- `include/fr_sprite_3d_item.h` carries the sprite sizing metadata used by the
  viewer runtime.
- `include/fr_models_3d.h` and `include/fr_shape_groups.h` expose the project
  interfaces for the shared runtime.

## Room Viewer Usage

- The room viewer uses a fixed 60-degree top-down floor tilt across every
  quarter-turn rotation.
- Wide scanline-rendered faces reserve a stable number of HDMA sprite slots per
  visible row so per-row split changes do not reorder overlapping geometry into
  horizontal bands.
- The player sprite uses a 64x64 sheet baked offline from the Blender preview
  renders by default, with the voxel-derived bake path kept as an optional
  offline asset-generation route that now bakes all imported mesh parts and
  projects them with a grounded, preview-like pitched camera target.
- The camera is constrained to 8 directions and turns quickly through each
  intermediate heading instead of teleporting to the target angle.
- After 1 second of no input, the camera recenters behind the player's
  current facing direction, except while the player is near the room center.
- `START` recenters the camera behind the player's current facing direction.
- `L` and `R` adjust camera distance.
- Door transitions run for a fixed number of frames and block movement while
  active.
- `BgDialog` is used for NPC interaction in the current baseline.

## Build Include Order

`Makefile` keeps project overrides first so `#include "fr_model_3d.h"` resolves
to the project header before the upstream copy.
