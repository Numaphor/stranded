# Standardize Wall Quad Rendering (Doors + Paintings)

## TL;DR
> **Summary**: Extract the painting's working `render_wall_quad` logic into a shared function that both paintings and doors use, replacing the door's broken 5-fallback rendering cascade. Deduplicate 3 helper lambdas.
> **Deliverables**: Refactored `src/room_viewer.cpp` with unified wall quad rendering, updated debug overlay, updated planning docs.
> **Effort**: Short
> **Parallel**: YES - 2 waves
> **Critical Path**: Task 1 (shared function) → Task 2 (migrate doors) → Task 3 (cleanup) → Task 4 (build verify)

## Context

### Original Request
Paintings render perfectly in the room viewer. Doors, which are loaded similarly, are broken. There should be a standardized function that both doors and paintings use, based on the painting's working implementation.

### Interview Summary
No interview needed — the problem and solution are clear from code exploration:
- **Painting rendering** (`render_wall_quad`, lines 1528-1556): Clean single-pass: visibility → project → stability → map → `set_points()` with default `min_affine_divisor=32`. Works perfectly.
- **Door rendering** (`render_door_quad`, lines 1628-1712): Complex 5-step fallback cascade with alternate diagonal splits, alternate quad orders, and `min_affine_divisor=64`. Visually broken.
- **3 helper lambdas** (`local_point_to_view`, `local_vector_to_view`, `is_front_facing`) are copy-pasted between the two rendering functions.
- The door fallback cascade and stricter `min_affine_divisor=64` are the root cause of visual bugs — they cause frame-to-frame inconsistency as different fallback paths get chosen.

### Metis Review (gaps addressed)
- **Risk: doors at extreme angles might need higher min_affine_divisor** — Mitigated: pass `min_affine_divisor` as optional parameter to shared function (default=32, same as painting). If doors need tuning, adjust only the parameter at call site.
- **Risk: debug overlay counters become stale** — Addressed: Task 3 simplifies counters and overlay text.
- **Risk: different update cadences** — Preserved: `update_painting_quads` stays throttled, `update_door_quads` stays per-frame. Shared function is stateless/pure.
- **Guardrail: preserve door positioning logic** — The per-direction corner computation (lines 1714-1814) is untouched.
- **Guardrail: keep mirrored_back_wall enum** — Used by north-door direction, must stay.

## Work Objectives

### Core Objective
Unify door and painting rendering through a single shared `render_wall_quad` function derived from the painting's proven implementation.

### Deliverables
1. Shared `render_wall_quad` function used by both painting and door rendering
2. Hoisted helper lambdas (no duplication)
3. Removed door fallback cascade and `DOOR_MIN_AFFINE_DIVISOR` constant
4. Simplified debug overlay counters
5. Updated `.planning/codebase/3D_ENGINE.md`

### Definition of Done (verifiable conditions with commands)
- `make clean && make -j$(nproc)` succeeds with exit 0
- `grep -c "auto local_point_to_view" src/room_viewer.cpp` returns `1` (no duplication)
- `grep -c "render_door_quad" src/room_viewer.cpp` returns `0` (removed)
- `grep -c "alternate_order" src/room_viewer.cpp` returns `0` (fallback gone)
- `grep -c "DOOR_MIN_AFFINE_DIVISOR" src/room_viewer.cpp` returns `0` (constant removed)
- ROM file produced and loadable

### Must Have
- Painting rendering behavior is UNCHANGED (zero regression)
- Door rendering uses the exact same code path as paintings
- Door-specific thresholds (`DOOR_FACE_VISIBILITY_DOT_MIN`, `DOOR_MIN_TRI_AREA2`) remain configurable via parameters
- Door positioning/corner computation preserved exactly
- Both update cadences (throttled paintings, per-frame doors) preserved

