# Codebase Structure

Analysis date: 2026-03-22
Last updated: 2026-04-03

## Top-level Layout

```text
stranded/
|- src/                # Room viewer runtime and support code
|- include/            # Project headers and tracked model headers
|- build/generated/    # Build-generated room-shell model header
|- graphics/           # Room-viewer asset inputs
|- audio/              # Surviving audio assets
|- scripts/            # Build, generation, and emulator helpers
|- butano/             # Butano engine submodule
|- .planning/          # Technical notes and baseline documentation
`- Makefile            # Build entrypoint
```

## Source Layout

- `src/main.cpp` boots directly into the room viewer.
- `src/viewer/room_viewer.cpp` forwards to the private room-viewer runtime.
- `src/viewer/runtime/room_viewer_runtime.cpp` contains the room-viewer loop.
- `src/core/minimap/minimap.cpp` and `src/core/minimap/minimap_layout.cpp`
  contain the minimap helper.
- `src/core/dialog/str_bg_dialog.cpp` and `src/core/dialog/str_bg_dialog_text.cpp` contain
  the dialog helper.
- `src/viewer/room_renderer.cpp`, `src/viewer/room_renderer.bn_iwram.cpp`, and
  `include/private/viewer/str_room_renderer.h` contain the private room-viewer renderer.
- `src/viewer/math/` contains private renderer math support units.

## Header Layout

- `include/str_scene_room_viewer.h` exposes the room-viewer entrypoint.
- `include/str_minimap.h`, `include/str_bg_dialog.h`, and
  `include/str_constants.h` hold room-viewer support code.
- `include/models/` keeps tracked prop model headers.
- `build/generated/include/models/` supplies the generated room-shell header at
  build time.

## Maintenance Notes

- Keep new room-viewer helpers close to `src/core/` or `src/viewer/`.
- Keep the room-shell generator and generated-include path aligned.
- Keep `.planning/` in sync with runtime behavior instead of preserving stale
  notes about removed systems.
