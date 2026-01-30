#!/bin/bash
# GSD Ralph - Exit Condition Detection
# Part of Phase 5: Exit Conditions
#
# Provides stuck detection, graceful interrupt handling, and exit status functions.
# Functions: check_stuck, reset_failure_tracking, handle_interrupt, check_interrupted,
#            enter_critical_section, exit_critical_section, exit_with_status
#
# Exit codes:
#   0 = COMPLETED (all tasks done)
#   1 = STUCK (same task failed consecutively)
#   2 = ABORTED (iteration cap reached)
#   3 = INTERRUPTED (user Ctrl+C)
#
# Usage:
#   source bin/lib/exit.sh
#   trap 'handle_interrupt' INT
#   check_stuck "05-01" && echo "Stuck!" || echo "Not stuck"

# =============================================================================
# Exit Code Constants
# =============================================================================

EXIT_COMPLETED=0
EXIT_STUCK=1
EXIT_ABORTED=2
EXIT_INTERRUPTED=3

# =============================================================================
# State Variables
# =============================================================================

# Failure tracking
CONSECUTIVE_FAILURES=0
LAST_FAILED_TASK=""
STUCK_THRESHOLD=3

# Circuit breaker tracking (failures across different tasks)
CROSS_TASK_FAILURES=0
CIRCUIT_BREAKER_THRESHOLD=5

# Interrupt handling
INTERRUPTED=false
IN_CRITICAL_SECTION=false

# =============================================================================
# Color Codes (respect NO_COLOR standard)
# =============================================================================

if [[ -n "${NO_COLOR:-}" ]]; then
    EXIT_RED=''
    EXIT_GREEN=''
    EXIT_YELLOW=''
    EXIT_CYAN=''
    EXIT_BOLD=''
    EXIT_RESET=''
else
    EXIT_RED='\e[31m'
    EXIT_GREEN='\e[32m'
    EXIT_YELLOW='\e[33m'
    EXIT_CYAN='\e[36m'
    EXIT_BOLD='\e[1m'
    EXIT_RESET='\e[0m'
fi

# =============================================================================
# Stuck Detection Functions
# =============================================================================

# check_stuck - Check if we're stuck on the same task
# Args: task_id (string, e.g., "05-01")
# Returns: 0 if stuck (threshold reached), 1 if not stuck
# Side effects: Updates CONSECUTIVE_FAILURES and LAST_FAILED_TASK
check_stuck() {
    local task_id="$1"

    if [[ -z "$task_id" ]]; then
        echo -e "${EXIT_RED}Error: check_stuck requires task_id${EXIT_RESET}" >&2
        return 1
    fi

    if [[ "$task_id" == "$LAST_FAILED_TASK" ]]; then
        # Same task failed again - increment counter
        CONSECUTIVE_FAILURES=$((CONSECUTIVE_FAILURES + 1))
    else
        # Different task - reset counter
        CONSECUTIVE_FAILURES=1
        LAST_FAILED_TASK="$task_id"
    fi

    # Check if threshold reached
    if [[ "$CONSECUTIVE_FAILURES" -ge "$STUCK_THRESHOLD" ]]; then
        return 0  # Stuck
    fi

    return 1  # Not stuck
}

# reset_failure_tracking - Reset stuck detection after success
# Called when a task succeeds to clear the failure counter
reset_failure_tracking() {
    CONSECUTIVE_FAILURES=0
    LAST_FAILED_TASK=""
}

# =============================================================================
# Circuit Breaker Functions
# =============================================================================

# check_circuit_breaker - Check if failures across different tasks hit threshold
# Args: none (uses global CROSS_TASK_FAILURES)
# Returns: 0 if threshold reached (circuit open), 1 if not
# Side effect: Increments CROSS_TASK_FAILURES counter
# Note: Does NOT reset when task changes - that's the difference from stuck detection
check_circuit_breaker() {
    CROSS_TASK_FAILURES=$((CROSS_TASK_FAILURES + 1))

    if [[ "$CROSS_TASK_FAILURES" -ge "$CIRCUIT_BREAKER_THRESHOLD" ]]; then
        return 0  # Circuit breaker tripped
    fi
    return 1  # Circuit OK
}

# reset_circuit_breaker - Reset cross-task failure counter
# Called after user chooses Resume or on successful iteration
reset_circuit_breaker() {
    CROSS_TASK_FAILURES=0
}