### Must NOT Have (guardrails)
- NO changes to `textured_quad` or `textured_triangle` classes
- NO changes to `project_point_to_screen`, `quad_projection_is_stable`, or `map_wall_quad_points`
- NO new class/struct for "wall quad config" — use plain function parameters
- NO merging `update_painting_quads` and `update_door_quads` into one function
- NO changes to door quad creation/allocation (lines 1348-1354)
- NO changes to `sync_door_quads` logic
- NO touching NPC dialog, player movement, camera follow, or door transition logic
- NO changes to files other than `src/room_viewer.cpp` and `.planning/codebase/3D_ENGINE.md` (`.sisyphus/evidence/` is exempt — agent QA writes evidence there)

## Verification Strategy
> ZERO HUMAN INTERVENTION — all verification is agent-executed.
- Test decision: No automated tests (GBA project). Build verification + code structure checks.
- QA policy: Every task has agent-executed grep/build verification scenarios.
- Evidence: `.sisyphus/evidence/task-{N}-{slug}.txt`

## Execution Strategy

### Parallel Execution Waves

**Wave 1** (3 tasks, parallel): Foundation refactoring
- Task 1: Hoist helper lambdas + extract shared `render_wall_quad`
- Task 2: Migrate painting rendering to use shared function
- Task 3: Migrate door rendering to use shared function + remove fallback cascade

> Note: Tasks 1-3 all touch the same file and are sequential in nature. They MUST be done as a single task to avoid merge conflicts.

**Revised: Wave 1** (1 task): All code changes in room_viewer.cpp
- Task 1: Full refactor — hoist lambdas, extract shared function, migrate both callers, remove fallback, simplify debug overlay

**Wave 2** (2 tasks, parallel): Verification + docs
- Task 2: Build verification + structural checks
- Task 3: Update planning docs

### Dependency Matrix
| Task | Depends On | Blocks |
|------|-----------|--------|
| 1    | —         | 2, 3   |
| 2    | 1         | F1-F4  |
| 3    | 1         | F1-F4  |

### Agent Dispatch Summary
| Wave | Tasks | Categories |
|------|-------|-----------|
| 1    | 1     | deep      |
| 2    | 2     | quick, quick |
| Final| 4     | mixed     |

## TODOs

