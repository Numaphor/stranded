#!/bin/bash
# GSD Ralph - Budget Configuration
# Part of Phase 1: Safety Foundation
#
# Provides budget prompting and persistence for the ralph loop.
# Functions: load_config, validate_number, prompt_budget, save_config

# Configuration file location (relative to project root)
RALPH_CONFIG_FILE="${RALPH_CONFIG_FILE:-.planning/.ralph-config}"

# Color codes for terminal output
BUDGET_RED='\e[31m'
BUDGET_GREEN='\e[32m'
BUDGET_YELLOW='\e[33m'
BUDGET_RESET='\e[0m'

# load_config - Load from .planning/.ralph-config or use defaults
# Sets: MAX_ITERATIONS, TIMEOUT_HOURS, CIRCUIT_BREAKER_THRESHOLD, STUCK_THRESHOLD (exported)
load_config() {
    # Source existing config if it exists
    if [ -f "$RALPH_CONFIG_FILE" ]; then
        # shellcheck source=/dev/null
        source "$RALPH_CONFIG_FILE"
    fi

    # Apply defaults for any missing values
    MAX_ITERATIONS="${MAX_ITERATIONS:-50}"
    TIMEOUT_HOURS="${TIMEOUT_HOURS:-8}"
    CIRCUIT_BREAKER_THRESHOLD="${CIRCUIT_BREAKER_THRESHOLD:-5}"
    STUCK_THRESHOLD="${STUCK_THRESHOLD:-3}"

    # Export for use by other scripts
    export MAX_ITERATIONS
    export TIMEOUT_HOURS
    export CIRCUIT_BREAKER_THRESHOLD
    export STUCK_THRESHOLD
}

# validate_number - Validate input is positive integer
# Args: value, field_name
# Returns: 0 on success, 1 on failure (with colored error message)
validate_number() {
    local value="$1"
    local field_name="$2"

    # Check if value matches positive integer pattern
    if ! [[ "$value" =~ ^[0-9]+$ ]]; then
        echo -e "${BUDGET_RED}Error: $field_name must be a positive integer${BUDGET_RESET}" >&2
        return 1
    fi

    # Check value is at least 1
    if [ "$value" -lt 1 ]; then
        echo -e "${BUDGET_RED}Error: $field_name must be at least 1${BUDGET_RESET}" >&2
        return 1
    fi

    return 0
}

# save_config - Persist to .planning/.ralph-config
# Writes current MAX_ITERATIONS, TIMEOUT_HOURS, CIRCUIT_BREAKER_THRESHOLD, STUCK_THRESHOLD
# Preserves existing GSD_MODE value if set
save_config() {
    # Create parent directory if needed
    local config_dir
    config_dir="$(dirname "$RALPH_CONFIG_FILE")"
    if [ ! -d "$config_dir" ]; then
        mkdir -p "$config_dir"
    fi

    # Load existing mode (if any) to preserve it
    local existing_mode=""
    if [ -f "$RALPH_CONFIG_FILE" ]; then
        # shellcheck source=/dev/null
        source "$RALPH_CONFIG_FILE" 2>/dev/null
        existing_mode="${GSD_MODE:-}"
    fi

    # Write bash variable assignments (can be sourced)
    cat > "$RALPH_CONFIG_FILE" << EOF
# GSD Ralph configuration
# Last updated: $(date)
MAX_ITERATIONS=$MAX_ITERATIONS
TIMEOUT_HOURS=$TIMEOUT_HOURS
CIRCUIT_BREAKER_THRESHOLD=$CIRCUIT_BREAKER_THRESHOLD
STUCK_THRESHOLD=$STUCK_THRESHOLD
EOF

    # Append mode if it was set
    if [ -n "$existing_mode" ]; then
        echo "GSD_MODE=$existing_mode" >> "$RALPH_CONFIG_FILE"
    fi

    echo -e "${BUDGET_GREEN}Configuration saved to $RALPH_CONFIG_FILE${BUDGET_RESET}"
}

