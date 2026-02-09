# Coding Conventions

**Analysis Date:** 2026-02-09

## Naming Patterns

- **Files:** snake_case.cpp / snake_case.h (e.g., `player.cpp`, `str_scene.h`)
- **Functions:** snake_case() (e.g., `execute()`, `update()`, `handle_input()`)
- **Variables:** snake_case (e.g., `_player`, `_camera`, `_selected_index`)
- **Types/Classes:** PascalCase (e.g., `Player`, `World`, `PlayerMovement`)
- **Constants:** UPPER_SNAKE_CASE (e.g., `MAIN_WORLD_SPAWN_X`, `PLAYER_HITBOX_WIDTH`)
- **Private members:** Prefix underscore (e.g., `_player`, `_camera`, `_current_world_id`)
- **Namespaces:** snake_case (e.g., `namespace str`, `namespace direction_utils`)

## Code Style

**Formatting:**
- No automated formatting tool detected
- Indentation: 4 spaces (observed in source files)
- Braces: Allman style - opening brace on new line
- Line length: No strict limit, reasonable length observed

**Linting:**
- No linting configuration detected
- Butano compiler warnings for GBA-specific issues

## Import / Include Organization

**Order:**
1. Project headers (`#include "str_*.h"`)
2. Butano headers (`#include "bn_*.h"`)
3. Standard headers (`#include <...>`)

**Path style:** 
- Project headers: Quoted with relative paths
- Butano headers: Quoted with bn_ prefix
- No relative/absolute aliases used

## Error Handling

**Pattern:**
```cpp
// Defensive programming with boundary checks
if (!str::ZoneManager::is_position_valid(_player->pos()))
    _player->revert_position();

// State validation with asserts (Butano handles these)
BN_ASSERT(sprite.has_value(), "Sprite must be available");

// Graceful degradation for optional features
if (_merchant)
    _merchant->update();
```

## Logging / Debug Output

- Framework: bn::core::log() to emulator debug window
- Pattern: Used for debugging build configuration
- Production: No runtime logging in release builds

## Comments

- When to comment: Section separators, complex algorithms, TODO items
- Doc comments: Minimal - some class-level documentation
- Section separators: `// =========================================================================` for major sections

## Function Design

- Size: Medium to large functions (hundreds of lines in some cases)
- Parameters: Mix of values, const references, and pointers
- Return values: Scene enums, void, bool for state queries
- Accessors: Mix of inline getters and explicit methods

## Module Design

- Exports: One major class per header pattern
- Encapsulation: Private members with underscore prefix, public getters/setters
- Forward declarations: Used extensively to reduce include dependencies
- Namespace organization: `namespace str` for all game code

---
*Convention analysis: 2026-02-09*