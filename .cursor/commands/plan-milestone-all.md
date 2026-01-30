---
name: gsd:plan-milestone-all
description: Generate all PLAN.md files for all phases in one interactive session
argument-hint: "[--resume] [--skip-research]"
agent: (none - orchestrator command)
allowed-tools:
  - Read
  - Write
  - Bash
  - Glob
  - Grep
  - Task
---

<execution_context>
@.cursor/gsd/references/ui-brand.md
</execution_context>

<objective>
Generate all PLAN.md files for all phases before autonomous execution.

**Flow:** Enumerate phases -> Plan each sequentially -> Present summary -> Refine -> Done

**Orchestrator role:** Track planning progress, spawn gsd-planner for each phase, handle failures with retry, enable conversational refinement, commit after each phase.

**Why sequential:** Dependencies between phases (Phase N may reference Phase N-1 outputs). Also avoids context explosion from parallel planning.
</objective>

<context>
Flags:
- `--resume` - Resume interrupted session (read progress from STATE.md)
- `--skip-research` - Skip research for all phases

Uses functions from:
- bin/lib/planning.sh (init_planning_session, show_planning_progress, get_planning_summary)
- bin/lib/state.sh (update_planning_progress, set_planning_session)
- bin/lib/parse.sh (get_all_phases, get_unplanned_phases, get_phase_name)
</context>

<process>

## 0. Validate Mode

Check if running in interactive mode (where this command is disabled):

```bash
# Read current mode
source .planning/.ralph-config 2>/dev/null || true
CURRENT_MODE="${GSD_MODE:-}"

if [[ "$CURRENT_MODE" == "interactive" ]]; then
    echo ""
    echo "Error: /gsd:plan-milestone-all is only available in Lazy mode."
    echo "Current mode: Interactive"
    echo ""
    echo "In Interactive mode, use /gsd:plan-phase to plan one phase at a time."
    echo "Run /gsd:lazy-mode to switch to Lazy mode."
    # Exit - do not continue
fi

# Also allow if mode not set (user can plan all even without mode selection)
```

If mode is "interactive", output the error and STOP. Do not proceed with planning.
Note: If mode is unset, allow the command (user may want to plan all regardless).

## 1. Validate Environment

```bash
ls .planning/ROADMAP.md 2>/dev/null
```

**If not found:** Error - run `/gsd:new-project` first.

## 2. Parse Arguments

Extract from $ARGUMENTS:
- `--resume` flag for session continuation
- `--skip-research` flag to skip all research

## 3. Check/Initialize Session

```bash
# Check for existing planning progress
grep "<!-- PLANNING_PROGRESS_START -->" .planning/STATE.md 2>/dev/null
```

**If `--resume` AND progress exists:**
- Read current session status from STATE.md
- Display: "Resuming planning session: {session_id}"
- Get list of unplanned phases (phases without PLAN.md)

**Otherwise:**
- Initialize new planning session (creates session ID, adds progress section)
- Get all phases from ROADMAP.md
- Display: "Starting planning session: {session_id}"

## 4. Display Planning Overview

Show what will be planned:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 GSD > PLAN MILESTONE ALL
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Session: {session_id}
Phases to plan: {count}

| Phase | Name | Status |
|-------|------|--------|
| 8 | Upfront Planning | pending |
| 9 | Mode Selection | pending |
| 10 | Execution Commands | pending |

Ready to generate all plans? (y/n)
```

Wait for user confirmation (or auto-proceed if non-interactive).

## 5. Plan Each Phase

For each phase in unplanned_phases (sequential):

### 5a. Show Progress

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 GSD > PLANNING PHASE {N}/{total}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Phase: {phase_name}
```

### 5b. Check for Context/Research Files

```bash
PHASE_DIR=$(ls -d .planning/phases/${PHASE}-* 2>/dev/null | head -1)
ls "${PHASE_DIR}/${PHASE}-CONTEXT.md" 2>/dev/null
ls "${PHASE_DIR}/${PHASE}-RESEARCH.md" 2>/dev/null
```

**If CONTEXT.md exists:** Use decisions as planning constraints.

**If RESEARCH.md exists AND not `--skip-research`:** Use as domain context.

**If RESEARCH.md missing AND CONTEXT.md suggests research AND not `--skip-research`:**
- Offer: "Phase {N} may benefit from research. Run research? (y/n/skip-all)"
- If yes: Spawn gsd-phase-researcher (same as plan-phase.md step 5)
- If skip-all: Set skip-research flag for remaining phases

### 5c. Spawn Planner with Retry

Track: retry_count = 0, MAX_RETRIES = 3

**Loop while retry_count < MAX_RETRIES:**

Update progress: `update_planning_progress "$phase" "in_progress"`

Spawn gsd-planner with context:

