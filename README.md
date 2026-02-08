<div align="center">

# 🌟 Stranded

### A Top-Down Sci-Fi Action RPG for Game Boy Advance

*Battle through hostile worlds with your trusty companion, master combat skills, and uncover the mysteries of a dangerous universe.*

[![Platform](https://img.shields.io/badge/Platform-Game%20Boy%20Advance-9bbc0f?style=for-the-badge&logo=nintendo&logoColor=white)](https://en.wikipedia.org/wiki/Game_Boy_Advance)
[![Engine](https://img.shields.io/badge/Engine-Butano-306998?style=for-the-badge)](https://github.com/GValiente/butano)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-Open%20Source-green?style=for-the-badge)](LICENSE)

---

[Features](#-features) • [Getting Started](#-getting-started) • [Controls](#-controls) • [Game Guide](#-game-guide) • [Development](#-development)

</div>

---

## 📖 Overview

**Stranded** is an action-packed adventure game that combines classic top-down RPG mechanics with fast-paced shooter elements. Built entirely for the Game Boy Advance using the modern [Butano](https://github.com/GValiente/butano) C++ engine, it showcases what's possible on 20+ year old hardware.

### Key Highlights

- 🎮 **Hybrid Combat** — Seamlessly switch between sword combat and ranged attacks
- 🐾 **AI Companion** — Fight alongside a loyal companion with revival mechanics  
- 🌍 **Multiple Worlds** — Explore diverse environments with unique challenges
- 💾 **Progress Saving** — World state persistence saves your journey
- 🎨 **Polished Presentation** — Smooth animations, visual effects, and dynamic HUD

---

## ✨ Features

<table>
<tr>
<td width="50%">

### 🎮 Gameplay
- **Dual Combat System** — Sword melee and gun ranged attacks
- **Movement Abilities** — Walking, running, rolling, dodging
- **Companion System** — AI ally with combat support and revival
- **Interactive NPCs** — Merchant for trading and upgrades
- **World State Saving** — Persistent player position and health

</td>
<td width="50%">

### ⚔️ Combat
- **Melee Attacks** — Chopping, slashing sword combos
- **Ranged Combat** — Gun with ammo management
- **Buff System** — Power, Energy, Heal, Defense buffs
- **Enemy AI** — Smart state machine behavior
- **Invulnerability Frames** — Roll through attacks

</td>
</tr>
<tr>
<td>

### 🌍 Exploration
- **Multiple Worlds** — Main World and Forest Area
- **Real-time Minimap** — Track positions at a glance
- **Dynamic Camera** — Smooth tracking with lookahead
- **Zone System** — Interaction and collision areas
- **Zoom Feature** — Tactical overview mode

</td>
<td>

### 🛠️ Technical
- **Entity-Component Architecture** — Clean, modular code
- **State Machine AI** — 6 distinct enemy behaviors
- **Advanced Hitbox System** — Precise collision detection
- **Animation Framework** — Smooth sprite transitions
- **Audio System** — Music tracks and sound effects

</td>
</tr>
</table>

---

## 🚀 Getting Started

### Prerequisites

| Requirement | Description |
|-------------|-------------|
| **devkitARM** | ARM cross-compiler from [devkitPro](https://devkitpro.org/wiki/Getting_Started) |
| **Python 3.x** | For build tools and asset processing |
| **GNU Make** | Build system (included with devkitPro) |
| **Git** | Version control with submodule support |

### Installation

```bash
# Clone with submodules
git clone --recursive https://github.com/Numaphor/stranded.git
cd stranded

# If you forgot --recursive
git submodule update --init --recursive
```

### Building

```bash
# Build the ROM (parallel compilation)
make -j8

# Output: stranded.gba
```

### Running

Use any GBA emulator to play:
- **[mGBA](https://mgba.io/)** — Recommended for accuracy
- **[Visual Boy Advance](https://visualboyadvance.org/)** — Popular alternative
- **Real Hardware** — Flash cart or GBA consolizer

**Windows on ARM64:** mGBA does not provide a native ARM64 Windows build. You can:

1. **Use mGBA x64 (portable)** — Download the [64-bit portable zip](https://mgba.io/downloads.html) (no installer). On Windows 11 ARM, it runs via Prism x64 emulation and usually works fine. Extract it, then point the project at it (see below).
2. **Use VisualBoyAdvance-M (native ARM64)** — Download the [ARM64 nightly build](https://nightly.visualboyadvance-m.org/) (`visualboyadvance-m-Win-ARM64.zip`). Extract and set it as your emulator (see below).

To choose which emulator runs when you use the **Run** task or debug:

- In VS Code: **File → Preferences → Settings**, search for `stranded.emulatorPath`, or edit `.vscode/settings.json` and set:
  - `"stranded.emulatorPath": "C:\\path\\to\\VisualBoyAdvance-m.exe"` (ARM64 native), or
  - `"stranded.emulatorPath": "${workspaceFolder}/tools/mGBA.exe"` (bundled x64 mGBA, runs under emulation on ARM).
- Then run **Terminal → Run Task → run** to build and launch `stranded.gba` in the selected emulator.

---

## 🎮 Controls

### Menu Navigation

| Button | Action |
|--------|--------|
| **D-Pad** | Navigate options |
| **A** | Confirm/Select |
| **B** | Back/Cancel |

### In-Game Controls

| Button | Action |
|--------|--------|
| **D-Pad** | Move character |
| **A** | Interact with NPCs |
| **B** | Attack (Sword/Gun) |
| **L** | Switch weapon |
| **R** | Roll/Dodge |
| **Select** | Toggle zoom view |
| **Select+A** | Open level select |

### Combat Tips

- 🔄 **Roll through enemies** — Invulnerability during roll animation
- ⚔️ **Combo attacks** — Chain sword strikes for maximum damage  
- 🎯 **Ammo management** — Gun reloads when rolling
- 🐾 **Revive companion** — Press A near fallen companion

---

## 📚 Game Guide

### 🦸 The Hero

Your character is a versatile combatant equipped with:

- **Sword** — Close-range melee weapon with multiple attack animations
- **Gun** — Ranged weapon with limited ammo (auto-reloads on roll)
- **Abilities** — Run, roll, chop, slash, and activate power buffs

### 🌍 Worlds

| World | Description |
|-------|-------------|
| **Main World** | Starting area with merchant NPC and balanced encounters |
| **Forest Area** | Dense woodland with unique tileset and enemy placement |

### 👾 Enemies

#### Spearguard
Elite melee combatants with intelligent behavior:
- **Idle** — Stationary watching for threats
- **Patrol** — Wander around designated areas
- **Chase** — Pursue detected players aggressively
- **Attack** — Strike when in range
- **Return** — Return to post when player escapes
- **Stunned** — Brief incapacitation after taking damage

### 🐾 Companion

Your AI companion provides invaluable support:
- Automatically follows your movement
- Engages enemies in combat
- Can be revived when defeated (press A while nearby)
- Independent targeting system

### 💊 Buff System

Activate powerful temporary buffs:
- **Power** — Increased damage output
- **Energy** — Enhanced mobility
- **Heal** — Restore health
- **Defense** — Damage reduction

---

## 🔧 Development

### Project Structure

```
stranded/
├── src/                    # C++ source files (28 modules)
│   ├── main.cpp           # Entry point and scene management
│   ├── fe_player*.cpp     # Player systems (6 files)
│   ├── fe_enemy*.cpp      # Enemy AI and states (4 files)
│   ├── fe_scene*.cpp      # Scene implementations (4 files)
│   └── ...                # Supporting systems
├── include/               # Header files
├── graphics/              # Visual assets
│   ├── bg/               # Background tiles and maps
│   └── sprite/           # Organized sprite assets
│       ├── player/       # Hero sprites
│       ├── enemy/        # Enemy sprites  
│       ├── npc/          # NPC sprites
│       ├── hud/          # UI elements
│       ├── item/         # Item sprites
│       └── vfx/          # Visual effects
├── audio/                 # Sound effects (.wav) and music (.it)
├── butano/               # Butano engine (submodule)
└── tools/                # Build utilities
```

### Architecture Overview

```
┌─────────────────────────────────────────────────────────┐
│                     Scene System                         │
│  ┌─────────┐  ┌──────────┐  ┌──────┐  ┌──────────────┐ │
│  │  Start  │→ │ Controls │→ │ Menu │→ │    World     │ │
│  └─────────┘  └──────────┘  └──────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────┐
│                    Entity Framework                      │
│  ┌────────────────────────────────────────────────────┐ │
│  │                      Entity                         │ │
│  │         ┌──────────┬──────────┬─────────┐         │ │
│  │         │  Player  │  Enemy   │   NPC   │         │ │
│  │         └──────────┴──────────┴─────────┘         │ │
│  └────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────┐
│                   Component Systems                      │
│  ┌─────────┐ ┌──────────┐ ┌───────────┐ ┌───────────┐  │
│  │ Hitbox  │ │ Movement │ │ Animation │ │  Bullets  │  │
│  └─────────┘ └──────────┘ └───────────┘ └───────────┘  │
└─────────────────────────────────────────────────────────┘
```

### Key Classes

| Class | Purpose |
|-------|---------|
| `Player` | Main character with movement, combat, and companion |
| `Enemy` | Hostile entities with state machine AI |
| `EnemyStateMachine` | Manages enemy behavior transitions |
| `HUD` | Health, ammo, weapon, and buff display |
| `Level` | Tile-based collision and zone management |
| `BulletManager` | Projectile pooling and collision |
| `WorldStateManager` | Save/load world progress |

### Enemy State Machine

```
        ┌──────────────┐
        │     IDLE     │ ←───────────┐
        └──────┬───────┘             │
               │ player detected     │ player lost
        ┌──────▼───────┐             │
        │    CHASE     │ ────────────┤
        └──────┬───────┘             │
               │ in range            │
        ┌──────▼───────┐             │
        │    ATTACK    │             │
        └──────┬───────┘             │
               │ hit                 │
        ┌──────▼───────┐      ┌──────┴──────┐
        │   STUNNED    │ ──→  │ RETURN_POST │
        └──────────────┘      └─────────────┘
```

### Build System

The Makefile automatically:
- Compiles C++ sources with devkitARM
- Processes graphics through grit (bitmap → GBA format)
- Converts audio via mmutil
- Links against Butano library
- Generates final `.gba` ROM

---

## 🐛 Troubleshooting

| Issue | Solution |
|-------|----------|
| `DEVKITARM not found` | Install devkitPro and set environment variable |
| Python errors | Ensure Python 3.x is in PATH |
| Missing submodules | Run `git submodule update --init --recursive` |
| Permission errors | Check write access to build directory |

---

## 📜 Credits

### Engine & Tools
- **[Butano](https://github.com/GValiente/butano)** — Modern C++ high-level engine for GBA by [GValiente](https://github.com/GValiente)
- **[devkitPro](https://devkitpro.org/)** — GBA development toolchain

### Development
- **Game Design & Programming** — Numa
- **Built for** — The Game Boy Advance homebrew community

---

## 📄 License

This project is open source. See the [LICENSE](LICENSE) file for details.

---

<div align="center">

**Made with 💜 for the GBA homebrew community**

*Star ⭐ this repository if you find it useful!*

</div>
