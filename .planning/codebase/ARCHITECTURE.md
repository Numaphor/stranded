# Architecture

**Analysis Date:** 2026-01-24

## Pattern Overview

**Overall:** Scene-based Entity Component System with State Machines

**Key Characteristics:**
- Scene-driven game flow with hierarchical state management
- Entity component architecture with spatial partitioning
- Chunk-based world streaming for large maps
- State machine pattern for enemy AI behaviors
- Memory-conscious design for GBA hardware constraints

## Layers

**Scene Layer:**
- Purpose: High-level game flow and state management
- Location: `src/main.cpp`, `include/str_scene*.h`, `src/core/scenes.cpp`
- Contains: Scene enumeration, scene implementations, main game loop
- Depends on: Entity layer, Butano framework
- Used by: Hardware bootstrap

**Entity Layer:**
- Purpose: Game object abstraction with component-based design
- Location: `include/str_entity.h`, `src/core/entity.cpp`, `src/actors/`
- Contains: Base entity class, player, enemies, NPCs
- Depends on: Component layer (hitboxes, movement), collision system
- Used by: Scene layer, world layer

**World Layer:**
- Purpose: Spatial management and chunk streaming
- Location: `include/str_scene_world.h`, `src/core/world.cpp`, `include/str_chunk_manager.h`
- Contains: World coordinates, chunk loading, camera management
- Depends on: Entity layer, collision layer
- Used by: Scene layer

**Component Layer:**
- Purpose: Modular functionality attachments to entities
- Location: `include/str_hitbox.h`, `include/str_movement.h`, `include/str_collision.h`
- Contains: Hitboxes, movement systems, collision detection
- Depends on: Utility layer
- Used by: Entity layer

**Manager Layer:**
- Purpose: System-wide resource coordination
- Location: `include/str_*_manager.h`, templates/
- Contains: Bullet management, chunk management, state coordination
- Depends on: Entity layer, component layer
- Used by: World layer, scene layer

## Data Flow

**Scene Update Flow:**

1. Main loop processes current scene in `src/main.cpp`
2. Scene-specific update calls (Start, Menu, Controls, World)
3. World scene updates entities, camera, and systems
4. Entity update propagates to components and state machines
5. Manager systems handle cross-entity coordination

**Entity Update Flow:**

1. Base entity update called from world/scene
2. Position update with hitbox synchronization
3. State machine evaluation (for enemies)
4. Component-specific updates (movement, collision)
5. Sprite and visual state updates

**Chunk Streaming Flow:**

1. Player movement triggers chunk boundary checks
2. ChunkManager determines required chunks in view
3. Stream chunks incrementally to VRAM buffer
4. Background updates with new tile data
5. Entity spawning based on chunk data

**State Management:**
- Scene state managed through enum-based switching
- Enemy AI handled through hierarchical state machines
- World state persisted across scene transitions
- Component state managed within entity lifecycle

## Key Abstractions

**Entity Abstraction:**
- Purpose: Base class for all game objects with spatial properties
- Examples: `include/str_entity.h`, `src/actors/player.cpp`, `src/actors/enemy.cpp`
- Pattern: Component-based with position, hitbox, and sprite integration

**Scene Abstraction:**
- Purpose: High-level game state container
- Examples: `include/str_scene.h`, `include/str_scene_world.h`
- Pattern: Enum-driven state machine with execute() methods

**Chunk Abstraction:**
- Purpose: Large world optimization through spatial partitioning
- Examples: `include/str_chunk_manager.h`, `include/str_world_map_data.h`
- Pattern: Streaming buffer with on-demand loading

**State Machine Abstraction:**
- Purpose: Complex AI behavior management
- Examples: `include/str_enemy_state_machine.h`, `include/str_enemy_states.h`
- Pattern: Polymorphic states with transition management

## Entry Points

**Main Entry Point:**
- Location: `src/main.cpp`
- Triggers: GBA hardware initialization
- Responsibilities: Scene loop initialization, Butano framework setup

**Scene Entry Points:**
- Location: `src/core/scenes.cpp`
- Triggers: Main loop scene switching
- Responsibilities: Scene-specific logic and transitions

**World Entry Point:**
- Location: `src/core/world.cpp::execute()`
- Triggers: Scene transition to WORLD
- Responsibilities: Game world initialization and main game loop

## Error Handling

**Strategy:** Graceful degradation with state validation

**Patterns:**
- Optional types for nullable resources (`bn::optional`)
- Boundary checking for chunk coordinates
- State validation in entity updates
- Resource cleanup in destructors

## Cross-Cutting Concerns

**Memory Management:** Manual RAII with stack allocation where possible, EWRAM for large buffers
**Validation:** Position bounds checking, entity count limits
**Hardware Constraints:** VRAM management, sprite limits, affine transform restrictions
**Performance:** Fixed-point arithmetic, chunk streaming budgeting

---

*Architecture analysis: 2026-01-24*