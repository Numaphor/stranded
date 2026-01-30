---
name: phases-skill
description: Interface to GSD phase workflows (execute, verify, plan, UAT). Load workflows from subrepo and follow them; update the subrepo to change behavior.
---

## Phases skill (interface)

Use this skill when the user runs **gsd:execute-phase**, **gsd:verify-work**, **gsd:plan-phase**, **gsd:list-phase-assumptions**, or related phase-level commands.

**Source of truth (subrepo).** Load and follow the relevant workflow(s) from the GSD subrepo; do not duplicate their steps here.

| Command / scenario | Workflow to load |
|--------------------|------------------|
| execute-phase | `.cursor/gsd/workflows/execute-phase.md` |
| execute-plan (single plan) | `.cursor/gsd/workflows/execute-plan.md` |
| verify-phase (goal verification) | `.cursor/gsd/workflows/verify-phase.md` |
| verify-work (UAT) | `.cursor/gsd/workflows/verify-work.md` |
| list-phase-assumptions | `.cursor/gsd/workflows/list-phase-assumptions.md` |
| discuss-phase | `.cursor/gsd/workflows/discuss-phase.md` |
| discovery-phase | `.cursor/gsd/workflows/discovery-phase.md` |
| diagnose-issues (after UAT issues) | `.cursor/gsd/workflows/diagnose-issues.md` |

Also load when referenced by the workflow:

- **Templates:** `.cursor/gsd/templates/` (e.g. `summary.md`, `UAT.md`, `verification-report.md`, `checkpoints.md`, `continuation-format.md`).
- **References:** `.cursor/gsd/references/` (e.g. `verification-patterns.md`, `git-integration.md`, `checkpoints.md`, `tdd.md`).

Read the workflow first. Follow its `<process>` and `<step>` sections. Use its `<success_criteria>` as your checklist.

**Cursor adaptations.**

- Use **Read** to load the workflow, templates, and references (and `.planning/STATE.md`, `.planning/ROADMAP.md`, etc.) at the start.
- Use **Shell** for any bash in the workflow. Prefer project root; adjust paths for Windows if needed.
- Use **Write** / **StrReplace** for `.planning/` and phase files.
- When the workflow spawns a subagent (e.g. gsd-executor, gsd-verifier, gsd-planner, gsd-plan-checker), use Cursor’s agent/Task mechanism and the agent definitions under `.cursor/agents/` (e.g. `gsd-executor`, `gsd-verifier`, `gsd-planner`, `gsd-plan-checker`). Inline into the prompt any context the workflow says to pass (plan content, state, config).
- Git: per-task and per-plan commits as in the workflow; never commit or tag without user confirmation. Honor `.planning/config.json` and `git check-ignore .planning`.

**When to use.**

- `gsd:execute-phase`, `gsd:verify-work`, `gsd:plan-phase`, `gsd:list-phase-assumptions`, `gsd:discuss-phase`, or “execute phase X” / “verify phase X” / “UAT phase X”.

Updating any of these workflows or templates under `.cursor/gsd/` will change how this skill behaves the next time they are loaded.
