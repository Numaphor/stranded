# Coding Conventions

**Analysis Date:** 2026-01-24

## Naming Patterns

**Files:**
- Headers: `str_[component].h` (e.g., `str_player.h`, `str_enemy.h`)
- Source: `[component].cpp` (e.g., `player.cpp`, `enemy.cpp`)
- Constants: All centralized in `str_constants.h`
- Type enums: `str_[component]_type.h` (e.g., `str_enemy_type.h`, `str_npc_type.h`)
- Templates: `new_[type]_template.h/.cpp` in `templates/` directory

**Functions:**
- Public methods: `camelCase()` (e.g., `update()`, `get_hitbox()`, `set_position()`)
- Private methods: `_camelCase()` with underscore prefix (e.g., `_update_idle_state()`)
- Accessors: `get_[property]()` and `set_[property]()` (e.g., `get_hp()`, `set_ammo()`)
- Boolean queries: `is_[condition]()` or `has_[property]()` (e.g., `is_active()`, `has_companion()`)

**Variables:**
- Member variables: `_camelCase` with underscore prefix (e.g., `_current_state`, `_action_timer`)
- Local variables: `camelCase` (e.g., `new_pos`, `is_moving`)
- Constants: `UPPER_SNAKE_CASE` (e.g., `PLAYER_HITBOX_WIDTH`, `MAX_AMMO`)
- Static variables: `snake_case` (e.g., `shared_gun_frame`)

**Types:**
- Classes: `PascalCase` (e.g., `Player`, `Enemy`, `BulletManager`)
- Enums: `PascalCase` (e.g., `State`, `Direction`, `HitboxType`)
- Namespaces: `lowercase` (`str` namespace for all game code)

## Code Style

**Formatting:**
- No automated formatting tools detected
- Manual formatting with consistent 4-space indentation
- Line length generally kept under 120 characters
- Braces on same line for class/method declarations
- Opening brace on new line for implementation sections

**Linting:**
- No linting configuration detected
- Manual code review process
- Pattern headers serve as linting guidelines

## Import Organization

**Order:**
1. System/Butano core headers (`bn_core.h`, `bn_keypad.h`, etc.)
2. Butano component headers (`bn_sprite_ptr.h`, `bn_fixed_point.h`, etc.)
3. Project headers (`str_*.h`)
4. Sprite item headers (`bn_sprite_items_*.h`)

**Path Aliases:**
- No path aliases used
- All includes use relative paths from project root
- Butano headers accessed via `bn::` namespace

## Header Organization Patterns

**Pattern Headers:**
All major classes include standardized pattern documentation:

```cpp
/**
 * ACTOR_PATTERN: Player Entity
 * - MUST inherit from Entity base class
 * - MUST use State Machine pattern for behavior
 * - MUST use bn::fixed_point for positions
 * - MUST use Hitbox class for collision detection
 * - NO direct input handling (use input processors)
 * - ALL state changes through state machine
 * - MANAGES: Health, ammo, combat, movement, companion
 */
```

**Include Guards:**
- Format: `STR_[FILENAME]_H` in uppercase
- Example: `#ifndef STR_PLAYER_H` / `#define STR_PLAYER_H` / `#endif // STR_PLAYER_H`

**Section Separators:**
```cpp
// =========================================================================
// ClassName Implementation
// =========================================================================
```

## Error Handling

**Patterns:**
- Use `bn::assert` for debugging assertions
- Return `bn::optional` for values that may not exist
- Validate array bounds before access
- Check sprite frame numbers within valid ranges
- Handle memory limits gracefully with `bn::vector` capacity limits

**GBA-Specific Constraints:**
- No exceptions (GBA compiler limitations)
- No heap allocation in game code
- Use Butano's custom containers with fixed capacity

## Logging

**Framework:** `bn::log` for emulator-only logging

**Patterns:**
- Logging only in debug builds
- Use for critical system initialization and errors
- Output goes to emulator console (mGBA, No$GBA)

## Comments

**When to Comment:**
- Pattern headers for all major classes
- Complex algorithms and state machine logic
- GBA-specific optimizations and constraints
- Performance-critical sections
- Workarounds for hardware limitations

**GBA-Specific Comments:**
- Memory usage notes for critical systems
- Sprite limit warnings (max 128 sprites)
- Performance timing notes (60 FPS target)
- Hardware-specific workarounds

**JSDoc/TSDoc:**
- Not used (C++ project)
- Use standard C++ comment style
- Doxygen-style comments for public APIs

## Function Design

**Size:** Keep methods focused and under 50 lines when possible

**Parameters:**
- Use `bn::fixed_point` for positions
- Use `bn::fixed` for all numeric calculations
- Pass by reference for large objects
- Use `const` reference for read-only parameters

**Return Values:**
- Use `bn::fixed` for numeric returns
- Use `bn::optional` for potentially missing values
- Return `bool` for success/failure states
- Use `[[nodiscard]]` for important return values

## Module Design

**Exports:**
- All game code in `str` namespace
- Explicit exports via header declarations
- No global variables (use class members)

**Barrel Files:**
- Not used (individual headers preferred)
- Centralized constants in `str_constants.h`

## GBA-Specific Conventions

**Memory Management:**
- No dynamic allocation (`new`/`delete`)
- Use `bn::vector` with fixed capacity
- Use `bn::optional` instead of pointers
- RAII patterns for resource management

**Performance:**
- Target 60 FPS (16.67ms per frame)
- Use fixed-point math (`bn::fixed`) instead of floating point
- Minimize sprite count (max 128)
- Profile memory usage regularly

**Hardware Constraints:**
- Screen resolution: 240x160 pixels
- Tile size: 8x8 pixels
- Sprite palette limitations
- Limited audio channels

**Butano Integration:**
- Use `bn::sprite_items::*` for sprite creation
- Use `bn::sprite_animate_action` for animations
- Use `bn::fixed_point` for all positions
- Use `bn::keypad` for input handling

## Asset Naming Conventions

**Sprites:**
- `sprite_items_[name].h` generated headers
- Graphics files in `graphics/sprite/[category]/`
- Categories: `player`, `enemy`, `npc`, `vfx`, `hud`, `item`
- JSON metadata files alongside BMP graphics

**Audio:**
- Music and sound effects in `audio/`
- Processed by Butano's build system

## State Machine Conventions

**Enum Structure:**
```cpp
enum class State
{
    IDLE,           // Default resting state
    ACTIVE,         // Main action state
    TRANSITION,     // Between states
    DISABLED        // Inactive/dead state
};
```

**State Management:**
- State changes through dedicated methods (`set_state()`)
- Animation tied to state changes
- Input filtered by current state
- State timers for timed actions

## Constants Management

**Centralization:**
- All game constants in `include/str_constants.h`
- Use `constexpr` for compile-time constants
- Group related constants with comments
- Include units and ranges in comments

**Naming Groups:**
- `PLAYER_*`: Player-specific constants
- `ENEMY_*`: Enemy behavior constants
- `HUD_*`: UI layout constants
- `CAMERA_*`: Camera behavior constants

---

*Convention analysis: 2026-01-24*