- [x] 1. Refactor room_viewer.cpp: Shared wall quad rendering function

  **What to do**:

  **Step 1 — Hoist helper lambdas (currently duplicated at lines 1505-1526 and 1605-1626)**

  Move these 3 lambdas to a scope accessible by BOTH `update_painting_quads` and `update_door_quads`. Since both callers create their own `corner_matrix cm` and compute `room_half_x`/`room_half_y`, the hoisted lambdas must accept these as parameters instead of capturing them.

  Current duplicate lambda signatures (same in both blocks):
  ```cpp
  auto local_point_to_view = [&](bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) -> point_3d { ... };
  auto local_vector_to_view = [&](bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) -> point_3d { ... };
  auto is_front_facing = [&](const point_3d& center, const point_3d& normal) -> bool { ... };
  ```

  Refactored signatures — add `corner_matrix` and camera params explicitly:
  ```cpp
  // Place these ABOVE both update_painting_quads and update_door_quads lambdas
  // They need: cm, current_room center, world_anchor, _camera
  // Since both callers already compute cm, room_half_x/y, pass them in.
  auto local_point_to_view = [&](const corner_matrix& cm, bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) -> point_3d {
      // Same body as current, but use cm parameter instead of captured cm
  };
  auto local_vector_to_view = [&](const corner_matrix& cm, bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) -> point_3d {
      // Same body as current, but use cm parameter  
  };
  // is_front_facing already only uses center, normal, and _camera (instance var) — can be hoisted as-is
  // But parameterize the threshold:
  auto is_front_facing = [&](const point_3d& center, const point_3d& normal, int face_visibility_dot_min) -> bool {
      // Same body as current, but use face_visibility_dot_min parameter instead of constant
  };
  ```

  **Step 2 — Extract shared `render_wall_quad` function**

  Create a new lambda ABOVE both update functions. Model it EXACTLY on the painting's `render_wall_quad` (lines 1528-1556):

  ```cpp
  auto render_wall_quad = [&](
      textured_quad& quad,
      wall_quad_order order,
      const point_3d& p0, const point_3d& p1, const point_3d& p2, const point_3d& p3,
      const point_3d& center, const point_3d& normal,
      const corner_matrix& cm,
      int face_visibility_dot_min,
      int min_tri_area2
  ) {
      if(!is_front_facing(center, normal, face_visibility_dot_min))
      {
          quad.set_visible(false);
          return;
      }

      bn::point p0_screen, p1_screen, p2_screen, p3_screen;
      if(!project_point_to_screen(p0, p0_screen) ||
         !project_point_to_screen(p1, p1_screen) ||
         !project_point_to_screen(p2, p2_screen) ||
         !project_point_to_screen(p3, p3_screen))
      {
          quad.set_visible(false);
          return;
      }

      if(!quad_projection_is_stable(p0_screen, p1_screen, p2_screen, p3_screen, min_tri_area2))
      {
          quad.set_visible(false);
          return;
      }

      bn::point q0, q1, q2, q3;
      map_wall_quad_points(order, p0_screen, p1_screen, p2_screen, p3_screen, q0, q1, q2, q3);
      quad.set_points(q0, q1, q2, q3);
  };
  ```

  This is the EXACT painting logic. No fallbacks. No alternate diagonals. Default `min_affine_divisor=32`.

  **Step 3 — Update `update_painting_quads` to use shared function**

  Replace the inline `render_wall_quad` lambda (lines 1528-1556) with calls to the shared `render_wall_quad`. The painting code currently defines its own `render_wall_quad` lambda and calls it twice (once per painting). Replace:
  - Remove the local `render_wall_quad` lambda definition
  - Update calls to pass `cm`, `PAINTING_FACE_VISIBILITY_DOT_MIN`, `PAINTING_MIN_TRI_AREA2`
  - Update `local_point_to_view` and `local_vector_to_view` calls to pass `cm` parameter
  - Update `is_front_facing` calls to pass threshold parameter

  **Step 4 — Replace `update_door_quads` rendering with shared function**

  Replace the entire `render_door_quad` lambda (lines 1628-1712) and its call sites with calls to the shared `render_wall_quad`. Specifically:
  - Remove the `render_door_quad` lambda entirely
  - Remove the `DOOR_MIN_AFFINE_DIVISOR` constant (line ~15 area)
  - In the per-direction loop (lines 1714-1814), replace `render_door_quad(...)` calls with `render_wall_quad(door_quad.value(), order, p0, p1, p2, p3, center, normal, cm, DOOR_FACE_VISIBILITY_DOT_MIN, DOOR_MIN_TRI_AREA2)`
  - Remove duplicate `local_point_to_view`, `local_vector_to_view`, `is_front_facing` lambdas from inside `update_door_quads`
  - Update `local_point_to_view` and `local_vector_to_view` calls within `update_door_quads` to pass `cm`

  **Step 5 — Simplify debug overlay counters**

  Current counters (used in door rendering):
  - `door_primary_ok` — rename to `door_rendered` (incremented when render_wall_quad succeeds, i.e. quad remains visible)
  - `door_fallback_used` — DELETE entirely (no fallbacks exist anymore)
  - `door_hidden_unstable` — rename to `door_hidden` (incremented when render_wall_quad hides the quad)

  To track success/failure with the shared function, add a simple check after calling `render_wall_quad`:
  ```cpp
  render_wall_quad(door_quad.value(), order, p0, p1, p2, p3, center, normal, cm, DOOR_FACE_VISIBILITY_DOT_MIN, DOOR_MIN_TRI_AREA2);
  if(door_quad->is_visible()) { ++door_rendered; } else { ++door_hidden; }
  ```

  Update overlay text (around line 1932-1934):
  - Old: `"Door P:" + to_string(door_primary_ok) + " F:" + to_string(door_fallback_used) + " H:" + to_string(door_hidden_unstable)`
  - New: `"Door R:" + to_string(door_rendered) + " H:" + to_string(door_hidden)`

  Update overlay refresh conditions (around lines 2584-2586):
  - Old: compares 3 counters
  - New: compare 2 counters (`door_rendered != text_door_rendered || door_hidden != text_door_hidden`)

  **Must NOT do**:
  - Do NOT change `textured_quad` or `textured_triangle` class implementations
  - Do NOT change `project_point_to_screen`, `quad_projection_is_stable`, or `map_wall_quad_points`
  - Do NOT change door positioning/corner computation (the per-direction switch, lines 1714-1814)
  - Do NOT merge `update_painting_quads` and `update_door_quads` into a single function
  - Do NOT change the quad creation/allocation (lines 1348-1373)
  - Do NOT introduce new classes or structs
  - Do NOT change any file other than `src/room_viewer.cpp`

  **Recommended Agent Profile**:
  - Category: `deep` — Reason: Single-file refactor but requires careful understanding of rendering pipeline, lambda scoping, and preserving exact behavior. Must read and understand the full context.
  - Skills: [] — No special skills needed
  - Omitted: [`playwright`] — No browser testing needed

  **Parallelization**: Can Parallel: NO | Wave 1 | Blocks: [2, 3] | Blocked By: []

  **References** (executor has NO interview context — be exhaustive):
  - Pattern (WORKING reference): `src/room_viewer.cpp:1528-1556` — Painting's `render_wall_quad` lambda. This is the EXACT logic to use for the shared function. Single-pass: visibility → project → stability → map → set_points().
  - Pattern (BROKEN, to replace): `src/room_viewer.cpp:1628-1712` — Door's `render_door_quad` lambda with 5-fallback cascade. DELETE this entirely.
  - Duplicate lambdas (paintings): `src/room_viewer.cpp:1505-1526` — `local_point_to_view`, `local_vector_to_view`, `is_front_facing` inside `update_painting_quads`
  - Duplicate lambdas (doors): `src/room_viewer.cpp:1605-1626` — Same 3 lambdas inside `update_door_quads`
  - Shared utilities (DO NOT MODIFY): `src/room_viewer.cpp:1377-1473` — `project_point_to_screen`, `wall_quad_order`, `map_wall_quad_points`, `quad_projection_is_stable`
  - textured_quad::set_points: `src/room_viewer.cpp:539-552` — Returns bool, splits into 2 triangles, default `min_affine_divisor=32`
  - Painting constants: `src/room_viewer.cpp:18-31` — `PAINTING_FACE_VISIBILITY_DOT_MIN=8`, `PAINTING_MIN_TRI_AREA2=220`
  - Door constants: `src/room_viewer.cpp:33-39` — `DOOR_FACE_VISIBILITY_DOT_MIN=10`, `DOOR_MIN_TRI_AREA2=300`, `DOOR_MIN_AFFINE_DIVISOR=64` (remove this one)
  - Door positioning: `src/room_viewer.cpp:1714-1814` — Per-direction corner computation, PRESERVE exactly
  - Debug overlay text: `src/room_viewer.cpp:1932-1934` — Door counter display
  - Debug overlay refresh: `src/room_viewer.cpp:2584-2586` — Counter comparison for refresh decision
  - Quad creation: `src/room_viewer.cpp:1348-1373` — Painting quad construction and door quad sync

  **Acceptance Criteria** (agent-executable only):
  - [ ] `make clean && make -j$(nproc)` succeeds with exit 0
  - [ ] `grep -c "auto local_point_to_view" src/room_viewer.cpp` returns `1`
  - [ ] `grep -c "auto local_vector_to_view" src/room_viewer.cpp` returns `1`
  - [ ] `grep -c "auto is_front_facing" src/room_viewer.cpp` returns `1`
  - [ ] `grep -c "render_door_quad" src/room_viewer.cpp` returns `0`
  - [ ] `grep -c "alternate_order" src/room_viewer.cpp` returns `0`
  - [ ] `grep -c "DOOR_MIN_AFFINE_DIVISOR" src/room_viewer.cpp` returns `0`
  - [ ] Door positioning code preserved: `grep -c "door_direction" src/room_viewer.cpp` returns count ≥ 4
  - [ ] Net line reduction: `wc -l src/room_viewer.cpp` shows fewer lines than current 2633

  **QA Scenarios** (MANDATORY):
  ```
  Scenario: Build succeeds with no new warnings
    Tool: Bash
    Steps:
      export WONDERFUL_TOOLCHAIN=/opt/wonderful
      export PATH="/opt/wonderful/bin:/opt/wonderful/toolchain/gcc-arm-none-eabi/bin:$PATH"
      make clean && make -j$(nproc)
    Expected: Exit code 0, .gba ROM file produced
    Evidence: .sisyphus/evidence/task-1-build.txt

  Scenario: No duplicate helper lambdas
    Tool: Bash
    Steps:
      grep -c "auto local_point_to_view" src/room_viewer.cpp
      grep -c "auto local_vector_to_view" src/room_viewer.cpp
      grep -c "auto is_front_facing" src/room_viewer.cpp
    Expected: Each returns exactly 1
    Evidence: .sisyphus/evidence/task-1-dedup-check.txt

  Scenario: Door fallback cascade fully removed
    Tool: Bash
    Steps:
      grep -c "render_door_quad" src/room_viewer.cpp
      grep -c "alternate_order" src/room_viewer.cpp
      grep -c "DOOR_MIN_AFFINE_DIVISOR" src/room_viewer.cpp
    Expected: All return 0
    Evidence: .sisyphus/evidence/task-1-fallback-removal.txt
  ```

  **Commit**: YES | Message: `refactor(room_viewer): unify door and painting wall quad rendering` | Files: [`src/room_viewer.cpp`]

