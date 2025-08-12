# Top-Down Adventure Camera System

This document describes the camera system implemented in the Stranded game, optimized for top-down adventure gameplay with deadzone and lookahead features.

## Overview

The camera system provides smooth, responsive camera movement that enhances gameplay by showing the player more of what's coming while maintaining stable visibility. It's designed specifically for top-down adventure games where movement is equally important in all directions.

## Key Features

### Deadzone System
- **Horizontal deadzone**: 20 pixels (half-width)
- **Vertical deadzone**: 20 pixels (half-height)
- Equal treatment for both axes (unlike side-scrollers)
- Camera only moves when player exits the deadzone
- Prevents jittery micro-adjustments

### Velocity-Based Lookahead
- Camera looks ahead based on player's current velocity
- **Equal lookahead**: 32 pixels in all directions
- No bias towards horizontal movement (appropriate for top-down games)
- Smoothly scales with player speed (0-100% based on velocity)

### Smooth Interpolation
- **Horizontal smoothing**: 10% per frame
- **Vertical smoothing**: 10% per frame (equal to horizontal)
- Provides natural camera movement without lag
- Balanced responsiveness for all directions

### Special Camera Events
- Instant camera snaps for distances > 100 pixels
- Fast camera movement (20% lerp per frame) for special events
- Useful for teleportation, level transitions, etc.
- Auto-disables when camera reaches target

### Level Boundary Clamping
- Camera is constrained to level bounds
- Prevents showing empty space outside the designed map
- Uses GBA screen dimensions (240x160) for viewport calculations

## API Reference

### Camera Update
The camera system is updated automatically each frame via `World::_update_camera(dt)`.

### Special Events
```cpp
// Trigger special camera movement (e.g., for trampolines)
world.trigger_special_camera_event(target_position);

// Disable special camera movement
world.disable_special_camera_event();
```

### Constants (fe_constants.h)
- `CAMERA_DEADZONE_X`: Horizontal deadzone half-width (20)
- `CAMERA_DEADZONE_Y`: Vertical deadzone half-height (20)
- `CAMERA_LOOKAHEAD_BASE`: Base lookahead distance (32)
- `CAMERA_MAX_SPEED`: Max player speed for lookahead calc (0.2)
- `CAMERA_VERTICAL_OFFSET`: Vertical bias (0 - no bias for top-down)
- `CAMERA_SMOOTH_FACTOR_X`: Horizontal smoothing (0.10)
- `CAMERA_SMOOTH_FACTOR_Y`: Vertical smoothing (0.10)
- `CAMERA_SPECIAL_LERP`: Special event lerp factor (0.20)
- `CAMERA_INSTANT_SNAP_THRESHOLD`: Distance for instant snaps (100)

## Tuning Guide

### Making Camera More/Less Responsive
- Increase `CAMERA_SMOOTH_FACTOR_X/Y` for faster camera movement
- Decrease for slower, more cinematic movement

### Adjusting Lookahead
- Increase `CAMERA_LOOKAHEAD_BASE` to see further ahead
- Adjust `CAMERA_MAX_SPEED` to match actual player movement speed

### Deadzone Tuning
- Larger deadzone = less camera movement, more stable
- Smaller deadzone = more responsive, potentially jittery
- Balanced square deadzone for equal treatment of all directions

### Player Positioning
- No vertical bias - player stays centered for top-down gameplay
- Equal lookahead in all directions for balanced visibility

## Implementation Details

The camera system follows this algorithm each frame:

1. **Calculate lookahead** based on player velocity and direction (equal for all directions)
2. **Test deadzone** boundaries separately for X and Y axes
3. **Apply smooth interpolation** toward desired camera position
4. **Handle special events** with fast movement or instant snaps
5. **Clamp to level bounds** to prevent showing empty space

This provides smooth camera movement that shows the player more of what's coming in their direction of travel while maintaining centered positioning appropriate for top-down adventure games.