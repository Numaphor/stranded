# Stranded Game - AI Coding Agent Instructions

This is a top-down adventure game built with the **Butano engine** for Game Boy Advance. The project follows a specific architecture and conventions that AI agents should understand for effective contributions.

## Architecture Overview

### Core Scene System

- Scene management via `fe::Scene` enum (MENU, WORLD) with state transitions in `main.cpp`
- `World::execute()` is the main game loop handling player, enemies, NPCs, and collision detection
- World-specific content loaded by `_init_world_specific_content()` based on `world_id` parameter
- State persistence through `WorldStateManager` singleton for saving/loading player progress across worlds

### Entity-Component Architecture

- Base `Entity` class with `Hitbox`, position, and sprite management
- `Player` extends Entity with movement states, health, weapons, and companion system
- `Enemy` uses state machine pattern (`EnemyStateMachine` + concrete states like `ChaseState`, `AttackState`)
- NPCs use polymorphic design (`MerchantNPC`, `PenguinNPC`, `TortoiseNPC` inherit from base `NPC`)

### Collision & Interaction Systems

- **Sprite-based collision**: Player vs enemies, bullets vs enemies using `Hitbox::collides_with()`
- **Tile-based collision**: Level boundaries, merchant zones via `Level::is_position_valid()`
- **Zone system**: Merchant interaction (100x100), collision (24x24), and sword zones with hardcoded coordinates
- Debug visualization via `HitboxDebug` class (toggle with SELECT+START)

### Level & World Management

- `Level` class handles tile-based collision detection and zone management
- Background tiles dynamically updated for debug visualization zones
- Camera follows player with precise tracking: `camera.set_x(player_pos.x())`
- Multi-world system (0=Main, 1=Forest, 2=Desert, 3=Ocean) with unique enemies/NPCs per world

## Build & Development Workflow

### Essential Commands

```bash
# Build and run (from project root)
make -j8

# Kill existing emulator before rebuild
taskkill /im mGBA.exe /F

# Full workflow
taskkill /im mGBA.exe /F; make -j8
```

### File Organization

- `src/` - All C++ implementation files
- `include/` - All header files (.h)
- `graphics/` - Sprite assets (.bmp + .json pairs)
- `audio/` - Sound effects and music
- Build artifacts in `build/` (never use `make clean`)

## Butano Engine Specifics

### Critical Patterns

- **Sprite positioning**: Butano sprites positioned by center, not top-left
- **Z-ordering**: Negative values for higher priority (`-32767` = topmost)
- **Camera system**: All sprites/backgrounds should `set_camera()` for proper scrolling
- **Memory management**: Use Butano containers (`bn::vector`, `bn::unique_ptr`) and avoid heap allocation
- **Tile system**: 1 tile = 8x8 pixels, sprites typically 16x16 or 32x32

### Asset Integration

- Graphics auto-generated as `bn::sprite_items::name` from `graphics/name.bmp`
- Audio as `bn::sound_items::name` from corresponding audio files
- Background maps from `bn::regular_bg_items::name`

## Debug & Testing Features

### Debug Controls

- `SELECT + START`: Toggle hitbox visualization
- `SELECT + A`: Access world selection menu
- Debug markers show collision zones, interaction areas, and entity hitboxes

### State Machine Debugging

- Enemy states visible through `EnemyStateMachine::get_current_state_id()`
- Collision logging via `fe::Collision::log_collision()`
- Visual debug tiles for zones (tile 3=collision, tile 4=interaction)

## Key Conventions

### Naming Patterns

- Namespace: `fe` for all game code
- Enums: `ENEMY_TYPE::SPEARGUARD`, `Scene::WORLD`
- Private members: `_member_name`
- Friend classes extensively used for tight coupling (World ↔ Player ↔ Enemy)

### Performance Considerations

- Zone bounds cached to avoid expensive map scanning each frame
- Bullet collision checks optimized with early exits
- Entity removal deferred until after iteration completion
- Background tile updates batched and conditionally reloaded

## Common Pitfalls

- Never modify position without checking `Level::is_position_valid()`
- Sprite creation requires camera assignment for scrolling worlds
- State machine transitions must call `exit()` → `enter()` sequence
- Zone collision uses exclusive upper bounds (`< zone_right` not `<= zone_right`)
- Background map reloading needed after tile updates via `bg_map_ptr.reload_cells_ref()`

When implementing features, always build and test immediately. The game should run without errors on mGBA emulator for successful integration.
