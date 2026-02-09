# Subagents

Custom subagents in this folder are used by the main Agent (e.g. in Ultrawork mode). See [Cursor docs: Subagents](https://cursor.com/docs/context/subagents).

**How to invoke:** Cursor exposes each `.md` file here as a subagent **tool** to the Agent. The orchestrator calls the tool for the desired agent (e.g. **butano**) with a research prompt. If no subagent tool appears in your client, start a new Agent chat (Ctrl+I / Cmd+I), select or @-mention the agent by name, run the task, then paste the result back.

| Agent | Purpose |
|-------|--------|
| **butano** | Readonly research for this Butano GBA project; deep-dives, no file edits. |
| **codebase-mapper** | Writes `.planning/codebase/` docs; used by the map-codebase command. |
