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
- **Multiple playable characters** (Hero, Agent, Bunny, Soldier, Swordmaster)
- **Diverse enemy types** (Bat, Slime, Mutant, Spearguard, Rat, Boss)
- **Interactive NPCs** including merchants and story characters
- **Companion system** for strategic gameplay
- **World state management** with save/load functionality

### 🌍 Worlds & Exploration
- **Multiple interconnected worlds** (Main World, Forest Area, Desert, Ocean)
- **Dynamic environments** with unique enemies and NPCs per world
- **Minimap system** for navigation
- **Zone-based interactions** and collision detection

### ⚔️ Combat System
- **Multiple movement states** (Idle, Walking, Running, Rolling, Combat)
- **Various attack types** (Chopping, Slashing, Ranged attacks)
- **Health and status management**
- **Power-ups and buffs** (Heal, Defense, Power, Energy)

### 🛠️ Technical Features
- **Advanced hitbox system** with debug visualization
- **Entity-Component architecture** for clean code organization
- **Scene management** (Menu, World transitions)
- **Camera system** with smooth player tracking
- **Audio system** with sound effects and background music
- **Sprite animation** and visual effects

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
  - `sprite/player/` - Player character sprites (hero.bmp, agent.bmp, etc.)
  - `sprite/enemy/` - Enemy sprites (mutant.bmp, spearguard.bmp, etc.)
  - `sprite/npc/` - NPC sprites for interactions
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
- **A Button** - Primary action/attack
- **B Button** - Secondary action
- **Start** - Pause menu
- **Select** - Secondary functions

### Debug Controls
- **Select + Start** - Toggle hitbox visualization
- **Select + A** - Access world selection menu

## Game Mechanics

### Character Progression
- Multiple character types with unique abilities
- Health and status management
- Equipment and power-up systems

### World Exploration
- Seamless transitions between areas
- Interactive zones and collision detection
- Merchant interactions and trading
- Environmental storytelling through NPCs

### Combat
- Real-time action combat
- Multiple attack patterns and strategies
- Enemy AI with state machines
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
- **Scene System**: Menu and World scenes with state management
- **Entity Framework**: Base Entity class extended by Player, Enemy, NPC
- **Component Systems**: Hitbox, Movement, Animation, Audio
- **State Management**: World persistence and save/load functionality

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
- Game design and implementation by the Stranded development team
- Built for the Game Boy Advance homebrew community

## License

This project is open source. Please see the LICENSE file for more details.

---

**Note**: This is a homebrew game for Game Boy Advance. It requires appropriate development tools and emulator software for play and development.