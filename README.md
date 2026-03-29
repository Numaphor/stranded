# Stranded

Stranded is a minimal Game Boy Advance homebrew project built with Butano.
The current build boots directly into the room viewer and keeps only the
runtime needed for that path.

## Build

```bash
make -j4
```

## Run

Launch the generated `stranded.gba` in mGBA or another GBA emulator.
The repo includes a bundled Windows mGBA build under `tools/mGBA-0.10.5-win64/`
for local validation, and `scripts/mgba_f12_capture.ps1` provides the native
`F12` screenshot flow used for visual checks.

## Current Runtime

The surviving gameplay surface is the room viewer:

- room navigation and collision
- door transitions
- camera follow, recentering, and distance changes
- minimap updates
- NPC dialog through `BgDialog`
- simplified generated room shells for floors and walls
- baked Interior-pack prop assets and helper scripts for future room-decor
  integration

## Docs

- `AGENTS.md` for contributor guidance
- `.planning/` for architecture, structure, conventions, and validation notes
- `.planning/features/MINIMAL_RUNTIME_CULL_BASELINE.md` for the cull baseline
