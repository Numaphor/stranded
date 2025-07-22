# Stranded

A Game Boy Advance homebrew game built with the Butano C++ engine, featuring top-down action gameplay with shooting mechanics. This project utilizes assets from Penusbmic's "Stranded Starter Pack" available on itch.io.

## Overview

Stranded is a sci-fi themed action game where players navigate through levels, battle various enemies, and explore an atmospheric world. The game features pixel-perfect collision detection, enemy AI with sophisticated state machines, and a comprehensive level system.

## Features

- **Player Movement & Combat**: Smooth 8-directional movement with gun mechanics and bullet system
- **Enemy AI System**: Multiple enemy types with state machines (Idle, Patrol, Chase, Attack, Return to Post)
- **Level System**: Tile-based collision detection and zone management
- **Minimap**: Real-time minimap showing player and enemy positions
- **Health System**: Player health with invulnerability frames and visual feedback
- **Audio System**: Support for both WAV sound effects and Impulse Tracker music modules

## Game Mechanics

### Player Controls
- **Movement**: D-pad for 8-directional movement with momentum and friction
- **Strafing**: Hold R button to strafe (move without changing facing direction)
- **Shooting**: A button to toggle gun, B button to fire bullets
- **Interaction**: Automatic interaction with NPCs and objects

### Enemy Types
- **Spearguard**: Patrol-based enemies that chase players and return to their posts
- **Slime**: Basic enemy type with simple AI patterns
- **Mutant**: Advanced enemy with complex behavior patterns

### Combat System
- Bullet-based ranged combat
- Enemy knockback and invulnerability frames
- Health system with visual damage feedback
- Death animations and removal system

## Assets

This project incorporates assets from **Penusbmic's "Stranded Starter Pack"**, including:

- **Character Sprites**: Animated player character, enemies, and NPCs
- **Environment Art**: Sci-fi themed tilesets, buildings, and terrain
- **UI Elements**: Health bars, minimap icons, and interface components
- **Audio**: Sound effects (WAV) and background music (Impulse Tracker modules)

### Graphics Assets
- Hero sprite with idle, walking, hit, and death animations
- Enemy sprites including spearguards, slimes, and mutants
- Environmental tilesets for dungeons, houses, labs, and outdoor areas
- UI elements including health bars, minimaps, and cursors

### Audio Assets
- **Sound Effects**: `death.wav`, `eek.wav`, `growl.wav`, `steps.wav`, `swipe.wav`, etc.
- **Music Modules**: `crystal.it`, `mellowdy.it`, `mystic.it`, `sanctuary.it`, `wiskedaway.it`

## Build System

### Requirements
- **Wonderful Toolchain**: GBA development toolchain with devkitARM
- **Butano Engine**: Included as a git submodule
- **Python**: For asset processing scripts
- **Make**: Build system automation

### Building the Game
1. Clone the repository with submodules:
   ```bash
   git clone --recursive https://github.com/Numaphor/stranded.git
   cd stranded
   ```

2. Build the ROM:
   ```bash
   make
   ```

3. The build process will generate `stranded.gba` ROM file

### Build Configuration
The build system processes multiple asset types:
- **Graphics**: Converted using `grit` tool for GBA compatibility
- **Audio**: Standard audio files processed with `mmutil`
- **DMG Audio**: Chiptune modules processed with `mod2gbt` and `s3m2gbt`

## Development Workflow

### VS Code Integration
The project includes comprehensive VS Code configuration:
- **IntelliSense**: C++ language support with proper include paths
- **Debugging**: Integrated mGBA emulator debugging
- **Tasks**: Automated build and launch workflows

### Testing
- **Local Testing**: Use mGBA emulator for development testing
- **Web Testing**: Upload ROM to https://gba.nicholas-vancise.dev/ for browser-based testing
- **Hardware Testing**: Deploy to real GBA hardware or flash carts

### Development Environment
- **Build-only Policy**: Development rules enforce compilation-only workflow
- **Error Handling**: Structured error resolution with priority hierarchy
- **Version Control**: Git-based workflow with proper submodule management

## Project Structure

```
stranded/
├── src/                    # C++ source files
│   ├── main.cpp           # Main game loop
│   ├── fe_player.cpp      # Player mechanics
│   ├── fe_enemy.cpp       # Enemy AI system
│   ├── fe_level.cpp       # Level management
│   └── ...
├── include/               # Header files
│   ├── fe_player.h        # Player class definitions
│   ├── fe_enemy.h         # Enemy system headers
│   └── ...
├── graphics/              # Sprite and tileset assets
│   ├── hero.bmp           # Player character sprites
│   ├── spearguard.bmp     # Enemy sprites
│   ├── adventure.tmx      # Tiled map files
│   └── ...
├── audio/                 # Sound effects and music
│   ├── *.wav              # Sound effects
│   └── *.it               # Impulse Tracker modules
├── dmg_audio/             # Game Boy style audio
├── butano/                # Butano engine (submodule)
├── .vscode/               # VS Code configuration
├── .windsurf/             # Development policies
└── Makefile               # Build configuration
```

## Asset Attribution

This project uses assets from **Penusbmic's "Stranded Starter Pack"** available on itch.io:
- **Creator**: Penusbmic
- **Asset Pack**: Stranded - Top Down Sci-fi Starter Pack
- **Style**: Isometric perspective sprites for top-down games
- **Theme**: Sci-fi with animated characters, environments, and UI elements
- **URL**: https://penusbmic.itch.io/stranded-starter-pack

The asset pack includes animated enemies, hero sprites with multiple color variations, companion sprites, gun sprites, tilesets for various environments, and original music files.

## Technical Details

### Engine
- **Butano**: Modern C++ engine for GBA development
- **Target Platform**: Game Boy Advance (ARM7TDMI processor)
- **Graphics**: 240x160 resolution, 32,768 colors, sprite-based rendering
- **Audio**: 6-channel audio with support for samples and tracker modules

### Performance
- **Frame Rate**: 60 FPS target
- **Memory Management**: Careful sprite and audio memory allocation
- **Collision Detection**: Optimized hitbox-based collision system
- **State Management**: Efficient enemy AI state machines

## License

This project is a homebrew game for educational and entertainment purposes. Please respect the original asset creator's rights and terms of use for the Stranded Starter Pack assets.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly on GBA emulator
5. Submit a pull request

## Links

- **Asset Pack**: [Stranded Starter Pack on itch.io](https://penusbmic.itch.io/stranded-starter-pack)
- **Butano Engine**: [GitHub Repository](https://github.com/GValiente/butano)
- **Web Emulator**: [GBA Web Emulator](https://gba.nicholas-vancise.dev/)
