# Stranded GBA Game - Development Rules & Build Guide

## 🎮 Project Overview
**Stranded** is a top-down sci-fi game for the Game Boy Advance (GBA) built using the Butano engine and DevkitPro toolchain.

## 🔧 Build Environment Setup

### Required Tools
- **DevkitPro/devkitARM** - GBA development toolchain
- **Butano Library** - 2D game engine for GBA (included as git submodule)
- **Docker** - For consistent build environment

### Docker Build Workflow

#### Method 1: Official DevkitPro Docker (Recommended)
```bash
# Pull the official DevkitPro ARM toolchain image
docker pull devkitpro/devkitarm

# Build the project to generate ROM
docker run --rm -v $(pwd):/src devkitpro/devkitarm make -C /src -j8
```

#### Method 2: Custom mGBA Docker (Testing)
```bash
# For testing with mGBA emulator
docker pull banhcanh/docker-mgba
docker run --rm -v $(pwd):/workspace banhcanh/docker-mgba
# Load stranded.gba in web mGBA interface
```

### Local Build Setup (Alternative)
If building locally without Docker:

1. **Install DevkitPro**:
   ```bash
   # Download and install DevkitPro pacman
   wget https://github.com/devkitPro/pacman/releases/latest/download/devkitpro-pacman.amd64.deb
   sudo dpkg -i devkitpro-pacman.amd64.deb
   
   # Install devkitARM
   sudo dkp-pacman -S gba-dev
   ```

2. **Set Environment Variables**:
   ```bash
   export DEVKITPRO=/opt/devkitpro
   export DEVKITARM=$DEVKITPRO/devkitARM
   export PATH="$DEVKITARM/bin:$PATH"
   ```

3. **Initialize Submodules**:
   ```bash
   git submodule update --init --recursive
   ```

4. **Build**:
   ```bash
   make -j8
   ```

## 🏗️ Build Process Details

### Expected Build Output
- `stranded.gba` - Main ROM file for emulator/flash cart
- `stranded.elf` - Debug symbols for GDB debugging
- Build artifacts in `build/` directory

### Common Build Issues & Solutions

#### 1. Missing Math Functions
**Error**: `undefined reference to bn::degrees_atan2`
**Solution**: Add `#include "bn_math.h"` to affected files
```cpp
#include "bn_math.h"  // Add this include
```

#### 2. Butano API Changes
**Error**: `no matching function for call to bn::regular_bg_ptr::create`
**Solution**: Update API calls to include position parameters
```cpp
// Old (broken)
bn::regular_bg_ptr bg = bn::regular_bg_ptr::create(bg_map_ptr);

// New (working)
bn::regular_bg_ptr bg = bn::regular_bg_ptr::create(0, 0, bg_map_ptr);
```

#### 3. C++ Version Compatibility
**Issue**: Butano may require specific C++ version
**Solution**: Use C++20 compatible Butano version (avoid C++23 commits)

### Butano Library Management
- **Location**: `butano/` subdirectory (git submodule)
- **Version**: Use commits before `2d01c209` to avoid C++23 issues
- **Update**: `git submodule update --remote butano`

## 🎯 Game Architecture

### Key Components
- **Player**: `src/fe_player.cpp` - Player character with dynamic z-order
- **NPCs**: `src/fe_npc.cpp` - Non-player characters with position-based z-order
- **World**: `src/fe_scene_world.cpp` - Main game scene and logic
- **Collision**: `src/fe_collision.cpp` - Collision detection system

### Z-Order System (Depth Sorting)
Critical for proper 2D layering:

#### Player Z-Order
```cpp
// Player z-order based on facing direction
if (facing_up) {
    player.set_z_order(-10);  // Player in front
    gun.set_z_order(-5);      // Gun behind
} else {
    gun.set_z_order(-10);     // Gun in front  
    player.set_z_order(-5);   // Player behind
}
```

#### NPC Z-Order
```cpp
// NPCs use position-based z-order for depth sorting
void NPC::update_z_order_by_position() {
    int calculated_z_order = static_cast<int>(_pos.y() / 8) - 1;
    _sprite.value().set_z_order(calculated_z_order);
}
```

**Z-Order Ranges**:
- Player: -10 to -5
- NPCs: Position-based (e.g., merchant at Y=-50 gets z-order=-7)
- Background: Higher positive values

## 🐛 Common Development Issues

### 1. NPC Z-Order Problems
**Symptom**: NPCs don't sort properly with player
**Cause**: Fixed z-order values instead of position-based
**Solution**: Implement `update_z_order_by_position()` and call in constructor + update loop

### 2. Build Failures
**Symptom**: Compilation errors with missing functions
**Cause**: Missing includes or API changes
**Solution**: Add required includes and update API calls

### 3. Sprite Animation Issues
**Symptom**: Sprites not animating
**Cause**: Animation actions not updated in game loop
**Solution**: Call `action.update()` in sprite update methods

## 📁 Project Structure
```
stranded/
├── src/                    # Source code
│   ├── fe_player.cpp      # Player character
│   ├── fe_npc.cpp         # NPCs (including merchant)
│   ├── fe_scene_world.cpp # Main game scene
│   └── ...
├── include/               # Header files
├── graphics/              # Sprite and background assets
├── audio/                 # Sound and music files
├── butano/               # Butano engine (submodule)
├── tools/                # Development tools (mGBA, etc.)
├── Makefile              # Build configuration
└── rules.md              # This file
```

## 🧪 Testing Workflow

### 1. Build ROM
```bash
docker run --rm -v $(pwd):/src devkitpro/devkitarm make -C /src -j8
```

### 2. Test in Emulator
```bash
# Using mGBA Docker
docker run --rm -v $(pwd):/workspace banhcanh/docker-mgba
# Load stranded.gba in web interface

# Or use local mGBA
./tools/mGBA.exe stranded.gba
```

### 3. Verify Functionality
- Player movement and animation
- NPC interactions (especially merchant)
- Z-order depth sorting
- Collision detection
- Audio playback

## 📝 Development Best Practices

### Code Style
- Use `fe::` namespace for game code
- Include guards in all headers
- Proper memory management (RAII)
- Clear variable naming

### Git Workflow
- Use feature branches: `codegen/feature-description`
- Commit build fixes separately from feature changes
- Update submodules when needed
- Test builds before pushing

### Performance Considerations
- GBA has limited resources (32KB RAM, 16MHz CPU)
- Optimize sprite usage and animations
- Minimize dynamic allocations
- Use fixed-point math (`bn::fixed`)

## 🔍 Debugging Tips

### Build Debugging
- Check DevkitPro environment variables
- Verify Butano submodule is initialized
- Look for missing includes in error messages
- Update API calls for Butano version compatibility

### Runtime Debugging
- Use `BN_LOG()` for debug output
- Test in mGBA with debug features
- Monitor sprite limits and memory usage
- Verify z-order calculations with logging

## 📚 Resources
- [Butano Documentation](https://gvaliente.github.io/butano/)
- [DevkitPro Documentation](https://devkitpro.org/wiki/Getting_Started)
- [GBA Development Resources](https://www.coranac.com/tonc/text/toc.htm)
- [mGBA Emulator](https://mgba.io/)

---

**Last Updated**: 2025-07-14  
**Build Status**: ✅ Working with DevkitPro Docker  
**Key Fix**: NPC z-order depth sorting implemented

