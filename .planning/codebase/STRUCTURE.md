# Codebase Structure

Analysis date: 2026-02-09
Last updated: 2026-03-03

## Top-level Layout

```text
stranded/
|- src/                # Game code (actors, core systems, 3D viewer runtime)
|- include/            # Project headers (str_* + fr_* overrides + model headers)
|- graphics/           # BMP and JSON asset inputs for Butano tools
|- audio/              # Music and SFX assets (.wav, .it)
|- dmg_audio/          # DMG-specific audio assets (reserved, currently empty)
|- tools/              # Local content/asset conversion scripts + bundled emulator
|- scripts/            # Dev helper scripts (emulator launch)
|- butano/             # Butano engine submodule
|- .github/workflows/  # CI build/release/container workflows
|- .planning/          # Technical planning docs
|- .agents/            # AI agent rules and skills
|- tests/              # Reserved for project tests (currently empty)
|- Makefile            # Build entrypoint
`- README.md
```

## Source Directories

### `src/`

- `src/main.cpp`: scene loop and scene dispatch (7 scenes).

### `src/actors/`

Gameplay entities with their own behavior and AI.

- `src/actors/player.cpp`: Player entity (movement, combat, companion, buffs, strafing, combo).
- `src/actors/soldier.cpp`: Soldier subclass of Player (burst fire, accuracy/damage boosts).
- `src/actors/enemy.cpp`: Enemy entity (AI state machine, knockback, patrol/chase/attack).
- `src/actors/npc.cpp`: NPC entity (dialog system, merchant quest integration).

### `src/core/`

Game systems, scenes, and infrastructure.

- `src/core/scenes.cpp`: START, CHARACTER_SELECT, MENU, CONTROLS scene rendering.
- `src/core/world.cpp`: Main gameplay loop coordinator (owns player, enemies, minimap, quests, camera).
- `src/core/world_state.cpp`: Save/load state management (8 max slots).
- `src/core/hud.cpp`: HUD rendering (healthbar, weapon, ammo, buffs, gun menu, energy, alerts).
- `src/core/minimap.cpp`: Sprite-based minimap (6 rooms, enemy dots, door indicators).
- `src/core/minimap_canvas.cpp`: BG tile-based minimap (EWRAM-backed, lower VRAM cost).
- `src/core/entity.cpp`: Base class for positioned game objects.
- `src/core/collision.cpp`: AABB collision detection.
- `src/core/movement.cpp`: Velocity-based movement with acceleration/friction.
- `src/core/direction_utils.cpp`: Direction-aware roll offsets, gun positioning, bullet spawns.
- `src/core/level.cpp`: Tile-based level (floor/zone tiles, merchant zones, position validation).
- `src/core/bullet.cpp`: Bullet pool (32 bullets) with direction-based velocity.
- `src/core/collectible.cpp`: Health pickup spawning and collection.
- `src/core/quest.cpp`: Quest management (start, progress, completion, rewards).
- `src/core/model_viewer.cpp`: 3D model viewer scene.

### `src/room_viewer.cpp`

Standalone room viewer scene (3D isometric viewer with dialog, camera follow, door transitions).

### `src/viewer/`

3D runtime implementation adapted from varooom-3d.

- `src/viewer/fr_models_3d.cpp`: Model transforms, projection, depth sorting, sprite submission.
- `src/viewer/fr_models_3d.bn_iwram.cpp`: IWRAM variant of performance-critical render loops.
- `src/viewer/fr_camera_3d.cpp`: Camera positioning and projection.
- `src/viewer/fr_shape_groups.cpp`: Sprite shape group management.
- `src/viewer/fr_shape_groups.bn_iwram.cpp`: IWRAM variant of shape group operations.
- `src/viewer/fr_sin_cos.cpp`: Fixed-point trig tables.
- `src/viewer/fr_div_lut.cpp`: Division lookup table.
- `src/viewer/fr_menu_keypad.cpp`: Menu input handling for viewer.

## Header Directories

### `include/`

- `str_*.h`: Game-specific interfaces (48 headers total).
- `fr_*.h`: Project overrides/extensions for 3D engine types.
- `include/models/`: Generated 3D model item headers.

### Key Game Headers

| Header | Purpose |
|--------|---------|
| `str_entity.h` | Base entity class (pos, hitbox, sprite) |
| `str_player.h` | Player entity (movement, combat, companion, buffs) |
| `str_player_companion.h` | Side companion (follow, death, revival) |
| `str_soldier.h` | Soldier subclass (burst fire, tactical buffs) |
| `str_enemy.h` | Enemy entity (AI state machine) |
| `str_enemy_type.h` | Enemy types enum (SPEARGUARD, SLIME, MUTANT) |
| `str_enemy_state.h` | Enemy state base class |
| `str_enemy_states.h` | Concrete AI states (Idle, Patrol, Chase, Attack, ReturnToPost, Stunned) |
| `str_enemy_state_machine.h` | State machine driver |
| `str_npc.h` | NPC entity (dialog system) |
| `str_npc_type.h` | NPC types enum (MERCHANT) |
| `str_npc_derived.h` | MerchantNPC (quest-driven dialog) |
| `str_collectible.h` | Health pickups |
| `str_quest.h` | Quest manager (COLLECT_HEARTS, SLAY_ENEMIES) |
| `str_minimap.h` | Sprite-based minimap |
| `str_minimap_canvas.h` | BG tile-based minimap |
| `str_hud.h` | HUD system |
| `str_hitbox.h` | AABB hitbox, zone types, ZoneManager |
| `str_collision.h` | Collision detection |
| `str_movement.h` | Movement system (base + EnemyMovement) |
| `str_bullet_manager.h` | Bullet pool and firing |
| `str_direction_utils.h` | Direction-aware positioning |
| `str_level.h` | Tile-based level |
| `str_world_state.h` | Save/load state |
| `str_scene.h` | Scene enum |
| `str_scene_world.h` | World scene interface |
| `str_scene_room_viewer.h` | RoomViewer class |
| `str_scene_model_viewer.h` | ModelViewer class |
| `str_scene_menu.h` | Menu scene interface |
| `str_scene_controls.h` | Controls scene interface |
| `str_scene_start.h` | Start scene interface |
| `str_scene_character_select.h` | Character select interface |
| `str_bg_dialog.h` | BG-layer dialog (no sprite VRAM) |
| `str_room_dialog.h` | Sprite-based dialog for room viewer |
| `str_constants.h` | All game constants (map, player, combat, camera, HUD, minimap, zoom, characters) |
| `str_model_viewer_items.h` | Model viewer item definitions |

### 3D Engine Override Headers

| Header | Purpose |
|--------|---------|
| `fr_model_3d.h` | Model: direct matrix, depth bias, layering mode |
| `fr_sprite_3d.h` | Sprite: horizontal flip controls |
| `fr_sprite_3d_item.h` | Sprite sizing metadata (8/16/32/64px) |
| `fr_models_3d.h` | Models runtime interface |
| `fr_shape_groups.h` | Shape group management |

### Model Headers (`include/models/`)

Generated 3D model definitions:

- `str_model_3d_items_room.h`
- `str_model_3d_items_table.h`
- `str_model_3d_items_chair.h`
- `str_model_3d_items_building.h`
- `str_model_3d_items_player_car.h`
- `str_model_3d_items_blaster.h`

### Build Include Order

`Makefile` keeps project overrides first:

1. `include`
2. `butano/common/include`
3. `butano/games/varooom-3d/include`

This ensures `#include "fr_model_3d.h"` resolves to the project override.

