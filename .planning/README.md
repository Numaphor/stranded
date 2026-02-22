# Planning Docs

Last updated: 2026-02-22

## Purpose

This folder holds technical planning and maintenance notes for the Stranded codebase.
Docs here are for engineering decisions, implementation details, and update checklists.

## Layout

- `codebase/`: stable references for architecture, structure, 3D engine behavior, conventions, and quality practices.
- `features/`: focused notes for specific systems that changed recently.

## Current Documents

### Codebase

- `codebase/ARCHITECTURE.md`
- `codebase/STRUCTURE.md`
- `codebase/3D_ENGINE.md`
- `codebase/CONVENTIONS.md`
- `codebase/PLATFORM_AND_INTEGRATIONS.md`
- `codebase/QUALITY_AND_TESTING.md`

### Features

- `features/ROOM_VIEWER_CORNER_TRANSITIONS.md`

## Notes on Reorganization (2026-02-22)

- Merged and replaced:
  - `STACK.md` + `INTEGRATIONS.md` -> `PLATFORM_AND_INTEGRATIONS.md`
  - `CONCERNS.md` + `TESTING.md` -> `QUALITY_AND_TESTING.md`
  - `COORDINATE_DRIFT_FIX.md` + `ROOM_VIEWER_SMOOTH_CORNER_TRANSITION.md` -> `features/ROOM_VIEWER_CORNER_TRANSITIONS.md`

## Maintenance Rules

- Keep docs ASCII-only to avoid encoding issues in terminals.
- Prefer relative repository paths in file references.
- Update constants and behavior notes after gameplay-facing changes.
- If a file is superseded by a merged doc, delete the old file in the same change.
