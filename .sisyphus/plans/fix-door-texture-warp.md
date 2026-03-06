# Fix Door Wall Texture Vertical Swap on South/North Walls

## TL;DR
> **Summary**: Door wall textures (door_wall_top / door_wall_bottom sprites) render vertically swapped on south and north wall doors due to a vertex z-ordering mismatch with the `back_wall` / `mirrored_back_wall` quad point mapping. East/west doors are unaffected.
> **Deliverables**: Corrected door vertex z-assignments for south and north walls in `src/room_viewer.cpp`
> **Effort**: Quick
> **Parallel**: NO
> **Critical Path**: Task 1 (fix) → Task 2 (build + visual verify)

## Context
### Original Request
User reported "door rendering is broken" with a screenshot showing warped/stretched door textures visible in the room viewer.

### Interview Summary
- **What**: Door wall textures appear stretched/warped — the top half of the door texture renders at the bottom of the door opening and vice versa.
- **Where**: All rooms, all camera angles. Every room has at least one south or north door.
- **Root Cause**: Confirmed via pipeline trace (see below).

### Root Cause Analysis

The room viewer renders door wall textures as `textured_quad` objects, each split into two affine-transformed triangles (`_top` using `door_wall_top` sprite, `_bottom` using `door_wall_bottom` sprite).

`textured_quad::set_points(p0, p1, p2, p3)` (line 539) creates:
- `_top.set_points(p2, p3, p0)` — triangle covering the p2-p3 edge area
- `_bottom.set_points(p0, p1, p2)` — triangle covering the p0-p1 edge area

For the _top sprite to render at the **visual top** of the quad, p2 and p3 must be geometrically higher (more negative z) after `map_wall_quad_points` remapping.

**Working case (east door, `right_wall` order — identity mapping):**
- d0=(wall, low_y, 0=floor), d1=(wall, high_y, 0=floor), d2=(wall, high_y, -35=ceiling), d3=(wall, low_y, -35=ceiling)
- After identity mapping: q0=d0(floor), q1=d1(floor), q2=d2(ceiling), q3=d3(ceiling)
- `_top(q2=ceiling, q3=ceiling, q0=floor)` → covers upper area → door_wall_top at top ✓

**Working reference (Painting A, `back_wall` order):**
- a0=(left, wall, -32.4=higher), a1=(right, wall, -32.4=higher), a2=(right, wall, -15.6=lower), a3=(left, wall, -15.6=lower)
- Vertices 0,1 are at **geometrically higher** z (more negative)
- After `back_wall` rotation (q0=p2, q1=p3, q2=p0, q3=p1): q2=a0(higher), q3=a1(higher)
- `_top(q2=higher, q3=higher, q0=lower)` → covers upper area ✓

**Broken case (south door, `back_wall` order):**
- d0=(left, wall, 0=floor), d1=(right, wall, 0=floor), d2=(right, wall, -35=ceiling), d3=(left, wall, -35=ceiling)
- Vertices 0,1 are at **geometrically lower** z (floor = 0) — OPPOSITE from Painting A
- After `back_wall` rotation: q2=d0(floor), q3=d1(floor)
- `_top(q2=floor, q3=floor, q0=ceiling)` → covers **lower** area → door_wall_top at bottom ✗ **SWAPPED**

**Same swap for north door (`mirrored_back_wall` order):**
- After mirrored_back_wall (q0=p3, q1=p2, q2=p1, q3=p0): q2=d1(floor), q3=d0(floor)
- `_top(q2=floor, q3=floor, q0=ceiling)` → covers lower area ✗ **SWAPPED**

### Metis Review (gaps addressed)
- Metis correctly challenged the initial analysis that ALL directions were affected. Detailed per-direction trace confirmed only south (back_wall) and north (mirrored_back_wall) doors are affected. East (right_wall) and west (mirrored_right_wall) are correct.
- Metis prompted verification that the painting convention differs from door convention for the back_wall rotation case specifically.

## Work Objectives
### Core Objective
Fix the door vertex z-ordering for south and north wall doors so that `door_wall_top` renders at the visual top and `door_wall_bottom` at the visual bottom.

