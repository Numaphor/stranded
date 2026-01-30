#!/bin/bash
# GSD Ralph - Recovery and Stuck Analysis
# Part of Phase 6: Circuit Breaker & Recovery
#
# Provides stuck analysis functions that examine failure patterns in STATE.md.
# Functions: generate_stuck_analysis, parse_failure_patterns, get_recent_failures
#
# Usage:
#   source bin/lib/recovery.sh
#   generate_stuck_analysis  # Outputs 3-5 line analysis to stdout

# Configuration
STATE_FILE="${STATE_FILE:-.planning/STATE.md}"
ANALYSIS_WINDOW=5  # Number of recent failures to analyze

# Color codes (respect NO_COLOR standard)
if [[ -n "${NO_COLOR:-}" ]]; then
    RECOVERY_YELLOW=''
    RECOVERY_CYAN=''
    RECOVERY_BOLD=''
    RECOVERY_RESET=''
else
    RECOVERY_YELLOW='\e[33m'
    RECOVERY_CYAN='\e[36m'
    RECOVERY_BOLD='\e[1m'
    RECOVERY_RESET='\e[0m'
fi

# =============================================================================
# Failure Pattern Analysis Functions
# =============================================================================

# get_recent_failures - Extract recent FAILURE entries from STATE.md history
# Args: [count] - number of entries to get (default: ANALYSIS_WINDOW)
# Output: Failure entries, one per line
# Returns: 0 on success, 1 if no failures found
get_recent_failures() {
    local count="${1:-$ANALYSIS_WINDOW}"

    if [[ ! -f "$STATE_FILE" ]]; then
        return 1
    fi

    # Extract entries from history section that contain FAILURE
    local failures
    failures=$(sed -n '/<!-- HISTORY_START -->/,/<!-- HISTORY_END -->/{
        /FAILURE/p
    }' "$STATE_FILE" | head -n "$count")

    if [[ -z "$failures" ]]; then
        return 1
    fi

    echo "$failures"
    return 0
}

# parse_failure_patterns - Analyze failures for common patterns
# Args: failure_entries (multiline string)
# Output: Sets global variables: PATTERN_ERROR, PATTERN_FILE, PATTERN_TASK_PREFIX
# Returns: 0 if patterns found, 1 if no clear patterns
parse_failure_patterns() {
    local failures="$1"

    PATTERN_ERROR=""
    PATTERN_FILE=""
    PATTERN_TASK_PREFIX=""

    if [[ -z "$failures" ]]; then
        return 1
    fi

    # Pattern 1: Look for repeated error keywords
    # Common patterns: "Error:", "Failed:", "cannot", "not found"
    local error_counts
    error_counts=$(echo "$failures" | grep -oiE '(error|failed|cannot|not found)[^|]*' | \
        sed 's/[[:space:]]*$//' | sort | uniq -c | sort -rn | head -1)

    if [[ -n "$error_counts" ]]; then
        # Extract just the pattern (remove count)
        PATTERN_ERROR=$(echo "$error_counts" | sed 's/^[[:space:]]*[0-9]*[[:space:]]*//')
    fi

    # Pattern 2: Look for repeated file references
    # Match common file extensions
    local file_counts
    file_counts=$(echo "$failures" | grep -oE '[a-zA-Z0-9_/-]+\.(sh|md|ts|js|tsx|jsx|json|yaml|yml)' | \
        sort | uniq -c | sort -rn | head -1)

    if [[ -n "$file_counts" ]]; then
        local count=$(echo "$file_counts" | awk '{print $1}')
        if [[ "$count" -gt 1 ]]; then
            PATTERN_FILE=$(echo "$file_counts" | awk '{print $2}')
        fi
    fi

    # Pattern 3: Look for repeated task prefixes (same phase)
    # Task IDs are like "05-01", "05-02" - check if same phase failing
    local task_prefix_counts
    task_prefix_counts=$(echo "$failures" | grep -oE '[0-9]{2}-[0-9]{2}' | \
        cut -d'-' -f1 | sort | uniq -c | sort -rn | head -1)

    if [[ -n "$task_prefix_counts" ]]; then
        local count=$(echo "$task_prefix_counts" | awk '{print $1}')
        if [[ "$count" -gt 1 ]]; then
            PATTERN_TASK_PREFIX=$(echo "$task_prefix_counts" | awk '{print $2}')
        fi
    fi

    # Return success if any pattern found
    [[ -n "$PATTERN_ERROR" || -n "$PATTERN_FILE" || -n "$PATTERN_TASK_PREFIX" ]]
}

# generate_stuck_analysis - Generate 3-5 line analysis summary
# Output: Analysis text to stdout (colorized if terminal)
# Returns: 0 always
#
# This is the main entry point called by handle_circuit_breaker_pause
generate_stuck_analysis() {
    local failures
    local failure_count=0

    # Get recent failures
    failures=$(get_recent_failures)
    if [[ $? -ne 0 || -z "$failures" ]]; then
        echo -e "${RECOVERY_YELLOW}Analysis:${RECOVERY_RESET} No recent failures in history to analyze"
        return 0
    fi

    # Count failures
    failure_count=$(echo "$failures" | wc -l | tr -d ' ')

    echo -e "${RECOVERY_YELLOW}${RECOVERY_BOLD}Failure Analysis:${RECOVERY_RESET}"

    # Parse for patterns
    parse_failure_patterns "$failures"

    local patterns_found=false

    # Report error pattern if found
    if [[ -n "$PATTERN_ERROR" ]]; then
        echo -e "  ${RECOVERY_CYAN}Common error:${RECOVERY_RESET} $PATTERN_ERROR"
        patterns_found=true
    fi

    # Report file pattern if found
    if [[ -n "$PATTERN_FILE" ]]; then
        echo -e "  ${RECOVERY_CYAN}Affected file:${RECOVERY_RESET} $PATTERN_FILE"
        patterns_found=true
    fi

    # Report task prefix pattern if found
    if [[ -n "$PATTERN_TASK_PREFIX" ]]; then
        echo -e "  ${RECOVERY_CYAN}Failing phase:${RECOVERY_RESET} Phase $PATTERN_TASK_PREFIX"
        patterns_found=true
    fi

    if [[ "$patterns_found" != "true" ]]; then
        echo -e "  ${RECOVERY_CYAN}Pattern:${RECOVERY_RESET} No clear pattern detected in $failure_count failures"
    fi

    # Provide actionable suggestion based on patterns (LOOP-04: alternative approaches)
    echo ""
    echo -e "  ${RECOVERY_YELLOW}Possible actions:${RECOVERY_RESET}"

    if [[ -n "$PATTERN_FILE" ]]; then
        echo -e "    - Check $PATTERN_FILE for syntax errors or missing dependencies"
        echo -e "    - Review recent changes to this file (git diff)"
    fi

    if [[ -n "$PATTERN_ERROR" ]]; then
        echo -e "    - Search codebase for similar issues: grep -r '$PATTERN_ERROR'"
    fi

    if [[ -n "$PATTERN_TASK_PREFIX" ]]; then
        echo -e "    - Phase $PATTERN_TASK_PREFIX may have environmental prerequisites"
        echo -e "    - Consider running earlier phase plans first"
    fi

    if [[ "$patterns_found" != "true" ]]; then
        echo -e "    - Check environment (missing tools, permissions, network)"
        echo -e "    - Review .planning/ralph.log for detailed error output"
        echo -e "    - Try running the failing task manually"
    fi

    return 0
}
