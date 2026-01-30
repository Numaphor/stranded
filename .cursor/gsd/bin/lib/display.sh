#!/bin/bash
# GSD Ralph - Progress Display
# Part of Phase 1: Safety Foundation
#
# Provides terminal progress display for the ralph loop.
# Functions: format_duration, show_progress, show_status

# Respect NO_COLOR standard (https://no-color.org/)
if [[ -n "${NO_COLOR:-}" ]]; then
    RED=''
    GREEN=''
    YELLOW=''
    CYAN=''
    BOLD=''
    RESET=''
else
    RED='\e[31m'
    GREEN='\e[32m'
    YELLOW='\e[33m'
    CYAN='\e[36m'
    BOLD='\e[1m'
    RESET='\e[0m'
fi

# format_duration - Convert seconds to "Nh NNm" format
# Args: seconds
# Output: Formatted duration string (e.g., "1h 05m", "0h 30m")
format_duration() {
    local seconds="$1"
    local hours=$((seconds / 3600))
    local minutes=$(((seconds % 3600) / 60))
    printf "%dh %02dm" "$hours" "$minutes"
}

# show_progress - Display iteration status
# Args: iteration, max_iterations, current_task, next_task (optional)
# Requires: START_TIME (global, epoch seconds when loop started)
#           TIMEOUT_HOURS (global, from budget config)
show_progress() {
    local iteration="$1"
    local max_iterations="$2"
    local current_task="$3"
    local next_task="${4:-}"

    # Calculate elapsed time
    local now
    now=$(date +%s)
    local elapsed=$((now - START_TIME))
    local elapsed_fmt
    elapsed_fmt=$(format_duration "$elapsed")

    # Calculate remaining time (based on timeout, not iterations)
    local timeout_seconds=$((TIMEOUT_HOURS * 3600))
    local remaining=$((timeout_seconds - elapsed))
    if [ "$remaining" -lt 0 ]; then
        remaining=0
    fi
    local remaining_fmt
    remaining_fmt=$(format_duration "$remaining")

    # Display progress line
    echo -e "${BOLD}${CYAN}Iteration $iteration/$max_iterations${RESET} | ${elapsed_fmt} elapsed | ${remaining_fmt} remaining"

    # Show completed task
    echo -e "  ${GREEN}Completed:${RESET} $current_task"

    # Show next task if provided
    if [ -n "$next_task" ]; then
        echo -e "  ${YELLOW}Starting:${RESET} $next_task"
    fi

    echo ""
}

# show_status - Show single-line status update
# Args: message, type (info/success/error/warning)
show_status() {
    local message="$1"
    local type="${2:-info}"

    local color
    case "$type" in
        success)
            color="$GREEN"
            ;;
        error)
            color="$RED"
            ;;
        warning)
            color="$YELLOW"
            ;;
        info|*)
            color="$CYAN"
            ;;
    esac

    echo -e "${color}${message}${RESET}"
}

# =============================================================================
# Spinner Functions (Cross-Platform)
# =============================================================================

# Spinner state
SPINNER_PID=""

# start_spinner - Start background spinner with message
# Args: message
# Uses ASCII-only characters for Git Bash compatibility
start_spinner() {
    local message="$1"

    # ASCII-only spinner characters (works everywhere)
    local spin_chars='|/-\'

    # Don't start spinner if not a terminal
    if [[ ! -t 1 ]]; then
        echo "$message"
        return 0
    fi

    # Disable job control messages
    set +m 2>/dev/null || true

    {
        local i=0
        while true; do
            local char="${spin_chars:$i:1}"
            printf "\r${CYAN}%s %s${RESET}" "$message" "$char"
            i=$(( (i + 1) % 4 ))
            sleep 0.25
        done
    } &

    SPINNER_PID=$!

    # Re-enable job control
    set -m 2>/dev/null || true
}

# stop_spinner - Stop the background spinner
# Cleans up spinner process and clears line
stop_spinner() {
    if [[ -n "$SPINNER_PID" ]]; then
        kill "$SPINNER_PID" 2>/dev/null || true
        wait "$SPINNER_PID" 2>/dev/null || true
        SPINNER_PID=""
        # Clear the spinner line
        printf "\r\033[2K"
    fi
}

# Ensure cleanup on script exit
trap 'stop_spinner' EXIT
