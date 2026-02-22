# Quality and Testing

Last updated: 2026-02-22

## Current Quality Snapshot

- Project-level automated gameplay tests are not yet present.
- CI validates build and release flow, not gameplay correctness.
- Runtime safeguards rely heavily on `BN_ASSERT(...)` and manual emulator testing.

## Primary Technical Risks

### `src/core/world.cpp` complexity

- Large execution loop with many responsibilities.
- High chance of regressions when changing combat/camera/UI in one patch.
- Recommendation: continue extracting focused helpers in small steps.

### Scene lifecycle ownership

- Most scenes are stack-owned in `main.cpp`, but viewer scenes still use manual `new/delete`.
- Recommendation: align viewer ownership with stack pattern where feasible.

### 3D behavior coupling in room viewer

- Input mapping, corner index, and transform updates are tightly linked.
- Recommendation: keep transition behavior documented and covered by manual checklist after edits.

## Testing Strategy (Current)

### Automated

- CI build in `.github/workflows/build.yml`:
  - toolchain setup
  - ROM build
  - artifact upload
  - release/package publish on changed ROM hash

What this catches:

- compile failures
- link/build pipeline failures
- release automation breakage

What this does not catch:

- gameplay regressions
- collision/AI correctness
- visual correctness in room/model viewer scenes

### Manual

Baseline manual pass after gameplay or rendering changes:

1. Start -> character select -> menu -> world -> return flow.
2. Core movement and combat interactions.
3. NPC interaction and quest progression paths.
4. Room viewer navigation, collision, and door transitions.
5. Model viewer open/close and camera controls.

## Room Viewer Regression Checklist

1. `START` triggers smooth corner transition with no snap.
2. Repeated `START` presses do not queue overlapping transitions.
3. Movement input is blocked during transition and restored after completion.
4. `L/R` zoom remains responsive while turning.
5. Furniture collision and door transitions still behave correctly after several rotations.

## Suggested Next Improvements

- Add smoke-test scripts for scene entry/exit sanity.
- Add deterministic math checks for room-viewer transform invariants.
- Add a lightweight regression script or checklist runner under `tests/`.
