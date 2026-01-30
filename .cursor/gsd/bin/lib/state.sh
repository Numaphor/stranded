#!/bin/bash
# GSD Ralph - State Management
# Part of Phase 2: State Extensions (extended in Phase 8)
#
# Provides STATE.md manipulation functions for the ralph loop.
# Functions: atomic_write, update_section, update_current_position,
#            update_next_action, add_iteration_entry, get_iteration_count,
#            log_exit_status, init_planning_progress, update_planning_progress,
#            set_planning_session, get_planning_status
#
# Usage:
#   source bin/lib/state.sh
#   add_iteration_entry 1 "SUCCESS" "02-01: Schema extensions"
#   update_next_action 2 "02-02" "Progress indicator"
#   init_planning_progress
#   update_planning_progress "08" "in_progress" "2"

# Configuration
STATE_FILE="${STATE_FILE:-.planning/STATE.md}"

# Color codes for error messages
STATE_RED='\e[31m'
STATE_GREEN='\e[32m'
STATE_YELLOW='\e[33m'
STATE_RESET='\e[0m'

# atomic_write - Write content to target file atomically
# Args: target (filepath), content (string)
# Returns: 0 on success, 1 on failure
# Uses write-to-temp-then-rename pattern for crash safety
atomic_write() {
    local target="$1"
    local content="$2"

    if [[ -z "$target" ]]; then
        echo -e "${STATE_RED}Error: atomic_write requires target path${STATE_RESET}" >&2
        return 1
    fi

    # Create temp file in same directory (same filesystem for atomic mv)
    local temp="${target}.tmp.$$"

    # Write content to temp file
    printf '%s' "$content" > "$temp"
    local write_result=$?

    if [[ $write_result -ne 0 ]]; then
        echo -e "${STATE_RED}Error: Failed to write to temp file${STATE_RESET}" >&2
        rm -f "$temp" 2>/dev/null
        return 1
    fi

    # Atomic rename
    mv "$temp" "$target"
    local mv_result=$?

    if [[ $mv_result -ne 0 ]]; then
        echo -e "${STATE_RED}Error: Failed to rename temp file to target${STATE_RESET}" >&2
        rm -f "$temp" 2>/dev/null
        return 1
    fi

    return 0
}

# update_section - Replace content between markers
# Args: file (filepath), start_marker (string), end_marker (string), new_content (string)
# Returns: 0 on success, 1 on failure
# Content between markers is replaced; markers are preserved
update_section() {
    local file="$1"
    local start_marker="$2"
    local end_marker="$3"
    local new_content="$4"

    if [[ ! -f "$file" ]]; then
        echo -e "${STATE_RED}Error: File not found: $file${STATE_RESET}" >&2
        return 1
    fi

    # Verify markers exist in file
    if ! grep -q "$start_marker" "$file"; then
        echo -e "${STATE_RED}Error: Start marker not found: $start_marker${STATE_RESET}" >&2
        return 1
    fi

    if ! grep -q "$end_marker" "$file"; then
        echo -e "${STATE_RED}Error: End marker not found: $end_marker${STATE_RESET}" >&2
        return 1
    fi

    # Create temp file
    local temp
    temp=$(mktemp)

    # Use awk to replace content between markers
    # - Print everything up to and including start marker
    # - Print new content
    # - Skip everything until end marker (exclusive)
    # - Print end marker and everything after
    awk -v start="$start_marker" -v end="$end_marker" -v content="$new_content" '
        BEGIN { in_section = 0 }
        index($0, start) > 0 {
            print
            print content
            in_section = 1
            next
        }
        index($0, end) > 0 {
            in_section = 0
        }
        !in_section { print }
    ' "$file" > "$temp"

    # Atomic replace
    mv "$temp" "$file"
    return $?
}

