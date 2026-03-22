# Codebase Structure

Analysis date: 2026-03-22
Last updated: 2026-03-22

## Top-level Layout

```text
stranded/
|- src/                # Room viewer runtime and minimal support code
|- include/            # Project headers, overrides, and generated models
|- graphics/           # Room-viewer asset inputs
|- audio/              # Surviving music and SFX assets
|- dmg_audio/          # DMG audio assets if the build still references them
|- tools/              # Content helpers and bundled emulator
|- scripts/            # Local build and emulator helpers
|- butano/             # Butano engine submodule
|- .planning/          # Technical notes and baseline documentation
|- .agents/            # Agent rules and skills
|- Makefile            # Build entrypoint
`- README.md
```

## Source Layout

- `src/main.cpp` boots the game.
- `src/room_viewer.cpp` contains the room-viewer scene.
- `src/core/minimap.cpp` provides the room minimap helper.
- `src/viewer/` contains the shared 3D runtime used by the room viewer.
- No gameplay, world, or model-viewer source tree remains in the current
  documented baseline.

## Header Layout

- `include/str_scene.h` and `include/str_scene_room_viewer.h` define the
  surviving scene surface.
- `include/str_minimap.h`, `include/str_bg_dialog.h`, and
  `include/str_constants.h` hold room-viewer support code.
- `include/fr_*.h` contains project overrides for the shared 3D runtime.
- `include/models/` contains generated model headers used by the room viewer.

## Tooling and Assets

- `tools/mGBA-0.10.5-win64/` provides the bundled Windows emulator.
- `scripts/mgba_f12_capture.ps1` and `scripts/launch_debug.sh` support local
  validation and debugging.
- `graphics/` and `audio/` now contain only the inputs needed by the surviving
  room-viewer build.
- `dmg_audio/` stays only if the build still references it.

## Maintenance Notes

- Keep new room-viewer helpers close to `src/room_viewer.cpp` or `src/viewer/`.
- Keep new shared runtime types in `include/fr_*.h`.
- Keep new planning notes in `.planning/` and delete superseded notes instead of
  keeping parallel copies.