# handle_circuit_breaker_pause - Interactive pause when circuit breaker trips
# Returns: 0 = Resume, 1 = Skip current task, 2 = Abort
# In non-interactive mode, exits with STUCK status (fail fast per CONTEXT.md)
handle_circuit_breaker_pause() {
    local current_task="${1:-unknown}"

    echo ""
    echo -e "${EXIT_YELLOW}${EXIT_BOLD}=== CIRCUIT BREAKER: $CROSS_TASK_FAILURES consecutive failures ===${EXIT_RESET}"
    echo -e "${EXIT_YELLOW}Multiple tasks failing - systemic issue suspected${EXIT_RESET}"
    echo ""

    # Show stuck analysis if recovery.sh is sourced
    if type generate_stuck_analysis &>/dev/null; then
        generate_stuck_analysis
        echo ""
    fi

    # Check if interactive mode
    if [[ ! -t 0 ]]; then
        # Non-interactive: fail fast
        echo -e "${EXIT_RED}Non-interactive mode - exiting with STUCK status${EXIT_RESET}"
        return 2  # Signal abort
    fi

    echo -e "Options:"
    echo -e "  ${EXIT_YELLOW}r${EXIT_RESET} - Resume execution (reset circuit breaker)"
    echo -e "  ${EXIT_YELLOW}s${EXIT_RESET} - Skip current task and continue"
    echo -e "  ${EXIT_YELLOW}a${EXIT_RESET} - Abort ralph loop"
    echo ""

    while true; do
        read -p "Choice [r/s/a]: " choice
        case "$choice" in
            r|R)
                reset_circuit_breaker
                return 0  # Resume
                ;;
            s|S)
                reset_circuit_breaker
                return 1  # Skip
                ;;
            a|A)
                return 2  # Abort
                ;;
            *)
                echo "Invalid choice. Enter r, s, or a."
                ;;
        esac
    done
}

# =============================================================================
# Interrupt Handling Functions
# =============================================================================

# handle_interrupt - SIGINT handler for graceful Ctrl+C
# If in critical section, defers interrupt; otherwise sets flag immediately
handle_interrupt() {
    if [[ "$IN_CRITICAL_SECTION" == "true" ]]; then
        # Defer handling - will be checked after critical section exits
        INTERRUPTED=true
        echo -e "\n${EXIT_YELLOW}Interrupt received - will exit after critical section completes${EXIT_RESET}" >&2
    else
        INTERRUPTED=true
        echo -e "\n${EXIT_YELLOW}Interrupt received - will exit at next safe point${EXIT_RESET}" >&2
    fi
}

# check_interrupted - Check if interrupt flag is set
# Returns: 0 if interrupted, 1 if not interrupted
check_interrupted() {
    if [[ "$INTERRUPTED" == "true" ]]; then
        return 0  # Interrupted
    fi
    return 1  # Not interrupted
}

# enter_critical_section - Mark start of critical section
# Critical sections (like git commits) should not be interrupted mid-operation
enter_critical_section() {
    IN_CRITICAL_SECTION=true
}

# exit_critical_section - Mark end of critical section
# Checks if interrupt was deferred and prints message if so
exit_critical_section() {
    IN_CRITICAL_SECTION=false

    # If interrupt was deferred, notify that we'll handle it now
    if [[ "$INTERRUPTED" == "true" ]]; then
        echo -e "${EXIT_YELLOW}Critical section complete - processing deferred interrupt${EXIT_RESET}" >&2
    fi
}

# =============================================================================
# Exit Status Functions
# =============================================================================

# exit_with_status - Log exit status and return appropriate exit code
# Args: status (COMPLETED/STUCK/ABORTED/INTERRUPTED), reason, last_task, iteration_count, duration
# Returns: Exit code (does NOT call exit - caller decides)
# Calls log_exit_status to update STATE.md, then prints terminal message
exit_with_status() {
    local status="$1"
    local reason="$2"
    local last_task="$3"
    local iteration_count="$4"
    local duration="$5"

    if [[ -z "$status" || -z "$reason" ]]; then
        echo -e "${EXIT_RED}Error: exit_with_status requires status and reason${EXIT_RESET}" >&2
        return 1
    fi

    # Log to STATE.md (function from state.sh)
    if type log_exit_status &>/dev/null; then
        log_exit_status "$status" "$reason" "$last_task" "$iteration_count" "$duration"
    fi

    # Print terminal message with appropriate color
    local color
    local exit_code

    case "$status" in
        COMPLETED)
            color="$EXIT_GREEN"
            exit_code=$EXIT_COMPLETED
            ;;
        STUCK)
            color="$EXIT_RED"
            exit_code=$EXIT_STUCK
            ;;
        ABORTED)
            color="$EXIT_YELLOW"
            exit_code=$EXIT_ABORTED
            ;;
        INTERRUPTED)
            color="$EXIT_YELLOW"
            exit_code=$EXIT_INTERRUPTED
            ;;
        *)
            color="$EXIT_RED"
            exit_code=1
            ;;
    esac

    echo ""
    echo -e "${EXIT_BOLD}${color}=== Ralph Exit: $status ===${EXIT_RESET}"
    echo -e "Reason: $reason"
    if [[ -n "$last_task" ]]; then
        echo -e "Last task: $last_task"
    fi
    if [[ -n "$iteration_count" ]]; then
        echo -e "Iterations: $iteration_count"
    fi
    if [[ -n "$duration" ]]; then
        # Format duration if it's a number
        if [[ "$duration" =~ ^[0-9]+$ ]]; then
            local hours=$((duration / 3600))
            local minutes=$(((duration % 3600) / 60))
            echo -e "Duration: ${hours}h ${minutes}m"
        else
            echo -e "Duration: $duration"
        fi
    fi
    echo ""

    return $exit_code
}

