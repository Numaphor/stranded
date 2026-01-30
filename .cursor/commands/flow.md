---
name: gsd:flow
description: One command for the full GSD flow — asks if codebase is new, then runs every step in order
allowed-tools:
  - Read
  - Write
  - Bash
  - Glob
  - Grep
  - Task
  - AskUserQuestion
  - TodoWrite
---

<objective>
Single entry point for the entire GSD workflow. Asks whether the codebase is new, then runs the right sequence of steps one by one so you never have to remember which command comes next.

**New codebase (autopilot setup):** Map (if needed) → Enable lazy mode → New project → Plan milestone all → Autopilot (walk away). Stay for steps 1–3 (~15–30 min questions + research, then review/approve plans); step 4 launches execution in a separate terminal so you can leave.

**Existing codebase:** Resume work → then run the next logical command (discuss/plan/execute/verify/complete) and keep going until you stop or the milestone is done.
</objective>

<flow_mode_critical>
**You are in flow mode.** The user ran /gsd:flow so they do not want to choose commands.

- **Never** present "What would you like to do?" or "Which command do you want to run?" or a numbered list of commands and then wait for the user to pick one.
- **Never** stop after a step to offer "run /gsd:X or /gsd:Y" — the next step is fixed by this flow.
- After each step completes, **proceed immediately** to the next step in this document. Do not ask. Just run the next step.
- The only time you wait for user input is: (1) the single "Is this a new codebase?" at the start, (2) questions that are part of a step (e.g. new-project questioning, autopilot settings), (3) the user says "stop" or "pause".
</flow_mode_critical>

<execution_context>
Load these when needed (do not load all at once):

- New path: @.cursor/commands/lazy-mode.md, @.cursor/commands/new-project.md, @.cursor/commands/plan-milestone-all.md, @.cursor/commands/autopilot.md
- Map codebase: @.cursor/gsd/workflows/map-codebase.md
- Existing path: @.cursor/gsd/workflows/resume-project.md, @.cursor/commands/discuss-phase.md, @.cursor/commands/plan-phase.md, @.cursor/commands/execute-phase.md
- Milestone end: @.cursor/commands/audit-milestone.md, @.cursor/commands/complete-milestone.md
</execution_context>

<process>

## Step 0: Ask if codebase is new

**First and only question before branching:**

Use AskUserQuestion:

- header: "GSD Flow"
- question: "Is this a new codebase (or new to GSD)?"
- options:
  - "Yes — new codebase" — Run the **autopilot** flow (lazy mode → new-project → plan-milestone-all → autopilot)
  - "No — existing project" — Run the **resume** flow (load state → run next step → keep going)

Do not ask anything else until this is answered.

---

## Branch A: New codebase

### A1. Brownfield check (before new-project)

Run these checks (same as new-project Phase 1):

```bash
[ -f .planning/PROJECT.md ] && echo "EXISTS" || echo "NEW"
ls .planning/codebase 2>/dev/null && echo "CODEBASE_MAPPED" || echo "NO_MAP"
# Detect existing code
CODE_FILES=$(find . -maxdepth 4 \( -name "*.ts" -o -name "*.js" -o -name "*.py" -o -name "*.go" -o -name "*.rs" -o -name "*.cpp" -o -name "*.h" \) 2>/dev/null | grep -v node_modules | grep -v .git | head -20)
HAS_PACKAGE=$([ -f package.json ] || [ -f requirements.txt ] || [ -f Cargo.toml ] || [ -f Makefile ] && echo "yes")
```

- **If .planning/PROJECT.md exists:** Project already initialized. Treat as **existing project** — go to Branch B (resume flow).
- **If PROJECT.md does not exist and (CODE_FILES non-empty or HAS_PACKAGE) and .planning/codebase/ does not exist:**  
  Ask: "I detected existing code. Map the codebase first?"  
  - "Map first" → Follow **map-codebase** workflow from `@.cursor/gsd/workflows/map-codebase.md` (full process). When done, continue to A2.  
  - "Skip mapping" → Continue to A2.
- **Otherwise:** Continue to A2.

### A2. Enable lazy mode

Follow **@.cursor/commands/lazy-mode.md** in full. Ensure mode is set to **Lazy** (if current mode is Interactive, the toggle will switch to Lazy; if unset, set to Lazy). Save to `.planning/.ralph-config`. **Do not offer "run /gsd:lazy-mode again" as a choice — proceed immediately to A3.**

### A3. New project (full flow)

Follow the full process in **@.cursor/commands/new-project.md** from start to finish (questioning → PROJECT.md → config → research decision → requirements → roadmap → STATE.md). Do not skip steps. **Stay for this** — ~15–30 min of questions and optional research. When new-project completion would normally say "Next: /gsd:discuss-phase 1" or list commands: **do not show that menu. Proceed immediately to A4 (plan-milestone-all).**

