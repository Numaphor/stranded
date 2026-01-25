# Codebase Structure

**Analysis Date:** 2026-01-24

## Directory Layout

```
stranded/
├── include/               # Public header files
│   ├── str_*.h           # Game system headers
│   └── .gitignore        # Ignore generated headers
├── src/                  # Source implementation files
│   ├── main.cpp          # Entry point and main game loop
│   ├── core/             # Core game systems
│   │   ├── world.cpp     # World management and chunk streaming
│   │   ├── scenes.cpp    # Scene implementations (menu, controls)
│   │   ├── entity.cpp    # Base entity implementation
│   │   ├── collision.cpp # Collision detection system
│   │   ├── hud.cpp       # Heads-up display system
│   │   ├── minimap.cpp   # Minimap rendering
│   │   ├── level.cpp     # Level data management
│   │   ├── chunk_manager.cpp # Chunk streaming system
│   │   ├── world_object.cpp # World objects (trees, buildings)
│   │   ├── world_state.cpp # World state persistence
│   │   ├── direction_utils.cpp # Direction utilities
│   │   ├── movement.cpp  # Movement system
│   │   └── bullet.cpp    # Bullet management
│   └── actors/           # Game entity implementations
│       ├── player.cpp     # Player character logic
│       ├── enemy.cpp      # Enemy AI and behaviors
│       └── npc.cpp        # Non-player character logic
├── graphics/             # Asset source files
│   ├── bg/               # Background graphics
│   ├── sprite/           # Sprite graphics organized by type
│   │   ├── player/       # Player sprite assets
│   │   ├── enemy/        # Enemy sprite assets
│   │   ├── npc/          # NPC sprite assets
│   │   ├── vfx/          # Visual effects
│   │   ├── hud/          # HUD elements
│   │   └── item/         # Item sprites
│   └── master/           # Original asset files
├── audio/                # Audio files (.it, .wav)
├── build/                # Build output and generated files
├── butano/               # Butano framework submodule
├── tools/                # Development tools
├── templates/            # Code generation templates
└── .planning/            # Planning documentation
```

## Directory Purposes

**include/:**
- Purpose: Public interface declarations for all game systems
- Contains: Header files with class declarations, constants, forward declarations
- Key files: `str_scene.h`, `str_entity.h`, `str_constants.h`, `str_chunk_manager.h`

**src/core/:**
- Purpose: Core game system implementations
- Contains: World management, collision, HUD, utilities
- Key files: `world.cpp`, `scenes.cpp`, `collision.cpp`, `chunk_manager.cpp`

**src/actors/:**
- Purpose: Game entity implementations
- Contains: Player, enemy, and NPC specific logic
- Key files: `player.cpp`, `enemy.cpp`, `npc.cpp`

**graphics/:**
- Purpose: Visual asset source files
- Contains: Backgrounds, sprites, visual effects organized by type
- Structure: Separated by asset type and entity categories

**build/:
- Purpose: Build artifacts and generated code
- Contains: Object files, generated headers, ROM output
- Generated: bn_*.h files from graphics/audio processing

**templates/:**
- Purpose: Code generation templates for consistent patterns
- Contains: Manager and actor templates with architectural guidance
- Usage: Boilerplate for new systems following established patterns

## Key File Locations

**Entry Points:**
- `src/main.cpp`: Main game loop and scene management
- Purpose: GBA initialization, scene switching, core update loop

**Configuration:**
- `include/str_constants.h`: Game constants and configuration values
- `Makefile`: Build configuration and dependencies
- Purpose: Build settings, game tuning parameters, asset paths

**Core Logic:**
- `src/core/world.cpp`: World management and chunk streaming
- `src/core/scenes.cpp`: Scene implementations (menu, controls, etc.)
- `src/actors/player.cpp`: Player character implementation
- Purpose: Main game systems, entity management

**Architecture:**
- `include/str_entity.h`: Base entity interface
- `include/str_scene*.h`: Scene system architecture
- `include/str_enemy_state_machine.h`: State machine pattern
- Purpose: Core architectural patterns and abstractions

**Testing:**
- Not applicable - no dedicated test structure found

## Naming Conventions

**Files:**
- str_[module].h for header files in include/
- [module].cpp for implementation files in src/
- Patterns: `str_entity.h`, `world.cpp`, `player.cpp`

**Classes:**
- PascalCase for class names: `Entity`, `World`, `Player`
- str:: namespace prefix for game classes: `str::Entity`

**Functions:**
- snake_case for method names: `update()`, `get_hitbox()`
- Private methods prefixed with underscore: `_update_camera()`

**Constants:**
- UPPER_SNAKE_CASE for constants: `PLAYER_HITBOX_WIDTH`, `TILE_SIZE`

## Where to Add New Code

**New Feature:**
- Primary code: `src/core/[feature].cpp` and `include/str_[feature].h`
- Tests: Not applicable - integration testing in scenes
- Example: New system - `src/core/new_system.cpp`, `include/str_new_system.h`

**New Component/Module:**
- Implementation: `src/core/[component].cpp`
- Interface: `include/str_[component].h`
- Integration: Include in relevant entity/scene files
- Example: New component - follow existing `str_collision.h` pattern

**New Entity Type:**
- Implementation: `src/actors/[entity].cpp`
- Interface: `include/str_[entity].h`
- Registration: Add to scene management and factory patterns
- Example: New enemy - follow `str_enemy.h` template patterns

**Utilities:**
- Shared helpers: `src/core/[utility].cpp` and `include/str_[utility].h`
- Example: `str_direction_utils.h` and `direction_utils.cpp`

## Special Directories

**build/:**
- Purpose: Build artifacts and generated files
- Generated: Yes - contains bn_*.h files from asset processing
- Committed: No - generated during build process

**templates/:**
- Purpose: Code generation templates for architectural consistency
- Generated: No - source templates
- Committed: Yes - version controlled templates

**butano/:**
- Purpose: Butano framework submodule
- Generated: No - external dependency
- Committed: Yes - as git submodule reference

**graphics/:
- Purpose: Asset source files
- Generated: No - original asset files
- Committed: Yes - source assets version controlled

---

*Structure analysis: 2026-01-24*