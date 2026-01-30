#!/bin/bash
# GSD Ralph - Mode Selection Library
# Part of Phase 9: Mode Selection & Base Commands
#
# Provides mode read/write functions for Interactive vs Lazy mode.
# Functions: get_mode, set_mode, is_mode, require_mode

# Configuration file location (same as budget.sh)
RALPH_CONFIG_FILE="${RALPH_CONFIG_FILE:-.planning/.ralph-config}"

# Color codes for terminal output
MODE_RED='\e[31m'
MODE_GREEN='\e[32m'
MODE_YELLOW='\e[33m'
MODE_RESET='\e[0m'

# get_mode - Return current GSD mode (interactive, lazy, or empty string if unset)
# Outputs: mode string or empty
get_mode() {
    local mode=""
    if [ -f "$RALPH_CONFIG_FILE" ]; then
        # shellcheck source=/dev/null
        source "$RALPH_CONFIG_FILE" 2>/dev/null
        mode="${GSD_MODE:-}"
    fi
    echo "$mode"
}

# set_mode - Set GSD mode and save to config
# Args: mode (interactive|lazy)
# Preserves MAX_ITERATIONS and TIMEOUT_HOURS from existing config
set_mode() {
    local mode="$1"

    # Validate mode value
    if [[ "$mode" != "interactive" && "$mode" != "lazy" ]]; then
        echo -e "${MODE_RED}Error: Mode must be 'interactive' or 'lazy'${MODE_RESET}" >&2
        return 1
    fi

    # Load existing config values (preserve budget settings)
    local existing_iterations=""
    local existing_timeout=""
    if [ -f "$RALPH_CONFIG_FILE" ]; then
        # shellcheck source=/dev/null
        source "$RALPH_CONFIG_FILE" 2>/dev/null
        existing_iterations="${MAX_ITERATIONS:-}"
        existing_timeout="${TIMEOUT_HOURS:-}"
    fi

    # Apply defaults if not set
    existing_iterations="${existing_iterations:-50}"
    existing_timeout="${existing_timeout:-8}"

    # Create parent directory if needed
    local config_dir
    config_dir="$(dirname "$RALPH_CONFIG_FILE")"
    if [ ! -d "$config_dir" ]; then
        mkdir -p "$config_dir"
    fi

    # Write all config values (preserving existing budget settings)
    cat > "$RALPH_CONFIG_FILE" << EOF
# GSD Ralph configuration
# Last updated: $(date)
MAX_ITERATIONS=$existing_iterations
TIMEOUT_HOURS=$existing_timeout
GSD_MODE=$mode
EOF

    # Export for immediate use
    GSD_MODE="$mode"
    export GSD_MODE

    return 0
}

# is_mode - Check if current mode matches expected
# Args: expected_mode
# Returns: 0 if match, 1 if not
is_mode() {
    local expected="$1"
    local current
    current=$(get_mode)
    [[ "$current" == "$expected" ]]
}

# require_mode - Return error code if mode doesn't match
# Args: expected_mode, command_name
# Returns: 0 if mode matches, 1 if mode unset or mismatch
# Note: Caller handles exit (functions never exit directly per project convention)
require_mode() {
    local expected="$1"
    local command="$2"
    local current
    current=$(get_mode)

    # Check if mode is set
    if [[ -z "$current" ]]; then
        echo -e "${MODE_RED}Error: Mode not set. Run /gsd:lazy-mode first.${MODE_RESET}" >&2
        return 1
    fi

    # Check if mode matches expected
    if [[ "$current" != "$expected" ]]; then
        echo -e "${MODE_RED}Error: $command is only available in $expected mode.${MODE_RESET}" >&2
        echo -e "Current mode: ${MODE_YELLOW}$current${MODE_RESET}" >&2
        echo -e "Run ${MODE_GREEN}/gsd:lazy-mode${MODE_RESET} to toggle." >&2
        return 1
    fi

    return 0
}
