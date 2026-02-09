# Architecture

**Analysis Date:** 2026-02-09

## Pattern Overview

**Overall:** Game loop with scene stack pattern and entity-component architecture

**Key Characteristics:**
- Scene-based navigation system with state machine pattern
- Entity hierarchy with component-based actors
- Fixed-point arithmetic for GBA hardware constraints
- Memory-conscious design with EWRAM/IWRAM management
- Frame-based update loop with 60 FPS target

## Layers

**Application Layer:**
- Purpose: Main game loop and scene orchestration
- Location: `src/main.cpp`
- Contains: Scene switching, initialization, core update loop
- Depends on: Scene implementations, Butano core
- Used by: Hardware/emulator entry point

**Scene Layer:**
- Purpose: UI screens and game flow management
- Location: `src/core/scenes.cpp`, `include/str_scene_*.h`
- Contains: Start screen, menu, controls, world scene
- Depends on: Player, World, UI components
- Used by: Application layer

**Game World Layer:**
- Purpose: Core gameplay mechanics and world state
- Location: `src/core/world.cpp`, `include/str_world_state.h`
- Contains: World management, camera, enemy spawning, collision
- Depends on: Player, Level, Enemies, NPCs
- Used by: World scene

**Entity Layer:**
- Purpose: Game actors and their behaviors
- Location: `src/actors/`, `include/str_*.h`
- Contains: Player, enemies, NPCs, bullets
- Depends on: Movement systems, animation, collision
- Used by: World layer

**Core Systems Layer:**
- Purpose: Shared game mechanics and utilities
- Location: `src/core/`, `include/str_*.h`
- Contains: Collision, movement, HUD, minimap, level
- Depends on: Butano engine, math utilities
- Used by: Entity layer, World layer

## Data Flow

**Game Update Cycle:**

1. Main loop (`src/main.cpp`) - Scene selection and bn::core::update()
2. Scene execution (`src/core/scenes.cpp`) - UI or world scene
3. World update (`src/core/world.cpp`) - Player, enemies, camera, collision
4. Entity updates (`src/actors/*.cpp`) - Movement, animation, state
5. Render frame - Butano handles sprite/background drawing

**State Management:**
- WorldStateManager singleton for persistent save data
- Scene enum for navigation state
- Player state machine for movement/actions
- Enemy state machines for AI behavior

## Key Abstractions

**Scene System:**
- Purpose: Encapsulate different game states/screens
- Examples: `str::Start`, `str::Menu`, `str::World`, `str::Controls`
- Pattern: Each scene has execute() method returning next scene

**Entity Base Class:**
- Purpose: Common interface for all game objects
- Examples: Player, Enemy, NPC inheritance
- Pattern: Virtual methods for position, hitbox, update

**State Machine Pattern:**
- Purpose: Manage complex behaviors and animations
- Examples: PlayerMovement::State, Enemy states
- Pattern: Enum states with transition logic and timers

**Fixed-Point Math:**
- Purpose: Hardware-compatible decimal arithmetic
- Examples: bn::fixed for all positions and velocities
- Pattern: bn::fixed_point for 2D coordinates

## Entry Points

**main():**
- Location: `src/main.cpp`
- Triggers: GBA boot/ROM load
- Responsibilities: Initialize Butano, run scene loop, handle core updates

**Scene::execute():**
- Location: Each scene class in `src/core/scenes.cpp`
- Triggers: User navigation or game events
- Responsibilities: Handle scene-specific logic, return next scene

**World::execute():**
- Location: `src/core/world.cpp`
- Triggers: Scene::WORLD selected
- Responsibilities: Main gameplay loop, entity updates, collision detection

## Error Handling

**Strategy:** Asserts and defensive programming with graceful degradation

**Patterns:**
- Butano asserts for debugging (STACKTRACE enabled)
- Health system for player damage/death
- Reset functionality for corrupted state
- Position validation with boundary checks

## Cross-Cutting Concerns

- Logging: bn::core::log() to emulator debug window
- Validation: Boundary checking, position validation
- Resource management: bn::optional for sprite lifecycle, manual memory management with new/delete
- Performance: EWRAM allocation for large data structures, frame-based updates

---
*Architecture analysis: 2026-02-09*