#!/bin/bash
# GSD Ralph - Planning Session Management
# Part of Phase 8: Upfront Planning
#
# Provides functions for multi-phase planning sessions.
# Functions: init_planning_session, show_planning_progress, get_dependent_plans,
#            warn_if_has_dependents, get_planning_summary, commit_phase_plans
#
# Usage:
#   source bin/lib/planning.sh
#   init_planning_session
#   show_planning_progress "1" "10" "Safety Foundation"
#   get_planning_summary

# Configuration
MAX_PLAN_RETRIES=3

# Color codes (respect NO_COLOR standard)
if [[ -n "${NO_COLOR:-}" ]]; then
    PLAN_RED=''
    PLAN_GREEN=''
    PLAN_YELLOW=''
    PLAN_CYAN=''
    PLAN_RESET=''
    PLAN_BOLD=''
else
    PLAN_RED='\e[31m'
    PLAN_GREEN='\e[32m'
    PLAN_YELLOW='\e[33m'
    PLAN_CYAN='\e[36m'
    PLAN_RESET='\e[0m'
    PLAN_BOLD='\e[1m'
fi

# Source dependencies if not already loaded
# Use type check for safe optional dependency loading
if ! type init_planning_progress &>/dev/null; then
    if [[ -f "${BASH_SOURCE[0]%/*}/state.sh" ]]; then
        source "${BASH_SOURCE[0]%/*}/state.sh"
    fi
fi

if ! type get_all_phases &>/dev/null; then
    if [[ -f "${BASH_SOURCE[0]%/*}/parse.sh" ]]; then
        source "${BASH_SOURCE[0]%/*}/parse.sh"
    fi
fi

# =============================================================================
# Session Management Functions
# =============================================================================

# init_planning_session - Initialize a planning session
# Returns: Session ID via stdout
# Return code: 0 on success, 1 on failure
# Creates planning progress section and sets session to in_progress
init_planning_session() {
    # Call init_planning_progress from state.sh
    if type init_planning_progress &>/dev/null; then
        init_planning_progress
        if [[ $? -ne 0 ]]; then
            echo -e "${PLAN_RED}Error: Failed to initialize planning progress${PLAN_RESET}" >&2
            return 1
        fi
    fi

    # Generate session ID
    local session_id
    session_id="planning-$(date '+%Y-%m-%d-%H%M')"

    # Update session info
    if type set_planning_session &>/dev/null; then
        set_planning_session "$session_id" "in_progress"
        if [[ $? -ne 0 ]]; then
            echo -e "${PLAN_RED}Error: Failed to set planning session${PLAN_RESET}" >&2
            return 1
        fi
    fi

    echo "$session_id"
    return 0
}

# =============================================================================
# Display Functions
# =============================================================================

# show_planning_progress - Display current planning state
# Args: current_phase, total_phases, phase_name
# Returns: 0 always
# Displays a visual banner with planning progress
show_planning_progress() {
    local current="$1"
    local total="$2"
    local name="$3"

    echo -e "${PLAN_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${PLAN_RESET}"
    echo -e "${PLAN_CYAN} GSD > PLANNING PHASE ${current}/${total}${PLAN_RESET}"
    echo -e "${PLAN_CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${PLAN_RESET}"
    echo ""
    echo -e "Phase: ${PLAN_BOLD}${name}${PLAN_RESET}"
    echo ""
    return 0
}

# =============================================================================
# Dependency Management Functions
# =============================================================================

