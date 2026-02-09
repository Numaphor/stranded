# Codebase Structure

**Analysis Date:** 2026-02-09

## Directory Layout

```
stranded/
├── src/                    # Main source code implementation
│   ├── main.cpp           # Game entry point and main loop
│   ├── actors/            # Entity classes (player, enemies, NPCs)
│   │   ├── player.cpp
│   │   ├── enemy.cpp
│   │   └── npc.cpp
│   └── core/             # Core game systems and utilities
│       ├── bullet.cpp
│       ├── collision.cpp
│       ├── direction_utils.cpp
│       ├── entity.cpp
│       ├── hud.cpp
│       ├── level.cpp
│       ├── minimap.cpp
│       ├── movement.cpp
│       ├── scenes.cpp
│       ├── world_state.cpp
│       └── world.cpp
├── include/               # Header declarations
│   ├── str_*.h          # Game-specific headers (str prefix for "stranded")
│   └── .gitignore        # Ignore generated files
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
├── tools/                # Development tools and utilities
│   └── generate_font.py
├── scripts/              # Build and utility scripts
│   └── launch_emulator.js
├── .cursor/              # Cursor IDE configuration
├── .github/              # GitHub workflows and templates
└── Makefile              # Build configuration
```

## Directory Purposes

**src/:**
- Purpose: Main implementation code for the game
- Contains: C++ source files organized by functionality
- Key files: `src/main.cpp` (entry point), `src/core/world.cpp` (main game loop)

**include/:**
- Purpose: Public API declarations and forward declarations
- Contains: Header files with `str_` prefix naming convention
- Key files: `include/str_scene.h` (scene enum), `include/str_player.h` (player class)

**src/actors/:**
- Purpose: Game entity implementations (player, enemies, NPCs)
- Contains: Entity classes that inherit from base Entity class
- Key files: `src/actors/player.cpp` (player mechanics), `src/actors/enemy.cpp` (enemy AI)

**src/core/:**
- Purpose: Core game systems and utilities
- Contains: Collision detection, scene management, world state
- Key files: `src/core/world.cpp` (main world), `src/core/scenes.cpp` (scene implementations)

**graphics/:**
- Purpose: Visual assets in GBA-compatible formats
- Contains: BMP images with corresponding JSON configuration files
- Key files: Background tiles, sprite sheets, animation data

**butano/:**
- Purpose: Game Boy Advance engine library
- Contains: Complete Butano engine source code and examples
- Generated: No (git submodule)
- Committed: Yes (as submodule reference)

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
- Implementation: `src/core/scenes.cpp` (add new scene class)
- Header: `include/str_scene_[scene].h`
- Update: `include/str_scene.h` enum and `src/main.cpp` switch statement

**New Game System:**
- Implementation: `src/core/[system].cpp`
- Header: `include/str_[system].h`
- Integration: Include in `src/core/world.cpp` or appropriate scene

## Special Directories

**butano/:**
- Purpose: Complete Butano GBA engine library
- Generated: No (external git submodule)
- Committed: Yes (as submodule reference)
- Note: Do not modify - contains engine code

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

*Structure analysis: 2026-02-09*