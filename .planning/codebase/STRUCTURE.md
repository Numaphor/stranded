# Codebase Structure

**Analysis Date:** 2026-02-09  
**Last Updated:** 2026-02-21 (3D room viewer, include overrides, tools)

## Directory Layout

```
stranded/
├── src/                    # Main source code implementation
│   ├── main.cpp           # Game entry point and main loop
│   ├── actors/            # Entity classes (player, enemies, NPCs)
│   │   ├── player.cpp
│   │   ├── enemy.cpp
│   │   └── npc.cpp
│   ├── core/             # Core game systems and utilities
│   │   ├── bullet.cpp
│   │   ├── collision.cpp
│   │   ├── direction_utils.cpp
│   │   ├── entity.cpp
│   │   ├── hud.cpp
│   │   ├── level.cpp
│   │   ├── minimap.cpp
│   │   ├── movement.cpp
│   │   ├── room_viewer.cpp   # 3D room scene (isometric walk-around)
│   │   ├── scenes.cpp
│   │   ├── world_state.cpp
│   │   └── world.cpp
│   └── viewer/           # 3D engine (from varooom-3d, project copy)
│       ├── fr_camera_3d.cpp
│       ├── fr_div_lut.cpp
│       ├── fr_menu_keypad.cpp
│       ├── fr_models_3d.cpp
│       ├── fr_models_3d.bn_iwram.cpp
│       ├── fr_shape_groups.cpp
│       ├── fr_shape_groups.bn_iwram.cpp
│       └── fr_sin_cos.cpp
├── include/               # Header declarations (project first in INCLUDES)
│   ├── str_*.h           # Game-specific headers (str prefix for "stranded")
│   ├── fr_*.h            # 3D engine overrides (extend varooom-3d; keep submodule clean)
│   │   ├── fr_model_3d.h       # + depth_bias, set_rotation_matrix
│   │   ├── fr_sprite_3d.h      # + horizontal_flip
│   │   ├── fr_sprite_3d_item.h # + pixel_size, sprite_size, 8/16/32/64 support
│   │   ├── fr_models_3d.h
│   │   └── fr_shape_groups.h
│   └── models/           # 3D model item headers (room, table, chair, etc.)
│       ├── str_model_3d_items_room.h
│       ├── str_model_3d_items_table.h
│       ├── str_model_3d_items_chair.h
│       └── ...
├── graphics/              # Game assets (BMP images and JSON configs)
│   ├── bg/              # Background tiles and maps
│   └── sprite/          # Sprite graphics organized by type
│       ├── player/
│       ├── creature/
│       ├── enemy/
│       ├── npc/
│       ├── vfx/
│       ├── hud/
│       └── item/
├── audio/                 # Audio files (IT, MOD, S3M, WAV)
├── dmg_audio/             # DMG audio format files
├── butano/               # Butano engine library (git submodule)
│   └── games/varooom-3d/ # 3D example — do not modify; we override in include/
├── tools/                # Development tools and utilities
│   ├── generate_font.py
│   ├── generate_room_obj.py  # Room OBJ generation (shell, table, chair)
│   └── obj_to_butano.py     # OBJ → Butano model headers
├── scripts/              # Build and utility scripts
│   └── launch_emulator.js
├── .planning/             # Codebase and planning docs
├── .cursor/              # Cursor IDE configuration
├── .github/              # GitHub workflows and templates
└── Makefile              # Build configuration (INCLUDES: include first)
```

## Directory Purposes

**src/:**
- Purpose: Main implementation code for the game
- Contains: C++ source files organized by functionality
- Key files: `src/main.cpp` (entry point), `src/core/world.cpp` (main game loop)

**include/:**
- Purpose: Public API declarations and forward declarations; **first** in `Makefile` INCLUDES so project headers override submodule.
- Contains: `str_*.h` (game), `fr_*.h` (3D engine overrides), `include/models/` (3D model items).
- Key files: `include/str_scene.h` (scene enum), `include/str_player.h` (player), `include/str_scene_room_viewer.h` (room viewer), `include/fr_model_3d.h` (3D model + depth_bias).

**src/actors/:**
- Purpose: Game entity implementations (player, enemies, NPCs)
- Contains: Entity classes that inherit from base Entity class
- Key files: `src/actors/player.cpp` (player mechanics), `src/actors/enemy.cpp` (enemy AI)

