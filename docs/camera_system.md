# Yoshi's Island-Style Camera System

This document describes the new camera system implemented in the Stranded game, based on the camera behavior from Yoshi's Island and Super Mario Advance 3.

## Overview

The camera system provides smooth, responsive camera movement that enhances gameplay by showing the player more of what's coming while maintaining stable visibility. It uses two main concepts:

1. **Deadzone (Follow Window)**: A rectangle around the current camera position where the player can move without triggering camera movement
2. **Look-ahead**: When the player moves, the camera shifts to show more space in the direction of movement

## Key Features

### Deadzone System
- **Horizontal deadzone**: 24 pixels (half-width)
- **Vertical deadzone**: 12 pixels (half-height)
- Camera only moves when player exits the deadzone
- Prevents jittery micro-adjustments

### Velocity-Based Lookahead
- Camera looks ahead based on player's current velocity
- Horizontal lookahead up to 48 pixels
- Vertical lookahead up to 24 pixels (reduced from horizontal)
- Smoothly scales with player speed (0-100% based on velocity)

### Smooth Interpolation
- **Horizontal smoothing**: 12% per frame (~0.08s to reach 90%)
- **Vertical smoothing**: 10% per frame (~0.10s to reach 90%)
- Provides natural camera movement without lag

### Special Camera Events
- Instant camera snaps for distances > 100 pixels
- Fast camera movement (20% lerp per frame) for special events
- Useful for trampolines, platforms, boss intros, etc.
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
- `CAMERA_DEADZONE_X`: Horizontal deadzone half-width (24)
- `CAMERA_DEADZONE_Y`: Vertical deadzone half-height (12)
- `CAMERA_LOOKAHEAD_BASE`: Base lookahead distance (48)
- `CAMERA_MAX_SPEED`: Max player speed for lookahead calc (2.0)
- `CAMERA_VERTICAL_OFFSET`: Vertical bias (4)
- `CAMERA_SMOOTH_FACTOR_X`: Horizontal smoothing (0.12)
- `CAMERA_SMOOTH_FACTOR_Y`: Vertical smoothing (0.10)
- `CAMERA_SPECIAL_LERP`: Special event lerp factor (0.20)
- `CAMERA_INSTANT_SNAP_THRESHOLD`: Distance for instant snaps (100)

## Tuning Guide

### Making Camera More/Less Responsive
- Increase `CAMERA_SMOOTH_FACTOR_X/Y` for faster camera movement
- Decrease for slower, more cinematic movement

### Adjusting Lookahead
- Increase `CAMERA_LOOKAHEAD_BASE` to see further ahead
- Adjust `CAMERA_MAX_SPEED` to change when full lookahead activates

### Deadzone Tuning
- Larger deadzone = less camera movement, more stable
- Smaller deadzone = more responsive, potentially jittery

### Vertical Bias
- Positive `CAMERA_VERTICAL_OFFSET` = player appears lower in frame
- Negative = player appears higher in frame

## Implementation Details

The camera system follows this algorithm each frame:

1. **Calculate lookahead** based on player velocity and direction
2. **Test deadzone** boundaries separately for X and Y axes
3. **Apply smooth interpolation** toward desired camera position
4. **Handle special events** with fast movement or instant snaps
5. **Clamp to level bounds** to prevent showing empty space

This provides the characteristic "Yoshi is left-of-center when running right" feeling that allows players to see incoming platforms and enemies while maintaining smooth, natural camera movement.