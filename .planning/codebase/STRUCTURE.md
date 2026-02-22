# Codebase Structure

Analysis date: 2026-02-09
Last updated: 2026-02-22

## Top-level Layout

```text
stranded/
|- src/                # Game code (actors, core systems, 3D viewer runtime)
|- include/            # Project headers (str_* + fr_* overrides + model headers)
|- graphics/           # BMP and JSON asset inputs for Butano tools
|- audio/              # Music and SFX assets
|- dmg_audio/          # DMG-specific audio assets
|- tools/              # Local content/asset conversion scripts
|- scripts/            # Dev helper scripts (for example emulator launch)
|- butano/             # Butano engine submodule
|- .github/workflows/  # CI build/release/container workflows
|- .planning/          # Technical planning docs
|- tests/              # Reserved for project tests (currently empty)
|- Makefile            # Build entrypoint
`- README.md
```

## Source Directories

### `src/`

- `src/main.cpp`: scene loop and scene dispatch.
- `src/actors/`: gameplay entities (`player`, `enemy`, `npc`, companion logic).
- `src/core/`: game systems and scenes (`world`, `scenes`, `room_viewer`, `model_viewer`, HUD, collision, level, quest, minimap).
- `src/viewer/`: 3D runtime implementation copied/adapted from varooom-3d.

### `include/`

- `str_*.h`: game-specific interfaces.
- `fr_*.h`: project overrides/extensions for 3D engine types.
- `include/models/`: generated model item headers (room/table/chair/building/etc).

Important build detail:
- `Makefile` keeps `include/` first in include order, so project `fr_*` headers override submodule headers.

## Room Viewer Related Files

- Scene logic: `src/core/room_viewer.cpp`
- Scene header: `include/str_scene_room_viewer.h`
- 3D model headers: `include/models/str_model_3d_items_room.h`, `include/models/str_model_3d_items_table.h`, `include/models/str_model_3d_items_chair.h`
- 3D runtime dependencies: `src/viewer/fr_models_3d.cpp`, `src/viewer/fr_models_3d.bn_iwram.cpp`, `src/viewer/fr_camera_3d.cpp`

## Tooling and Content Scripts

Current local scripts under `tools/`:

- `create_starter_level_obj.py`
- `generate_level_headers.py`
- `model_utils.py`
- `obj_to_header.py`

Dev script under `scripts/`:

- `launch_emulator.js`

## Where to Add New Code

- New gameplay entity: `src/actors/` + matching `include/str_*.h`.
- New scene/system: `src/core/` + matching `include/str_*.h`.
- 3D runtime changes: `src/viewer/` and, if needed, project `include/fr_*.h` overrides.
- New planning notes: `.planning/` (feature-specific docs go in `.planning/features/`).