# update_current_position - Update the Current Position section
# Args: phase_num, plan_num, phase_name, status
# Returns: 0 on success, 1 on failure
# Updates Phase, Plan, Status, and Last activity lines (not Progress)
update_current_position() {
    local phase_num="$1"
    local plan_num="$2"
    local phase_name="$3"
    local status="$4"

    if [[ -z "$phase_num" || -z "$plan_num" || -z "$phase_name" || -z "$status" ]]; then
        echo -e "${STATE_RED}Error: update_current_position requires phase_num, plan_num, phase_name, status${STATE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    # Create temp file
    local temp
    temp=$(mktemp)

    # Use awk to update specific lines in Current Position section
    awk -v phase="$phase_num" -v plan="$plan_num" -v pname="$phase_name" -v stat="$status" -v ts="$timestamp" '
        /^Phase: / && /of [0-9]+ \(/ {
            # Match "Phase: X of Y (Name)" pattern
            match($0, /of ([0-9]+)/, arr)
            total = arr[1]
            if (total == "") total = "10"
            print "Phase: " phase " of " total " (" pname ")"
            next
        }
        /^Plan: / && /of [0-9]+ in current phase/ {
            # Match "Plan: X of Y in current phase" pattern
            match($0, /of ([0-9]+) in/, arr)
            total = arr[1]
            if (total == "") total = "2"
            print "Plan: " plan " of " total " in current phase"
            next
        }
        /^Status: / {
            print "Status: " stat
            next
        }
        /^Last activity: / {
            print "Last activity: " ts
            next
        }
        { print }
    ' "$STATE_FILE" > "$temp"

    # Atomic replace
    mv "$temp" "$STATE_FILE"
    return $?
}

# update_next_action - Update the Next Action section
# Args: phase_num, plan_id (e.g., "02-02"), plan_name, [last_failure_note]
# Returns: 0 on success, 1 on failure
update_next_action() {
    local phase_num="$1"
    local plan_id="$2"
    local plan_name="$3"
    local last_failure_note="${4:-}"

    if [[ -z "$phase_num" || -z "$plan_id" || -z "$plan_name" ]]; then
        echo -e "${STATE_RED}Error: update_next_action requires phase_num, plan_id, plan_name${STATE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Create temp file
    local temp
    temp=$(mktemp)

    # Build the new Next Action content
    local note_line=""
    if [[ -n "$last_failure_note" ]]; then
        note_line="
**Note:** $last_failure_note"
    fi

    # Use awk to replace the Next Action section content
    # We need to match lines between "## Next Action" and the next "##" section
    awk -v phase="$phase_num" -v planid="$plan_id" -v pname="$plan_name" -v note="$note_line" '
        BEGIN { in_section = 0 }
        /^## Next Action/ {
            print
            print ""
            print "Command: /gsd:execute-phase " phase
            print "Description: Execute plan " planid " (" pname ")"
            print "Read: ROADMAP.md, " planid "-PLAN.md"
            if (note != "") print note
            in_section = 1
            next
        }
        /^## / && in_section {
            in_section = 0
            print ""
            print
            next
        }
        !in_section { print }
    ' "$STATE_FILE" > "$temp"

    # Atomic replace
    mv "$temp" "$STATE_FILE"
    return $?
}

# add_iteration_entry - Add entry to Iteration History
# Args: iteration_num, outcome (SUCCESS/FAILURE/RETRY/SKIPPED), task_name
# Returns: 0 on success, 1 on failure
# Prepends new entry (newest first), preserves table header and markers
# Creates Iteration History section if it doesn't exist
add_iteration_entry() {
    local iteration_num="$1"
    local outcome="$2"
    local task_name="$3"

    if [[ -z "$iteration_num" || -z "$outcome" || -z "$task_name" ]]; then
        echo -e "${STATE_RED}Error: add_iteration_entry requires iteration_num, outcome, task_name${STATE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Check if history markers exist, create section if missing
    if ! grep -q "<!-- HISTORY_START -->" "$STATE_FILE" 2>/dev/null; then
        _init_iteration_history_section
    fi

    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    # Format the new entry
    local entry="| $iteration_num | $timestamp | $outcome | $task_name |"

    # Extract existing entries (lines starting with | followed by number)
    local existing_entries
    existing_entries=$(sed -n '/<!-- HISTORY_START -->/,/<!-- HISTORY_END -->/{
        /^| [0-9]/p
    }' "$STATE_FILE")

    # Build new history content (table header + new entry + existing entries)
    local new_history="| # | Timestamp | Outcome | Task |
|---|-----------|---------|------|
$entry"

    # Append existing entries if any
    if [[ -n "$existing_entries" ]]; then
        new_history="$new_history
$existing_entries"
    fi

    # Update the section between markers
    update_section "$STATE_FILE" "<!-- HISTORY_START -->" "<!-- HISTORY_END -->" "$new_history"
    return $?
}

