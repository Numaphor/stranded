---
name: milestones-skill
description: Interface to GSD milestone-completion ritual. Load workflow from subrepo and follow it; update the subrepo to change behavior.
---

## Milestones skill (interface)

Use this skill when the user runs **gsd:complete-milestone** or asks to mark a milestone/version as shipped.

**Source of truth (subrepo).** Load and follow these files from the GSD subrepo; do not duplicate their steps here.

- **Workflow:** `.cursor/gsd/workflows/complete-milestone.md`
- **Template:** `.cursor/gsd/templates/milestone-archive.md`
- **Template:** `.cursor/gsd/templates/milestone.md`

Read the workflow first. Follow its `<process>` and `<step>` sections. Use its `<success_criteria>` and `<critical_rules>` as your checklist.

**Cursor adaptations.**

- Use the **Read** tool to load the workflow and templates (and `.planning/*` files) at the start.
- Use the **Shell** tool for any bash in the workflow (e.g. `git`, `ls`, `cat`). Prefer project root; adjust paths for Windows if needed.
- Use the **Write** tool to create/update `.planning/` files and archives.
- Do not assume a separate “orchestrator”; you are the executor. If the workflow mentions spawning subagents, use Cursor’s Task/agent mechanism and point to agents under `.cursor/agents/` (e.g. no change to workflow intent—just use local agent definitions).
- Git: never commit or tag without explicit user confirmation. Honor `.planning/config.json` and `git check-ignore .planning` before staging planning docs.

**When to use.**

- `gsd:complete-milestone` or “mark vX.Y shipped” / “archive this milestone”.
- You want to collapse ROADMAP/REQUIREMENTS for the completed milestone and prepare for the next.

Updating the workflow or templates under `.cursor/gsd/` will change how this skill behaves the next time the workflow is loaded.
