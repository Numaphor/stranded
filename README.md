# Stranded

A top-down sci-fi RPG shooter built with the **Butano** engine for Game Boy Advance.

![Game Boy Advance](https://img.shields.io/badge/Platform-Game%20Boy%20Advance-green)
![Butano Engine](https://img.shields.io/badge/Engine-Butano-blue)
![C++](https://img.shields.io/badge/Language-C%2B%2B-orange)

## Overview

Stranded is an action-packed adventure game that combines classic top-down RPG mechanics with modern shooter elements. Explore multiple worlds, battle various enemies, interact with NPCs, and uncover the mysteries of this sci-fi universe.

## Features

### 🎮 Gameplay
- **Top-down action combat** with multiple weapon types
- **Hero character** with sword combat abilities
- **Enemy encounters** with Spearguard enemies
- **Interactive NPCs** including merchants
- **Companion system** with combat assistance and revival mechanics
- **World state management** with save/load functionality

### 🌍 Worlds & Exploration
- **Multiple interconnected worlds** (Main World, Forest Area)
- **Dynamic environments** with unique enemies per world
- **Minimap system** for navigation
- **Zone-based interactions** and collision detection
- **Zoom feature** for tactical overview (toggle with SELECT key)

### ⚔️ Combat System
- **Multiple movement states** (Idle, Walking, Running, Rolling, Combat)
- **Various attack types** (Chopping, Slashing, Ranged attacks)
- **Health and status management**
- **Power-ups and buffs** (Heal, Defense, Power, Energy)

### 🛠️ Technical Features
- **Advanced hitbox system** with optional debug visualization
- **Entity-Component architecture** for clean code organization
- **Scene management** (Start Screen, Controls Screen, Level Selector, World transitions)
- **Camera system** with smooth player tracking
- **Audio system** with sound effects and background music
- **Sprite animation** and visual effects
- **State machine pattern** for enemy AI and player states
- **World state persistence** using save/load system
- **Screen shake effects** for impactful combat feedback

## Requirements

### Development Environment
- **devkitARM** - ARM cross-compilation toolchain from devkitPro
- **Wonderful Toolchain** - Alternative ARM toolchain (optional)
- **Python 3.x** - Required for build tools and asset processing
- **Make** - Build system (GNU Make recommended)
- **Git** - Version control with submodule support

### Runtime
- **Game Boy Advance** hardware or compatible emulator
- **mGBA**, Visual Boy Advance, or similar emulator for testing

## Setup & Installation

1. **Clone the repository** with submodules:
   ```bash
   git clone --recursive https://github.com/Numaphor/stranded.git
   cd stranded
   ```

2. **Initialize submodules** (if not cloned with `--recursive`):
   ```bash
   git submodule update --init --recursive
   ```

3. **Install development tools**:
   - Install [devkitPro](https://devkitpro.org/wiki/Getting_Started) with devkitARM
   - Ensure `DEVKITARM` environment variable is set
   - Ensure Python 3.x is available in your PATH
   - Install Make (available on most Unix systems, or via MSYS2 on Windows)

## Building

### Quick Build
```bash
make -j8
```

### Development Workflow
```bash
# Kill any running emulator (Windows example)
taskkill /im mGBA.exe /F   # Windows
# pkill mGBA                # Linux/macOS

# Build and run
make -j8
```

### Troubleshooting
If you encounter build errors:

1. **"DEVKITARM not found"**: Install devkitPro and ensure environment variables are set
2. **Python errors**: Ensure Python 3.x is installed and in PATH
3. **Submodule issues**: Run `git submodule update --init --recursive`
4. **Permission errors**: Ensure write access to build directory

### Build Artifacts
- `stranded.gba` - Final ROM file for emulator or hardware
- `stranded.elf` - Debug symbols for development

### Build Configuration
The build system automatically processes assets and uses the following directories:
- `src/` - C++ source files
- `include/` - Header files  
- `graphics/` - Visual assets organized by type:
  - `sprite/player/` - Player character sprites (hero_sword.bmp, soldier.bmp)
  - `sprite/enemy/` - Enemy sprites (spearguard.bmp)
  - `sprite/npc/` - NPC sprites (merchant.bmp, companion.bmp)
  - `sprite/vfx/` - Visual effects
  - `bg/` - Background tiles and maps
- `audio/` - Sound effects (`.wav`) and music (`.it` modules)
- `build/` - Generated build artifacts

## Controls

### Menu Navigation
- **D-Pad Up/Down** - Navigate menu options
- **A Button** - Select/Confirm
- **B Button** - Back/Cancel

### Gameplay
- **D-Pad** - Move character
- **A Button** - Interact/Confirm
- **B Button** - Attack/Back
- **L Button** - Switch Weapon
- **R Button** - Roll/Dodge
- **Start** - Pause menu
- **Select** - Secondary functions

### Debug Controls
- **Select + A** - Access world selection menu
- **Select** - Toggle zoom (tactical view)

## Game Content

### Playable Character
The game features the **Hero** character with complete sword combat animations and abilities.

### Worlds
Explore two distinct environments:
- **Main World (World 0)** - Starting area with merchant NPC and balanced enemy encounters
- **Forest Area (World 1)** - Forest-themed environment with unique tile sets and enemy configurations

### Enemies
Battle against enemy encounters:
- **Spearguard** - Melee combatants with patrol and chase patterns

### NPCs
Interact with non-player characters:
- **Merchant** - Trade and shop keeper

### Companion System
A loyal companion that follows and assists in combat:
- Automatically follows the player and attacks enemies
- Can be revived by the player if defeated
- Provides tactical combat support

## Game Mechanics

### Character Progression
- Health and status management
- Equipment and power-up systems

### World Exploration
- Seamless transitions between areas
- Interactive zones and collision detection
- Merchant interactions and trading
- Environmental storytelling through NPCs
- World state persistence (saves player position and health per world)

### Combat
- Real-time action combat
- Multiple attack patterns and strategies
- Enemy AI with state machines (Idle, Patrol, Chase, Attack, Return to Post, Stunned)
- Tactical companion usage

## Development

### Project Structure
```
stranded/
├── src/                 # C++ implementation files
├── include/             # Header files
├── graphics/            # Visual assets
│   ├── bg/             # Background tiles
│   └── sprite/         # Character and object sprites
├── audio/              # Sound effects and music
├── butano/             # Butano engine (submodule)
├── build/              # Build artifacts
└── tools/              # Development utilities
```

### Architecture
- **Scene System**: Start Screen, Controls Screen, Level Selector (Menu), and World scenes with state management
- **Entity Framework**: Base Entity class extended by Player, Enemy, and NPC
- **Component Systems**: Hitbox, Movement, Animation, Audio, Bullet Management
- **State Management**: World persistence with save/load functionality via WorldStateManager
- **Enemy AI**: State machine with multiple states (Idle, Patrol, Chase, Attack, Return to Post, Stunned)
- **Camera System**: Smooth tracking with screen shake effects for combat feedback

### Debugging
- Comprehensive hitbox visualization system
- Console logging for collision detection
- Performance monitoring tools
- Memory usage tracking

## Credits

### Engine
- **Butano** - Modern C++ high-level engine for Game Boy Advance
- **GValiente** - Butano engine creator

### Development
- Game design and implementation by Numa
- Built for the Game Boy Advance homebrew community

## License

This project is open source. Please see the LICENSE file for more details.
