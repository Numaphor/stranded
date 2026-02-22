# Room Viewer Corner Transitions

Last updated: 2026-02-22

## Scope

This document describes how corner switching works in `src/core/room_viewer.cpp`.
It covers the current smooth transition implementation and tuning points.

## Current Behavior

- `START` triggers a corner change only if no corner transition is active.
- Transition rotates the world by one quarter turn over time instead of snapping.
- Transition timing is controlled by:
  - `QUARTER_TURN_ANGLE = 16384` (90 degrees in a 16-bit angle domain)
  - `CORNER_TURN_DURATION_FRAMES = 20`
- Easing uses smoothstep:
  - `eased = t * t * (3 - 2 * t)`
- During the transition:
  - room + decor rotation matrices are updated every frame
  - movement input is ignored to avoid control/view mismatch
  - zoom (`L`/`R`) and debug toggle (`SELECT`) still work
- On completion:
  - `_corner_index` advances to the next corner
  - player facing is remapped via `_rotate_player_dir(...)`

## Implementation Overview

Core logic lives in `src/core/room_viewer.cpp`:

- `compute_corner_matrices(...)` creates the base isometric orientation set.
- `rotate_corner_matrix(...)` rotates a base corner matrix by an interpolated angle.
- Runtime transition state:
  - `current_view_angle`
  - `corner_transition_active`
  - `corner_transition_elapsed`
  - `corner_transition_start_angle`
  - `corner_transition_target_angle`
- `update_all_orientations()` applies the active matrix to room/table/chair models.

## Historical Note

Older planning notes described a pseudo-inverse coordinate conversion workflow.
The current implementation uses a single floor coordinate space and rotates view
orientation over time, so that older description is no longer the active design.

## Tuning

- Slower transition: increase `CORNER_TURN_DURATION_FRAMES`.
- Faster transition: decrease `CORNER_TURN_DURATION_FRAMES`.
- Different feel: replace smoothstep with linear or another easing function.

## Manual Verification Checklist

1. Press `START` once and confirm a smooth turn (no snap).
2. Press `START` repeatedly during a turn and confirm no overlapping transitions.
3. Hold movement input during turn and confirm movement is blocked until complete.
4. Change zoom with `L`/`R` during turn and confirm zoom still updates.
5. Confirm player facing and movement mapping are correct after each completed turn.
