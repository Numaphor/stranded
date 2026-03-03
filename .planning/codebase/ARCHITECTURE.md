# Architecture

Analysis date: 2026-02-09
Last updated: 2026-03-03

## Scene System

The game uses a scene enum dispatched in `src/main.cpp`.

### Scene Enum

```cpp
enum class Scene { START, CHARACTER_SELECT, MENU, CONTROLS, WORLD, MODEL_VIEWER, ROOM_VIEWER };
```

Default scene is **ROOM_VIEWER**. The `START` case immediately redirects to `ROOM_VIEWER`.

### Scene Lifecycle

| Scene | File(s) | Ownership |
|-------|---------|-----------|
| START | `src/core/scenes.cpp` | Stack (trivial redirect) |
| CHARACTER_SELECT | `src/core/scenes.cpp` | Stack |
| MENU | `src/core/scenes.cpp` | Stack |
| CONTROLS | `src/core/scenes.cpp` | Stack |
| WORLD | `src/core/world.cpp` | Stack |
| MODEL_VIEWER | `src/core/model_viewer.cpp` | Heap (`new`/`delete`) |
| ROOM_VIEWER | `src/room_viewer.cpp` | Heap (`new`/`delete`) |

Viewer scenes (MODEL_VIEWER, ROOM_VIEWER) use manual heap allocation. All other scenes are stack-owned in `main.cpp`.

### Scene Flow

```text
ROOM_VIEWER (default)
    |
    v
[Gameplay scenes when implemented]
START -> CHARACTER_SELECT -> CONTROLS -> MENU -> WORLD
                                                   |
                                              MODEL_VIEWER
```

Shared state between scenes: `selected_character_id`, `selected_world_id`, `spawn_location`, `WorldState`.

## Architecture Layers

### Presentation Layer

Scenes, rendering, HUD, and user-facing display.

| File | Responsibility |
|------|---------------|
| `src/core/scenes.cpp` | Menu/select/controls scene rendering |
| `src/core/hud.cpp` | Healthbar, weapon icon, ammo counter, soul indicator, buff menu, gun selection menu, energy display, alert display |
| `src/core/minimap.cpp` | Sprite-based minimap (6 rooms, player arrow, enemy dots, door indicators, pulse animation) |
| `src/core/minimap_canvas.cpp` | BG tile-based minimap (EWRAM-backed, palette-colored room states) |
| `src/core/model_viewer.cpp` | 3D model viewer scene |
| `src/room_viewer.cpp` | 3D isometric room viewer scene with dialog |

### Gameplay Layer

Entity behavior, combat, and interaction logic.

| File | Responsibility |
|------|---------------|
| `src/actors/player.cpp` | Player entity: movement, combat (chop/slash/shoot), rolling, buffs, companion, strafing, combo system |
| `src/actors/soldier.cpp` | Soldier subclass: burst fire, accuracy boost, tactical buffs |
| `src/actors/enemy.cpp` | Enemy entity: AI state machine, knockback, death animation, patrol/chase/attack |
| `src/actors/npc.cpp` | NPC entity: dialog system, options, merchant integration |
| `src/core/bullet.cpp` | Bullet pool (32 bullets), direction-based velocity, enemy collision |
| `src/core/collectible.cpp` | Health pickups, hitbox-based collection |
| `src/core/quest.cpp` | Quest management: start, progress tracking, completion, rewards |

### State Layer

Game state, world coordination, and persistence.

| File | Responsibility |
|------|---------------|
| `src/core/world.cpp` | Main gameplay coordinator: owns Player, enemies, minimap, quest manager, camera updates, zoom |
| `src/core/world_state.cpp` | Save/load state: world_id, player position/health, character_id (8 max slots) |

### Engine Integration Layer

Low-level systems that bridge game logic to Butano/GBA hardware.

| File | Responsibility |
|------|---------------|
| `src/core/entity.cpp` | Base class for all positioned game objects: pos, hitbox, sprite |
| `src/core/collision.cpp` | AABB collision detection between Hitbox objects |
| `src/core/movement.cpp` | Velocity-based movement with acceleration/friction (base + EnemyMovement) |
| `src/core/direction_utils.cpp` | Roll offsets, gun positioning, bullet spawn positions per direction |
| `src/core/level.cpp` | Tile-based level: floor/zone tiles, merchant zones, position validation |

### 3D Viewer Runtime

