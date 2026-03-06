# Learnings: fix-door-texture-warp

## Project Conventions
- GBA Butano project, no automated tests. Validation = build ROM + emulator.
- Build: `export WONDERFUL_TOOLCHAIN=/opt/wonderful && export PATH="/opt/wonderful/bin:/opt/wonderful/toolchain/gcc-arm-none-eabi/bin:$PATH" && make -j$(nproc)`
- Output ROM: `stranded.gba` (local repo) or `workspace.gba` (cloud)
- Emulator: `DISPLAY=:1 /usr/bin/VisualBoyAdvance <rom> &` (NOT mgba-qt)
- Evidence dir: `.sisyphus/evidence/`
- Worktree: `/home/numa/repos/stranded-door-fix`

## Root Cause
- Door wall textures (door_wall_top / door_wall_bottom) swap vertically on SOUTH and NORTH walls.
- Pipeline: `textured_quad::set_points(p0,p1,p2,p3)` → `_top(p2,p3,p0)`, `_bottom(p0,p1,p2)`
- `back_wall` mapping rotates: q0=p2, q1=p3, q2=p0, q3=p1
- For `_top` to cover the visual top, q2 and q3 must be geometrically higher (more negative z)
- Painting A (working): a0,a1 at z=-32.4 (higher/ceiling-ward). After back_wall rotation, q2=a0,q3=a1 → higher → _top at top ✓
- Door south (broken): d0,d1 at z=0 (floor). After back_wall rotation, q2=d0,q3=d1 → floor → _top at bottom ✗
- Same issue with mirrored_back_wall for north wall doors
- East (right_wall) and west (mirrored_right_wall) use identity/mirror — NOT affected ✓

## Fix
- South wall: swap d0,d1 to use `door_top_z` (-35) and d2,d3 to use `door_bottom_z` (0)
- North wall (default: branch): same swap
- DO NOT touch east or west wall cases