### Deliverables
- Corrected south wall door vertex z-assignments (d0,d1 at ceiling z, d2,d3 at floor z)
- Corrected north wall door vertex z-assignments (same swap)
- Successful ROM build
- Visual confirmation that door textures render correctly

### Definition of Done (verifiable conditions with commands)
- `make -j$(nproc)` completes without errors
- Door wall textures visually correct when ROM is run in emulator (top texture at top, bottom texture at bottom)

### Must Have
- South wall door vertices swapped: d0,d1 use door_top_z, d2,d3 use door_bottom_z
- North wall door vertices swapped: same pattern
- East and west wall door vertices UNCHANGED (they are correct)

### Must NOT Have (guardrails)
- Do NOT change east wall (right_wall) door vertices — they work correctly
- Do NOT change west wall (mirrored_right_wall) door vertices — they work correctly
- Do NOT change painting vertex ordering — paintings work correctly
- Do NOT change `textured_quad::set_points` or `map_wall_quad_points` — these are shared infrastructure
- Do NOT change `textured_triangle::_update_impl` — shared affine math
- Do NOT modify room model headers (`str_model_3d_items_room.h`)
- Do NOT add new files or change any headers

## Verification Strategy
> ZERO HUMAN INTERVENTION — all verification is agent-executed.
- Test decision: No automated tests (none exist in project). Build verification + visual inspection via emulator.
- QA policy: Build ROM, launch in emulator, capture screenshot of door rendering.
- Evidence: `.sisyphus/evidence/task-1-door-vertex-fix.png`

## Execution Strategy
### Parallel Execution Waves
Wave 1: [Task 1 - fix + build + verify] (single task, sequential operations)

### Dependency Matrix
| Task | Depends On | Blocks |
|------|-----------|--------|
| 1    | —         | F1-F4  |

### Agent Dispatch Summary
Wave 1: 1 task, category: quick

## TODOs

