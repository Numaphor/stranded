# Quality and Testing

Last updated: 2026-03-22

## Current Practice

- There are no automated tests.
- `make -j4` is the primary build check.
- Validation is local and emulator-driven.

## Manual Validation

For room-viewer changes, verify:

1. The ROM boots directly into the room viewer.
2. Movement and collision still behave correctly.
3. Door transitions still run and block movement while active.
4. Camera recentering and distance changes still work.
5. The minimap still updates correctly.
6. `BgDialog` still opens and advances correctly.

When a change is visual, use native mGBA `F12` screenshots and inspect the
result directly.

## Main Risks

- `src/room_viewer.cpp` couples input, camera motion, and room transitions.
- `src/viewer/fr_models_3d.bn_iwram.cpp` is the performance-sensitive render
  path.
- Asset or header changes can break the generated-room model pipeline if the
  references drift.

## Suggested Next Improvements

- Keep room-viewer changes small and verify them in mGBA before merging.
- Add lightweight notes or scripts only if they help the current room-viewer
  baseline.
