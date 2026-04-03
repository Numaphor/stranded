# RALPLAN: Higher-Delta LOC Reduction In `src/` And `include/`

## Requirements Summary

- Source of truth: `.omx/specs/deep-interview-reduce-loc-src-include.md`
- Current measured baseline: `4340` LOC across `src/` and `include/`
- Goal: achieve a much larger LOC drop than the initial formatting-heavy pass while preserving the current room-viewer play contract
- Non-negotiable behavior contract:
  - boot lands in the room viewer
  - movement and collision still work
  - door transitions still work
  - camera recentering and auto-fit distance still work
  - minimap updates still work
  - `BgDialog` still displays and advances correctly
- Scope constraints:
  - keep room-viewer-only scope
  - do not reintroduce removed world, combat, menu, or model-viewer systems
  - `make -j4` must pass
  - baseline plan must not physically merge [room_renderer.bn_iwram.cpp](d:\repo\stranded\src\viewer\room_renderer.bn_iwram.cpp)

## Acceptance Criteria

1. Gate A lands at `<= 4100` LOC minimum, with stretch `<= 4025`.
2. Gate B lands at least `120` net lines below the measured Gate A result.
3. Gate C lands at least `80` net lines below the measured Gate B result.
4. `make -j4` and `make -B -j4` succeed at each gate.
5. Manual runtime validation still passes for boot, movement/collision, door transitions, camera recentering/auto-fit, minimap, and `BgDialog`.
6. Any renderer-adjacent or visual-affecting change passes native mGBA `F12` parity checks.

## RALPLAN-DR Summary

### Principles

- Delete and collapse gameplay-support modules before touching renderer internals.
- Preserve IWRAM and scanline behavior by keeping the IWRAM translation unit physically separate.
- Use staged gates with measurable LOC checkpoints and explicit verification.
- Perform move-only helper relocation before pruning to preserve bisectability.
- Stop on the first contract regression instead of pushing through instability.

### Decision Drivers

- Guaranteed LOC wins from the existing deletion inventory
- Lowest regression risk first
- Fast regression detection with the current build and emulator workflow

### Viable Options

#### Option A: Gameplay/Support-First Plus Renderer API Shrink Only

- Execute Gate A, Gate B, and Gate C below.
- Do not merge renderer implementation files.
- Use staged checkpoints and hard stops.

#### Option B: Deferred Renderer Implementation Collapse

- Not part of the baseline plan.
- Only becomes eligible after Gate C passes and a separate ADR re-approves touching deeper renderer internals.

## Implementation Steps

### Gate A: Support-Module Collapse

Target:
- `<= 4100` LOC minimum
- stretch `<= 4025`

Deletion inventory basis:
- [room_viewer_runtime_state.cpp](d:\repo\stranded\src\viewer\runtime\room_viewer_runtime_state.cpp): `60`
- [room_viewer_runtime_state.h](d:\repo\stranded\include\private\viewer\runtime\room_viewer_runtime_state.h): `13`
- [minimap_layout.cpp](d:\repo\stranded\src\core\minimap\minimap_layout.cpp): `139`
- [str_bg_dialog_text.cpp](d:\repo\stranded\src\core\dialog\str_bg_dialog_text.cpp): `131`
- Raw removable inventory before reinsertion: `343` lines

File operations:
- Edit [room_viewer_runtime_systems.cpp](d:\repo\stranded\src\viewer\runtime\room_viewer_runtime_systems.cpp) to absorb:
  - `NPC_INTERACT_DIST`
  - NPC hat colors
  - NPC dialog option tables
  - `begin_npc_dialog`