# _init_iteration_history_section - Create Iteration History section if missing
# Returns: 0 on success, 1 on failure
# Inserts section before ## Session Continuity (or appends to end)
_init_iteration_history_section() {
    if [[ ! -f "$STATE_FILE" ]]; then
        return 1
    fi

    # Create temp file
    local temp
    temp=$(mktemp)

    # Insert Iteration History section before ## Session Continuity
    # If that section doesn't exist, append to end of file
    if grep -q "^## Session Continuity" "$STATE_FILE" 2>/dev/null; then
        awk '
            /^## Session Continuity/ {
                print "## Iteration History"
                print ""
                print "<!-- HISTORY_START -->"
                print "| # | Timestamp | Outcome | Task |"
                print "|---|-----------|---------|------|"
                print "<!-- HISTORY_END -->"
                print ""
            }
            { print }
        ' "$STATE_FILE" > "$temp"
    else
        # Append to end of file
        cp "$STATE_FILE" "$temp"
        {
            echo ""
            echo "## Iteration History"
            echo ""
            echo "<!-- HISTORY_START -->"
            echo "| # | Timestamp | Outcome | Task |"
            echo "|---|-----------|---------|------|"
            echo "<!-- HISTORY_END -->"
        } >> "$temp"
    fi

    # Atomic replace
    mv "$temp" "$STATE_FILE"
    return $?
}

# get_iteration_count - Get count of entries in Iteration History
# Returns: Number of iteration entries (prints to stdout)
# Return code: 0 on success, 1 on failure
get_iteration_count() {
    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Count lines that match iteration entry pattern (| number | ...)
    local count
    count=$(sed -n '/<!-- HISTORY_START -->/,/<!-- HISTORY_END -->/{
        /^| [0-9]/p
    }' "$STATE_FILE" | wc -l)

    # Trim whitespace
    count=$(echo "$count" | tr -d ' ')

    echo "$count"
    return 0
}

# =============================================================================
# History Management Functions (Plan 02-02)
# =============================================================================

# Configuration for history rolling
HISTORY_WINDOW=15
ARCHIVE_FILE="${ARCHIVE_FILE:-.planning/iteration-history.md}"

# get_history_entry_count - Count entries in Iteration History
# Returns: Number of history entries (prints to stdout)
# Return code: 0 on success, 1 on failure
# Note: This is an alias for get_iteration_count but named for clarity
get_history_entry_count() {
    get_iteration_count
}

# rotate_history_at_phase_boundary - Archive old entries at phase boundaries
# Args: current_phase (string, e.g., "Phase 2" or "02")
# Returns: 0 on success, 1 on failure
# Archives entries beyond HISTORY_WINDOW to iteration-history.md
rotate_history_at_phase_boundary() {
    local current_phase="$1"

    if [[ -z "$current_phase" ]]; then
        echo -e "${STATE_RED}Error: rotate_history_at_phase_boundary requires current_phase${STATE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Ensure archive file exists with header
    if [[ ! -f "$ARCHIVE_FILE" ]]; then
        _init_archive_file
    fi

    # Get current entry count
    local entry_count
    entry_count=$(get_history_entry_count)

    # If within window, nothing to do
    if [[ "$entry_count" -le "$HISTORY_WINDOW" ]]; then
        return 0
    fi

    # Extract all entries (lines starting with | followed by number)
    local all_entries
    all_entries=$(sed -n '/<!-- HISTORY_START -->/,/<!-- HISTORY_END -->/{
        /^| [0-9]/p
    }' "$STATE_FILE")

    # Calculate how many entries to archive
    local entries_to_archive=$((entry_count - HISTORY_WINDOW))

    # Split entries: keep recent (first HISTORY_WINDOW), archive old (rest)
    # Note: newest entries are first (prepend pattern), so we keep the first N
    local recent_entries
    local archived_entries

    recent_entries=$(echo "$all_entries" | head -n "$HISTORY_WINDOW")
    archived_entries=$(echo "$all_entries" | tail -n "$entries_to_archive")

    # Append archived entries to archive file with phase boundary marker
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    {
        echo ""
        echo "## Phase $current_phase Boundary - $timestamp"
        echo ""
        echo "| # | Timestamp | Outcome | Task |"
        echo "|---|-----------|---------|------|"
        echo "$archived_entries"
    } >> "$ARCHIVE_FILE"

    # Update STATE.md with only recent entries
    local new_history="| # | Timestamp | Outcome | Task |
|---|-----------|---------|------|"

    if [[ -n "$recent_entries" ]]; then
        new_history="$new_history
$recent_entries"
    fi

    update_section "$STATE_FILE" "<!-- HISTORY_START -->" "<!-- HISTORY_END -->" "$new_history"
    return $?
}

