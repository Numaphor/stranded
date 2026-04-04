# Planning Docs

Last updated: 2026-04-04

## Purpose

This folder holds the current technical notes for the minimal room-viewer
baseline.

## Layout

- `codebase/` holds the stable references for architecture, layout,
  conventions, 3D runtime behavior, toolchain notes, and validation.
- `features/` holds focused notes that are still useful during the cull.

## Current Documents

### Codebase

- `codebase/ARCHITECTURE.md` - room-viewer architecture and runtime layers.
- `codebase/STRUCTURE.md` - current repo layout and surviving directories.
- `codebase/3D_ENGINE.md` - room-viewer 3D runtime, transforms, and limits.
- `codebase/CONVENTIONS.md` - naming, includes, types, formatting, and hot
  spots.
- `codebase/PLATFORM_AND_INTEGRATIONS.md` - build toolchain and local tooling.
- `codebase/QUALITY_AND_TESTING.md` - local validation and regression notes.

### Features

- `features/MGBA_DEBUG_GUIDE.md` - local mGBA debugging workflow.
- `features/MINIMAL_RUNTIME_CULL_BASELINE.md` - baseline note for the cull.
- `features/LOC_REDUCTION_EXECUTION.md` - approved Gate A/B/C execution and doc-sync note for the current LOC-reduction push.

## Maintenance Rules

- Keep docs ASCII-only.
- Prefer relative repository paths in file references.
- Update notes when room-viewer behavior or build requirements change.
- Delete superseded notes instead of keeping parallel historical copies.
