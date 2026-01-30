#!/bin/bash
# GSD Path Resolution - Runtime environment detection and path conversion
# Part of Phase 13: Terminal Path Resolution
#
# Detects which bash variant is running (Git Bash, WSL, Cygwin) and provides
# path conversion functions that work across all environments.
#
# Usage:
#   source path-resolve.sh
#   env=$(detect_bash_env)
#   unix_path=$(resolve_win_path "C:\Users\foo")

# =============================================================================
# Environment Detection
# =============================================================================

# detect_bash_env - Identify which bash variant is running
# Returns: "msys" (Git Bash/MSYS2), "wsl", "cygwin", or "unknown"
detect_bash_env() {
  # WSL sets WSL_DISTRO_NAME
  if [ -n "$WSL_DISTRO_NAME" ]; then
    echo "wsl"
    return 0
  fi

  # MSYS2/Git Bash sets MSYSTEM (MINGW64, MINGW32, MSYS, UCRT64, CLANG64)
  if [ -n "$MSYSTEM" ]; then
    echo "msys"
    return 0
  fi

  # Cygwin has distinct OSTYPE
  if [[ "$OSTYPE" == cygwin* ]]; then
    echo "cygwin"
    return 0
  fi

  # Native Linux/macOS or unknown Windows shell
  echo "unknown"
  return 0
}

# =============================================================================
# Path Resolution
# =============================================================================

# resolve_win_path - Convert Windows path to Unix format for current environment
# Args: $1 - Windows path (e.g., "C:\Users\foo" or "C:/Users/foo")
# Returns: Unix path appropriate for current bash variant
# Exit code: 0 on success, 1 if path cannot be resolved
resolve_win_path() {
  local win_path="$1"

  # Empty path - return empty
  if [ -z "$win_path" ]; then
    echo ""
    return 0
  fi

  # Already a Unix path (starts with /) - return as-is
  if [[ "$win_path" == /* ]]; then
    echo "$win_path"
    return 0
  fi

  # Try cygpath first (available in Git Bash, MSYS2, Cygwin)
  if command -v cygpath >/dev/null 2>&1; then
    local converted
    converted=$(cygpath -u "$win_path" 2>/dev/null)
    if [ $? -eq 0 ] && [ -n "$converted" ]; then
      echo "$converted"
      return 0
    fi
  fi

  # Try wslpath (available in WSL)
  if command -v wslpath >/dev/null 2>&1; then
    local converted
    converted=$(wslpath -u "$win_path" 2>/dev/null)
    if [ $? -eq 0 ] && [ -n "$converted" ]; then
      echo "$converted"
      return 0
    fi
  fi

  # Manual fallback: C:\foo or C:/foo -> detect mount prefix
  if [[ "$win_path" =~ ^([A-Za-z]):[/\\](.*)$ ]]; then
    local drive="${BASH_REMATCH[1],,}"  # lowercase
    local rest="${BASH_REMATCH[2]}"
    rest="${rest//\\//}"  # backslash to forward slash

    # Try each mount prefix format
    # WSL format: /mnt/c/...
    if [ -d "/mnt/$drive" ]; then
      echo "/mnt/$drive/$rest"
      return 0
    fi

    # Git Bash format: /c/...
    if [ -d "/$drive" ]; then
      echo "/$drive/$rest"
      return 0
    fi

    # Cygwin format: /cygdrive/c/...
    if [ -d "/cygdrive/$drive" ]; then
      echo "/cygdrive/$drive/$rest"
      return 0
    fi
  fi

  # Could not resolve - return original and signal failure
  echo "$win_path"
  return 1
}

# resolve_and_cd - Resolve path and change directory
# Args: $1 - Windows or Unix path
# Returns: 0 on success, 1 on failure
resolve_and_cd() {
  local target="$1"

  # Already a valid directory?
  if [ -d "$target" ]; then
    cd "$target" && return 0
  fi

  # Try to resolve the path
  local resolved
  resolved=$(resolve_win_path "$target")

  if [ -d "$resolved" ]; then
    cd "$resolved" && return 0
  fi

  # Failed to resolve
  echo "ERROR: Could not resolve path: $target" >&2
  echo "  Tried: $resolved" >&2
  echo "  Environment: $(detect_bash_env)" >&2
  return 1
}

# =============================================================================
# Diagnostics
# =============================================================================

# path_resolve_diag - Print diagnostic information about path resolution
# Useful for troubleshooting when paths don't resolve correctly
path_resolve_diag() {
  echo "=== Path Resolution Diagnostics ==="
  echo ""
  echo "Environment:"
  echo "  OSTYPE:          ${OSTYPE:-not set}"
  echo "  MSYSTEM:         ${MSYSTEM:-not set}"
  echo "  WSL_DISTRO_NAME: ${WSL_DISTRO_NAME:-not set}"
  echo "  Detected env:    $(detect_bash_env)"
  echo ""
  echo "Available tools:"
  echo "  cygpath:         $(command -v cygpath 2>/dev/null || echo 'not found')"
  echo "  wslpath:         $(command -v wslpath 2>/dev/null || echo 'not found')"
  echo ""
  echo "Mount points:"
  echo "  /mnt/c:          $([ -d /mnt/c ] && echo 'exists' || echo 'not found')"
  echo "  /c:              $([ -d /c ] && echo 'exists' || echo 'not found')"
  echo "  /cygdrive/c:     $([ -d /cygdrive/c ] && echo 'exists' || echo 'not found')"
  echo ""

  # Test path resolution if argument provided
  if [ -n "$1" ]; then
    echo "Test path: $1"
    local resolved
    resolved=$(resolve_win_path "$1")
    local exit_code=$?
    echo "  Resolved to: $resolved"
    echo "  Exit code:   $exit_code"
    echo "  Is directory: $([ -d "$resolved" ] && echo 'yes' || echo 'no')"
  fi

  echo "==================================="
}

# CLI support - run directly for testing
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  path_resolve_diag "$@"
fi
