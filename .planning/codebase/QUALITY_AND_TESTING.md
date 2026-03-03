# Quality and Testing

Last updated: 2026-03-03

## Current Quality Snapshot

- Project-level automated gameplay tests are not yet present.
- CI validates build and release flow, not gameplay correctness.
- Runtime safeguards rely heavily on `BN_ASSERT(...)` and manual emulator testing.
- `STACKTRACE = true` is enabled in Makefile for debug builds.

## Primary Technical Risks

### `src/core/world.cpp` complexity

- Large execution loop with many responsibilities (player, enemies, minimap, quests, camera, zoom).
- High chance of regressions when changing combat/camera/UI in one patch.
- Recommendation: continue extracting focused helpers in small steps.

### Scene lifecycle ownership

- Most scenes are stack-owned in `main.cpp`, but viewer scenes (MODEL_VIEWER, ROOM_VIEWER) still use manual `new`/`delete`.
- Recommendation: align viewer ownership with stack pattern where feasible.

### 3D behavior coupling in room viewer

- Input mapping, corner index, and transform updates are tightly linked.
- Camera follow uses continuous heading-based tracking with easing.
- Recommendation: keep transition behavior documented and covered by manual checklist after edits.

### Dialog system VRAM coordination

- `RoomDialog` frees HUD sprites for VRAM during dialog display.
- `BgDialog` avoids sprite VRAM entirely using BG-layer rendering.
- Recommendation: document VRAM budget constraints when adding new dialog features.

## Testing Strategy (Current)

### Automated

- CI build in `.github/workflows/build.yml`:
  - Wonderful Toolchain setup
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
- dialog system behavior
- minimap rendering accuracy

### Manual

Baseline manual pass after gameplay or rendering changes:

1. Start -> character select -> menu -> world -> return flow.
2. Core movement and combat interactions (chop, slash, shoot, roll, combo).
3. Weapon switching (sword/gun) and gun selection menu.
4. Buff activation (Heal/Energy/Power) and cooldown behavior.
5. NPC interaction: merchant dialog, quest acceptance, progress, turn-in.
6. Enemy behavior: patrol, chase, attack, knockback, return-to-post, death.
7. Collectible spawning and pickup.
8. Companion follow, death, and revival.
9. Minimap room tracking and enemy dot display.
10. Room viewer navigation, collision, and door transitions.
11. Room viewer dialog system (BgDialog and RoomDialog).
12. Model viewer open/close and camera controls.

## Room Viewer Regression Checklist

1. Camera follows committed heading smoothly with no snap.
2. `L/R` zoom adjusts camera distance within 100-500 range.
3. `START` recenters toward committed heading with boost.
4. Door transitions interpolate smoothly (16 frames, smoothstep).
5. Furniture collision and door transitions still behave correctly after several rotations.
6. Movement input is blocked during door transitions and restored after completion.
7. Player animation and facing direction update correctly per heading.
8. Paintings/orientations refresh when view angle moves by `CAMERA_RENDER_UPDATE_ANGLE_STEP` (64).

## World/Combat Regression Checklist

1. Player HP (0-3), ammo (0-10), and energy (0-3) display correctly in HUD.
2. Health bar transitions animate correctly on gain/loss.
3. Combo window (60 frames) chains attacks correctly.
4. Roll provides invulnerability during animation.
5. Enemy state machine transitions: idle -> patrol -> chase -> attack -> return correctly.
6. Knockback pushes enemy away from player.
7. Bullets fire in correct direction, collide with enemies, despawn after lifetime.
8. Merchant quest dialog options update based on quest state.
9. Quest progress notifications trigger correctly (heart collection, enemy kills).

## Suggested Next Improvements

- Add smoke-test scripts for scene entry/exit sanity.
- Add deterministic math checks for room-viewer transform invariants.
- Add a lightweight regression script or checklist runner under `tests/`.
- Add VRAM budget monitoring to catch sprite overflow issues.
