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

## Headless Game Testing with mGBA Python Bindings

AI agents can run and test the game headlessly (without a display server) using mGBA's Python bindings. This enables automated visual testing, screenshot capture, and programmatic game interaction.

### Setup Requirements

1. **Build mGBA from source with Python bindings:**
```bash
# Install dependencies
sudo apt-get install -y cmake libpng-dev libsqlite3-dev python3-dev python3-cffi \
  libavcodec-dev libavformat-dev libavutil-dev libswscale-dev libavfilter-dev

# Clone mGBA (use version 0.10.3 for stability)
git clone --depth 1 --branch 0.10.3 https://github.com/mgba-emu/mgba.git /tmp/mgba-src

# Configure with Python bindings and FFmpeg (required for video buffer capture)
cd /tmp/mgba-src && mkdir build && cd build
cmake -DBUILD_PYTHON=ON -DBUILD_QT=OFF -DBUILD_SDL=OFF \
  -DUSE_PNG=ON -DUSE_FFMPEG=ON -DUSE_DISCORD_RPC=OFF ..

# Build
make -j8

# Install Python dependencies
pip install cached_property Pillow
```

2. **Set environment variables:**
```bash
export PYTHONPATH="/tmp/mgba-src/build/python/lib.linux-x86_64-cpython-312:$PYTHONPATH"
export LD_LIBRARY_PATH="/tmp/mgba-src/build:$LD_LIBRARY_PATH"
```

### GBA Key Constants

```python
GBA_KEY_A = 0
GBA_KEY_B = 1
GBA_KEY_SELECT = 2
GBA_KEY_START = 3
GBA_KEY_RIGHT = 4
GBA_KEY_LEFT = 5
GBA_KEY_UP = 6
GBA_KEY_DOWN = 7
GBA_KEY_R = 8
GBA_KEY_L = 9
```

### Basic Usage Example

```python
import mgba.core
import mgba.image
from PIL import Image

# Load the ROM
core = mgba.core.load_path("src.gba")
width, height = core.desired_video_dimensions()

# Create framebuffer and set video output
framebuffer = mgba.image.Image(width, height)
core.set_video_buffer(framebuffer)

# Reset and run
core.reset()

# Run frames (create new framebuffer each frame for proper capture)
for i in range(300):
    framebuffer = mgba.image.Image(width, height)
    core.set_video_buffer(framebuffer)
    core.run_frame()

# Save screenshot
pil_image = framebuffer.to_pil().convert('RGB')
pil_image.save("screenshot.png")
```

### Sending Key Inputs

```python
def press_key(core, key, hold_frames=5, wait_frames=10):
    """Press a key for specified frames, then wait"""
    # Hold the key
    core.set_keys(key)
    for _ in range(hold_frames):
        framebuffer = mgba.image.Image(width, height)
        core.set_video_buffer(framebuffer)
        core.run_frame()
    
    # Release the key
    core.clear_keys(key)
    for _ in range(wait_frames):
        framebuffer = mgba.image.Image(width, height)
        core.set_video_buffer(framebuffer)
        core.run_frame()

# Example: Navigate menu and select option
press_key(core, GBA_KEY_A)  # Confirm selection
press_key(core, GBA_KEY_DOWN)  # Navigate down
press_key(core, GBA_KEY_START)  # Start game
```

### Complete Testing Script Template

```python
#!/usr/bin/env python3
import sys
import os

# Set up paths
sys.path.insert(0, '/tmp/mgba-src/build/python/lib.linux-x86_64-cpython-312')
os.environ['LD_LIBRARY_PATH'] = '/tmp/mgba-src/build:' + os.environ.get('LD_LIBRARY_PATH', '')

import mgba.core
import mgba.image
from PIL import Image

# GBA Key constants
GBA_KEY_A, GBA_KEY_B, GBA_KEY_SELECT, GBA_KEY_START = 0, 1, 2, 3
GBA_KEY_RIGHT, GBA_KEY_LEFT, GBA_KEY_UP, GBA_KEY_DOWN = 4, 5, 6, 7

ROM_PATH = "src.gba"

# Load ROM
core = mgba.core.load_path(ROM_PATH)
width, height = core.desired_video_dimensions()
framebuffer = mgba.image.Image(width, height)
core.set_video_buffer(framebuffer)
core.reset()

def run_frames(count, save_path=None):
    global framebuffer
    for _ in range(count):
        framebuffer = mgba.image.Image(width, height)
        core.set_video_buffer(framebuffer)
        core.run_frame()
    if save_path:
        framebuffer.to_pil().convert('RGB').save(save_path)

def press_key(key, hold=5, wait=10):
    core.set_keys(key)
    run_frames(hold)
    core.clear_keys(key)
    run_frames(wait)

# Test sequence
run_frames(300, "step1_menu.png")      # Wait for menu
press_key(GBA_KEY_A)                    # Select Play Game
run_frames(60, "step2_world_select.png")
press_key(GBA_KEY_A)                    # Select first world
run_frames(120, "step3_gameplay.png")   # Capture gameplay
```

### Use Cases

1. **Visual Regression Testing**: Capture screenshots at key points and compare against baselines
2. **Automated Gameplay Testing**: Navigate menus, trigger actions, verify game state
3. **CI/CD Integration**: Run headless tests in GitHub Actions without display server
4. **Bug Reproduction**: Script exact input sequences to reproduce issues
5. **Screenshot Documentation**: Generate screenshots for README and documentation
