#!/bin/bash
# GSD Ralph - Fail-Fast Error Handling
# Part of Phase 1: Safety Foundation
#
# Provides:
#   run_with_retry    - Retry command up to 3 times
#   run_claude_task   - Execute Claude CLI with error handling
#   mark_checkpoint   - Record git HEAD as rollback point
#   rollback_to_checkpoint - Reset to last checkpoint
#   check_limits      - Verify iteration/timeout within budget
#   handle_task_failure - Process task failure with rollback
#   handle_limit_reached - Process budget exhaustion with rollback

# Color codes (same as display.sh for consistency)
RED='\e[31m'
GREEN='\e[32m'
YELLOW='\e[33m'
BOLD='\e[1m'
RESET='\e[0m'

# Retry configuration
MAX_RETRIES=3

# Checkpoint state
CHECKPOINT_COMMIT=""

# Limit state
LIMIT_REASON=""

# run_with_retry - Execute command with retry logic
# Args: command (string), task_name (string)
# Returns: 0 on success, 1 after all retries exhausted
run_with_retry() {
    local cmd="$1"
    local task_name="$2"
    local attempt=1

    while [ $attempt -le $MAX_RETRIES ]; do
        echo -e "  ${YELLOW}Attempt $attempt/$MAX_RETRIES for:${RESET} $task_name"

        # Execute command and capture exit code
        eval "$cmd"
        local exit_code=$?

        if [ $exit_code -eq 0 ]; then
            echo -e "  ${GREEN}Success${RESET}"
            return 0
        fi

        echo -e "  ${RED}Failed (exit code: $exit_code)${RESET}"
        attempt=$((attempt + 1))

        if [ $attempt -le $MAX_RETRIES ]; then
            echo -e "  ${YELLOW}Retrying in 5 seconds...${RESET}"
            sleep 5
        fi
    done

    # All retries exhausted
    echo -e "  ${RED}${BOLD}Task failed after $MAX_RETRIES attempts${RESET}"
    return 1
}

# run_claude_task - Wrapper specifically for Claude CLI
# Args: prompt (string), output_file (optional, defaults to temp file)
# Returns: Claude's exit code
run_claude_task() {
    local prompt="$1"
    local output_file="${2:-}"
    local using_temp=0

    # Create temp file if no output file specified
    if [ -z "$output_file" ]; then
        output_file=$(mktemp)
        using_temp=1
    fi

    # Run Claude CLI
    claude -p "$prompt" --output-format json > "$output_file" 2>&1
    local exit_code=$?

    # Clean up temp file on success
    if [ $using_temp -eq 1 ] && [ $exit_code -eq 0 ]; then
        rm -f "$output_file"
    fi

    return $exit_code
}

# mark_checkpoint - Record current git HEAD as checkpoint
# Returns: 0 on success, 1 if not in git repo
mark_checkpoint() {
    # Check if we're in a git repo
    if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
        echo -e "  ${RED}Warning: Not in a git repository${RESET}"
        return 1
    fi

    CHECKPOINT_COMMIT=$(git rev-parse HEAD)
    local short_sha=$(git rev-parse --short HEAD)
    echo -e "  ${GREEN}Checkpoint:${RESET} $short_sha"
    return 0
}

# rollback_to_checkpoint - Reset to last checkpoint
# Returns: 0 on success, 1 if no checkpoint set
rollback_to_checkpoint() {
    if [ -z "$CHECKPOINT_COMMIT" ]; then
        echo -e "  ${RED}Error: No checkpoint set, cannot rollback${RESET}"
        return 1
    fi

    local short_sha=$(echo "$CHECKPOINT_COMMIT" | cut -c1-7)
    echo -e "  ${YELLOW}Rolling back to checkpoint:${RESET} $short_sha"
    git reset --hard "$CHECKPOINT_COMMIT" > /dev/null 2>&1
    echo -e "  ${YELLOW}Partial work discarded${RESET}"
    return 0
}

# check_limits - Check if iteration or timeout limit reached
# Args: current_iteration
# Requires globals: MAX_ITERATIONS, TIMEOUT_HOURS, START_TIME
# Returns: 0 if within limits, 1 if limit reached
# Sets: LIMIT_REASON global ("iteration" or "timeout")
check_limits() {
    local current_iteration="$1"

    # Use current time as start if not set
    if [ -z "$START_TIME" ]; then
        START_TIME=$(date +%s)
    fi

    # Check iteration limit
    if [ -n "$MAX_ITERATIONS" ] && [ "$current_iteration" -ge "$MAX_ITERATIONS" ]; then
        LIMIT_REASON="iteration"
        return 1
    fi

    # Check timeout limit
    if [ -n "$TIMEOUT_HOURS" ]; then
        local now=$(date +%s)
        local elapsed=$((now - START_TIME))
        local timeout_seconds=$((TIMEOUT_HOURS * 3600))

        if [ $elapsed -ge $timeout_seconds ]; then
            LIMIT_REASON="timeout"
            return 1
        fi
    fi

    return 0
}

# handle_task_failure - Called when task fails after retries
# Args: task_name
# Returns: 1 (caller should exit)
handle_task_failure() {
    local task_name="$1"

    echo ""
    echo -e "${RED}${BOLD}FATAL: Task '$task_name' failed after $MAX_RETRIES attempts${RESET}"
    echo -e "${RED}Stopping execution to prevent further token burn${RESET}"
    echo ""

    rollback_to_checkpoint

    return 1
}

# handle_limit_reached - Called when budget exhausted
# Args: none (reads LIMIT_REASON global)
# Returns: 1 (caller should exit)
handle_limit_reached() {
    echo ""
    echo -e "${RED}${BOLD}LIMIT REACHED: $LIMIT_REASON${RESET}"
    echo ""

    rollback_to_checkpoint

    return 1
}
