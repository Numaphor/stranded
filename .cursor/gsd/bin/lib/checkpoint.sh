#!/bin/bash
# GSD Ralph - Git Checkpointing
# Part of Phase 4: Git Checkpointing
#
# Provides:
#   validate_git_state       - Pre-flight validation at startup
#   create_checkpoint_commit - Commit after successful iteration

# Color codes (same as display.sh for consistency)
# Respect NO_COLOR standard (https://no-color.org/)
if [[ -n "${NO_COLOR:-}" ]]; then
    RED=''
    GREEN=''
    YELLOW=''
    RESET=''
else
    RED='\e[31m'
    GREEN='\e[32m'
    YELLOW='\e[33m'
    RESET='\e[0m'
fi

# validate_git_state - Pre-flight validation at startup
# Ensures clean git state before entering main loop
# Returns: 0 on success, 1 if git state is not acceptable
validate_git_state() {
    # Check if in git repo
    if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
        # Not in a git repo
        if [[ -t 0 ]]; then
            # Interactive mode - offer to initialize
            echo -e "${YELLOW}Not in a git repository${RESET}"
            read -p "Initialize git repository? [y/N]: " response
            if [[ "$response" =~ ^[Yy]$ ]]; then
                git init
                echo -e "${GREEN}Git repository initialized${RESET}"
            else
                echo -e "${RED}Error: Cannot proceed without a git repository${RESET}"
                return 1
            fi
        else
            # Non-interactive mode - error
            echo -e "${RED}Error: Not in a git repository${RESET}"
            return 1
        fi
    fi

    # Check for uncommitted changes
    if [[ -n "$(git status --porcelain 2>/dev/null)" ]]; then
        echo -e "${RED}Error: Working tree has uncommitted changes${RESET}"
        echo "Please commit or stash your changes before running Ralph:"
        git status --short
        return 1
    fi

    # Check for detached HEAD (warning only)
    if ! git symbolic-ref -q HEAD > /dev/null 2>&1; then
        echo -e "${YELLOW}Warning: HEAD is detached (commits will still work)${RESET}"
    fi

    return 0
}

# create_checkpoint_commit - Commit after successful iteration
# Args: task_id, summary
# Returns: 0 on success, 1 on failure (FATAL)
create_checkpoint_commit() {
    local task_id="$1"
    local summary="$2"

    # Stage STATE.md
    git add .planning/STATE.md 2>/dev/null

    # Check if anything is staged
    if git diff --cached --quiet 2>/dev/null; then
        echo -e "${YELLOW}No changes to commit after task $task_id${RESET}"
        return 0
    fi

    # Build commit message
    local commit_msg
    commit_msg="Ralph checkpoint: ${task_id} complete

${summary}"

    # Create commit
    if ! git commit -m "$commit_msg" > /dev/null 2>&1; then
        echo -e "${RED}Error: Failed to create checkpoint commit${RESET}"
        return 1
    fi

    # Print confirmation
    local short_sha
    short_sha=$(git rev-parse --short HEAD)
    echo -e "${GREEN}Checkpoint: $short_sha ($task_id)${RESET}"

    return 0
}

# get_last_checkpoint_task - Extract last completed task from git history
# Searches for most recent Ralph checkpoint commit
# Returns: task ID (e.g., "04-01") or empty string if no checkpoints
# Return code: 0 (always succeeds)
get_last_checkpoint_task() {
    # Search for most recent Ralph checkpoint commit
    local commit_line
    commit_line=$(git log --oneline --grep="Ralph checkpoint:" -1 2>/dev/null)

    if [[ -z "$commit_line" ]]; then
        # No checkpoint commits found
        echo ""
        return 0
    fi

    # Extract task ID using pattern NN-MM
    local task_id
    task_id=$(echo "$commit_line" | grep -oE '[0-9]{2}-[0-9]{2}' | head -1)

    echo "$task_id"
    return 0
}

# validate_state_against_history - Compare STATE.md with git checkpoint history
# Ensures STATE.md position agrees with last checkpoint commit
# Returns: 0 on success/no conflict, 1 on conflict (user should restart)
validate_state_against_history() {
    # Get current task from STATE.md
    local state_task
    state_task=$(parse_next_task)

    # Get last completed task from git
    local git_task
    git_task=$(get_last_checkpoint_task)

    # If no git history, nothing to validate against
    if [[ -z "$git_task" ]]; then
        return 0
    fi

    # If state shows COMPLETE, no conflict possible
    if [[ "$state_task" == "COMPLETE" ]]; then
        return 0
    fi

    # Extract phase and plan numbers for comparison
    local state_phase="${state_task%%-*}"
    local state_plan="${state_task##*-}"
    local git_phase="${git_task%%-*}"
    local git_plan="${git_task##*-}"

    # Remove leading zeros for numeric comparison
    state_phase=$((10#$state_phase))
    state_plan=$((10#$state_plan))
    git_phase=$((10#$git_phase))
    git_plan=$((10#$git_plan))

    # Check for conflict: STATE.md next task appears BEFORE git's last completed
    # This means STATE.md is behind git history (e.g., STATE says do 04-01, but git shows 04-02 complete)
    local conflict=false
    if [[ $state_phase -lt $git_phase ]]; then
        conflict=true
    elif [[ $state_phase -eq $git_phase && $state_plan -le $git_plan ]]; then
        conflict=true
    fi

    if [[ "$conflict" == "true" ]]; then
        if [[ -t 0 ]]; then
            # Interactive mode - prompt user
            echo -e "${YELLOW}STATE.md and git history conflict${RESET}"
            echo "STATE.md next task: $state_task"
            echo "Git last completed: $git_task"
            echo ""
            read -p "Trust [s]tate or [g]it history? " response
            case "$response" in
                [Ss])
                    # User trusts STATE.md
                    return 0
                    ;;
                [Gg])
                    # User trusts git history
                    # Calculate next task after git_task
                    local next_after_git
                    next_after_git=$(get_next_plan_after "$git_task")
                    if [[ "$next_after_git" == "COMPLETE" ]]; then
                        echo "All plans are complete according to git history."
                    else
                        echo "Run: ./bin/ralph.sh --start-from $next_after_git"
                    fi
                    return 1
                    ;;
                *)
                    echo -e "${YELLOW}Invalid choice. Aborting.${RESET}"
                    return 1
                    ;;
            esac
        else
            # Non-interactive mode - fail safe
            echo -e "${YELLOW}STATE.md and git history conflict${RESET}"
            echo "STATE.md next task: $state_task"
            echo "Git last completed: $git_task"
            echo -e "${RED}Cannot resolve conflict in non-interactive mode${RESET}"
            return 1
        fi
    fi

    # No conflict - STATE.md is ahead of or matches git history
    return 0
}
