# Debug Mode Implementation

## Overview

Debug mode has been reimplemented to provide visual feedback for hitboxes and collision zones during gameplay. This feature helps developers understand collision detection, interaction zones, and spatial relationships between game entities.

## How to Use

### Toggle Debug Mode
- Press **SELECT + START** to toggle debug visualization on/off
- Debug mode persists until toggled off or the game exits

### What's Visualized

When debug mode is active, colored tile borders are drawn to show:

1. **Player Hitbox** (Tile 5 - White/Bright)
   - Shows the player's collision boundary
   - Updates every frame as the player moves

2. **Enemy Hitboxes** (Tile 6 - Red/Danger)
   - Shows collision boundaries for all active enemies
   - Helps identify enemy attack ranges and collision zones

3. **Merchant Zones**
   - **Interaction Zone** (Tile 4 - Interaction color)
     - Large 100x100 pixel area for triggering conversations
     - Player can press A within this zone to talk to merchant
   - **Collision Zone** (Tile 3 - Collision color)
     - Smaller 24x24 pixel solid collision boundary
     - Prevents player from walking through merchant

4. **Sword Zone** (Tile 7 - Special zone)
   - Shows the sword's interaction boundary in the world
   - Marks the special sword area at world origin (0, 0)

## Technical Details

### Architecture

The debug system is implemented through the `HitboxDebug` class:

- **Header**: `include/fe_hitbox_debug.h`
- **Implementation**: `src/fe_hitbox_debug.cpp`

### Integration

Debug mode is integrated into the `World` class:
1. Initialized when world starts (`execute()` method)
2. Toggle handled in main game loop with SELECT+START input
3. Updated every frame when active
4. Cleaned up in destructor

### Performance

- Debug visualization only updates when active
- Uses background tile manipulation (no sprite overhead)
- **Optimized clearing**: Only modified tiles are tracked and cleared (up to 512 cells per frame)
- Typically clears <100 tiles per frame vs. 102,400 tiles in naive implementation
- Background map is reloaded after updates for immediate visual feedback
- Minimal CPU overhead even with many entities visible

### Tile Coordinate System

- World coordinates are converted to tile coordinates
- Each tile is 8x8 pixels
- Map offset is applied to convert from centered world coordinates to tile grid
- Only border tiles are drawn (not filled) to minimize visual noise

## Development Notes

### Adding New Debug Visualizations

To add new hitbox visualizations:

1. Add a call to `_draw_hitbox()` or `_draw_point()` in `HitboxDebug::update()`
2. Choose an appropriate tile ID (3-7 are currently used)
3. Pass the hitbox/point and tile ID

Example:
```cpp
Hitbox new_zone = create_some_hitbox();
_draw_hitbox(new_zone, 8);  // Use tile 8 for new visualization
```

### Customizing Colors

Debug tile colors are determined by the tile graphics in `graphics/bg/tiles.bmp`:
- Tile 3: Collision zones
- Tile 4: Interaction zones
- Tile 5: Player hitbox
- Tile 6: Enemy hitboxes
- Tile 7: Special zones

Modify the tile graphics to change colors.

## Known Limitations

1. Debug visualization is 2D only (uses background tiles)
2. Overlapping zones may make individual boundaries hard to distinguish
3. Only rectangular hitboxes are supported (no circular or complex shapes)
4. Performance impact is minimal but measurable when many entities are visible

## Future Enhancements

Potential improvements:
- Add text labels showing entity types or health
- Implement collision point markers (where collisions occur)
- Add velocity/direction indicators
- Toggle individual visualization layers independently
- Performance profiling overlay