- [ ] 1. Fix south and north wall door vertex z-ordering in room_viewer.cpp

  **What to do**:
  In `src/room_viewer.cpp`, locate the door vertex computation block (around lines 1753-1812 in the `update_door_quads` lambda). For the **south wall** case (identified by `door_direction::south` and `wall_quad_order::back_wall`) and the **north wall** case (identified by `door_direction::north` and `wall_quad_order::mirrored_back_wall`), swap the z-values assigned to d0/d1 vs d2/d3.

  **Current south wall code** (approx lines 1786-1798):
  ```cpp
  // South wall door
  const bn::fixed wall_y = room_half_y - DOOR_WALL_INSET;
  const bn::fixed left_x = center_x - DOOR_HALF_WIDTH;
  const bn::fixed right_x = center_x + DOOR_HALF_WIDTH;
  d0 = local_point_to_view({left_x,  wall_y, door_bottom_z});  // z=0 (floor)
  d1 = local_point_to_view({right_x, wall_y, door_bottom_z});  // z=0 (floor)
  d2 = local_point_to_view({right_x, wall_y, door_top_z});     // z=-35 (ceiling)
  d3 = local_point_to_view({left_x,  wall_y, door_top_z});     // z=-35 (ceiling)
  ```

  **Fix**: Swap z-values so d0,d1 use `door_top_z` and d2,d3 use `door_bottom_z`:
  ```cpp
  d0 = local_point_to_view({left_x,  wall_y, door_top_z});     // z=-35 (ceiling)
  d1 = local_point_to_view({right_x, wall_y, door_top_z});     // z=-35 (ceiling)
  d2 = local_point_to_view({right_x, wall_y, door_bottom_z});  // z=0 (floor)
  d3 = local_point_to_view({left_x,  wall_y, door_bottom_z});  // z=0 (floor)
  ```

  **Apply the identical swap to the north wall case** (approx lines 1800-1812, implemented as the `default:` branch — there is no explicit `case door_direction::north:`):
  ```cpp
  // North wall door - same swap
  d0 = local_point_to_view({right_x, wall_y, door_top_z});     // was door_bottom_z
  d1 = local_point_to_view({left_x,  wall_y, door_top_z});     // was door_bottom_z
  d2 = local_point_to_view({left_x,  wall_y, door_bottom_z});  // was door_top_z
  d3 = local_point_to_view({right_x, wall_y, door_bottom_z});  // was door_top_z
  ```

  **Do NOT touch east or west wall cases** — they use right_wall/mirrored_right_wall order which works correctly with the current vertex convention.

  Also update the `d_center` and `d_normal` for south/north if they reference the z values (they don't — center uses door_center_z which is the midpoint, and normal is horizontal).

  **Must NOT do**:
  - Change east or west wall door vertices
  - Change painting vertices
  - Change textured_quad/textured_triangle classes
  - Change map_wall_quad_points

  **Recommended Agent Profile**:
  - Category: `quick` — Reason: Single-file, ~8 lines changed, clear before/after
  - Skills: [`stranded-windows-e2e-testing`] — For visual verification after build
  - Omitted: [`playwright`] — Not a web UI

  **Parallelization**: Can Parallel: NO | Wave 1 | Blocks: F1-F4 | Blocked By: none

  **References** (executor has NO interview context — be exhaustive):
  - Pattern: `src/room_viewer.cpp:1489-1608` — Painting A vertex convention (a0,a1 at higher z for back_wall) to follow
  - Bug location: `src/room_viewer.cpp:1786-1812` — South and north wall door vertex z-assignments to fix
  - Infrastructure: `src/room_viewer.cpp:539-552` — textured_quad::set_points showing how p0-p3 map to _top/_bottom triangles
  - Infrastructure: `src/room_viewer.cpp:1409-1443` — map_wall_quad_points showing back_wall rotation (q0=p2,q1=p3,q2=p0,q3=p1)
  - Correct reference: `src/room_viewer.cpp:1753-1770` — East wall door vertices (CORRECT, do not change)

  **Acceptance Criteria** (agent-executable only):
  - [ ] `make -j$(nproc)` completes with exit code 0
  - [ ] South wall door vertices d0,d1 use `door_top_z` (-35) and d2,d3 use `door_bottom_z` (0)
  - [ ] North wall door vertices d0,d1 use `door_top_z` (-35) and d2,d3 use `door_bottom_z` (0)
  - [ ] East wall door vertices UNCHANGED (d0,d1 use door_bottom_z, d2,d3 use door_top_z)
  - [ ] West wall door vertices UNCHANGED (d0,d1 use door_bottom_z, d2,d3 use door_top_z)

  **QA Scenarios** (MANDATORY):
  ```
  Scenario: Door textures render correctly after fix
    Tool: Bash (build) + VisualBoyAdvance (emulator)
    Steps:
      1. Build ROM: make -j$(nproc)
      2. Launch emulator: DISPLAY=:1 /usr/bin/VisualBoyAdvance <rom_path> &
      3. Navigate to room viewer (press Start, navigate menu)
      4. Observe south and north wall doors
      5. Take screenshot for evidence
    Expected: Door wall textures render as proper rectangles with top texture at top and bottom texture at bottom. No stretching, warping, or V-shaped artifacts.
    Evidence: .sisyphus/evidence/task-1-door-fix-after.png

  Scenario: East/west doors still render correctly (no regression)
    Tool: Same as above
    Steps:
      1. Same ROM build
      2. Navigate to room with east or west door (Room 0 has east, Room 1 has west)
      3. Observe east/west wall doors
      4. Take screenshot
    Expected: East and west door textures render correctly (unchanged from before fix)
    Evidence: .sisyphus/evidence/task-1-door-fix-eastwest.png
  ```

  **Commit**: YES | Message: `fix(room_viewer): swap door vertex z-order for south/north walls to fix texture mapping` | Files: [`src/room_viewer.cpp`]

## Final Verification Wave (4 parallel agents, ALL must APPROVE)
- [ ] F1. Plan Compliance Audit — oracle
- [ ] F2. Code Quality Review — unspecified-high
- [ ] F3. Real Manual QA — unspecified-high
- [ ] F4. Scope Fidelity Check — deep

## Commit Strategy
Single atomic commit after Task 1 passes all acceptance criteria and QA scenarios.

## Success Criteria
- Door wall textures render correctly on all 4 wall directions (south, north, east, west)
- No visual regression in room viewer (paintings, floor, walls, furniture all unchanged)
- ROM builds without errors or new warnings