# prompt_budget - Interactive prompts with editable defaults
# Prompts user for MAX_ITERATIONS and TIMEOUT_HOURS, validates, and saves
prompt_budget() {
    load_config

    echo -e "${BUDGET_YELLOW}=== Budget Configuration ===${BUDGET_RESET}"
    echo ""

    # Check if running interactively
    local is_interactive=0
    if [[ -t 0 ]]; then
        is_interactive=1
    fi

    # Prompt for max iterations
    while true; do
        local input
        if [ "$is_interactive" -eq 1 ]; then
            # Interactive mode: use editable defaults
            read -e -p "Max iterations [default: $MAX_ITERATIONS]: " -i "$MAX_ITERATIONS" input
        else
            # Non-interactive mode: simple read
            read -p "Max iterations [default: $MAX_ITERATIONS]: " input
        fi
        input="${input:-$MAX_ITERATIONS}"

        if validate_number "$input" "Max iterations"; then
            MAX_ITERATIONS="$input"
            break
        fi
    done

    # Prompt for timeout
    while true; do
        local input
        if [ "$is_interactive" -eq 1 ]; then
            # Interactive mode: use editable defaults
            read -e -p "Timeout hours [default: $TIMEOUT_HOURS]: " -i "$TIMEOUT_HOURS" input
        else
            # Non-interactive mode: simple read
            read -p "Timeout hours [default: $TIMEOUT_HOURS]: " input
        fi
        input="${input:-$TIMEOUT_HOURS}"

        if validate_number "$input" "Timeout hours"; then
            TIMEOUT_HOURS="$input"
            break
        fi
    done

    echo ""
    echo -e "Running with: ${BUDGET_GREEN}$MAX_ITERATIONS iterations${BUDGET_RESET}, ${BUDGET_GREEN}${TIMEOUT_HOURS}h timeout${BUDGET_RESET}"

    # Export the values
    export MAX_ITERATIONS
    export TIMEOUT_HOURS

    save_config
}

# prompt_all_settings - Interactive prompts for all 4 config settings
# Prompts user for MAX_ITERATIONS, TIMEOUT_HOURS, CIRCUIT_BREAKER_THRESHOLD, STUCK_THRESHOLD
# Validates each as positive integer and saves all to config
prompt_all_settings() {
    load_config

    echo -e "${BUDGET_YELLOW}=== Autopilot Configuration ===${BUDGET_RESET}"
    echo ""

    # Check if running interactively
    local is_interactive=0
    if [[ -t 0 ]]; then
        is_interactive=1
    fi

    # Prompt for max iterations
    while true; do
        local input
        if [ "$is_interactive" -eq 1 ]; then
            read -e -p "Max iterations [default: $MAX_ITERATIONS]: " -i "$MAX_ITERATIONS" input
        else
            read -p "Max iterations [default: $MAX_ITERATIONS]: " input
        fi
        input="${input:-$MAX_ITERATIONS}"

        if validate_number "$input" "Max iterations"; then
            MAX_ITERATIONS="$input"
            break
        fi
    done

    # Prompt for timeout hours
    while true; do
        local input
        if [ "$is_interactive" -eq 1 ]; then
            read -e -p "Timeout hours [default: $TIMEOUT_HOURS]: " -i "$TIMEOUT_HOURS" input
        else
            read -p "Timeout hours [default: $TIMEOUT_HOURS]: " input
        fi
        input="${input:-$TIMEOUT_HOURS}"

        if validate_number "$input" "Timeout hours"; then
            TIMEOUT_HOURS="$input"
            break
        fi
    done

    # Prompt for circuit breaker threshold
    while true; do
        local input
        if [ "$is_interactive" -eq 1 ]; then
            read -e -p "Circuit breaker threshold [default: $CIRCUIT_BREAKER_THRESHOLD]: " -i "$CIRCUIT_BREAKER_THRESHOLD" input
        else
            read -p "Circuit breaker threshold [default: $CIRCUIT_BREAKER_THRESHOLD]: " input
        fi
        input="${input:-$CIRCUIT_BREAKER_THRESHOLD}"

        if validate_number "$input" "Circuit breaker threshold"; then
            CIRCUIT_BREAKER_THRESHOLD="$input"
            break
        fi
    done

    # Prompt for stuck threshold
    while true; do
        local input
        if [ "$is_interactive" -eq 1 ]; then
            read -e -p "Stuck threshold [default: $STUCK_THRESHOLD]: " -i "$STUCK_THRESHOLD" input
        else
            read -p "Stuck threshold [default: $STUCK_THRESHOLD]: " input
        fi
        input="${input:-$STUCK_THRESHOLD}"

        if validate_number "$input" "Stuck threshold"; then
            STUCK_THRESHOLD="$input"
            break
        fi
    done

    echo ""
    echo -e "${BUDGET_GREEN}Settings configured:${BUDGET_RESET}"
    echo -e "  Max iterations:      ${BUDGET_GREEN}$MAX_ITERATIONS${BUDGET_RESET}"
    echo -e "  Timeout hours:       ${BUDGET_GREEN}$TIMEOUT_HOURS${BUDGET_RESET}"
    echo -e "  Circuit breaker:     ${BUDGET_GREEN}$CIRCUIT_BREAKER_THRESHOLD${BUDGET_RESET} failures before halt"
    echo -e "  Stuck threshold:     ${BUDGET_GREEN}$STUCK_THRESHOLD${BUDGET_RESET} retries on same task"

    # Export all values
    export MAX_ITERATIONS
    export TIMEOUT_HOURS
    export CIRCUIT_BREAKER_THRESHOLD
    export STUCK_THRESHOLD

    save_config
}