### A4. Plan milestone all

Follow **@.cursor/commands/plan-milestone-all.md** in full. Generate all phase plans for the milestone. **Stay for this** — review and approve plans. When plan-milestone-all completes (all phases have PLAN.md files), **do not offer "what's next?" or a command list — proceed immediately to A5 (autopilot).**

### A5. Autopilot (walk away)

Follow **@.cursor/commands/autopilot.md** in full:

1. Validate mode is Lazy (already set in A2).
2. Prompt for autopilot settings (max iterations, timeout, circuit breaker, stuck threshold) and save to `.planning/.ralph-config`.
3. Detect existing plans (use them or regenerate per autopilot logic).
4. Detect incomplete runs (offer resume or restart).
5. Launch ralph.sh in a new terminal window via terminal-launcher; do not wait for it to finish.

After launch, present the autopilot completion message: Ralph is running in another terminal; you can close this session; when it completes, run `/gsd:progress` then `/gsd:complete-milestone` when ready. **Flow complete for new codebase — walk away.**

---

## Branch B: Existing project

### B1. Resume and run next action (no menu)

Follow **@.cursor/gsd/workflows/resume-project.md** only for: detect project, load state, check incomplete work, present brief status, **determine the primary next action**.

**Do not present "What would you like to do?" or a numbered list of commands.** Do not wait for the user to choose a command. As soon as you have determined the primary next action (e.g. "Execute phase 2", "Plan phase 3", "Discuss phase 1"), **run that action immediately** by following the corresponding command in B2. Skip the "offer options" / "wait for user selection" part of resume-project — in flow mode you always run the primary action.

### B2. Run the chosen/primary command

- **Resume interrupted agent** — Use Task tool with resume parameter as in resume-project. When done, go back to B1 (resume again, get next action, run it).
- **Execute phase N** — Follow **@.cursor/commands/execute-phase.md** for phase N. When done, if it says "Next: discuss phase N+1" or "Next: plan phase N+1", run that next (B3/B4). If it says "Milestone complete", go to B5. If gaps_found, run plan-phase N --gaps then execute-phase N again, then B1.
- **Plan phase N** — Follow **@.cursor/commands/plan-phase.md** for phase N. When done, run execute-phase N (B2 "Execute phase N").
- **Discuss phase N** — Follow **@.cursor/commands/discuss-phase.md** for phase N. When done, run plan-phase N (above).
- **Complete incomplete plan / transition / etc.** — Follow the workflow referenced in resume-project for that action. When done, go back to B1.

### B3. After execute-phase (more phases remain)

Present the "Next up" from execute-phase (e.g. discuss phase N+1 or plan phase N+1). Then **run that command** (discuss-phase N+1 or plan-phase N+1) and continue as in B2 (plan → execute, or execute → verify, etc.) until milestone complete or user stops.

### B4. After execute-phase (milestone complete)

Follow **@.cursor/commands/audit-milestone.md**, then **@.cursor/commands/complete-milestone.md** with version. Present: "Flow complete. For the next milestone run /gsd:new-milestone or /gsd:flow again."

### B5. User stops

If at any "offer options" the user says they want to stop or do something else, end the flow and summarize what was done and what the next step would be if they run /gsd:flow again.

</process>

<rules>
- **One step at a time:** Execute each GSD step fully (by following its command/workflow) before starting the next. Do not summarize or skip steps.
- **No command menus:** When a sub-command would normally end with "What would you like to do?" or "Next: /gsd:X or /gsd:Y" or a list of commands — **skip that.** Proceed immediately to the next step in this flow. The user ran /gsd:flow so they do not want to pick commands.
- **No command names as user instructions in the middle of flow:** When you are running the flow, you perform the steps; only at the very end tell the user they can run /gsd:flow again for the next milestone.
- **State between steps:** After each step, re-read STATE.md (and ROADMAP.md if needed) so the next step has correct phase numbers and context.
- **Stopping:** If the user says "stop", "pause", "that's enough", or "something else", end the flow gracefully and state what would come next.
</rules>

<success_criteria>
- [ ] Asks "Is this a new codebase?" once at the start.
- [ ] New path (autopilot): map (if chosen) → lazy-mode → new-project → plan-milestone-all → autopilot (launch ralph, walk away).
- [ ] Existing path: resume-work → then runs the next logical command and continues until milestone complete or user stops.
- [ ] Each step is executed by following its official command/workflow, not summarized.
- [ ] User never has to remember "which command next" during the flow.
</success_criteria>
