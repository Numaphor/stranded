---
name: gsd:lazy-mode
description: Toggle between Interactive and Lazy execution modes
allowed-tools:
  - Read
  - Write
  - Bash
---

<objective>
Toggle GSD execution mode between Interactive and Lazy.

**Mode behaviors:**
- **Interactive mode**: Plan one phase at a time, execute with supervision, repeat
- **Lazy mode**: Plan ALL phases upfront, then run autonomously until complete

This command toggles between the two modes. If no mode is set, it enables Lazy mode.
</objective>


<process>

<step name="read_mode">
**Read current mode from config:**

```bash
# Check for existing config
if [ -f ".planning/.ralph-config" ]; then
    source .planning/.ralph-config 2>/dev/null
fi
CURRENT_MODE="${GSD_MODE:-}"
```

If CURRENT_MODE is empty, this is first time setting mode.
</step>

<step name="determine_new_mode">
**Determine the new mode:**

| Current Mode | New Mode |
|--------------|----------|
| (empty)      | lazy     |
| interactive  | lazy     |
| lazy         | interactive |

The toggle always goes: empty -> lazy -> interactive -> lazy -> ...
</step>

<step name="check_active_work">
**Check for active work (mid-milestone warning):**

If ROADMAP.md exists, check for incomplete phases:

```bash
if [ -f ".planning/ROADMAP.md" ]; then
    incomplete=$(grep -c "^- \[ \]" .planning/ROADMAP.md 2>/dev/null || echo "0")
    if [ "$incomplete" -gt 0 ]; then
        # Show warning but allow
        echo ""
        echo "Warning: You have $incomplete incomplete phases."
        echo "Switching modes mid-milestone may cause workflow issues."
        echo ""
    fi
fi
```

This is a warning only - do not block mode switching.
</step>

<step name="show_explainer">
**Display mode explainer based on NEW mode:**

**If switching TO Lazy mode:**

```
## Lazy Mode Enabled

In Lazy mode, you plan everything upfront, then walk away.

**What changes:**
- Use `/gsd:plan-milestone-all` to generate ALL plans at once
- Use `/gsd:run-milestone` for autonomous execution
- Individual phase commands (plan-phase, execute-phase) are disabled

**The workflow becomes:**
new-project -> plan-milestone-all -> configure ralph -> run-milestone -> done

Fire and forget. Wake up to completed work.
```

**If switching TO Interactive mode:**

```
## Interactive Mode Enabled

In Interactive mode, you work phase-by-phase with Claude.

**What changes:**
- Use `/gsd:plan-phase` to plan one phase at a time
- Use `/gsd:execute-phase` to execute with supervision
- Lazy mode commands (plan-milestone-all, run-milestone) are disabled

**The workflow becomes:**
new-project -> discuss-phase -> plan-phase -> execute-phase -> repeat

Collaborate with Claude on each phase.
```
</step>

<step name="save_mode">
**Save the new mode to .ralph-config:**

Preserve existing budget values (MAX_ITERATIONS, TIMEOUT_HOURS) while updating GSD_MODE.

```bash
# Load existing values
if [ -f ".planning/.ralph-config" ]; then
    source .planning/.ralph-config 2>/dev/null
fi
existing_iterations="${MAX_ITERATIONS:-50}"
existing_timeout="${TIMEOUT_HOURS:-8}"

# Write all config values
mkdir -p .planning
cat > .planning/.ralph-config << EOF
# GSD Ralph configuration
# Last updated: $(date)
MAX_ITERATIONS=$existing_iterations
TIMEOUT_HOURS=$existing_timeout
GSD_MODE=$NEW_MODE
EOF
```
</step>

<step name="confirm">
**Confirm the mode change:**

```
Mode set to: [Lazy | Interactive]

Run /gsd:lazy-mode again to toggle back.
```
</step>

</process>


<success_criteria>
- [ ] Current mode read from .planning/.ralph-config (or detected as empty)
- [ ] New mode determined correctly (toggle behavior)
- [ ] Mid-milestone warning shown if incomplete phases exist
- [ ] Mode explainer displayed for the NEW mode
- [ ] Mode saved to .ralph-config (preserving budget values)
- [ ] Confirmation message shown
</success_criteria>