- Delete [room_viewer_runtime_state.cpp](d:\repo\stranded\src\viewer\runtime\room_viewer_runtime_state.cpp).
- Delete [room_viewer_runtime_state.h](d:\repo\stranded\include\private\viewer\runtime\room_viewer_runtime_state.h).
- Move methods from [minimap_layout.cpp](d:\repo\stranded\src\core\minimap\minimap_layout.cpp) into [minimap.cpp](d:\repo\stranded\src\core\minimap\minimap.cpp), then delete [minimap_layout.cpp](d:\repo\stranded\src\core\minimap\minimap_layout.cpp).
- Move methods from [str_bg_dialog_text.cpp](d:\repo\stranded\src\core\dialog\str_bg_dialog_text.cpp) into [str_bg_dialog.cpp](d:\repo\stranded\src\core\dialog\str_bg_dialog.cpp), then delete [str_bg_dialog_text.cpp](d:\repo\stranded\src\core\dialog\str_bg_dialog_text.cpp).
- Update include edges in affected files:
  - remove the runtime-state include from [room_viewer_runtime_systems.cpp](d:\repo\stranded\src\viewer\runtime\room_viewer_runtime_systems.cpp)
  - keep or tighten [str_minimap.h](d:\repo\stranded\include\str_minimap.h) and [str_bg_dialog.h](d:\repo\stranded\include\str_bg_dialog.h) based on the merged symbol surface

