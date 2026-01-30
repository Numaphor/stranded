#!/bin/bash
# GSD Ralph - STATE.md and ROADMAP.md Parsing
# Part of Phase 3: Outer Loop Core (extended in Phase 8)
#
# Provides functions for extracting task information from planning files.
# Functions: parse_next_task, find_plan_file, get_plan_name, get_next_plan_after,
#            get_all_phases, phase_has_plans, get_phase_name, get_unplanned_phases,
#            count_phase_plans
#
# Usage:
#   source bin/lib/parse.sh
#   parse_next_task           # Returns "03-01" or "COMPLETE"
#   find_plan_file "03-01"    # Returns path to plan file
#   get_plan_name "03-01"     # Returns plan objective
#   get_next_plan_after "03-01"  # Returns "03-02" or next uncompleted plan
#   get_all_phases            # Returns "1 2 3 4 5 6 7 8 9 10"
#   phase_has_plans "1"       # Returns 0 if phase 1 has plans
#   get_unplanned_phases      # Returns phases without PLAN.md files

# Configuration
STATE_FILE="${STATE_FILE:-.planning/STATE.md}"
ROADMAP_FILE="${ROADMAP_FILE:-.planning/ROADMAP.md}"

# Color codes for error messages
PARSE_RED='\e[31m'
PARSE_YELLOW='\e[33m'
PARSE_RESET='\e[0m'

# parse_next_task - Extract next task from STATE.md Description line
# Returns: Plan ID (e.g., "03-01") on success, "COMPLETE" if no next task
# Return code: 0 on success, 1 on failure
parse_next_task() {
    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${PARSE_RED}Error: STATE_FILE not found: $STATE_FILE${PARSE_RESET}" >&2
        return 1
    fi

    # Read the Description line from Next Action section
    local description
    description=$(grep "^Description:" "$STATE_FILE" | head -1)

    if [[ -z "$description" ]]; then
        # No Description line found - might be complete
        echo "COMPLETE"
        return 0
    fi

    # Extract plan ID using pattern: NN-MM (e.g., "03-01")
    local plan_id
    plan_id=$(echo "$description" | grep -oE '[0-9]{2}-[0-9]{2}' | head -1)

    if [[ -z "$plan_id" ]]; then
        # No plan ID found in description
        echo "COMPLETE"
        return 0
    fi

    echo "$plan_id"
    return 0
}

# find_plan_file - Find the full path to a plan file
# Args: plan_id (e.g., "03-01")
# Returns: Full path to PLAN.md file
# Return code: 0 on success, 1 if not found
find_plan_file() {
    local plan_id="$1"

    if [[ -z "$plan_id" ]]; then
        echo -e "${PARSE_RED}Error: find_plan_file requires plan_id${PARSE_RESET}" >&2
        return 1
    fi

    # Extract phase number from plan_id (e.g., "03" from "03-01")
    local phase_num="${plan_id%%-*}"

    # Find the phase directory matching that number
    # Handles both zero-padded (03-name) and unpadded (3-name) folder names
    local phase_dir
    phase_dir=$(ls -d .planning/phases/${phase_num}-* 2>/dev/null | head -1)

    if [[ -z "$phase_dir" ]]; then
        # Try without leading zero
        local phase_num_unpadded="${phase_num#0}"
        phase_dir=$(ls -d .planning/phases/${phase_num_unpadded}-* 2>/dev/null | head -1)
    fi

    if [[ -z "$phase_dir" || ! -d "$phase_dir" ]]; then
        echo -e "${PARSE_RED}Error: Phase directory not found for plan $plan_id${PARSE_RESET}" >&2
        return 1
    fi

    local plan_file="${phase_dir}/${plan_id}-PLAN.md"

    if [[ ! -f "$plan_file" ]]; then
        echo -e "${PARSE_RED}Error: Plan file not found: $plan_file${PARSE_RESET}" >&2
        return 1
    fi

    echo "$plan_file"
    return 0
}