# get_dependent_plans - Find plans that depend on a given plan
# Args: plan_id (e.g., "08-01")
# Returns: List of dependent plan IDs via stdout
# Searches all PLAN.md frontmatter for depends_on containing this plan
get_dependent_plans() {
    local plan_id="$1"

    if [[ -z "$plan_id" ]]; then
        echo -e "${PLAN_RED}Error: get_dependent_plans requires plan_id${PLAN_RESET}" >&2
        return 1
    fi

    # Search all PLAN.md files for depends_on containing this plan
    local dependents
    dependents=$(grep -l "depends_on:.*${plan_id}" .planning/phases/*/*-PLAN.md 2>/dev/null | \
        while read -r file; do
            basename "$file" 2>/dev/null | sed 's/-PLAN\.md//'
        done)

    echo "$dependents"
    return 0
}

# warn_if_has_dependents - Warn if plan has dependents
# Args: plan_id
# Returns: 0 if no dependents, 1 if has dependents
# Prints warning to stderr if dependents exist
warn_if_has_dependents() {
    local plan_id="$1"

    if [[ -z "$plan_id" ]]; then
        echo -e "${PLAN_RED}Error: warn_if_has_dependents requires plan_id${PLAN_RESET}" >&2
        return 1
    fi

    local dependents
    dependents=$(get_dependent_plans "$plan_id")

    if [[ -n "$dependents" ]]; then
        echo -e "${PLAN_YELLOW}Warning: Plan $plan_id has dependent plans:${PLAN_RESET}" >&2
        echo "$dependents" | while read -r dep; do
            if [[ -n "$dep" ]]; then
                echo -e "  ${PLAN_YELLOW}- $dep${PLAN_RESET}" >&2
            fi
        done
        echo -e "${PLAN_YELLOW}Modifying this plan may affect downstream plans.${PLAN_RESET}" >&2
        return 1
    fi

    return 0
}

# =============================================================================
# Summary Functions
# =============================================================================

# get_planning_summary - Generate summary table of all phases
# Returns: Formatted markdown table via stdout
# Shows phase, name, plan count, and status for each phase
get_planning_summary() {
    echo "| Phase | Name | Plans | Status |"
    echo "|-------|------|-------|--------|"

    local all_phases
    if type get_all_phases &>/dev/null; then
        all_phases=$(get_all_phases)
    else
        all_phases=""
    fi

    for phase in $all_phases; do
        local name
        local count
        local status="pending"

        # Get phase name
        if type get_phase_name &>/dev/null; then
            name=$(get_phase_name "$phase")
        else
            name="Phase $phase"
        fi

        # Get plan count
        if type count_phase_plans &>/dev/null; then
            count=$(count_phase_plans "$phase")
        else
            count="0"
        fi

        # Determine status based on plan count
        if [[ "$count" -gt 0 ]]; then
            status="complete"
        fi

        echo "| $phase | $name | $count | $status |"
    done

    return 0
}

# =============================================================================
# Git Integration Functions
# =============================================================================

# commit_phase_plans - Git commit plans for a phase
# Args: phase_num
# Returns: 0 on success, 1 on failure
# Stages and commits all PLAN.md files for the specified phase
commit_phase_plans() {
    local phase_num="$1"

    if [[ -z "$phase_num" ]]; then
        echo -e "${PLAN_RED}Error: commit_phase_plans requires phase_num${PLAN_RESET}" >&2
        return 1
    fi

    # Pad to 2 digits
    local padded
    local clean_num
    if [[ "$phase_num" =~ ^[0-9]+$ ]]; then
        clean_num=$((10#$phase_num))
    else
        clean_num="$phase_num"
    fi
    padded=$(printf "%02d" "$clean_num" 2>/dev/null || echo "$phase_num")

    # Find phase directory
    local phase_dir
    phase_dir=$(ls -d .planning/phases/${padded}-* 2>/dev/null | head -1)

    if [[ ! -d "$phase_dir" ]]; then
        echo -e "${PLAN_YELLOW}Warning: No phase directory found for phase $phase_num${PLAN_RESET}" >&2
        return 1
    fi

    # Check if there are PLAN.md files to commit
    local plan_files
    plan_files=$(ls "${phase_dir}"/*-PLAN.md 2>/dev/null)

    if [[ -z "$plan_files" ]]; then
        echo -e "${PLAN_YELLOW}Warning: No PLAN.md files found in $phase_dir${PLAN_RESET}" >&2
        return 1
    fi

    # Stage PLAN.md files
    git add "${phase_dir}"/*-PLAN.md 2>/dev/null

    if [[ $? -ne 0 ]]; then
        echo -e "${PLAN_RED}Error: Failed to stage PLAN.md files${PLAN_RESET}" >&2
        return 1
    fi

    # Get phase name for commit message
    local phase_name=""
    if type get_phase_name &>/dev/null; then
        phase_name=$(get_phase_name "$phase_num")
    fi

    # Commit with standardized message
    local commit_msg="docs(${padded}): create phase plans"
    if [[ -n "$phase_name" ]]; then
        commit_msg="docs(${padded}): create ${phase_name} phase plans"
    fi

    git commit -m "$commit_msg" 2>/dev/null

    if [[ $? -ne 0 ]]; then
        echo -e "${PLAN_RED}Error: Failed to commit PLAN.md files${PLAN_RESET}" >&2
        return 1
    fi

    echo -e "${PLAN_GREEN}Committed plans for phase $phase_num${PLAN_RESET}"
    return 0
}