**src/core/:**
- Purpose: Core game systems and utilities
- Contains: Collision detection, scene management, world state, room viewer scene
- Key files: `src/core/world.cpp` (main world), `src/core/scenes.cpp` (scene implementations), `src/core/room_viewer.cpp` (3D room scene)

**src/viewer/:**
- Purpose: 3D rendering pipeline (project copy of varooom-3d sources)
- Contains: Camera, models 3D, shape groups, sin/cos LUTs; IWRAM variants for hot paths
- Key files: `src/viewer/fr_models_3d.bn_iwram.cpp` (projection, depth sort, sprite_3d), `src/viewer/fr_models_3d.cpp`

**graphics/:**
- Purpose: Visual assets in GBA-compatible formats
- Contains: BMP images with corresponding JSON configuration files
- Key files: Background tiles, sprite sheets, animation data

**butano/:**
- Purpose: Game Boy Advance engine library
- Contains: Complete Butano engine source code and examples (e.g. `games/varooom-3d/`)
- Generated: No (git submodule)
- Committed: Yes (as submodule reference)
- **Submodule hygiene:** Do not modify `butano/games/varooom-3d/`; Stranded extends 3D via headers in `include/` (fr_model_3d.h, fr_sprite_3d.h, fr_sprite_3d_item.h) and uses project `src/viewer/` for 3D implementation. Build picks `include/` first.

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Main game loop and scene management
- `include/str_scene.h`: Scene enumeration and navigation

**Configuration:**
- `Makefile`: Build system configuration and paths
- `include/str_constants.h`: Game constants and configuration values

**Core Logic:**
- `src/core/world.cpp`: Main game world and update loop
- `src/actors/player.cpp`: Player character implementation
- `src/core/collision.cpp`: Collision detection system

**Assets:**
- `graphics/`: All visual assets (backgrounds, sprites)
- `audio/`: Music and sound effects

## Naming Conventions

**Files:**
- `str_*.h` for game-specific headers (str = stranded)
- `*.cpp` for implementation files matching header names
- `snake_case` for all filenames

**Directories:**
- `src/` for source code
- `include/` for headers
- `graphics/` for visual assets
- `audio/` for sound assets

**Classes/Namespaces:**
- `str::` namespace for all game code
- PascalCase for class names (Player, Enemy, World)
- enum class for type-safe enums

## Where to Add New Code

**New Feature:**
- Primary code: `src/core/` (if system) or `src/actors/` (if entity)
- Headers: `include/str_[feature].h`
- Assets: `graphics/` (visual) or `audio/` (sound)

**New Entity Type:**
- Implementation: `src/actors/[entity].cpp`
- Header: `include/str_[entity].h`
- Inherits from: `Entity` base class

**New Scene:**
- Implementation: `src/core/scenes.cpp` (add new scene class) or dedicated `src/core/[scene].cpp` (e.g. room_viewer.cpp)
- Header: `include/str_scene_[scene].h`
- Update: `include/str_scene.h` enum and `src/main.cpp` switch statement

**3D / Room viewer:**
- Scene: `src/core/room_viewer.cpp`, `include/str_scene_room_viewer.h`
- Models: `include/models/str_model_3d_items_*.h` (generated via `tools/obj_to_butano.py`)
- Room geometry: `tools/generate_room_obj.py` → OBJ → Butano headers

**New Game System:**
- Implementation: `src/core/[system].cpp`
- Header: `include/str_[system].h`
- Integration: Include in `src/core/world.cpp` or appropriate scene

## Special Directories

**butano/:**
- Purpose: Complete Butano GBA engine library
- Generated: No (external git submodule)
- Committed: Yes (as submodule reference)
- Note: Do not modify. For 3D (varooom-3d), add overrides in `include/` (fr_*.h) and use `src/viewer/` for implementation; see 3D_ENGINE.md.

**graphics/ :**
- Purpose: GBA-compatible graphical assets
- Generated: Yes (JSON files generated during build)
- Committed: Yes (source BMP files)
- Format: BMP images with JSON configuration for each asset

**build/ :**
- Purpose: Build output directory (created by Makefile)
- Generated: Yes
- Committed: No
- Contains: Object files, final ROM binary

---

*Structure analysis: 2026-02-09. Updated 2026-02-21: room viewer, src/viewer, include overrides, tools, submodule hygiene.*