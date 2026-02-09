# Subagents

Custom subagents in this folder are used by the main Agent (e.g. in Ultrawork mode). See [Cursor docs: Subagents](https://cursor.com/docs/context/subagents).

**How to invoke:** Per Cursor docs, Agent includes custom subagents in its **available tools** and delegates via **Task tool** calls. Invoke by name:
- **Slash:** `/butano` or `/butano <research task>`
- **Natural:** "Use the butano subagent to …", "Run the butano subagent to …", "Have the butano subagent …"

The orchestrator must not claim it lacks a subagent/Task tool — it has delegation; use it when the Ultrawork rule requires research. Fallback: user sends `Run /butano to research: [question]` and pastes the result back.

| Agent | Purpose |
|-------|--------|
| **butano** | Readonly research for this Butano GBA project; deep-dives, no file edits. |
| **codebase-mapper** | Writes `.planning/codebase/` docs; used by the map-codebase command. |
