# World Selection Menu

## Overview
The game now includes a world selection menu system that allows players to choose between multiple worlds and retain their progress when switching between them.

## How to Use

### Starting the Game
- The game now starts with a **World Selection Menu** instead of going directly into the world
- Use **UP/DOWN** arrow keys to navigate between available worlds
- Press **A** to enter the selected world
- Press **B** to exit to the default world (Main World)

### Available Worlds
1. **Main World** - Original game world with Merchant NPC and 3 Spearguard enemies
2. **Forest Area** - Features Golem NPC and 2 Spearguard enemies (easier difficulty)
3. **Desert Zone** - Features Tortoise NPC and 4 Spearguard enemies (harder difficulty)
4. **Ocean Side** - Features Penguin NPC and 2 stronger Spearguard enemies

### In-World Controls
- **START** - Opens the world selection menu (allows switching to other worlds)
- **START + SELECT** - Toggle debug hitbox visualization (original functionality)
- All other controls remain the same

### State Persistence
- When you switch worlds, your current position and health are automatically saved
- Returning to a previously visited world will restore your last position and health
- Each world maintains its own state independently
- Enemy states and NPC interactions are reset when re-entering a world

## Technical Implementation

### New Files Added
- `include/fe_scene_menu.h` - Menu scene header
- `src/fe_scene_menu.cpp` - Menu scene implementation
- `include/fe_world_state.h` - World state management header  
- `src/fe_world_state.cpp` - World state management implementation

### Modified Files
- `include/fe_scene.h` - Added MENU scene type
- `include/fe_scene_world.h` - Enhanced World class for multi-world support
- `src/fe_scene_world.cpp` - Added world-specific content and state management
- `src/main.cpp` - Updated main loop to handle menu and world scenes

### Key Features
- **WorldStateManager**: Singleton class that handles saving/loading player state per world
- **World-Specific Content**: Each world has different NPCs, enemy configurations, and spawn points
- **Seamless Transitions**: State is automatically saved when leaving worlds and restored when returning
- **Extensible Design**: Easy to add new worlds by extending the world configuration system