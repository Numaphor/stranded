---
name: codebase-mapping-skill
description: Interface to GSD codebase-mapping workflow. Load workflow from .cursor/gsd and follow it; edit files there to change behavior.
---

## Codebase-mapping skill (interface)

Use this skill when the user runs **gsd:map-codebase** or **gsd:analyze-codebase** (for the mapping part), or asks to analyze the codebase and produce `.planning/codebase/` docs.

**Source of truth.** Load and follow the workflow from `.cursor/gsd/`; do not duplicate its steps here.

- **Workflow:** `.cursor/gsd/workflows/map-codebase.md`
- **Mapper agent definition:** `.cursor/agents/gsd-codebase-mapper.md`
- **Templates for output:** `.cursor/gsd/templates/codebase/` (e.g. `stack.md`, `architecture.md`, `structure.md`, `conventions.md`, `testing.md`, `integrations.md`, `concerns.md`) if the workflow or mapper references them.

Read the workflow first. Follow its `<process>` and `<step>` sections (resolve_model_profile, check_existing, create_structure, spawn_agents, collect_confirmations, verify_output, commit_codebase_map, offer_next). Use its `<success_criteria>` as your checklist.

**Cursor adaptations.**

- Use **Read** to load the workflow and, when spawning mappers, the agent definition under `.cursor/agents/gsd-codebase-mapper.md`.
- Use **Shell** for any bash in the workflow (e.g. `ls .planning/codebase/`, `wc -l`). Prefer project root; adjust for Windows if needed.
- Use **Write** to create `.planning/codebase/` and the 7 docs; the mapper agent (or you, if you are doing the mapping) should write directly to `.planning/codebase/*.md`.
- When the workflow says to spawn 4 parallel gsd-codebase-mapper agents (tech, arch, quality, concerns), use Cursor's Task/agent mechanism with the agent in `.cursor/agents/gsd-codebase-mapper.md`. Pass the focus (tech/arch/quality/concerns) and any model profile from `.planning/config.json`. Agents write documents directly; you only collect confirmations and line counts.
- Git: do not commit without user confirmation. Honor `.planning/config.json` and `git check-ignore .planning`.

**When to use.**

- `gsd:map-codebase` or "map codebase" / "analyze codebase" for producing STACK, ARCHITECTURE, STRUCTURE, CONVENTIONS, TESTING, INTEGRATIONS, CONCERNS under `.planning/codebase/`.

Updating `.cursor/gsd/workflows/map-codebase.md` or `.cursor/gsd/templates/codebase/` will change how this skill behaves the next time the workflow is loaded.