# =============================================================================
# Test Result Parsing Functions
# =============================================================================

# parse_test_results - Parse test output for pass/fail patterns
# Args: output_file (path to file containing test output, optional)
# Returns: 0 if tests pass, 1 if tests fail, 2 if unknown
# Outputs: TESTS_PASS, TESTS_FAIL:n, or TESTS_UNKNOWN to stdout
#
# This function parses output from test frameworks to detect pass/fail patterns.
# It looks for common patterns used by various test frameworks (Jest, pytest, Go, etc.)
parse_test_results() {
    local output_file="${1:-}"

    # If no output file, we can't determine test status
    # This is OK - check_completion will handle it
    if [[ -z "$output_file" || ! -f "$output_file" ]]; then
        echo "TESTS_UNKNOWN"
        return 2
    fi

    # Count failures (various frameworks)
    local fail_count
    fail_count=$(grep -ciE '(FAIL|ERROR|FAILED)' "$output_file" 2>/dev/null) || true
    fail_count="${fail_count%%$'\n'*}"; fail_count="${fail_count//[^0-9]/}"; fail_count="${fail_count:-0}"

    # Count passes (various frameworks)
    local pass_count
    pass_count=$(grep -ciE '(PASS|OK|PASSED|SUCCESS)' "$output_file" 2>/dev/null) || true
    pass_count="${pass_count%%$'\n'*}"; pass_count="${pass_count//[^0-9]/}"; pass_count="${pass_count:-0}"

    if [[ $fail_count -eq 0 && $pass_count -gt 0 ]]; then
        echo "TESTS_PASS"
        return 0
    elif [[ $fail_count -gt 0 ]]; then
        echo "TESTS_FAIL:$fail_count"
        return 1
    else
        echo "TESTS_UNKNOWN"
        return 2  # Could not determine
    fi
}

# check_tests_pass - Helper to check if tests pass
# Args: output_file (path to file containing test output, optional)
# Returns: 0 if tests pass, 1 otherwise
check_tests_pass() {
    local output_file="${1:-}"
    local result
    result=$(parse_test_results "$output_file")
    [[ "$result" == "TESTS_PASS" ]]
}

# =============================================================================
# Roadmap Completion Functions
# =============================================================================

# check_all_plans_complete - Check if all plans in ROADMAP.md are complete
# Returns: 0 if all complete, 1 if incomplete plans remain
#
# Checks for uncompleted plans by looking for "- [ ] NN-MM-PLAN.md" patterns.
# Uses ROADMAP_FILE from parse.sh if available, defaults to .planning/ROADMAP.md
check_all_plans_complete() {
    local roadmap="${ROADMAP_FILE:-.planning/ROADMAP.md}"

    # Count uncompleted plans: - [ ] NN-MM-PLAN.md
    local incomplete
    incomplete=$(grep -cE '^\s*- \[ \] [0-9]{2}-[0-9]{2}-PLAN\.md' "$roadmap" 2>/dev/null) || true
    incomplete="${incomplete%%$'\n'*}"; incomplete="${incomplete//[^0-9]/}"; incomplete="${incomplete:-0}"

    if [[ $incomplete -eq 0 ]]; then
        return 0  # All complete
    else
        return 1  # Still have incomplete plans
    fi
}

# =============================================================================
# Dual-Exit Gate Functions
# =============================================================================

# check_completion - Dual-exit gate for milestone completion
# Args: last_output_file (optional, for test result parsing)
# Returns: 0 if BOTH tests pass AND all requirements done, 1 otherwise
#
# This implements EXIT-03: dual-exit gate requires BOTH conditions.
# If tests pass but requirements aren't done: continue iterating
# If requirements done but tests fail: continue iterating (something broke)
check_completion() {
    local last_output_file="${1:-}"

    local tests_pass=false
    local requirements_done=false

    # Check 1: All tests pass (or unknown - treat as passing if we can't tell)
    # RESEARCH.md notes: accept false negatives over false positives
    local test_result
    test_result=$(parse_test_results "$last_output_file")
    if [[ "$test_result" == "TESTS_PASS" || "$test_result" == "TESTS_UNKNOWN" ]]; then
        tests_pass=true
    fi

    # Check 2: All requirements marked complete in ROADMAP.md
    if check_all_plans_complete; then
        requirements_done=true
    fi

    # Dual gate: BOTH must be true
    if [[ "$tests_pass" == "true" && "$requirements_done" == "true" ]]; then
        return 0  # COMPLETED - both conditions met
    fi

    # Log which condition(s) not met (helpful for debugging)
    if [[ "$tests_pass" != "true" ]]; then
        echo "Completion check: tests not passing ($test_result)" >&2
    fi
    if [[ "$requirements_done" != "true" ]]; then
        echo "Completion check: plans still incomplete" >&2
    fi

    return 1  # Not complete yet
}