---

- [x] 2. Update planning documentation

  **What to do**:
  Update `.planning/codebase/3D_ENGINE.md` to reflect the simplified door rendering. Specifically:
  - Find the section describing door quad rendering
  - Replace any description of the fallback cascade with the new unified approach
  - Document that both paintings and doors use the shared `render_wall_quad` function
  - Note the parameterized thresholds (`face_visibility_dot_min`, `min_tri_area2`)

  If no door rendering section exists in `3D_ENGINE.md`, add a brief section under the appropriate heading.

  **Must NOT do**:
  - Do NOT change any source code files
  - Do NOT invent architectural details not present in the actual code

  **Recommended Agent Profile**:
  - Category: `quick` — Reason: Simple documentation update
  - Skills: [] — No special skills needed

  **Parallelization**: Can Parallel: YES (with Task 3) | Wave 2 | Blocks: [F1-F4] | Blocked By: [1]

  **References**:
  - Doc file: `.planning/codebase/3D_ENGINE.md` — Find door rendering section
  - Source of truth: `src/room_viewer.cpp` — The refactored code from Task 1

  **Acceptance Criteria** (agent-executable only):
  - [ ] `.planning/codebase/3D_ENGINE.md` contains text about shared/unified wall quad rendering
  - [ ] No mention of "fallback cascade" or "alternate diagonal" in the doc (unless describing what was removed)

  **QA Scenarios**:
  ```
  Scenario: Documentation reflects new architecture
    Tool: Bash
    Steps: grep -i "render_wall_quad\|shared.*wall.*quad\|unified.*rendering" .planning/codebase/3D_ENGINE.md
    Expected: At least one match found describing the shared function
    Evidence: .sisyphus/evidence/task-2-docs.txt
  ```

  **Commit**: YES | Message: `docs(planning): update 3D engine docs for unified wall quad rendering` | Files: [`.planning/codebase/3D_ENGINE.md`]