Adapted from varooom-3d. See `3D_ENGINE.md` for details.

| File | Responsibility |
|------|---------------|
| `src/viewer/fr_models_3d.cpp` | Model transforms, projection, depth sorting, sprite submission |
| `src/viewer/fr_models_3d.bn_iwram.cpp` | Performance-critical IWRAM variant of render loops |
| `src/viewer/fr_camera_3d.cpp` | Camera positioning and projection |
| `src/viewer/fr_shape_groups.cpp` | Sprite shape group management |
| `src/viewer/fr_shape_groups.bn_iwram.cpp` | IWRAM variant of shape group operations |
| `src/viewer/fr_sin_cos.cpp` | Fixed-point trig tables |
| `src/viewer/fr_div_lut.cpp` | Division lookup table |
| `src/viewer/fr_menu_keypad.cpp` | Menu input handling for viewer |

## Entity Hierarchy

```text
Entity (base: pos, hitbox, sprite, update())
  |-- Player (movement, combat, companion, buffs, HP, ammo, energy)
  |     `-- Soldier (burst fire, accuracy boost, damage boost)
  |-- Enemy (AI state machine, knockback, patrol, chase, attack)
  `-- NPC (dialog system, options, merchant quest integration)
        `-- MerchantNPC (quest-driven dynamic dialog)

Collectible (standalone, not Entity: health pickups)
```

## Key Subsystems

### Combat System

- **Weapons**: GUN (ranged, BulletManager) and SWORD (melee, chop/slash/attack)
- **Combo system**: COMBO_WINDOW=60 frames, sequential attack combos
- **Buffs**: Heal, Energy, Power via buff menu (L button hold, cooldown 600 frames)
- **Bullets**: 32-bullet pool, direction-based velocity, enemy collision
- **Enemy knockback**: 3.5 strength, 10 frame duration

### Enemy AI State Machine

Six states: Idle(60f) -> Patrol(90f, random) -> Chase(speed 0.35) -> Attack(60f) -> ReturnToPost(speed 0.25) -> Stunned(10f).

Enemy types: SPEARGUARD, SLIME, MUTANT.

### Dialog Systems

Two dialog implementations:

1. **BgDialog** (`str_bg_dialog.h`): BG-layer bitmap font dialog using no sprite VRAM. Renders text via BG map tiles. Typewriter effect. Used in world scenes.
2. **RoomDialog** (`str_room_dialog.h`): Sprite-based dialog for room viewer. Frees HUD sprites for VRAM during dialog.

Both share: IDLE/GREETING/SHOWING_OPTIONS/SHOWING_RESPONSE states, scrollable options (max 8, 3 visible), DialogOption struct.

### Quest System

QuestManager with max 8 quest slots. Quest IDs: COLLECT_HEARTS, SLAY_ENEMIES. Status: NotStarted -> Active -> Completed. MerchantNPC offers/tracks quests dynamically.

### Minimap System

Two implementations:

1. **Minimap** (`str_minimap.h`): Sprite-based, 6 rooms in 2x3 grid. Player arrow, enemy dots (max 16), door indicators (7 pairs), pulse animation on current room. Room states: UNVISITED/VISITED/CURRENT.
2. **MinimapCanvas** (`str_minimap_canvas.h`): BG tile-based, EWRAM-backed 32x32 map. Palette-colored room states. Lower VRAM cost.

### HUD System

Manages: healthbar (BG-based, 0-3 HP with transition animations), soul indicator (animated, defense/silver buff visuals), weapon icon + ammo counter, buff menu (3 options with hold activation), gun selection menu (2x3 grid, 6 guns), energy display, alert display.

### Character System

Two characters: Hero (id=0, balanced) and Soldier (id=1, ranged specialist). Both unlocked by default. Soldier extends Player with burst fire and tactical buffs.

## Room Viewer Architecture

See `3D_ENGINE.md` for the full 3D engine reference.

Key points:

- Isometric projection with fixed angles: phi=6400, theta=59904, psi=6400
- 6 rooms in 2x3 grid with door transitions (16 frames, smoothstep easing)
- Camera follows committed heading with behind-offset, quantized to quarter turns for `_corner_index`
- Camera distance: adjustable via L/R, clamped 100-500
- Runtime budget: 100 max vertices, 300 max faces
- Player direction: 5 states (down, down_side, side, up_side, up)
