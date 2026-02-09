# Coding Conventions

**Analysis Date:** 2026-02-09

## Naming Patterns

**Files:**
- Snake case for source files: `player.cpp`, `enemy.cpp`, `scene_world.cpp`
- Headers match source files: `str_player.h`, `str_enemy.h`
- Prefix with `str_` for project-specific headers
- GBA asset items use snake_case with underscores: `bn_sprite_items_hero.h`

**Functions:**
- Snake case: `get_roll_offset()`, `_update_display()`, `_handle_input()`
- Private methods prefixed with underscore: `_init_worlds()`, `_update_display()`
- Public methods use clear action verbs: `execute()`, `spawn()`, `update()`

**Variables:**
- Snake case for local variables: `_selected_index`, `frames_remaining`
- Constants use SCREAMING_SNAKE_CASE: `PLAYER_ROLL_SPEED`, `MAP_COLUMNS`
- Private member variables prefixed with underscore: `_worlds`, `_text_sprites`

**Types:**
- PascalCase for classes: `Player`, `Enemy`, `World`, `Menu`
- PascalCase for structs: `WorldData`, `DialogOption`, `EnemyDot`
- PascalCase for enums: `Scene`, `ENEMY_TYPE`, `WEAPON_TYPE`
- Strongly typed enums preferred: `enum class Direction`

## Code Style

**Formatting:**
- No explicit formatting tool detected (no .clang-format, .editorconfig found)
- 4-space indentation observed in source files
- Opening braces on same line for functions/classes
- Opening braces on new line for control flow (if, for, while)

**Linting:**
- No explicit linting configuration detected
- Butano engine likely provides its own style guidelines through the engine's API

## Import Organization

**Order:**
1. Local headers (str_*.h)
2. Butano engine headers (bn_*.h) 
3. Standard library headers (rare in this GBA project)
4. Asset item headers (bn_sprite_items_*.h, bn_regular_bg_items_*.h)
5. Common font headers (common_variable_8x8_sprite_font.h)

**Path Aliases:**
- No path aliases used; direct includes relative to project structure

## Error Handling

**Patterns:**
- Uses Butano's assert system (`BN_ASSERT`) for runtime checks
- No exceptions (GBA doesn't support C++ exceptions well)
- Optional values use `bn::optional<T>` type from Butano
- Return-based error handling for functions that can fail
- Friend class pattern for controlled access to private members

## Logging

**Framework:** Butano's `bn::core::log()` for debug output

**Patterns:**
- Debug output sent to emulator's log window
- No visible logging in production builds
- Emulator-specific debugging (mGBA log window support)

## Comments

**When to Comment:**
- Complex mathematical calculations (camera movement, momentum)
- State transition logic and game mechanics
- Memory management considerations (unique_ptr usage)
- Performance-critical sections
- Platform-specific GBA limitations

**Doc Comments:**
- Class-level documentation with purpose and inheritance requirements
- Method-level comments for public APIs
- Friend class declarations documented with access reason
- Header files contain extensive class documentation

## Function Design

**Size:** Functions tend to be small and focused (10-30 lines typical)
- Large functions broken into smaller helper methods
- Update methods often handle input, then update display separately

**Parameters:** 
- Reference parameters for modifying caller values: `execute(int &wid, bn::fixed_point &sl)`
- Pass by value for small types (int, bn::fixed)
- Const references for large objects

**Return Values:**
- Enum values for state transitions: `str::Scene Menu::execute()`
- Boolean returns for success/failure
- Butano types for engine resources

## Module Design

**Exports:**
- Header files declare public interfaces
- Implementation in corresponding .cpp files
- Clear separation between interface and implementation

**Barrel Files:**
- `str_scene.h` contains common scene enum used across the project
- `str_constants.h` centralizes all game constants
- No other barrel files observed

---

*Convention analysis: 2026-02-09*