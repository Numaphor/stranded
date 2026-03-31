# Quality and Testing

Last updated: 2026-03-30

## Current Practice

- There are no automated tests.
- `make -j4` is the primary build check.
- `make PROFILE_ENGINE=1 -j4` is the investigation build when full engine
  profiler logging is needed.
- Validation is local and emulator-driven.
- `scripts/mgba_f12_capture.ps1` is the canonical screenshot helper for native
  mGBA `F12` capture in this repo.

## Manual Validation

For room-viewer changes, verify:

1. The ROM boots directly into the room viewer.
2. Movement and collision still behave correctly.
3. Door transitions still run and block movement while active.
4. The camera still turns quickly through 8-direction headings, recenters
   after 1 second of no input away from the room center, and `START`/distance
   changes still work.
5. The minimap still updates correctly and hides while the debug menu or
   profiler menu is active.
6. `BgDialog` still opens and advances correctly with `A`.
7. `SELECT` opens and closes the BG debug menu, and `SELECT` + `L` still
   toggles room-model visibility.
8. The `B` profiler menu opens, closes, scrolls, switches total/max with `A`,
   and resets samples with `START` without corrupting the room viewer.

When a change is visual, use native mGBA `F12` screenshots and inspect the
result directly.

For overlay and profiler changes, capture at least:

- `powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1 -SkipBuild -Sequence toggle_select`
- `powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1 -SkipBuild -Sequence toggle_profiler`
- `powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1 -SkipBuild -Sequence profiler_toggle_mode`
- `powershell -ExecutionPolicy Bypass -File scripts/mgba_f12_capture.ps1 -SkipBuild -Sequence profiler_reset`

For the Maria-style interior pipeline, also verify:

9. Room shells still render through the existing world path while only their
   shell shapes and colors change.
10. Offline-baked Interior-pack sprite sheets build cleanly and keep the
   intended sharp 8-view ordering.
11. When reviewing baked prop frames, imported props stay grounded and keep
   their silhouette centered consistently across the full turntable.

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
