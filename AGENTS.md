# AGENTS.md

## Project Overview

Stranded is a minimal room-viewer-only Game Boy Advance homebrew project built
with Butano. The build produces a `.gba` ROM and the current runtime boots
straight into the room viewer.

## Grounding

Before making changes, read the current planning docs in `.planning/codebase/`
and use the local Butano headers as the source of truth for engine behavior.
If the headers are not enough, check the official Butano docs.

## Build

```bash
make -j4
```

For a clean rebuild:

```bash
make clean && make -j4
```

## Validation

There are no automated tests. Validate changes by building the ROM and running
it in mGBA or another GBA emulator.

For room-viewer changes, check:

- boot lands in the room viewer
- movement and collision still work
- door transitions still work
- camera recentering and distance changes still work
- minimap updates still work
- `BgDialog` still displays and advances correctly

When a change is visual, use native mGBA `F12` screenshots for inspection.

## Working Rules

- Keep `.planning/` docs in sync with runtime changes.
- Do not reintroduce deleted world, combat, menu, or model-viewer systems
  unless the task explicitly asks for them.
- Prefer small, incremental edits in `src/room_viewer.cpp`, `src/core/`, and
  `src/viewer/`.
- Follow the naming, include, and type conventions documented in
  `.planning/codebase/CONVENTIONS.md`.
