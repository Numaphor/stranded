# Quality and Testing

Last updated: 2026-04-03

## Current Practice

- There are no automated tests.
- Run `make -j4` for the normal incremental build check.
- Run `make -B -j4` for the forced rebuild check.
- If `objcopy` fails because the ROM is still open in the emulator, close the
  emulator and rerun the build. Do not use `make clean` as recovery.
- Visual validation is local and emulator-driven.
- `scripts/mgba_f12_capture.ps1` is the repo helper for native mGBA `F12`
  screenshots.

## Manual Validation

For room-viewer changes, verify:

1. The ROM boots directly into the room viewer.
2. Movement and collision still behave correctly.
3. Door transitions still run and block movement while active.
4. Camera turning, idle recentering, `START` recentering, and auto-fit
   distance still behave correctly.
5. The minimap still updates correctly.
6. `BgDialog` still opens and advances correctly with `A`.
7. Paintings and NPC sprites still render correctly.
8. If room decor is active in the current pass, decor rendering and decor
   collision still behave correctly, including during door transitions.

When a change is visual, use native mGBA `F12` screenshots and inspect them
directly.

## Main Risks

- `src/viewer/runtime/room_viewer_runtime_systems.cpp` still couples input, movement,
  camera motion, and door transitions in one large runtime loop.
- `src/viewer/room_renderer.bn_iwram.cpp` is the performance-sensitive render
  path.
- The room-shell generator and the build-generated include path must stay in
  sync.
