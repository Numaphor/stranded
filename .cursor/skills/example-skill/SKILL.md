---
name: example-skill
description: Demonstrates a simple, well-structured Cursor skill with a small workflow and examples. Use when you want a reference template for creating new project-specific skills.
---

# Example Skill

This is a minimal but complete example of a Cursor skill for this project.

## Instructions

When this skill is active, follow this workflow for any small, self-contained task:

1. **Clarify the goal**
   - Restate the user's request in 1–2 sentences.
   - Identify the main file(s) or subsystem(s) likely to be involved.

2. **Plan briefly**
   - Outline 2–5 bullet points describing the steps you will take.
   - Keep the plan high-level and focused on outcomes, not implementation details.

3. **Execute in small steps**
   - Make one focused change at a time.
   - After each meaningful change, re-evaluate whether the original goal is closer to being met.

4. **Check for issues**
   - Look for obvious compile or linter errors in the files you touched.
   - If you introduced an error and can see the fix, correct it before proceeding.

5. **Summarize succinctly**
   - Provide a short summary (2–4 sentences) of what changed and why.
   - Avoid pasting large code blocks unless explicitly requested.

## Examples

### Example: Small code tweak

1. Clarify: "User wants the player movement speed slightly increased."
2. Plan:
   - Find the movement-related constants.
   - Adjust the speed value by a small percentage.
   - Re-check for side effects in the same function or module.
3. Execute:
   - Edit the constant or configuration value.
4. Check:
   - Ensure there are no compile-time issues in the modified file.
5. Summarize:
   - Explain which value changed and the intended gameplay impact.

### Example: Simple bug fix

1. Clarify: "User reports that a particular enemy never spawns."
2. Plan:
   - Locate the enemy spawn logic.
   - Inspect conditions that allow or prevent spawning.
   - Adjust logic or data so the enemy can spawn as intended.
3. Execute:
   - Modify the condition, configuration, or spawn table entry.
4. Check:
   - Confirm there are no obvious logic errors in the edited code.
5. Summarize:
   - State the root cause you addressed and how the fix resolves it.

