# Door Peek Realism — Replace Fake Floor Peeks With Actual Doors

## Status: COMPLETED

## TL;DR
> **Summary**: Replaced the 6 fake door-peek floor quads (colored patches meant to simulate adjacent room floors) with actual vertical door panel models — proper rectangular doors that fill the doorway openings.
> **Deliverables**: New shared `door_panel` model in header, simplified room_viewer logic, successful build
> **Effort**: Quick
> **Files Changed**: `include/models/str_model_3d_items_room.h`, `src/room_viewer.cpp`

## Context

### Original Request
User reported the colored floor quads placed in doorway openings to simulate seeing the adjacent room looked fake. Requested removing them all and replacing with actual door models.

### What Was Done

1. **`include/models/str_model_3d_items_room.h`**:
   - Removed all 6 individual `door_peek_N` flat horizontal floor quads (each had different color index 0-5 matching room floor colors)
   - Created a single shared `door_panel` model: vertical quad (20 wide × 35 tall), color index 8 (door_frame dark brown), normal `(0, -1, 0)` facing into the room
   - Added `constexpr` reference aliases (`door_peek_0` through `door_peek_5` → `door_panel`) for backward compatibility

2. **`src/room_viewer.cpp`**:
   - `DOOR_PEEK_HALF_DEPTH`: 7.5 → 0.5 (door sits nearly flush with wall edge)
   - `get_door_peek_model()`: Simplified to always return `door_panel` (no per-room color switching)
   - Layering mode: `room_floor_only` → `none` (door is a solid 3D object, not a floor layer)

### Verification
- Build passes cleanly with zero errors and zero warnings