Build graph note:
- No Makefile source-list edit is required because [Makefile](d:\repo\stranded\Makefile#L35) uses directory-based `SOURCES`.

### Gate B: Shared Runtime Helper Localization And Pruning

Target:
- at least `120` net lines lower than the measured Gate A result

Scope:
- Work in [room_viewer_runtime_systems_shared.h](d:\repo\stranded\include\private\viewer\runtime\room_viewer_runtime_systems_shared.h).
- Use a move-only pass first, compile, then prune only after deadness or successful localization is proven.

First-wave relocation candidates:
- `NUM_ROOMS`
- `ENABLE_PAINTING_QUADS`
- `ENABLE_NPC_SPRITES`
- `ENABLE_MINIMAP`
- `NPC_*`
- `PLAYER_*`
- `MOVE_SPEED`
- `DOOR_TRANSITION_*`
- `SPAWN_*`
- `room_has_decor`
- `room_decor_model`
- `check_door_transition`
- `near_door_approach`
- `update_player_anim_tiles`

Second-wave candidates, only after compile-stable proof:
- `normalize_angle`
- `shortest_angle_delta`
- `corner_from_view_angle`
- `wrap_linear8`
- `view_angle_steps_8`
- `heading_angle_from_linear8`
- `step_angle_toward_target`
- `dir_to_linear8`
- `linear8_to_dir`
- `dir_face_from_screen_delta`
- `minimap_dir_from_player_dir`
- `screen_to_room_delta`
- `rotate_corner_matrix`
- `compute_corner_matrices`

### Gate C: Renderer-Safe API Compression

Target:
- at least `80` net lines lower than the measured Gate B result

Scope:
- Shrink [str_room_renderer.h](d:\repo\stranded\include\private\viewer\str_room_renderer.h) to the runtime-consumed surface only.
- Keep [room_renderer.cpp](d:\repo\stranded\src\viewer\room_renderer.cpp) and [room_renderer.bn_iwram.cpp](d:\repo\stranded\src\viewer\room_renderer.bn_iwram.cpp) physically separate.
- Do not baseline any merge of renderer implementation files.
- Preserve IWRAM and scanline placement assumptions.

### Docs Sync

- Update `.planning/` only if structure or ownership changes materially enough that the docs become stale.

## Risks And Mitigations

- Risk: gameplay-support collapses change NPC, minimap, or dialog behavior.
  - Mitigation: Gate A ends with full manual runtime validation before any deeper pruning.
- Risk: helper relocation in [room_viewer_runtime_systems_shared.h](d:\repo\stranded\include\private\viewer\runtime\room_viewer_runtime_systems_shared.h) introduces semantic drift.
  - Mitigation: move-only first, compile, then prune incrementally.
- Risk: renderer-adjacent API shrink causes subtle visual regressions.
  - Mitigation: Gate C stays API-only, keeps the IWRAM TU intact, and requires `F12` parity captures.
- Risk: rollback becomes fuzzy after multiple edits.
  - Mitigation: isolate each gate with its own checkpoint artifact before continuing.

## Verification Steps

Run these at each gate:

1. `make -j4`
2. `make -B -j4`
3. LOC measurement:

```powershell
Get-ChildItem src,include -Recurse -File |
  Where-Object { $_.Extension -in '.cpp', '.h', '.hpp' } |
  ForEach-Object { ([System.IO.File]::ReadAllLines($_.FullName)).Length } |
  Measure-Object -Sum
```

4. Include-edge sanity:

```powershell
rg -n "runtime_state.h|str_minimap.h|str_bg_dialog.h|room_viewer_runtime_systems_shared.h|str_room_renderer.h" src include
```

5. Emulator checks:
  - boot to room viewer
  - movement and collision
  - door transitions
  - camera recentering and auto-fit distance
  - minimap updates
  - `BgDialog` open and advance
6. Visual parity:
  - capture native mGBA `F12` screenshots before and after any renderer-adjacent or visual-affecting gate
  - use the same room, facing, and transition moments for comparison
7. Create an isolated checkpoint after each passing gate:
  - use one consistent mechanism across the run: `commit`, `tag`, or saved patch artifact

## Hard Stop And Rollback Criteria

- Stop a gate if `make -j4` fails twice after targeted fixes.
- Stop on any play-contract regression.
- Stop if minimap, dialog, or NPC behavior changes unexpectedly after Gate A.
- Stop if Gate C changes renderer behavior beyond API-layer edits.
- Roll back only the current gate’s isolated patch-set and keep earlier passing gates intact.
- Scanline and render-math internals remain out of scope unless explicitly re-approved after Gate C.

## ADR

- Decision:
  - execute Gate A, Gate B, and Gate C as the baseline plan
  - defer renderer implementation collapse
- Drivers:
  - guaranteed LOC wins
  - lower regression risk
  - staged verification
- Alternatives Considered:
  - immediate renderer implementation merge
  - formatting-only densification
- Why Chosen:
  - inventory-backed deletions provide measurable wins before taking on the riskiest subsystem
- Consequences:
  - denser, less modular runtime code
  - stronger checkpoint discipline is required
- Follow-ups:
  - if Gate C passes and the result is still insufficient, open a separate ADR for renderer implementation collapse

## Available-Agent-Types Roster

- `planner`
- `architect`
- `critic`
- `executor`
- `build-fixer`
- `debugger`
- `test-engineer`
- `verifier`
- `code-reviewer`
- `security-reviewer`
- `explore`
- `researcher`
- `writer`

## Follow-up Staffing Guidance

### Ralph Lane

- `executor` at `high`: Gate A collapse and deletion work
- `build-fixer` at `high`: compile-break resolution between gates
- `verifier` at `high`: per-gate LOC and runtime-contract checks
- `critic` or `code-reviewer` at `medium`: regression review before proceeding to the next gate

Launch hint:
- `$ralph .omx/plans/ralplan-reduce-loc-src-include.md`

### Team Lane

- Worker A, `executor` at `high`: runtime-state fold
- Worker B, `executor` at `high`: minimap collapse
- Worker C, `executor` at `high`: dialog collapse
- Worker D, `build-fixer` at `high`: integration and build triage
- Worker E, `verifier` at `high`: checkpoint verification and LOC accounting

Launch hints:
- `$team .omx/plans/ralplan-reduce-loc-src-include.md`
- `omx team .omx/plans/ralplan-reduce-loc-src-include.md`

Team verification path:
- team proves Gate A artifacts compile and pass contract checks before shutdown
- team isolates each gate with a checkpoint artifact
- any later Ralph pass verifies Gate B and Gate C before deeper renderer work is even considered

## Changelog

- Replaced the ungrounded single `+800` target with staged Gate A/B/C targets.
- Narrowed renderer scope to API compression only in the baseline plan.
- Added explicit per-gate verification commands, hard stops, rollback rules, and checkpoint isolation.
- Added a concrete deletion and include-update plan for the files that can disappear in Gate A.
