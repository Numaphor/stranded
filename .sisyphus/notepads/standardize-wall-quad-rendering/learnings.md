# Learnings — standardize-wall-quad-rendering

## 2026-03-04 Session ses_345489c1dffekoyRyS5q9JJyZx

### Architecture
- `textured_quad::set_points(p0,p1,p2,p3, min_affine_divisor=32, allow_winding_swap=true)` returns bool
- Two triangles: _top=set_points(p2,p3,p0), _bottom=set_points(p0,p1,p2)
- All rendering happens inside the `execute()` method of RoomViewer, via lambdas defined within it

### Key Reference Lines (room_viewer.cpp)
- Working painting render_wall_quad: 1528-1556 (THE TEMPLATE)
- Broken door render_door_quad: 1628-1712 (DELETE)
- Duplicate lambdas paintings: 1505-1526; doors: 1605-1626
- Shared utilities (DO NOT MODIFY): 1377-1473
- Painting constants: lines 18-31
- Door constants: lines 33-39 (DOOR_MIN_AFFINE_DIVISOR=64 → REMOVE)
- Door positioning: 1714-1814 (PRESERVE exactly)
- Debug overlay text: 1932-1934; refresh: 2584-2586
- Quad creation: 1348-1373

### Root Cause
Door fallback cascade (5 tries with alternate diagonal splits + alternate wall orders) + min_affine_divisor=64 causes visual glitches. Painting's simple single-pass approach with min_affine_divisor=32 works perfectly.

### Conventions
- 4-space indentation
- Local lambdas inside execute() scope
- bn::fixed for all math (no float/double)
- Braces on next line

## [2026-03-04] Task 1: Completed - unified wall-quad rendering path
- Hoisted `local_point_to_view`, `local_vector_to_view`, and `is_front_facing` to single shared definitions and parameterized `is_front_facing` with a visibility threshold.
- Extracted a shared `render_wall_quad` used by both painting and door paths; removed door fallback cascade (`render_door_quad`, alternate diagonal/order retries) and the stricter `DOOR_MIN_AFFINE_DIVISOR` constant.
- Simplified door debug counters to `door_rendered` and `door_hidden`, removed fallback counter plumbing in overlay and refresh checks.
- Build verification succeeded (`make clean && make -j$(nproc)`), and `stranded.gba` is produced.
