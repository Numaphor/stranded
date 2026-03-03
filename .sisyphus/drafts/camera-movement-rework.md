# Draft: Camera-Movement Rework

## Requirements (confirmed)
- **Problem**: When moving, the whole room moves (camera rotates), which makes the player miss doors
- **Root cause**: Camera rotation changes the 3D projection; even though movement is camera-relative via `screen_to_room_delta()`, the visual world shifting during movement makes precise door targeting feel unreliable
- **Desired feel**: Smooth follow with deadzone — player moves freely in a zone, camera only adjusts when needed

## Technical Findings

### Architecture
This is a **3D isometric room viewer** — NOT a standard 2D top-down camera:
- Camera sits at fixed position (0, 274, 0), ROTATES around phi angle
- The view angle rotation changes the entire 3D projection of the room
- Movement IS camera-relative (`screen_to_room_delta(screen_dx, screen_dy, current_view_angle)`)
- But the camera rotation still shifts the visual appearance of the entire room during movement

### Current Camera Follow System (`room_viewer.cpp`)
1. **Deadzone accumulator**: Movement must accumulate 8+ distance units before camera responds
2. **Commit system**: New heading must persist for 18 frames before camera commits to it
3. **Retarget cooldown**: 36 frames between direction changes
4. **Lerp**: gain=0.05, max_step=256 per frame, snap when within epsilon(32)
5. **Initial lock**: 120 frames on room entry (camera stays still for 2 seconds)
6. **Door approach lock**: `near_door_approach()` disables camera steering when near a door

### Key Function: `screen_to_room_delta` (line 108)
```
base_dx = screen_dx + screen_dy
base_dy = screen_dy - screen_dx
Then rotate by view_angle: (base_dx * c - base_dy * s, base_dx * s + base_dy * c)
```
This maps screen-space d-pad to room-local coords accounting for camera rotation.

### Door System
- Room bounds: [-55, 55] per axis
- Door trigger: position crosses FLOOR_MAX/MIN within DOOR_HALF_WIDTH(10) of door center
- Door approach detection: EDGE_MARGIN=18, LANE_MARGIN=12 — when detected, locks camera

### The Real Problem
Even though:
- Movement is already camera-relative (d-pad "right" = screen right regardless of view angle)
- There's already a deadzone and commit system
- Door approach already locks camera

The issue is that **camera rotation during active movement shifts the entire visual scene**. As the player walks toward a door, the camera slowly rotates to follow, which:
1. Shifts where the door APPEARS to be on screen
2. Makes the player feel like they're "drifting" even though they're walking straight in room coords
3. The apparent shift of the door position makes the player adjust their input, overcorrecting

## Research: What Similar Games Do

### Best Patterns for This Problem
1. **Zelda-like deadzone**: Camera box where player moves freely. Camera only scrolls when player exits box. But this is for 2D panning, not 3D rotation.
2. **Golden Sun approach**: Trigger magnetism — near a door, game pulls player toward door center
3. **Fixed-view-angle approach**: Don't rotate camera at all. Room is always viewed from the same angle. Camera rotation is optional (L/R buttons or Start to recenter).

### Recommendations (ranked)
1. **Reduce/remove auto-rotation**: Make camera rotation a MANUAL action (L/R shoulder buttons) rather than auto-following movement. This eliminates the visual shifting entirely.
2. **Stronger door magnetism**: When near a door, add a pull force that guides the player to door center
3. **Larger deadzone + higher commit threshold**: Make camera much more reluctant to rotate
4. **Instant snap instead of lerp**: When camera does rotate, snap quickly instead of slow lerp (reduces prolonged visual shifting)

## Decisions Made

### Camera Rotation Behavior
- **Decision**: Camera auto-follow only triggers AFTER player stops moving for ~1 second
- **Rationale**: User wants to eliminate the 'room shifting while moving' feel entirely
- **During movement**: Camera stays at current angle — no rotation at all
- **After stopping ~1 second**: Camera smoothly rotates to follow player's new position/heading
- **Manual override**: L/R buttons and Start recenter still work anytime
- **Initial lock (2 sec)**: Keep as-is on room entry

### Door System
- **Decision**: No door magnetism — keep doors as they currently work
- Player just needs the visual stability from camera fix to reach doors reliably

### Test Strategy
- **No automated tests** — project has no test infrastructure
- **Verification**: Build ROM, run in emulator, verify:
  1. Camera doesn't rotate during active movement
  2. Camera smoothly rotates ~1 second after player stops
  3. Doors are reachable on first try without fighting the camera
  4. L/R manual rotation still works during movement
  5. Start recenter still works
  6. Room entry 2-second lock still works

## Open Questions
- None — all critical decisions made
## Scope Boundaries
- INCLUDE: Camera follow system rework (delay auto-rotation until player idle for ~1s)
- INCLUDE: Keep existing manual rotation (L/R, Start recenter)
- INCLUDE: Keep existing door approach lock, initial lock
- EXCLUDE: Player movement physics changes
- EXCLUDE: Door magnetism or door system changes
- EXCLUDE: Combat system
- EXCLUDE: Room transition animation changes