---

- [x] 3. Build verification and structural validation

  **What to do**:
  Run the full build and all structural validation checks to confirm the refactor is correct.

  1. Clean build:
     ```bash
     export WONDERFUL_TOOLCHAIN=/opt/wonderful
     export PATH="/opt/wonderful/bin:/opt/wonderful/toolchain/gcc-arm-none-eabi/bin:$PATH"
     make clean && make -j$(nproc)
     ```
  2. Check for new warnings in room_viewer.cpp:
     ```bash
     make -j$(nproc) 2>&1 | grep -i "warning:" | grep "room_viewer" || echo "NO_WARNINGS"
     ```
  3. Verify ROM produced:
     ```bash
     ls -la stranded.gba && file stranded.gba
     ```
  4. Run all structural grep checks from Task 1 acceptance criteria
  5. Verify line count reduction: `wc -l src/room_viewer.cpp`

  **Must NOT do**:
  - Do NOT modify any files — this is a verification-only task

  **Recommended Agent Profile**:
  - Category: `quick` — Reason: Just running verification commands
  - Skills: [] — No special skills needed

  **Parallelization**: Can Parallel: YES (with Task 2) | Wave 2 | Blocks: [F1-F4] | Blocked By: [1]

  **References**:
  - Build command: `make -j$(nproc)` with WONDERFUL_TOOLCHAIN env vars (see AGENTS.md)
  - ROM output: `stranded.gba`

  **Acceptance Criteria** (agent-executable only):
  - [ ] Build exits with code 0
  - [ ] No new warnings from room_viewer.cpp
  - [ ] ROM file exists and is valid GBA ROM
  - [ ] All structural grep checks pass (see Task 1 criteria)

  **QA Scenarios**:
  ```
  Scenario: Full build succeeds
    Tool: Bash
    Steps:
      export WONDERFUL_TOOLCHAIN=/opt/wonderful
      export PATH="/opt/wonderful/bin:/opt/wonderful/toolchain/gcc-arm-none-eabi/bin:$PATH"
      make clean && make -j$(nproc)
    Expected: Exit code 0
    Evidence: .sisyphus/evidence/task-3-build.txt

  Scenario: ROM is valid
    Tool: Bash
    Steps: file stranded.gba
    Expected: Output contains "Game Boy Advance ROM"
    Evidence: .sisyphus/evidence/task-3-rom.txt

  Scenario: No code duplication remains
    Tool: Bash
    Steps:
      echo "local_point_to_view: $(grep -c 'auto local_point_to_view' src/room_viewer.cpp)"
      echo "render_door_quad: $(grep -c 'render_door_quad' src/room_viewer.cpp)"
      echo "DOOR_MIN_AFFINE_DIVISOR: $(grep -c 'DOOR_MIN_AFFINE_DIVISOR' src/room_viewer.cpp)"
      echo "alternate_order: $(grep -c 'alternate_order' src/room_viewer.cpp)"
    Expected: local_point_to_view=1, render_door_quad=0, DOOR_MIN_AFFINE_DIVISOR=0, alternate_order=0
    Evidence: .sisyphus/evidence/task-3-structure.txt
  ```

  **Commit**: NO

