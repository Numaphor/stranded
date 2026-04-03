# 3D Engine Reference

Last updated: 2026-04-03

## Scope

This note covers the private room-viewer renderer:

- `include/private/viewer/str_room_renderer.h`
- `src/viewer/room_renderer.cpp`
- `src/viewer/room_renderer.bn_iwram.cpp`
- `src/viewer/math/fr_sin_cos.cpp`
- `src/viewer/math/fr_div_lut.cpp`
- `build/generated/include/models/str_model_3d_items_room.h`
- `include/models/str_model_3d_items_books.h`
- `include/models/str_model_3d_items_potted_plant.h`

The upstream Butano and varooom-3d headers remain the source of truth for the
base math and model data structures.

## Coordinate System

```text
Engine X -> screen horizontal axis
Engine Y -> depth axis
Engine Z -> screen vertical axis
```

## Runtime Limits

- Max dynamic models: `3`
- Max sprite billboards: `3`
- Max projected vertices: `240`
- Max projected faces: `192`

The current room viewer stays within those limits by loading at most two room
shells and one decor model at the same time.

## Supported Features

- Per-model rotation matrices
- Per-model depth bias
- Room-shell layering modes for the active room and preview room
- Double-sided shell rendering for the active room
- 64x64 sprite billboards with horizontal flip
- Scanline sprite reservation for wide polygon spans

The old public project-local `fr_*` renderer API has been removed. The renderer
is now a private implementation detail used only by the room viewer.

## Build Generation

- `scripts/generate_room_shell_header.py` writes the room-shell header into
  `build/generated/include/models/`.
- `Makefile` places `build/generated/include` ahead of `include` in the include
  search order so the generated room-shell header is preferred automatically.

## Room Viewer Usage

- The room viewer keeps a fixed 60-degree top-down presentation.
- The camera turns through intermediate quarter-view steps instead of snapping.
- The current room shell uses perspective wall layering and double-sided walls.
- The transition-room shell uses floor-only layering and a depth bias.
- Paintings remain sprite-based textured quads; they are not part of the 3D
  model pipeline.
