# 3D Engine Reference

Last updated: 2026-03-30

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

The room viewer keeps within these limits by loading only the active room shell
plus the transition-room shell when needed, while room decor stays within a
small dynamic-model budget.

## Project Overrides

- `include/fr_model_3d.h` adds direct matrix control and depth bias support.
- `include/fr_sprite_3d.h` adds explicit horizontal flip controls.
- `include/fr_sprite_3d_item.h` carries the sprite sizing metadata used by the
  viewer runtime.
- `include/fr_models_3d.h` and `include/fr_shape_groups.h` expose the project
  interfaces for the shared runtime.
- `scripts/generate_room_shell_header.py` emits the simplified room-shell
  header in `include/models/str_model_3d_items_room.h`.
- `scripts/blender_render_static_prop_frames.py` and
  `scripts/build_interior_prop_assets.py` bake static Interior-pack props into
  Butano-ready sprite sheets under `graphics/sprite/interior_props/`.

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
- Static Interior-pack props can now be baked to sharp 8-view sprite sheets by
  the offline asset pipeline, ready for future `sprite_3d` room-prop
  integration.
- Floors and walls stay on the existing room-shell `model_3d` path. The
  current shell generator keeps the original renderer contract intact by using
  floor color slots `0..5` and shell color slots `6..8`, while adding a denser
  parquet-like floor split and a wainscot-plus-wall banding treatment.
- Room-shell doorway openings are generated from the same aligned door-center
  math used by `src/room_viewer.cpp` for transitions, and the wall band above
  each door stays filled with the normal wall color so the opening top reads as
  part of the same wall surface.
- In room-viewer local space, `north` is the hidden front wall at `Y = -half`
  and `south` is the visible back wall at `Y = +half`; the shell generator must
  keep that same convention so visible exits match both collision and minimap
  directions.
- The camera is constrained to 8 directions and turns quickly through each
  intermediate heading instead of teleporting to the target angle.
- Player walking and door transitions use capped carried missed-frame catch-up
  budgets so room-viewer movement speed still tunes by feel without losing
  distance to brief heavy-room spikes, and door transitions avoid visibly
  chunky interpolation jumps.
- After 1 second of no input, the camera recenters behind the player's
  current facing direction, except while the player is near the room center.
- `START` recenters the camera behind the player's current facing direction.
- Camera distance auto-fits the active room shell to the viewport for the
  current look angle.
- Door transitions run for a fixed transition budget, block movement while
  active, and smooth brief missed-frame bursts instead of consuming them in
  one visible jump.
- `BgDialog` is used for NPC interaction in the current baseline.

## Build Include Order

`Makefile` keeps project overrides first so `#include "fr_model_3d.h"` resolves
to the project header before the upstream copy.
