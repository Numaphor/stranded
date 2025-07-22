# Stranded

A Game Boy Advance adventure game built with the [Butano](https://github.com/GValiente/butano) engine.

## About

Stranded is a top-down action-adventure game where you play as a hero exploring mysterious lands, battling enemies, and uncovering ancient secrets.

## Features

### Core Gameplay
- **Top-down exploration** - Navigate through diverse environments including dungeons, houses, cliffs, and plains
- **Combat system** - Engage enemies with sword-based combat mechanics
- **Health system** - Manage your health as you face various threats
- **Story discovery** - Find and translate ancient stone tablets to uncover the game's lore

### Technical Features
- **Advanced Enemy AI** - Sophisticated state machine with multiple behaviors:
  - Idle, Patrol, Chase, Attack, Return to Post, and Stunned states
  - Dynamic behavior based on player proximity and actions
- **Minimap system** - Keep track of your location and nearby enemies
- **Multiple enemy types** - Face various creatures including:
  - Spear Guards
  - Bats, Cats, Slimes
  - Golems, Penguins, Tortoises
  - And many more!
- **Rich environments** - Explore detailed pixel art locations
- **Sound system** - Immersive audio with sound effects and music

### Story Elements
- **Ancient tablets** - Discover stone plaques with mysterious text
- **Translation system** - Hold 'A' to reveal English translations of ancient languages
- **Multiple story fragments** - Piece together the narrative through exploration
- **Environmental storytelling** - Learn about the world through visual design

## Controls

- **Arrow Keys / D-Pad** - Move character
- **A Button** - Interact with objects, take out gun
- **B Button** - Shoot
- **START Button** - Cancel/Exit interactions
- **Up Arrow** - Interact with nearby objects when prompted

## Build Requirements

- **Butano Engine** - Game framework for GBA development
- **devkitARM** - ARM cross-compiler toolchain
- **Python** - For asset processing
- **Make** - Build system

## Building the Game

1. Ensure you have devkitARM and the required toolchain installed
2. Initialize the Butano submodule:
   ```bash
   git submodule update --init --recursive
   ```
3. Build the game:
   ```bash
   make
   ```
4. The compiled ROM will be generated as `stranded.gba`

## Development

### Project Structure
- `src/` - C++ source code
- `include/` - Header files
- `graphics/` - Sprite and background assets
- `audio/` - Sound effects and music
- `butano/` - Butano engine submodule

### Key Components
- **Player System** (`fe_player.*`) - Character movement and controls
- **Enemy AI** (`fe_enemy.*`, `fe_enemy_state_machine.*`) - Advanced enemy behavior
- **Level System** (`fe_level.*`) - World management and collision
- **Story System** (`fe_story_save.*`) - Narrative elements and tablet interactions
- **Combat System** - Weapon handling and health management

### Architecture Highlights
The enemy AI system has been completely refactored to follow SOLID principles:
- **Single Responsibility** - Each state handles one specific behavior
- **Open/Closed** - Easy to add new enemy behaviors without modifying existing code
- **State Machine Pattern** - Clean separation of enemy behaviors into distinct states
