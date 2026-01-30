---
name: planning-files-skill
description: Interface to GSD planning layout and semantics. Points at subrepo templates and references so updating the subrepo keeps Cursor in sync.
---

## Planning-files skill (interface)

Use this skill whenever you work with `.planning/` (PROJECT, ROADMAP, REQUIREMENTS, MILESTONES, STATE, phases, codebase, intel). It tells you where the **definitions** live so you can stay in sync with the subrepo.

**Source of truth (subrepo).** The layout and semantics of `.planning/` are defined by the GSD subrepo. Load these when you need to create or interpret planning files:

- **Templates (canonical shapes):**
  - `.cursor/gsd/templates/project.md`
  - `.cursor/gsd/templates/roadmap.md`
  - `.cursor/gsd/templates/requirements.md`
  - `.cursor/gsd/templates/state.md`
  - `.cursor/gsd/templates/milestone.md`
  - `.cursor/gsd/templates/summary.md`
  - `.cursor/gsd/templates/verification-report.md`
  - `.cursor/gsd/templates/phase-prompt.md`
  - `.cursor/gsd/templates/context.md`
  - `.cursor/gsd/templates/UAT.md`
  - `.cursor/gsd/templates/codebase/*.md`
- **Config and references:**
  - `.cursor/gsd/templates/config.json`
  - `.cursor/gsd/references/planning-config.md`
  - `.cursor/gsd/references/git-integration.md`

**Minimal layout (for quick reference).** Do not duplicate full docs here; treat the subrepo as authoritative.

- `.planning/PROJECT.md` — project identity, core value, requirements (Validated/Active/Out of Scope), key decisions. Updated at milestone boundaries.
- `.planning/ROADMAP.md` — milestones and phases, goals, status. Completed milestone details archived under `.planning/milestones/vX.Y-ROADMAP.md`.
- `.planning/REQUIREMENTS.md` — current milestone only; archived to `.planning/milestones/vX.Y-REQUIREMENTS.md` at completion, then recreated for next milestone.
- `.planning/MILESTONES.md` — log of shipped versions (stats, accomplishments).
- `.planning/STATE.md` — current position, progress, decisions, blockers, session continuity.
- `.planning/phases/` — per-phase dirs with PLAN.md, SUMMARY.md, VERIFICATION.md, optional UAT.md, USER-SETUP.md.
- `.planning/milestones/` — archives (vX.Y-ROADMAP.md, vX.Y-REQUIREMENTS.md, optional vX.Y-MILESTONE-AUDIT.md).
- `.planning/codebase/` — STACK, ARCHITECTURE, STRUCTURE, CONVENTIONS, TESTING, INTEGRATIONS, CONCERNS (see codebase-mapping-skill).
- `.planning/intel/` — optional; index and entities for codebase intelligence.

**Cursor adaptations.**

- Use **Read** to load the templates above when creating or updating `.planning/` files so structure matches the subrepo.
- Respect `.planning/config.json` (e.g. `commit_docs`, `model_profile`) and `git check-ignore .planning` before committing planning docs.
- Never commit or tag without explicit user confirmation.

**When to use.**

- Any command or flow that reads/writes PROJECT, ROADMAP, REQUIREMENTS, MILESTONES, STATE, or phase/codebase/milestone artifacts.

Updating the templates or references under `.cursor/gsd/` will change how planning files are shaped and interpreted; no need to change this skill when you edit those files.
