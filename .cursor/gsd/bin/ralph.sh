#!/bin/bash
# GSD Ralph - Outer Loop Entry Point
# Part of Phase 3: Outer Loop Core
#
# Main script that spawns fresh Claude Code instances to execute plans sequentially.
# Reads STATE.md to determine next task, invokes Claude with full GSD context,
# parses results, updates state, and loops until completion or budget cap reached.
#
# Usage:
#   ./bin/ralph.sh                    # Start from current STATE.md position
#   ./bin/ralph.sh --start-from 03-02 # Override starting point

set -euo pipefail

# =============================================================================
# Script Setup
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source all lib files
source "${SCRIPT_DIR}/lib/budget.sh"
source "${SCRIPT_DIR}/lib/state.sh"
source "${SCRIPT_DIR}/lib/display.sh"
source "${SCRIPT_DIR}/lib/failfast.sh"
source "${SCRIPT_DIR}/lib/parse.sh"
source "${SCRIPT_DIR}/lib/invoke.sh"
source "${SCRIPT_DIR}/lib/checkpoint.sh"
source "${SCRIPT_DIR}/lib/exit.sh"
source "${SCRIPT_DIR}/lib/recovery.sh"
source "${SCRIPT_DIR}/lib/learnings.sh"
source "${SCRIPT_DIR}/lib/path-resolve.sh"

# Log file configuration
LOG_FILE="${LOG_FILE:-.planning/ralph.log}"
PAUSE_FILE="${PAUSE_FILE:-.planning/.pause}"

# =============================================================================
# Logging Functions
# =============================================================================

# log_iteration - Append iteration entry to log file
# Args: iteration, status, task, summary, duration
log_iteration() {
    local iteration_num="$1"
    local status="$2"
    local task="$3"
    local summary="$4"
    local duration="$5"

    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    # Ensure log file directory exists
    local log_dir
    log_dir=$(dirname "$LOG_FILE")
    if [[ ! -d "$log_dir" ]]; then
        mkdir -p "$log_dir"
    fi

    # Append to log file
    {
        echo "---"
        echo "Iteration: $iteration_num"
        echo "Timestamp: $timestamp"
        echo "Task: $task"
        echo "Status: $status"
        echo "Duration: ${duration}s"
        echo "Summary: $summary"
    } >> "$LOG_FILE"
}

# log_task_start - Log when a task begins execution
# Args: iteration, task
log_task_start() {
    local iteration_num="$1"
    local task="$2"

    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    # Ensure log file directory exists
    local log_dir
    log_dir=$(dirname "$LOG_FILE")
    if [[ ! -d "$log_dir" ]]; then
        mkdir -p "$log_dir"
    fi

    # Append start entry to log file
    {
        echo "---"
        echo "Iteration: $iteration_num"
        echo "Timestamp: $timestamp"
        echo "Task: $task"
        echo "Status: RUNNING"
        echo "Duration: -"
        echo "Summary: Executing plan..."
    } >> "$LOG_FILE"
}