# get_plan_name - Extract objective/name from a plan file
# Args: plan_id (e.g., "03-01")
# Returns: Short description (first ~50 chars of objective)
# Return code: 0 on success, 1 on failure
get_plan_name() {
    local plan_id="$1"

    if [[ -z "$plan_id" ]]; then
        echo -e "${PARSE_RED}Error: get_plan_name requires plan_id${PARSE_RESET}" >&2
        return 1
    fi

    # Find the plan file
    local plan_file
    plan_file=$(find_plan_file "$plan_id")
    if [[ $? -ne 0 ]]; then
        return 1
    fi

    # Extract first line after <objective> tag
    local objective
    objective=$(sed -n '/<objective>/,/<\/objective>/{
        /<objective>/d
        /<\/objective>/d
        p
    }' "$plan_file" | head -1 | sed 's/^[[:space:]]*//')

    if [[ -z "$objective" ]]; then
        echo "Plan $plan_id"
        return 0
    fi

    # Return first ~50 characters
    echo "${objective:0:50}"
    return 0
}

# get_next_plan_after - Find the next uncompleted plan after current
# Args: current_plan_id (e.g., "03-01")
# Returns: Next plan ID or "COMPLETE" if current is last
# Return code: 0 on success, 1 on failure
get_next_plan_after() {
    local current_plan_id="$1"

    if [[ -z "$current_plan_id" ]]; then
        echo -e "${PARSE_RED}Error: get_next_plan_after requires current_plan_id${PARSE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$ROADMAP_FILE" ]]; then
        echo -e "${PARSE_RED}Error: ROADMAP_FILE not found: $ROADMAP_FILE${PARSE_RESET}" >&2
        return 1
    fi

    # Extract phase and plan numbers from current
    local current_phase="${current_plan_id%%-*}"
    local current_plan="${current_plan_id##*-}"

    # Remove leading zeros for numeric comparison
    current_phase=$((10#$current_phase))
    current_plan=$((10#$current_plan))

    # Get all uncompleted plan checkboxes: - [ ] NN-MM-PLAN.md
    local uncompleted_plans
    uncompleted_plans=$(grep -E '^\s*- \[ \] [0-9]{2}-[0-9]{2}-PLAN\.md' "$ROADMAP_FILE" | grep -oE '[0-9]{2}-[0-9]{2}')

    if [[ -z "$uncompleted_plans" ]]; then
        echo "COMPLETE"
        return 0
    fi

    # Find the first plan that comes AFTER current_plan_id
    local found_next=""
    while IFS= read -r plan_id; do
        local plan_phase="${plan_id%%-*}"
        local plan_num="${plan_id##*-}"

        # Remove leading zeros for comparison
        plan_phase=$((10#$plan_phase))
        plan_num=$((10#$plan_num))

        # Check if this plan comes after current
        if [[ $plan_phase -gt $current_phase ]]; then
            found_next="$plan_id"
            break
        elif [[ $plan_phase -eq $current_phase && $plan_num -gt $current_plan ]]; then
            found_next="$plan_id"
            break
        fi
    done <<< "$uncompleted_plans"

    if [[ -z "$found_next" ]]; then
        echo "COMPLETE"
        return 0
    fi

    echo "$found_next"
    return 0
}

# =============================================================================
# Phase Enumeration Functions (Plan 08-01)
# =============================================================================

# get_all_phases - Return list of all phases from ROADMAP.md
# Returns: Space-separated list of phase numbers (e.g., "1 2 3 4 5 6 7 8 9 10")
# Return code: 0 on success, 1 on failure
# Handles both integer (5) and decimal (2.1) phase numbers
get_all_phases() {
    local roadmap="${ROADMAP_FILE:-.planning/ROADMAP.md}"

    if [[ ! -f "$roadmap" ]]; then
        echo -e "${PARSE_RED}Error: ROADMAP_FILE not found: $roadmap${PARSE_RESET}" >&2
        return 1
    fi

    # Parse "### Phase N:" headers and extract phase numbers
    # Handles both integer (5) and decimal (2.1) phase numbers
    grep -E '^### Phase [0-9]+(\.[0-9]+)?:' "$roadmap" | \
        sed 's/### Phase \([0-9.]*\):.*/\1/' | \
        sort -t. -k1,1n -k2,2n | \
        tr '\n' ' ' | \
        sed 's/ $//'
    return 0
}

# phase_has_plans - Check if phase has PLAN.md files
# Args: phase_num
# Returns: 0 if plans exist, 1 if not
phase_has_plans() {
    local phase_num="$1"

    if [[ -z "$phase_num" ]]; then
        echo -e "${PARSE_RED}Error: phase_has_plans requires phase_num${PARSE_RESET}" >&2
        return 1
    fi

    # Pad to 2 digits for directory matching
    local padded
    local clean_num
    # Handle potential non-numeric input
    if [[ "$phase_num" =~ ^[0-9]+$ ]]; then
        clean_num=$((10#$phase_num))
    else
        clean_num="$phase_num"
    fi
    padded=$(printf "%02d" "$clean_num" 2>/dev/null || echo "$phase_num")

    # Check for PLAN.md files in phase directory
    local plans
    plans=$(ls .planning/phases/${padded}-*/*-PLAN.md 2>/dev/null | head -1)
    if [[ -n "$plans" ]]; then
        return 0
    else
        return 1
    fi
}

# get_phase_name - Get phase name from ROADMAP.md
# Args: phase_num
# Returns: Phase name (e.g., "Safety Foundation" for phase 1)
# Return code: 0 on success, 1 on failure
get_phase_name() {
    local phase_num="$1"
    local roadmap="${ROADMAP_FILE:-.planning/ROADMAP.md}"

    if [[ -z "$phase_num" ]]; then
        echo -e "${PARSE_RED}Error: get_phase_name requires phase_num${PARSE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$roadmap" ]]; then
        echo -e "${PARSE_RED}Error: ROADMAP_FILE not found: $roadmap${PARSE_RESET}" >&2
        return 1
    fi

    # Parse "### Phase N: Name" header and extract name
    local name
    name=$(grep -E "^### Phase ${phase_num}:" "$roadmap" | \
        sed "s/### Phase ${phase_num}: //")

    if [[ -z "$name" ]]; then
        echo "Phase $phase_num"
        return 0
    fi

    echo "$name"
    return 0
}

# get_unplanned_phases - Return list of phases without plans
# Returns: Space-separated list of phase numbers without PLAN.md files
# Return code: 0 on success
get_unplanned_phases() {
    local all_phases
    all_phases=$(get_all_phases)

    if [[ -z "$all_phases" ]]; then
        return 0
    fi

    local unplanned=""
    for phase in $all_phases; do
        if ! phase_has_plans "$phase"; then
            if [[ -n "$unplanned" ]]; then
                unplanned="$unplanned $phase"
            else
                unplanned="$phase"
            fi
        fi
    done

    echo "$unplanned"
    return 0
}

# count_phase_plans - Count PLAN.md files for a phase
# Args: phase_num
# Returns: Number of PLAN.md files (prints to stdout)
# Return code: 0 on success
count_phase_plans() {
    local phase_num="$1"

    if [[ -z "$phase_num" ]]; then
        echo "0"
        return 0
    fi

    # Pad to 2 digits for directory matching
    local padded
    local clean_num
    # Handle potential non-numeric input
    if [[ "$phase_num" =~ ^[0-9]+$ ]]; then
        clean_num=$((10#$phase_num))
    else
        clean_num="$phase_num"
    fi
    padded=$(printf "%02d" "$clean_num" 2>/dev/null || echo "$phase_num")

    # Count PLAN.md files
    local count
    count=$(ls .planning/phases/${padded}-*/*-PLAN.md 2>/dev/null | wc -l | tr -d ' ')

    echo "$count"
    return 0
}
