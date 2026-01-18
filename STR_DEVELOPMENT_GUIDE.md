# Stranded Game Development Patterns

## Project Structure
- **Headers**: `include/str_*.h` - All class declarations and interfaces
- **Source**: `src/actors/*.cpp` - Actor implementations
- **Core**: `src/core/*.cpp` - Game systems and managers
- **Constants**: `include/str_constants.h` - All game constants and configuration
- **Graphics**: `graphics/` - All sprite assets with .json metadata
- **Audio**: `audio/` - Music and sound effects

## Actor System Pattern
All game entities (Player, Enemy, NPC) must follow:

```cpp
/**
 * ACTOR_PATTERN: Standard Entity Actor
 * - MUST inherit from Entity base class
 * - MUST use State Machine pattern for behavior
 * - MUST use bn::fixed_point for positions
 * - MUST use Hitbox class for collision
 * - NO direct input handling in actor classes
 * - ALL state changes through state machine
 */
```

### Actor Requirements
- Inherit from `Entity` base class
- Use dedicated `Movement` class for position/velocity
- Implement state machine with `State` enum
- Provide `update()` method called each frame
- Use `Hitbox` for collision detection
- Handle animation through `bn::sprite_animate_action`

## Manager System Pattern
Game systems (HUD, BulletManager, etc.) must follow:

```cpp
/**
 * MANAGER_PATTERN: Centralized System
 * - MUST provide update() method for frame loop
 * - MUST handle resource cleanup automatically
 * - NO direct game logic (coordinate between entities)
 * - MUST use singleton or centralized instance
 */
```

### Manager Requirements
- Single instance per game
- Update method called each frame
- Automatic resource management
- Coordinate between multiple entities
- No direct game state changes

## Butano Integration Standards

### Required Includes
```cpp
#include "bn_core.h"           // Core engine functionality
#include "bn_keypad.h"         // Input handling
#include "bn_sprite_ptr.h"     // Sprites
#include "bn_fixed_point.h"    // Fixed-point math
#include "bn_fixed.h"           // Fixed-point numbers
```

### Math and Positioning
- **Positions**: Always use `bn::fixed_point`
- **Numbers**: Use `bn::fixed` for all calculations
- **No floating point**: GBA doesn't have FPU
- **Tile coordinates**: 8x8 pixels per tile

### Sprite Management
- Use `bn::sprite_items::*` for sprite creation
- Enable `bn::sprite_double_size_mode::ENABLED` for scaled sprites
- Use `bn::sprite_affine_mat_ptr` for rotation/scaling
- Manage z_order for proper layering

## File Naming Conventions
- **Headers**: `str_[component].h` (e.g., `str_player.h`)
- **Source**: `[component].cpp` (e.g., `player.cpp`)
- **Constants**: All in `str_constants.h`
- **Enums**: `str_[component]_type.h` (e.g., `str_enemy_type.h`)

## State Machine Pattern
All actors with complex behavior must use state machines:

```cpp
enum class State
{
    IDLE,           // Default state
    ACTIVE,         // Main action state
    TRANSITION,     // Between states
    DISABLED        // Inactive state
};
```

### State Requirements
- Each state has clear entry/exit conditions
- State changes through dedicated methods
- Animation tied to state changes
- Input handling filtered by current state

## Memory Management
- **No heap allocation**: Use Butano's custom containers
- **RAII patterns**: Objects manage their own resources
- **bn::vector**: Dynamic arrays with fixed capacity
- **bn::optional**: Safe optional values

## Input Handling Pattern
Input must be processed through dedicated systems:

```cpp
/**
 * INPUT_PATTERN: Centralized Input Processing
 * - MUST use bn::keypad for raw input
 * - MUST filter input by game state
 * - NO direct keypad calls in game logic
 * - ALL input through input processors
 */
```

## Constants and Configuration
All game constants must be in `str_constants.h`:
- Use `constexpr` for compile-time constants
- Group related constants together
- Use descriptive names with prefixes
- Document units and ranges

## Error Handling
- Use `bn::assert` for debugging
- Check array bounds before access
- Validate sprite frame numbers
- Handle memory limits gracefully

## Performance Guidelines
- **60 FPS target**: All updates must complete in 16.67ms
- **Sprite limit**: Maximum 128 sprites on screen
- **Memory limits**: Stay within GBA constraints
- **No dynamic allocation**: Use stack/Butano containers

## Development Workflow
1. **Build and test after every edit**
2. **Use mGBA emulator for testing**
3. **Check for stack traces in debug**
4. **Test on real hardware when possible**
5. **Profile performance regularly**

## Debug Features
- **Hitbox visualization**: SELECT + START to toggle
- **Camera zoom**: Hold SELECT for zoom out
- **Debug controls**: Various button combinations for testing

## Common Patterns to Avoid
- **Don't**: Use raw pointers
- **Don't**: Allocate memory dynamically
- **Don't**: Use floating point math
- **Don't**: Mix input handling with game logic
- **Don't**: Create sprites without proper management

## Code Style
- **Namespace**: All code in `str` namespace
- **Naming**: PascalCase for classes, camelCase for methods
- **Constants**: UPPER_SNAKE_CASE
- **Comments**: Use pattern headers for major classes