```markdown
<planning_context>

**Phase:** {phase_number}
**Mode:** standard

**Project State:**
@.planning/STATE.md

**Roadmap:**
@.planning/ROADMAP.md

**Requirements (if exists):**
@.planning/REQUIREMENTS.md

**Phase Context (if exists):**
@{phase_dir}/{phase}-CONTEXT.md

**Research (if exists):**
@{phase_dir}/{phase}-RESEARCH.md

**Prior Phase Summaries (only if needed for dependencies):**
{For phases that this phase depends on, include their SUMMARY.md}

</planning_context>

<downstream_consumer>
Output consumed by /gsd:execute-phase and ralph.sh
Plans must be executable prompts with frontmatter and tasks.
</downstream_consumer>
```

```
Task(
  prompt=planning_prompt,
  subagent_type="gsd-planner",
  description="Plan Phase {phase}"
)
```

**Handle planner return:**

- **PLANNING COMPLETE:**
  - Update progress: `update_planning_progress "$phase" "complete" "$plan_count"`
  - Proceed to step 5d

- **PLANNING INCONCLUSIVE or error:**
  - retry_count++
  - Display: "Retry {retry_count}/{MAX_RETRIES}..."
  - Continue loop

**If all retries exhausted:**
- Display: "Phase {N} planning failed after {MAX_RETRIES} attempts"
- Update progress: `update_planning_progress "$phase" "failed"`
- Offer: (R)etry with guidance, (S)kip phase, (A)bort session
- Wait for user response

### 5d. Commit Phase Plans

After successful planning:

```bash
# Stage and commit plans for this phase
git add .planning/phases/${PHASE}-*/*-PLAN.md
git commit -m "docs(${PHASE}): create phase plans"
```

This ensures progress survives crashes. Each phase is a checkpoint.

### 5e. Continue to Next Phase

- Move to next phase in unplanned_phases
- Repeat from step 5a

## 6. Mark Session Complete

After all phases planned:

```bash
set_planning_session "$session_id" "completed"
```

## 7. Present Summary for Review

Display all generated plans:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 GSD > ALL PLANS GENERATED
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Session: {session_id}
Total plans: {count}

| Phase | Name | Plans | Status |
|-------|------|-------|--------|
| 1 | Safety Foundation | 2 | complete |
| 2 | State Extensions | 2 | complete |
...
| 10 | Execution Commands | 4 | complete |

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Review commands:
  cat .planning/phases/08-*/*-PLAN.md    # View Phase 8 plans
  cat .planning/phases/*/*-PLAN.md       # View all plans

Ready to proceed with execution? Type "proceed" or describe changes needed.
```

## 8. Refinement Loop

**While user has not typed "proceed":**

Parse user input for refinement requests:

**Pattern: "Plan {XX-NN} needs {change}"**
- Extract plan_id from input
- Check for dependents: `get_dependent_plans "$plan_id"`
- If has dependents, warn: "Plans {list} depend on {plan_id}. Continue? (y/n)"
- Spawn gsd-planner in revision mode:

```markdown
<revision_context>

**Phase:** {extracted_phase}
**Mode:** revision
**Plan to revise:** {plan_id}

**User feedback:**
{user_message}

**Existing plan:**
@.planning/phases/{phase_dir}/{plan_id}-PLAN.md

**Dependent plans (if any):**
{list_of_dependent_plans}

</revision_context>

<instructions>
Read the existing PLAN.md. Make targeted updates based on user feedback.
Do NOT replan from scratch unless fundamental changes requested.
Return what changed.
</instructions>
```

```
Task(
  prompt=revision_prompt,
  subagent_type="gsd-planner",
  description="Revise plan {plan_id}"
)
```

After revision:
- Display: "Plan {plan_id} updated. Changes: {summary}"
- Commit revision: `git add ... && git commit -m "fix(XX): revise plan based on feedback"`
- Re-display summary (step 7)

**Pattern: "Phase {N} {needs redesign/is wrong/etc}"**
- Suggest: "For phase redesign, consider running `/gsd:plan-phase {N}` directly."
- Or offer to delete phase plans and regenerate

**Pattern: "proceed" or "done" or "ready"**
- Exit refinement loop
- Continue to step 9

## 9. Present Completion

Display final status:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 GSD > PLANNING COMPLETE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

All {total_plans} plans ready for execution.

Session: {session_id}

---

## Next Steps

**Configure autonomous execution:**
/gsd:ralph                    # Set iteration limits, timeouts

**Start autonomous execution:**
/gsd:run-milestone            # Fire and forget

/clear first -> fresh context window

---

**Also available:**
- /gsd:plan-phase {N}         - Replan specific phase
- /gsd:execute-phase {N}      - Execute single phase interactively
- cat .planning/phases/*/*-PLAN.md  - Review all plans

---
```

</process>

<success_criteria>
- [ ] .planning/ROADMAP.md validated
- [ ] Planning session initialized (new or resumed)
- [ ] All phases enumerated from roadmap
- [ ] Each phase planned with gsd-planner (with retry on failure)
- [ ] Git commit after each successful phase
- [ ] Summary presented for user review
- [ ] Refinement requests handled (plan revision or phase replan)
- [ ] User typed "proceed" to finalize
- [ ] User knows next steps (ralph configure, run-milestone)
</success_criteria>
