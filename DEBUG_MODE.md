# Debug Mode Implementation

## Overview

This implementation adds a debug visualization system that can be toggled on/off during gameplay to help visualize hitboxes and collision zones.

## Usage

### Activating Debug Mode

Press **SELECT + START** during gameplay to toggle debug mode on/off.

When activated:
- Console will log: "Debug mode: ON"
- Visual markers will appear at the center of all hitboxes
- Markers are rendered with high z-order to appear above game objects

When deactivated:
- Console will log: "Debug mode: OFF"
- All debug markers are cleared from the screen

## What Gets Visualized

When debug mode is active, the following hitboxes and zones are visualized:

1. **Player Hitbox** - Center of the player's collision box
2. **Enemy Hitboxes** - Center of each enemy's collision box
3. **Merchant Interaction Zone** - The area where player can interact with NPCs
4. **Sword Zone** - Static collision zone for the sword in the world

## Implementation Details

### Files Added

- `include/fe_debug.h` - Header file defining the DebugSystem class
- `src/fe_debug.cpp` - Implementation of the debug visualization system

### Files Modified

- `include/fe_scene_world.h` - Added `_debug_system` member to World class
- `src/fe_scene_world.cpp` - Integrated debug toggle and visualization rendering

### Key Features

1. **Toggle Functionality**: SELECT+START input handling added to the main game loop
2. **Visual Markers**: Uses bullet sprites as debug markers (small red dots)
3. **Camera Integration**: All markers properly follow the camera
4. **Performance**: Markers are cleared and regenerated each frame only when debug mode is active
5. **Z-ordering**: Debug markers use z-order of -32000 to appear above most game elements

### Technical Notes

- Markers are implemented using `bn::sprite_ptr` with the bullet sprite item
- Maximum of 32 debug markers can be displayed simultaneously (configurable in the vector size)
- The system automatically manages marker lifecycle through Butano's sprite system
- Debug state persists across frames until toggled again

## Testing

To test the debug mode:

1. Build and run the game: `make -j8`
2. During gameplay, press SELECT+START
3. Verify that red dots appear at hitbox centers
4. Move around and observe markers following entities
5. Press SELECT+START again to disable and verify markers disappear

## Future Enhancements

Possible improvements for the debug system:

- Different colors for different hitbox types
- Hitbox boundary visualization (not just center points)
- Frame rate display
- Memory usage display
- Collision event visualization
- Enemy state machine state display
- Adjustable marker size/style