## Tooling and Content Scripts

### `tools/`

- `create_starter_level_obj.py`: Creates level .obj from template parameters.
- `generate_level_headers.py`: Converts OBJ files to C++ level headers.
- `model_utils.py`: Shared OBJ/MTL parsing utilities.
- `obj_to_header.py`: Standalone OBJ-to-C++ header converter.
- `generate_bg_font.py`: Generates 8x8 BMP font atlas for BG-based text rendering.
- `convert_axulart_npc.py`: Converts axulart-style NPC sprite sheets.
- `generate_wall_painting_sprites.py`: Generates triangular wall painting sprites.
- `mGBA-0.10.5-win64/`: Bundled Windows emulator for local testing.

### `scripts/`

- `launch_emulator.js`: Build-and-run script; reads `.vscode/settings.json` for emulator path.

### `.agents/`

AI agent configuration for development workflows:

- `.agents/rules/butano-gba.md`: Butano GBA development guide.
- `.agents/rules/codebase-documentation.md`: Planning docs usage guide.
- `.agents/skills/stranded-windows-e2e-testing/`: E2E testing skill for Windows ARM64.

## Asset Directories

### `graphics/`

- `graphics/bg/`: Background images (BMP + JSON metadata).
- `graphics/sprite/`: Sprite sheets organized by category:
  - `creature/`, `decor/`, `enemy/`, `hud/`, `item/`, `npc/`, `player/`, `vfx/`
- `graphics/shape_group_textures/`: 80+ texture files for 3D shape groups.

### `audio/`

12 sound effects: death, eek, growl, hello, hum, mutant_hit, slime, slime2, steps, swipe, tablet, teleport.

### `dmg_audio/`

Reserved for DMG-specific audio assets (currently empty).

## Where to Add New Code

- New gameplay entity: `src/actors/` + matching `include/str_*.h`.
- New scene/system: `src/core/` + matching `include/str_*.h`.
- 3D runtime changes: `src/viewer/` and, if needed, project `include/fr_*.h` overrides.
- New 3D model: `include/models/str_model_3d_items_*.h`.
- New planning notes: `.planning/` (feature-specific docs go in `.planning/features/`).
- New tools/scripts: `tools/` for content pipeline, `scripts/` for dev workflow.
- New game constants: `include/str_constants.h`.
