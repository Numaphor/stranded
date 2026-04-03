# Deep Interview Spec: Reduce LOC In `src/` And `include/`

## Metadata

- Profile: `standard`
- Context type: `brownfield`
- Final ambiguity: `0.125`
- Threshold: `0.20`
- Rounds: `4`
- Context snapshot: `.omx/context/reduce-loc-src-include-20260403T215603Z.md`
- Transcript: `.omx/interviews/reduce-loc-src-include-20260403T220038Z.md`

## Clarity Breakdown

| Dimension | Score |
| --- | ---: |
| Intent | 0.95 |
| Outcome | 0.95 |
| Scope | 0.95 |
| Constraints | 0.90 |
| Success | 0.90 |
| Context | 0.85 |

## Intent

Reduce total checked-in LOC in `src/` and `include/` as aggressively as practical. Denser code is preferred over maintainability-oriented structure.

## Desired Outcome

The repository ends with materially fewer lines in `src/` and `include/` while preserving the currently shipped room-viewer runtime behavior during play.

## In Scope

- Aggressive behavior-preserving LOC reduction across `src/` and `include/`
- Collapsing file/module boundaries
- Making renderer/runtime code denser
- Changing public/private internal boundaries if runtime behavior still holds
- Updating `.planning/` docs to match runtime or structural changes

## Out Of Scope / Non-goals

- Preserving current maintainability, readability, or future extensibility for their own sake
- Keeping the current file decomposition intact
- Protecting performance-sensitive files from refactor solely because they are sensitive

## Decision Boundaries

OMX may decide without further confirmation:

- which files to target
- whether to merge, inline, or densify code
- whether renderer internals are touched
- whether headers shrink, move, or disappear
- whether `.planning/` docs are updated to match the new structure

OMX must preserve without asking again:

- boot lands in the room viewer
- movement and collision still work
- door transitions still work
- camera recentering and distance changes still work
- minimap updates still work
- `BgDialog` still displays and advances correctly

## Constraints

- Preserve current room-viewer functionality while playing.
- Build the ROM successfully with `make -j4`.
- Validate against the documented manual room-viewer checklist.
- Keep the project within the documented room-viewer-only scope; do not reintroduce removed systems.

## Testable Acceptance Criteria

1. Net LOC across `src/` and `include/` is lower after the change.
2. The project still builds successfully with `make -j4`.
3. The resulting ROM still boots directly into the room viewer.
4. Movement, collision, door transitions, camera recentering/auto-fit, minimap behavior, and `BgDialog` interaction still behave correctly during manual play validation.

## Assumptions Exposed And Resolved

- Assumption: the user mainly wanted maintainability improvements.
  - Resolved: false; the user prefers denser code and lower LOC.
- Assumption: renderer and header internals might be off-limits because they are sensitive.
  - Resolved: false; they are in scope if play behavior is preserved.
- Assumption: extensibility might still matter as a secondary goal.
  - Resolved: false; extensibility can regress if current play behavior survives.

## Pressure-pass Findings

- Earlier answer revisited: “denser code is the goal”
- Pressure question: whether to still take the biggest LOC win if it collapses module boundaries or hurts future extensibility
- Final answer: yes
- Execution consequence: choose the highest-yield LOC reductions even when they make internals harder to extend later

## Technical Context Findings

- Highest-LOC targets in scope:
  - `src/viewer/runtime/room_viewer_runtime_systems.cpp`
  - `src/viewer/room_renderer.bn_iwram.cpp`
  - `include/private/viewer/runtime/room_viewer_runtime_systems_shared.h`
  - `include/private/viewer/str_room_renderer.h`
  - `src/core/dialog/str_bg_dialog.cpp`
  - `src/viewer/room_renderer.cpp`
- Current manual validation contract comes from `.planning/codebase/QUALITY_AND_TESTING.md`
- Room-viewer-only scope and main hotspots are documented in `.planning/codebase/ARCHITECTURE.md` and `.planning/codebase/CONVENTIONS.md`

## Condensed Transcript

1. Goal favors denser code and lower LOC over maintainability.
2. Anything may change if current gameplay functionality remains preserved.
3. The documented room-viewer validation checklist is the full execution contract.
4. Extensibility can be sacrificed if the LOC reduction is real and the runtime still behaves correctly.