# _init_archive_file - Create archive file with initial header (internal function)
# Returns: 0 on success, 1 on failure
_init_archive_file() {
    # Ensure directory exists
    local archive_dir
    archive_dir=$(dirname "$ARCHIVE_FILE")
    if [[ ! -d "$archive_dir" ]]; then
        mkdir -p "$archive_dir"
    fi

    # Create archive file with header
    cat > "$ARCHIVE_FILE" << 'EOF'
# Iteration History Archive

Entries rolled off from STATE.md at phase boundaries.
Git preserves full history; this file is for quick reference.

---
EOF
    return $?
}

# =============================================================================
# Progress Bar Functions (Plan 02-02)
# =============================================================================

# Configuration for progress bar
PROGRESS_WIDTH=30

# generate_progress_bar - Generate ASCII progress bar
# Args: completed (int), total (int), [width (int)]
# Returns: Bar string via stdout (e.g., "[##########                    ] 33%")
# Default width: 30 characters
generate_progress_bar() {
    local completed="$1"
    local total="$2"
    local width="${3:-$PROGRESS_WIDTH}"

    # Handle edge case: 0 total
    if [[ "$total" -eq 0 ]]; then
        printf '[%*s] 0%%\n' "$width" ""
        return 0
    fi

    # Calculate percentage (integer division)
    local percent=$((completed * 100 / total))

    # Calculate filled portion
    local filled=$((completed * width / total))
    local empty=$((width - filled))

    # Build the bar string
    local bar=""
    for ((i = 0; i < filled; i++)); do
        bar+="#"
    done
    for ((i = 0; i < empty; i++)); do
        bar+=" "
    done

    printf '[%s] %d%%\n' "$bar" "$percent"
    return 0
}

# update_progress - Update Progress line in STATE.md
# Args: completed (int), total (int)
# Returns: 0 on success, 1 on failure
update_progress() {
    local completed="$1"
    local total="$2"

    if [[ -z "$completed" || -z "$total" ]]; then
        echo -e "${STATE_RED}Error: update_progress requires completed and total${STATE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Generate the progress bar
    local bar
    bar=$(generate_progress_bar "$completed" "$total")

    # Create temp file
    local temp
    temp=$(mktemp)

    # Use sed to replace the Progress line
    sed "s|^Progress: \[.*\] [0-9]*%$|Progress: $bar|" "$STATE_FILE" > "$temp"

    # Atomic replace
    mv "$temp" "$STATE_FILE"
    return $?
}

# get_plans_completed - Count completed plans from ROADMAP.md
# Returns: Number of completed plans (prints to stdout)
# Counts lines matching `- [x]` pattern in phase Plans sections
get_plans_completed() {
    local roadmap="${ROADMAP_FILE:-.planning/ROADMAP.md}"

    if [[ ! -f "$roadmap" ]]; then
        echo -e "${STATE_RED}Error: ROADMAP_FILE not found: $roadmap${STATE_RESET}" >&2
        return 1
    fi

    # Count lines matching "- [x]" pattern (completed plans)
    local count
    count=$(grep -c '^\- \[x\]' "$roadmap" 2>/dev/null) || true

    # Ensure count is a valid number (grep exits 1 on no matches)
    count="${count%%$'\n'*}"  # Take only first line
    count="${count//[^0-9]/}" # Remove non-digits
    count="${count:-0}"

    echo "$count"
    return 0
}