# handle_iteration_success - Process successful iteration
# Args: iteration, task, summary, duration
# Updates STATE.md and advances to next plan
handle_iteration_success() {
    local iteration_num="$1"
    local task="$2"
    local summary="$3"
    local duration="$4"

    # Log the iteration
    log_iteration "$iteration_num" "SUCCESS" "$task" "$summary" "$duration"

    # Add entry to STATE.md history
    add_iteration_entry "$iteration_num" "SUCCESS" "$task: $summary"

    # Determine next plan
    local next_plan
    next_plan=$(get_next_plan_after "$task")

    # Check for phase boundary - clear failure learnings when phase completes
    if type clear_phase_failures &>/dev/null; then
        local current_phase="${task%%-*}"
        current_phase=$((10#$current_phase))  # Remove leading zero

        if [[ "$next_plan" == "COMPLETE" ]]; then
            # All phases done - clear current phase failures
            clear_phase_failures "$current_phase"
        else
            local next_phase="${next_plan%%-*}"
            next_phase=$((10#$next_phase))

            if [[ "$next_phase" -gt "$current_phase" ]]; then
                # Phase boundary crossed - clear completed phase failures
                clear_phase_failures "$current_phase"
            fi
        fi
    fi

    if [[ "$next_plan" == "COMPLETE" ]]; then
        # All plans done - update next action to reflect completion
        update_next_action "COMPLETE" "COMPLETE" "All plans executed"
    else
        # Advance to next plan
        local phase_num="${next_plan%%-*}"
        local plan_name
        plan_name=$(get_plan_name "$next_plan")
        update_next_action "$phase_num" "$next_plan" "$plan_name"
    fi

    # Update progress bar
    local completed total
    completed=$(get_plans_completed)
    total=$(get_total_plans)
    update_progress "$completed" "$total"
}

# handle_iteration_failure_state - Process failed iteration (state only)
# Args: iteration, task, error, duration, output_file (optional)
# Updates STATE.md but does NOT advance next_action (retry same task)
handle_iteration_failure_state() {
    local iteration_num="$1"
    local task="$2"
    local error="$3"
    local duration="$4"
    local output_file="${5:-}"

    # Log the iteration
    log_iteration "$iteration_num" "FAILURE" "$task" "$error" "$duration"

    # Add entry to STATE.md history
    add_iteration_entry "$iteration_num" "FAILURE" "$task: $error"

    # Extract and store failure learning for retry context
    if [[ -n "$output_file" ]]; then
        extract_and_store_failure "$task" "$output_file" "$error"
    fi

    # Do NOT update next_action - stay on same task for retry
}

# extract_and_store_failure - Extract failure details and store as learning
# Args: task_id, output_file, error_msg
# Called when a task fails to capture context for retries
extract_and_store_failure() {
    local task_id="$1"
    local output_file="$2"
    local error_msg="$3"

    # Skip if learnings functions not available
    if ! type extract_failure_reason &>/dev/null; then
        return 0
    fi

    # Extract detailed failure reason from Claude's output
    local failure_reason=""
    if [[ -n "$output_file" && -f "$output_file" ]]; then
        failure_reason=$(extract_failure_reason "$output_file")
    fi

    # Build "attempted" from task context
    local attempted="Executed task ${task_id}"

    # Extract file paths mentioned in output (best effort)
    local files=""
    if [[ -n "$output_file" && -f "$output_file" ]]; then
        # Look for file paths in the output (common patterns)
        files=$(grep -oE '[a-zA-Z0-9_/-]+\.(sh|ts|js|md|json)' "$output_file" 2>/dev/null | sort -u | head -5 | tr '\n' ', ' | sed 's/,$//')
    fi
    files="${files:-unknown}"

    # Use failure_reason as context if different from error_msg
    local context="$error_msg"
    if [[ -n "$failure_reason" && "$failure_reason" != "$error_msg" ]]; then
        context="$failure_reason"
    fi

    # Store the failure learning
    append_failure_learning "$task_id" "$error_msg" "$attempted" "$files" "$context"
}

# check_pause - Wait while pause file exists
# Returns when pause file is removed or loop is interrupted
check_pause() {
    if [[ -f "$PAUSE_FILE" ]]; then
        echo -e "${YELLOW}=== PAUSED ===${RESET}"
        echo -e "Pause file detected: $PAUSE_FILE"
        echo -e "Waiting for resume (delete $PAUSE_FILE or press 'r' in progress watcher)..."
        while [[ -f "$PAUSE_FILE" ]]; do
            sleep 2
        done
        echo -e "${GREEN}=== RESUMED ===${RESET}"
    fi
}

# =============================================================================
# Argument Parsing
# =============================================================================

START_FROM=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --start-from)
            if [[ -n "${2:-}" ]]; then
                START_FROM="$2"
                shift 2
            else
                echo -e "${RED}Error: --start-from requires a plan ID (e.g., 03-02)${RESET}" >&2
                exit 1
            fi
            ;;
        --help|-h)
            echo "Usage: ralph.sh [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --start-from NN-MM  Override starting plan (e.g., 03-02)"
            echo "  --help, -h          Show this help message"
            echo ""
            echo "Ralph spawns fresh Claude instances to execute plans sequentially."
            echo "Reads STATE.md for next task, invokes Claude, updates state, and loops."
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${RESET}" >&2
            echo "Use --help for usage information." >&2
            exit 1
            ;;
    esac
done

# Validate --start-from format if provided
if [[ -n "$START_FROM" ]]; then
    if ! [[ "$START_FROM" =~ ^[0-9]{2}-[0-9]{2}$ ]]; then
        echo -e "${RED}Error: --start-from must be in NN-MM format (e.g., 03-02)${RESET}" >&2
        exit 1
    fi
fi

# =============================================================================
# Startup Sequence
# =============================================================================

# Load configuration (sets MAX_ITERATIONS, TIMEOUT_HOURS)
load_config

# Record start time
START_TIME=$(date +%s)

# Show startup summary
show_startup_summary() {
    echo -e "${BOLD}${CYAN}=== Ralph Outer Loop ===${RESET}"
    echo ""
    echo -e "Config: ${GREEN}$MAX_ITERATIONS iterations${RESET}, ${GREEN}${TIMEOUT_HOURS}h timeout${RESET}"

    # Parse current position from STATE.md
    local phase plan status
    phase=$(grep "^Phase:" "$STATE_FILE" 2>/dev/null | head -1 | sed 's/Phase: //')
    plan=$(grep "^Plan:" "$STATE_FILE" 2>/dev/null | head -1 | sed 's/Plan: //')
    status=$(grep "^Status:" "$STATE_FILE" 2>/dev/null | head -1 | sed 's/Status: //')

    echo -e "Position: Phase ${phase:-unknown}, Plan ${plan:-unknown}"
    echo -e "Status: ${status:-unknown}"

    # Determine starting task
    local next_task
    if [[ -n "$START_FROM" ]]; then
        next_task="$START_FROM"
        echo -e "Starting from: ${YELLOW}${next_task}${RESET} (override)"
    else
        next_task=$(parse_next_task)
        echo -e "Next task: ${YELLOW}${next_task}${RESET}"
    fi

    echo ""
}

show_startup_summary

# Validate git state before entering main loop
if ! validate_git_state; then
    echo -e "${RED}Cannot proceed without clean git state${RESET}"
    exit 1
fi

# Validate STATE.md against git checkpoint history
if ! validate_state_against_history; then
    echo -e "${YELLOW}Please resolve the conflict and restart${RESET}"
    exit 1
fi

# Mark checkpoint for potential rollback
mark_checkpoint

# Set up SIGINT (Ctrl+C) handler for graceful interrupt
trap 'handle_interrupt' INT

# =============================================================================
# Main Loop
# =============================================================================

iteration=0
next_task=""
iteration_start=0
iteration_duration=0
output_file=""
last_output_file=""
exit_code=0
summary=""
error_msg=""
choice=0
next_plan=""
skip_phase_num=""
skip_plan_name=""
while true; do
    iteration=$((iteration + 1))

    # Check for pause signal at start of iteration
    check_pause

    # Check budget limits
    if ! check_limits "$iteration"; then
        loop_duration=$(($(date +%s) - START_TIME))
        exit_with_status "ABORTED" "Iteration cap reached" "$next_task" "$iteration" "$loop_duration"
        exit $EXIT_ABORTED
    fi

    # Get next task
    if [[ -n "$START_FROM" && $iteration -eq 1 ]]; then
        # Use override for first iteration only
        next_task="$START_FROM"
    else
        next_task=$(parse_next_task)
    fi

    if [[ "$next_task" == "COMPLETE" || -z "$next_task" ]]; then
        # All plans done - verify completion gate before exiting
        # Use last_output_file if available from previous iteration
        if check_completion "${last_output_file:-}"; then
            show_status "All tasks complete!" "success"
            loop_duration=$(($(date +%s) - START_TIME))
            exit_with_status "COMPLETED" "All tests pass and all plans done" "${next_task:-COMPLETE}" "$iteration" "$loop_duration"
            exit $EXIT_COMPLETED
        else
            # Plans done but completion gate failed
            show_status "All plans marked done but completion check failed - investigate" "warning"
            loop_duration=$(($(date +%s) - START_TIME))
            exit_with_status "COMPLETED" "All plans done (completion check inconclusive)" "${next_task:-COMPLETE}" "$iteration" "$loop_duration"
            exit $EXIT_COMPLETED
        fi
    fi

    # Record iteration start time
    iteration_start=$(date +%s)

    # Log task start for progress watcher visibility
    log_task_start "$iteration" "$next_task"

    # Show running indicator with spinner
    start_spinner "[$iteration/$MAX_ITERATIONS] Running ${next_task}..."

    # Invoke Claude
    output_file=$(invoke_claude "$next_task")
    exit_code=$?

    stop_spinner

    # Check for duration alert (30 min)
    check_iteration_duration "$iteration_start"

    # Calculate duration
    iteration_end=$(date +%s)
    iteration_duration=$((iteration_end - iteration_start))

    # Handle result based on exit code
    if [[ $exit_code -eq 0 ]]; then
        # Success - parse output for summary
        summary=$(parse_claude_output "$output_file")
        show_status "[$iteration] Completed: $next_task (${iteration_duration}s)" "success"

        # Reset stuck detection on success
        reset_failure_tracking

        # Reset circuit breaker on success
        reset_circuit_breaker

        # Update state
        handle_iteration_success "$iteration" "$next_task" "$summary" "$iteration_duration"

        # Extract learnings from completed task's SUMMARY.md
        if type extract_learnings_from_summary &>/dev/null; then
            # Find SUMMARY file (same location as PLAN file but different suffix)
            summary_file=$(find .planning/phases -name "${next_task}-SUMMARY.md" 2>/dev/null | head -1)
            if [[ -n "$summary_file" && -f "$summary_file" ]]; then
                extract_learnings_from_summary "$summary_file" "$next_task"
                # Prune if getting too large
                prune_agents_if_needed
            fi
        fi

        # Create checkpoint commit (protected from interrupt)
        enter_critical_section
        if ! create_checkpoint_commit "$next_task" "$summary"; then
            exit_critical_section
            echo -e "${RED}FATAL: Cannot continue without successful checkpoint${RESET}"
            exit 1
        fi

        # Mark checkpoint after successful iteration
        mark_checkpoint
        exit_critical_section

        # Save output file reference for completion check before cleanup
        # Clean up previous last_output_file and save current one
        rm -f "$last_output_file" 2>/dev/null
        last_output_file="$output_file"

    else
        # Check if this is a Claude crash (abnormal exit)
        # Exit codes > 1 typically indicate crashes/errors vs normal failure
        if [[ $exit_code -gt 1 ]]; then
            handle_claude_crash "$exit_code" "$next_task"
            rm -f "$output_file" 2>/dev/null
            exit 1
        fi

        # Normal failure - parse error and offer user choice
        error_msg=$(parse_claude_output "$output_file")

        # Record failure in state
        handle_iteration_failure_state "$iteration" "$next_task" "$error_msg" "$iteration_duration" "$output_file"

        # Clean up output file after extracting failure context
        rm -f "$output_file" 2>/dev/null

        # Check if stuck on same task
        if check_stuck "$next_task"; then
            loop_duration=$(($(date +%s) - START_TIME))
            exit_with_status "STUCK" "Same task failed $STUCK_THRESHOLD times" "$next_task" "$iteration" "$loop_duration"
            exit $EXIT_STUCK
        fi

        # Check circuit breaker (cross-task failures)
        if check_circuit_breaker; then
            # Circuit breaker tripped - handle pause
            handle_circuit_breaker_pause "$next_task"
            cb_choice=$?

            case $cb_choice in
                0)  # Resume - counter already reset in handle_circuit_breaker_pause
                    show_status "Circuit breaker reset - resuming..." "warning"
                    # Fall through to normal failure handling below
                    ;;
                1)  # Skip - advance to next task
                    add_iteration_entry "$iteration" "SKIPPED" "$next_task: Skipped after circuit breaker"
                    next_plan=$(get_next_plan_after "$next_task")
                    if [[ "$next_plan" == "COMPLETE" ]]; then
                        update_next_action "COMPLETE" "COMPLETE" "All plans executed (last task skipped)"
                        show_status "All tasks complete (last was skipped)" "warning"
                        loop_duration=$(($(date +%s) - START_TIME))
                        exit_with_status "COMPLETED" "All plans done (last skipped)" "$next_task" "$iteration" "$loop_duration"
                        exit $EXIT_COMPLETED
                    else
                        skip_phase_num="${next_plan%%-*}"
                        skip_plan_name=$(get_plan_name "$next_plan")
                        update_next_action "$skip_phase_num" "$next_plan" "$skip_plan_name (skipped ${next_task})"
                        show_status "Skipped $next_task, advancing to $next_plan" "warning"
                        continue  # Next iteration
                    fi
                    ;;
                2)  # Abort
                    add_iteration_entry "$iteration" "ABORTED" "$next_task: User aborted at circuit breaker"
                    loop_duration=$(($(date +%s) - START_TIME))
                    exit_with_status "ABORTED" "User aborted at circuit breaker" "$next_task" "$iteration" "$loop_duration"
                    exit $EXIT_ABORTED
                    ;;
            esac
        fi

        # Present user with Retry/Skip/Abort choice
        handle_iteration_failure "$next_task" "$error_msg"
        choice=$?

        case $choice in
            0)  # Retry
                show_status "Retrying $next_task..." "warning"
                add_iteration_entry "$iteration" "RETRY" "$next_task: Retrying..."
                continue  # Loop again on same task
                ;;
            1)  # Skip - advance to next plan
                add_iteration_entry "$iteration" "SKIPPED" "$next_task: Skipped by user"
                # Use get_next_plan_after to determine and set next task
                next_plan=$(get_next_plan_after "$next_task")
                if [[ "$next_plan" == "COMPLETE" ]]; then
                    update_next_action "COMPLETE" "COMPLETE" "All plans executed (last task skipped)"
                    show_status "All tasks complete (last was skipped)" "warning"
                    break
                else
                    skip_phase_num="${next_plan%%-*}"
                    skip_plan_name=$(get_plan_name "$next_plan")
                    update_next_action "$skip_phase_num" "$next_plan" "$skip_plan_name (skipped ${next_task})"
                    show_status "Skipped $next_task, advancing to $next_plan" "warning"
                fi
                ;;
            2)  # Abort
                add_iteration_entry "$iteration" "ABORTED" "$next_task: User aborted"
                loop_duration=$(($(date +%s) - START_TIME))
                exit_with_status "ABORTED" "User aborted" "$next_task" "$iteration" "$loop_duration"
                exit $EXIT_ABORTED
                ;;
        esac
    fi

    # Check for pause signal at end of iteration (safe point)
    check_pause

    # Check for deferred interrupt at end of iteration (safe point)
    if check_interrupted; then
        loop_duration=$(($(date +%s) - START_TIME))
        exit_with_status "INTERRUPTED" "User interrupt" "$next_task" "$iteration" "$loop_duration"
        exit $EXIT_INTERRUPTED
    fi
done

# =============================================================================
# Post-Loop Cleanup
# =============================================================================

# Clean up any remaining temp file
rm -f "$last_output_file" 2>/dev/null

# Calculate duration
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

# Exit with COMPLETED status (fallback - normally exits in completion check above)
if check_completion "${last_output_file:-}"; then
    exit_with_status "COMPLETED" "All tests pass and all plans done" "$next_task" "$iteration" "$DURATION"
else
    exit_with_status "COMPLETED" "All plans done (completion check inconclusive)" "$next_task" "$iteration" "$DURATION"
fi
exit $EXIT_COMPLETED
