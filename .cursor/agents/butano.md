---
name: butano
model: big-pickle
description: Ultrawork-mode subagent suite for this Butano GBA game (C++23, devkitARM, Butano engine).
readonly: true
---

## Purpose

- **This agent defines the subagents that MUST be launched whenever `.cursor/rules/ultrawork.mdc` calls for subagents in this Butano GBA project.**
- It assumes Ultrawork Mode is active and that this repository is the Butano GBA game described by `.cursor/rules/butano-gba.mdc` and `.cursor/rules/codebase-documentation.mdc`.
- The main orchestrator agent **must not** skip these subagents before making non-trivial edits.

## Global Rules

- **Always read**:
  - `.cursor/rules/butano-gba.mdc`
  - `.planning/codebase/CONVENTIONS.md`
  - `.planning/codebase/ARCHITECTURE.md`
  - `.planning/codebase/STRUCTURE.md`
- **Never introduce** `float`/`double`, STL containers, or heap allocations in hot paths (see Butano rule file).
- **All subagents run in readonly mode**; only the orchestrator edits files.

## Subagent 1 — Codebase Mapper (Butano Focus)

- **Type**: `codebase-mapper`
- **Model**: fast
- **When to use**: Before any substantial feature, refactor, or bugfix touching core gameplay, world, or rendering.
- **Scope**:
  - Map high-level architecture: scenes, world loop, actors/entities, input, rendering pipeline.
  - Identify key files:
    - `include/str_player.h`
    - `src/core/world.cpp`
    - `include/str_constants.h`
    - Other `include/str_*.h` and `src/actors/*`
- **Expected output back to orchestrator**:
  - Short summary of architecture and scene flow.
  - Pointers to the exact files + functions that should be modified for the requested task.
  - Any Butano-specific constraints relevant to the change (memory, performance, containers).

Example Task tool prompt:

```yaml
subagent_type: codebase-mapper
model: fast
description: "Map Butano GBA codebase for requested feature/refactor"
prompt: |
  Analyze this Butano GBA project and update / create documents under .planning/codebase/.
  Focus on:
  - Scene system and world loop (World::execute and friends)
  - Player and actor systems in include/ and src/actors/
  - Constants and configuration (include/str_constants.h, Makefile)
  Return to the main agent:
  - A concise summary of architecture and control flow.
  - The specific files and functions most relevant to the current requested task.
readonly: true
```

## Subagent 2 — Structure & Hotspot Explorer

- **Type**: `explore`
- **Model**: fast
- **When to use**: After the codebase-mapper has run, before deciding exactly where to edit.
- **Scope**:
  - Explore and index:
    - `src/core/world.cpp`
    - `include/str_player.h`
    - `src/actors/`
    - Any subsystem related to the current task (e.g., `src/core/`, `src/ui/`, etc.).
- **Expected output**:
  - List of key classes/functions with line ranges.
  - Pointers to similar patterns or existing implementations to copy.

Example Task tool prompt:

```yaml
subagent_type: explore
model: fast
description: "Explore Butano GBA hotspots for the requested change"
prompt: |
  Thoroughly explore the following paths:
  - src/core/world.cpp
  - include/str_player.h
  - src/actors/
  - Any other files referenced by the current task description.
  Find:
  - Where the requested behavior currently happens (if at all).
  - Existing patterns for input handling, entity updates, collisions, and rendering.
  Return to the main agent:
  - Function/class signatures and short descriptions.
  - File + line pointers for the most relevant places to modify or extend.
readonly: true
```

## Subagent 3 — Build & Runtime Context

- **Type**: `generalPurpose`
- **Model**: fast
- **When to use**: For tasks affecting build flags, assets, or performance-sensitive changes.
- **Scope**:
  - Understand `Makefile` flags (`LIBBUTANO`, `USERFLAGS`, `BN_CFG_*`, `STACKTRACE`).
  - Locate asset configuration under `graphics/`, `audio/`, and any JSON asset descriptors.
  - Summarize how to build and test changes (mGBA, etc.).

Example Task tool prompt:

```yaml
subagent_type: generalPurpose
model: fast
description: "Analyze Butano build, asset, and runtime context"
prompt: |
  Inspect the Makefile and asset folders (graphics/, audio/, dmg_audio/).
  Determine:
  - How this project configures Butano (USERFLAGS, BN_CFG_*).
  - How sources and subfolders are wired into the build.
  - Any asset pipelines that are relevant to the requested change.
  Return concise notes:
  - Commands the orchestrator should run to rebuild.
  - Files or flags that must be updated for the requested change.
readonly: true
```

## Subagent 4 — Focused System Deep-Dive (Optional)

- **Type**: `explore`
- **Model**: fast
- **When to use**: When a task is isolated to a particular system (e.g., player, enemies, UI) and needs deeper exploration.
- **Scope examples**:
  - Player state machine and input handling (`include/str_player.h` and related `src` files).
  - World update and collision (`src/core/world.cpp`, `src/actors/*`).
  - UI or HUD systems if present.

Example Task tool prompt:

```yaml
subagent_type: explore
model: fast
description: "Deep-dive into specific Butano subsystem"
prompt: |
  For the subsystem involved in the current task (for example, the player, world, or UI),
  explore all relevant headers in include/ and sources in src/.
  Return:
  - The state machine or lifecycle for that subsystem.
  - Where inputs are processed, where updates happen, and where rendering is triggered.
  - Any invariants or constraints that the orchestrator must preserve when editing.
readonly: true
```

## Orchestrator Checklist (Must Follow)

- **Before non-trivial edits in this repo**:
  1. Run **Subagent 1 (Codebase Mapper)** if `.planning/codebase/` docs are missing or outdated for the task area.
  2. Run **Subagent 2 (Structure & Hotspot Explorer)** to get concrete file/function pointers.
  3. Run **Subagent 3 (Build & Runtime Context)** when changes may affect build flags, performance, or assets.
  4. Optionally run **Subagent 4 (Focused Deep-Dive)** for complex subsystems (player, world, etc.).
- **Then**:
  - Use the gathered pointers and summaries to plan and implement changes in the orchestrator.
  - Respect all constraints from `.cursor/rules/butano-gba.mdc` and `.cursor/rules/codebase-documentation.mdc`.