# get_total_plans - Count total plans from ROADMAP.md
# Returns: Number of total plans (prints to stdout)
# Counts lines matching `- [ ]` OR `- [x]` pattern
get_total_plans() {
    local roadmap="${ROADMAP_FILE:-.planning/ROADMAP.md}"

    if [[ ! -f "$roadmap" ]]; then
        echo -e "${STATE_RED}Error: ROADMAP_FILE not found: $roadmap${STATE_RESET}" >&2
        return 1
    fi

    # Count lines matching "- [ ]" or "- [x]" pattern (all plans)
    local count
    count=$(grep -cE '^\- \[(x| )\]' "$roadmap" 2>/dev/null) || true

    # Ensure count is a valid number (grep exits 1 on no matches)
    count="${count%%$'\n'*}"  # Take only first line
    count="${count//[^0-9]/}" # Remove non-digits
    count="${count:-0}"

    echo "$count"
    return 0
}

# =============================================================================
# Exit Logging Functions (Plan 05-01)
# =============================================================================

# log_exit_status - Log exit status to STATE.md
# Args: status (COMPLETED/STUCK/ABORTED/INTERRUPTED), reason, last_task, iteration_count, duration
# Returns: 0 on success, 1 on failure
# Updates Status line in Current Position section and adds final history entry
log_exit_status() {
    local status="$1"
    local reason="$2"
    local last_task="${3:-}"
    local iteration_count="${4:-}"
    local duration="${5:-}"

    if [[ -z "$status" || -z "$reason" ]]; then
        echo -e "${STATE_RED}Error: log_exit_status requires status and reason${STATE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Create temp file
    local temp
    temp=$(mktemp)

    # Update Status line using sed
    # Also update Last activity line with exit timestamp
    local timestamp
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')

    sed -e "s/^Status: .*/Status: $status/" \
        -e "s/^Last activity: .*/Last activity: $timestamp - $reason/" \
        "$STATE_FILE" > "$temp"

    # Atomic replace
    mv "$temp" "$STATE_FILE"

    # Add final history entry with exit status
    local task_desc="${last_task:-final}"
    local exit_msg="Exit: $status - $reason"
    if [[ -n "$iteration_count" ]]; then
        exit_msg="$exit_msg (${iteration_count} iterations"
        if [[ -n "$duration" ]]; then
            # Format duration if numeric
            if [[ "$duration" =~ ^[0-9]+$ ]]; then
                local hours=$((duration / 3600))
                local minutes=$(((duration % 3600) / 60))
                exit_msg="$exit_msg, ${hours}h ${minutes}m)"
            else
                exit_msg="$exit_msg, $duration)"
            fi
        else
            exit_msg="$exit_msg)"
        fi
    fi

    add_iteration_entry "$iteration_count" "$status" "$exit_msg"

    return 0
}

# =============================================================================
# Planning Progress Functions (Plan 08-01)
# =============================================================================

# init_planning_progress - Add planning progress section to STATE.md if not present
# Returns: 0 on success, 1 on failure
# Creates the Planning Progress section with markers after ## Next Action
init_planning_progress() {
    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Check if planning progress section already exists
    if grep -q "<!-- PLANNING_PROGRESS_START -->" "$STATE_FILE" 2>/dev/null; then
        return 0  # Already exists, nothing to do
    fi

    # Create temp file
    local temp
    temp=$(mktemp)

    # Insert planning progress section after ## Next Action section
    # We find the next ## after Next Action and insert before it
    awk '
        BEGIN { found_next_action = 0; inserted = 0 }
        /^## Next Action/ {
            found_next_action = 1
            print
            next
        }
        /^## / && found_next_action && !inserted {
            # Insert planning progress section before this section
            print ""
            print "## Planning Progress"
            print ""
            print "<!-- PLANNING_PROGRESS_START -->"
            print "**Session:** none"
            print "**Status:** not_started"
            print ""
            print "| Phase | Plans | Status | Generated |"
            print "|-------|-------|--------|-----------|"
            print "<!-- PLANNING_PROGRESS_END -->"
            print ""
            inserted = 1
        }
        { print }
    ' "$STATE_FILE" > "$temp"

    # Atomic replace
    mv "$temp" "$STATE_FILE"
    return $?
}

