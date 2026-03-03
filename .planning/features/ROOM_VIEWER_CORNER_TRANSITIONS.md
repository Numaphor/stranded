# Room Viewer Camera and Transitions

Last updated: 2026-03-03

## Scope

This document describes how the camera and door transitions work in `src/room_viewer.cpp`. It covers the continuous heading-based camera follow system, corner index derivation, and door transition mechanics.

## Camera Follow System (Current)

The room viewer uses a **continuous heading-based camera follow** system. There is no discrete START-triggered corner transition. Instead:

- The camera view angle tracks the player's committed movement heading plus a behind-offset angle.
- `_corner_index` is derived from the quantized view angle (divided into quarter turns).
- The view angle updates smoothly with easing gains and max step clamps.

### Key Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `QUARTER_TURN_ANGLE` | `16384` | 90 degrees in 16-bit angle domain |
| `CAMERA_BEHIND_OFFSET_ANGLE` | `24576` | Offset to position camera behind player |
| `CAMERA_RENDER_UPDATE_ANGLE_STEP` | `64` | Threshold for refreshing orientations and paintings |
| `CAMERA_START_BOOST_FRAMES` | `10` | Short recenter boost when START is pressed |
| `iso_phi` | `6400` | Base isometric phi angle |
| `iso_theta` | `59904` | Base isometric theta angle |
| `iso_psi` | `6400` | Base isometric psi angle |

### Behavior

- Movement **continues** while the camera turns corners; input is not blocked for camera rotation.
- `START` (without `SELECT`) recenters the camera toward the committed heading with a short boost.
- `L/R` adjusts camera distance (zoom), clamped 100-500.
- `SELECT` toggles debug mode.
- Orientations and paintings refresh when the view angle moves by `CAMERA_RENDER_UPDATE_ANGLE_STEP` (64).

### Implementation

Core logic lives in `src/room_viewer.cpp`:

- `compute_corner_matrices(...)` creates the base isometric orientation set from phi/theta/psi angles.
- `rotate_corner_matrix(...)` rotates a base corner matrix by a given angle.
- `update_all_orientations()` applies the active matrix to room, table, chair, and painting models.
- `_corner_index` derives from quantized view angle.

## Door Transitions

When the player steps onto a door tile, a room-to-room transition triggers:

### Key Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `DOOR_TRANSITION_DURATION_FRAMES` | `16` | Transition duration in frames |

### Behavior

1. Transition interpolates player position and global position/anchor over 16 frames with smoothstep easing.
2. Decor and models for the target room are preloaded during the transition.
3. Movement input is blocked during the transition and restored on completion.
4. Depth bias is applied to transition decor to prevent Z-fighting.
5. Furniture is reloaded after the room swap completes.

### Room Layout

6 rooms in a 2x3 grid with 7 door pairs connecting adjacent rooms. Rooms are indexed 0-5.

## Player Representation

The room viewer manages a 3D player representation:

- Player direction: 5 states (`_player_dir` 0-4: down, down_side, side, up_side, up).
- Player facing: `_player_facing_left` boolean.
- Player position: `_player_fx`, `_player_fy`, `_player_fz` in fixed-point.
- Animation: `_anim_frame_counter` with tile updates via `_update_player_anim_tiles()`.

## Dialog in Room Viewer

The room viewer uses `RoomDialog` (sprite-based dialog) for NPC interaction. During dialog:

- HUD sprites are freed for VRAM.
- Dialog state machine: IDLE -> GREETING -> SHOWING_OPTIONS -> SHOWING_RESPONSE.
- Typewriter effect at half-speed tick.
- Scrollable options (max 8 options, 3 visible).

## Historical Note

Earlier versions of the room viewer used a discrete corner transition system triggered by START button press, with `CORNER_TURN_DURATION_FRAMES` controlling the transition duration. This was replaced with the continuous heading-based camera follow system described above. The old transition state variables (`corner_transition_active`, `corner_transition_elapsed`, `corner_transition_start_angle`, `corner_transition_target_angle`) no longer exist in the codebase.

## Manual Verification Checklist

1. Camera follows committed heading smoothly with no snap.
2. Press `START` to recenter camera toward heading -- confirm smooth boost.
3. Hold movement input during camera rotation and confirm movement is NOT blocked.
4. Change zoom with `L`/`R` and confirm camera distance adjusts within 100-500.
5. Walk through a door and confirm smooth 16-frame transition with no snap.
6. Confirm movement is blocked during door transition and restored after.
7. Confirm player facing and direction update correctly per heading.
8. Confirm paintings/orientations refresh when view angle changes by step threshold.