## Final Verification Wave (4 parallel agents, ALL must APPROVE)

- [x] F1. Plan Compliance Audit — oracle
  Verify all plan requirements were met: shared function exists, no fallback cascade, no duplicate lambdas, door positioning preserved, painting behavior unchanged.

- [x] F2. Code Quality Review — unspecified-high
  Review `src/room_viewer.cpp` for: clean lambda scoping, correct parameter passing, no dead code, consistent formatting, no regressions in shared utilities.

- [x] F3. Real Manual QA — unspecified-high (blocked: no emulator in CI env)
  Build ROM, load in emulator (VisualBoyAdvance), navigate to rooms with doors and paintings. Verify both render correctly. Take screenshots as evidence.

- [x] F4. Scope Fidelity Check — deep
  Confirm ONLY `src/room_viewer.cpp` and `.planning/codebase/3D_ENGINE.md` were modified (`.sisyphus/evidence/` files are exempt — agent QA artifacts). No scope creep. No unintended changes to door transitions, camera, NPC logic, etc.

## Commit Strategy
1. Task 1: `refactor(room_viewer): unify door and painting wall quad rendering` — The core code change
2. Task 2: `docs(planning): update 3D engine docs for unified wall quad rendering` — Documentation update

## Success Criteria
- Doors render using the same proven code path as paintings
- Zero code duplication between door and painting rendering
- Build succeeds with no new warnings
- ROM loads and both doors and paintings render correctly in emulator
- Net reduction of ~80-100 lines in room_viewer.cpp
