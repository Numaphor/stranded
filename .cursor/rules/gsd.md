# GSD Rules

- **Subrepo is source of truth:** Workflows, templates, and references live in `.cursor/gsd/` (workflows/, templates/, references/). GSD skills in `.cursor/skills/` are thin interfaces that point at those paths—load and follow them; do not duplicate their content. Edit files under `.cursor/gsd/` to change behavior.
- Prefer using .planning docs (PROJECT.md, ROADMAP.md, REQUIREMENTS.md, MILESTONES.md, STATE.md, phases/, codebase/) instead of ad-hoc notes.
- When GSD commands are active, follow project skills: milestones-skill, phases-skill, codebase-mapping-skill, planning-files-skill (each tells you which subrepo paths to load).
- Never commit or tag in git without explicit confirmation from the user.
- Respect .planning/config.json and gitignore when deciding whether to commit planning docs.
- Treat execute-phase/execute-plan/verify-work/complete-milestone as rituals: use their success checklists to decide when you are done.

