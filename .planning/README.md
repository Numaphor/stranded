# Planning Docs

Last updated: 2026-03-03

## Purpose

This folder holds technical planning and maintenance notes for the Stranded codebase.
Docs here are for engineering decisions, implementation details, and update checklists.

## Layout

- `codebase/`: stable references for architecture, structure, 3D engine behavior, conventions, and quality practices.
- `features/`: focused notes for specific systems that changed recently.

## Current Documents

### Codebase

- `codebase/ARCHITECTURE.md` -- scene system, architecture layers, entity hierarchy, key subsystems.
- `codebase/STRUCTURE.md` -- file/directory inventory, header reference, tooling, asset layout.
- `codebase/3D_ENGINE.md` -- coordinate system, transform pipeline, runtime limits, project extensions, room viewer setup, dialog systems.
- `codebase/CONVENTIONS.md` -- naming, includes, types, memory, formatting, constants, hotspots.
- `codebase/PLATFORM_AND_INTEGRATIONS.md` -- toolchain, build config, CI/CD, emulator setup, asset organization.
- `codebase/QUALITY_AND_TESTING.md` -- technical risks, testing strategy, manual checklists, regression checks.

### Features

- `features/ROOM_VIEWER_CORNER_TRANSITIONS.md` -- camera follow system, door transitions, player representation, dialog.

## Maintenance Rules

- Keep docs ASCII-only to avoid encoding issues in terminals.
- Prefer relative repository paths in file references.
- Update constants and behavior notes after gameplay-facing changes.
- If a file is superseded by a merged doc, delete the old file in the same change.
