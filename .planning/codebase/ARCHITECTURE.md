# Architecture

**Analysis Date:** 2026-02-09

## Pattern Overview

**Overall:** Scene-based Game Architecture with Entity-Component Pattern

**Key Characteristics:**
- Scene-driven flow with centralized state management
- Entity-based actor system with inheritance hierarchy
- State machine pattern for player and enemy behaviors
- Component-based design for reusable game systems
- Hardware-optimized rendering pipeline for GBA constraints

## Layers

**Presentation Layer:**
- Purpose: Scene management, UI rendering, and visual effects
- Location: `src/core/scenes.cpp`, `include/str_scene_*.h`
- Contains: Start screen, Menu, Controls, World scenes
- Depends on: Butano rendering engine, input system
- Used by: Main game loop

**Game Logic Layer:**
- Purpose: Core game mechanics, rules, and entity behaviors
- Location: `src/actors/`, `src/core/`
- Contains: Player, Enemy, NPC classes, collision, movement
- Depends on: Entity base classes, state machines
- Used by: World scene

**Data Management Layer:**
- Purpose: Game state persistence, world state management
- Location: `src/core/world_state.cpp`, `include/str_world_state.h`
- Contains: Save/load functionality, world-specific data
- Depends on: SRAM storage, entity positions
- Used by: World scene

**Engine Abstraction Layer:**
- Purpose: Hardware-specific optimizations and rendering pipeline
- Location: `butano/` library integration
- Contains: Sprite management, background rendering, audio
- Depends on: GBA hardware, devkitARM toolchain
- Used by: All game components

## Data Flow

**Scene Transitions:**

1. Main loop reads Scene enum and instantiates appropriate scene
2. Scene executes and returns next Scene enum
3. Main loop destroys current scene and creates new one
4. State (spawn location, world ID) passed between scenes

**Game Loop Update:**

1. `bn::core::update()` processes hardware input
2. Player movement system updates position and state
3. Enemy AI updates based on player position
4. Collision detection handles interactions
5. Rendering system updates sprites and backgrounds
6. Camera system follows player with lookahead

**State Management:**
- Global state managed by `WorldStateManager` singleton
- Player state through `PlayerMovement` state machine
- Enemy state through `EnemyStateMachine`
- Scene state through enum-based navigation

## Key Abstractions

**Entity Base Class:**
- Purpose: Common interface for all game objects
- Examples: `src/actors/player.cpp`, `src/actors/enemy.cpp`, `src/actors/npc.cpp`
- Pattern: Inheritance with virtual methods for update and render

**Scene System:**
- Purpose: Encapsulate game states and transitions
- Examples: `src/core/scenes.cpp` (Menu, Start, Controls, World)
- Pattern: State pattern with execute() methods returning next scene

**Hitbox System:**
- Purpose: Collision detection between game objects
- Examples: `src/core/collision.cpp`, `include/str_hitbox.h`
- Pattern: Component-based collision with AABB detection

**State Machine Pattern:**
- Purpose: Manage complex entity behaviors
- Examples: Player states (IDLE, WALKING, ROLLING, ATTACKING)
- Pattern: Enum-based states with transition logic

## Entry Points

**Main Entry Point:**
- Location: `src/main.cpp`
- Triggers: GBA boot sequence
- Responsibilities: Initialize Butano, run scene loop, handle global state

**Scene Entry Points:**
- Location: `src/core/scenes.cpp` (Menu, Start, Controls, World classes)
- Triggers: Scene transitions from main loop
- Responsibilities: Scene-specific initialization, input handling, render loop

**World Entry Point:**
- Location: `src/core/world.cpp` (World::execute method)
- Triggers: Transition from Menu scene
- Responsibilities: Initialize game world, spawn entities, run game loop

## Error Handling

**Strategy:** Assertions with graceful degradation

**Patterns:**
- Butano assertions for critical failures
- State validation in entity updates
- Collision bounds checking
- Resource cleanup in destructors

## Cross-Cutting Concerns

**Logging:** Butano's `bn::core::log()` for debug output

**Validation:** Hitbox boundary checks, state transitions

**Authentication:** Not applicable (single-player game)

---

*Architecture analysis: 2026-02-09*