# update_planning_progress - Update planning progress for a phase
# Args: phase_num, status (pending|in_progress|complete|failed), [plan_count]
# Returns: 0 on success, 1 on failure
# Updates or appends the row for the specified phase in the progress table
update_planning_progress() {
    local phase_num="$1"
    local status="$2"
    local plan_count="${3:-0}"

    if [[ -z "$phase_num" || -z "$status" ]]; then
        echo -e "${STATE_RED}Error: update_planning_progress requires phase_num and status${STATE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Ensure planning progress section exists
    if ! grep -q "<!-- PLANNING_PROGRESS_START -->" "$STATE_FILE" 2>/dev/null; then
        init_planning_progress
    fi

    # Pad phase number for display (ensure clean numeric input)
    local padded
    local clean_num=$((10#$phase_num))  # Remove leading zeros, convert to decimal
    padded=$(printf "%02d" "$clean_num" 2>/dev/null || echo "$phase_num")

    # Get current timestamp for Generated column
    local timestamp
    timestamp=$(date '+%Y-%m-%d')

    # Create temp file
    local temp
    temp=$(mktemp)

    # Check if phase row already exists
    if grep -qE "^\| *${padded} *\|" "$STATE_FILE" 2>/dev/null || grep -qE "^\| *${phase_num} *\|" "$STATE_FILE" 2>/dev/null; then
        # Update existing row
        awk -v phase="$padded" -v status="$status" -v count="$plan_count" -v ts="$timestamp" '
            /^<!-- PLANNING_PROGRESS_START -->$/,/^<!-- PLANNING_PROGRESS_END -->$/ {
                if ($0 ~ "^\\| *" phase " *\\|" || $0 ~ "^\\| *" int(phase) " *\\|") {
                    print "| " phase " | " count " | " status " | " ts " |"
                    next
                }
            }
            { print }
        ' "$STATE_FILE" > "$temp"
    else
        # Append new row before PLANNING_PROGRESS_END marker
        awk -v phase="$padded" -v status="$status" -v count="$plan_count" -v ts="$timestamp" '
            /^<!-- PLANNING_PROGRESS_END -->$/ {
                print "| " phase " | " count " | " status " | " ts " |"
            }
            { print }
        ' "$STATE_FILE" > "$temp"
    fi

    # Atomic replace
    mv "$temp" "$STATE_FILE"
    return $?
}

# set_planning_session - Update session info in planning progress
# Args: session_id (e.g., "planning-2026-01-19"), status (not_started|in_progress|completed|needs_refinement)
# Returns: 0 on success, 1 on failure
# Updates the **Session:** and **Status:** lines
set_planning_session() {
    local session_id="$1"
    local status="$2"

    if [[ -z "$session_id" || -z "$status" ]]; then
        echo -e "${STATE_RED}Error: set_planning_session requires session_id and status${STATE_RESET}" >&2
        return 1
    fi

    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Ensure planning progress section exists
    if ! grep -q "<!-- PLANNING_PROGRESS_START -->" "$STATE_FILE" 2>/dev/null; then
        init_planning_progress
    fi

    # Create temp file
    local temp
    temp=$(mktemp)

    # Update Session and Status lines between planning markers
    awk -v session="$session_id" -v status="$status" '
        /^<!-- PLANNING_PROGRESS_START -->$/,/^<!-- PLANNING_PROGRESS_END -->$/ {
            if (/^\*\*Session:\*\*/) {
                print "**Session:** " session
                next
            }
            if (/^\*\*Status:\*\*/) {
                print "**Status:** " status
                next
            }
        }
        { print }
    ' "$STATE_FILE" > "$temp"

    # Atomic replace
    mv "$temp" "$STATE_FILE"
    return $?
}

# get_planning_status - Read current planning status
# Returns: Current session status (not_started|in_progress|completed|needs_refinement)
# Return code: 0 on success, 1 on failure
# Parses the **Status:** line between planning markers
get_planning_status() {
    if [[ ! -f "$STATE_FILE" ]]; then
        echo -e "${STATE_RED}Error: STATE_FILE not found: $STATE_FILE${STATE_RESET}" >&2
        return 1
    fi

    # Check if planning progress section exists
    if ! grep -q "<!-- PLANNING_PROGRESS_START -->" "$STATE_FILE" 2>/dev/null; then
        echo "not_started"
        return 0
    fi

    # Extract status from between planning markers
    local status
    status=$(sed -n '/<!-- PLANNING_PROGRESS_START -->/,/<!-- PLANNING_PROGRESS_END -->/{
        /^\*\*Status:\*\*/ {
            s/^\*\*Status:\*\* *//
            p
        }
    }' "$STATE_FILE")

    if [[ -z "$status" ]]; then
        echo "not_started"
    else
        echo "$status"
    fi
    return 0
}
