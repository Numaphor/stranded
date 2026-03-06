#!/usr/bin/env bash
# Launch mGBA with debugging features for the Stranded GBA project.
# Usage: launch_debug.sh [gdb|cli|log|lua]
#
#   gdb  - Start mGBA with GDB server on port 2345 (default)
#   cli  - Start mGBA with built-in command-line debugger
#   log  - Start mGBA with verbose logging (all log levels)
#   lua  - Start mGBA and auto-load the GBA debug Lua script

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# --- Detect WSL ---
IS_WSL=false
if [[ -n "${WSL_INTEROP:-}" || -n "${WSL_DISTRO_NAME:-}" ]]; then
    IS_WSL=true
elif [[ -f /proc/version ]] && grep -qi microsoft /proc/version 2>/dev/null; then
    IS_WSL=true
fi

# --- Resolve ROM path ---
ROM=""
if [[ -f "$PROJECT_DIR/stranded.gba" ]]; then
    ROM="$PROJECT_DIR/stranded.gba"
else
    BASENAME="$(basename "$PROJECT_DIR")"
    if [[ -f "$PROJECT_DIR/${BASENAME}.gba" ]]; then
        ROM="$PROJECT_DIR/${BASENAME}.gba"
    fi
fi

if [[ -z "$ROM" ]]; then
    echo "ERROR: No .gba ROM found in $PROJECT_DIR" >&2
    exit 1
fi

# --- Read emulator path from settings.json ---
SETTINGS_FILE="$PROJECT_DIR/.vscode/settings.json"
CONFIGURED_PATH=""
if [[ -f "$SETTINGS_FILE" ]]; then
    # Extract stranded.emulatorPath value (simple grep, no jq dependency)
    CONFIGURED_PATH="$(grep -oP '"stranded\.emulatorPath"\s*:\s*"\K[^"]+' "$SETTINGS_FILE" 2>/dev/null || true)"
    CONFIGURED_PATH="${CONFIGURED_PATH//\$\{workspaceFolder\}/$PROJECT_DIR}"
fi

# --- Resolve emulator and launch method ---
if [[ "$IS_WSL" == true ]]; then
    # In WSL: use Windows mGBA via cmd.exe, just like launch_emulator.js
    MGBA_POSIX="${CONFIGURED_PATH:-$PROJECT_DIR/tools/mGBA-0.10.5-win64/mGBA.exe}"

    if [[ ! -f "$MGBA_POSIX" ]]; then
        echo "ERROR: Windows mGBA not found at $MGBA_POSIX" >&2
        echo "Set stranded.emulatorPath in .vscode/settings.json" >&2
        exit 1
    fi

    MGBA_WIN="$(wslpath -w "$MGBA_POSIX")"
    ROM_WIN="$(wslpath -w "$ROM")"
    CMD_EXE=""
    if command -v cmd.exe &>/dev/null; then
        CMD_EXE="$(command -v cmd.exe)"
    elif [[ -x /mnt/c/Windows/System32/cmd.exe ]]; then
        CMD_EXE="/mnt/c/Windows/System32/cmd.exe"
    fi

    if [[ -z "$CMD_EXE" ]]; then
        echo "ERROR: cmd.exe not found. Windows interop may be disabled." >&2
        exit 1
    fi

    # --- Kill any existing Windows mGBA instance ---
    "$CMD_EXE" /c "taskkill /im mGBA.exe /F" 2>/dev/null || true
    sleep 0.3
else
    # Native Linux: find mgba-qt or mgba
    MGBA=""
    if command -v mgba-qt &>/dev/null; then
        MGBA="$(command -v mgba-qt)"
    elif [[ -x /usr/games/mgba-qt ]]; then
        MGBA="/usr/games/mgba-qt"
    elif command -v mgba &>/dev/null; then
        MGBA="$(command -v mgba)"
    fi

    if [[ -z "$MGBA" ]]; then
        echo "ERROR: mgba-qt or mgba not found. Install with: sudo apt install mgba-qt" >&2
        exit 1
    fi

    export DISPLAY="${DISPLAY:-:1}"

    # --- Kill any existing mGBA instance ---
    pkill -f "mgba" 2>/dev/null || true
    sleep 0.3
fi

MODE="${1:-gdb}"

# --- Helper to launch mGBA with extra args ---
launch_mgba() {
    local extra_args=("$@")
    if [[ "$IS_WSL" == true ]]; then
        # Match the launch pattern from launch_emulator.js:
        # cmd.exe /c start "" <emulator> [extra_args...] <rom>
        # Note: no /wait — cmd.exe /c start with /wait gets stopped by job control
        echo "  cmd: $CMD_EXE /c start \"\" $MGBA_WIN ${extra_args[*]} $ROM_WIN"
        "$CMD_EXE" /c start "" "$MGBA_WIN" "${extra_args[@]}" "$ROM_WIN"
        # Windows process is now running; use tasklist to track it
        MGBA_PID="windows"
    else
        "$MGBA" "${extra_args[@]}" "$ROM" &
        MGBA_PID=$!
    fi
}

# --- Helper to wait for Windows mGBA to exit ---
wait_mgba() {
    if [[ "$IS_WSL" == true ]]; then
        # Poll until mGBA.exe exits
        while cmd.exe /c "tasklist /fi \"imagename eq mGBA.exe\" /nh" 2>/dev/null | grep -qi "mGBA.exe"; do
            sleep 2
        done
    else
        wait "$MGBA_PID" 2>/dev/null || true
    fi
}

case "$MODE" in
    gdb)
        echo "Launching mGBA with GDB server on port 2345..."
        launch_mgba -g
        # Give mGBA time to start the GDB server
        sleep 3
        # Verify port is reachable (nc works across WSL/Windows boundary)
        if timeout 3 nc -zw1 localhost 2345 2>/dev/null; then
            echo "mGBA GDB server ready (port 2345)"
            echo "Connect with: gdb-multiarch -x gba-debug.gdb ${PROJECT_DIR}/stranded.elf"
        elif ss -tlnp 2>/dev/null | grep -q ":2345"; then
            echo "mGBA GDB server ready (port 2345)"
            echo "Connect with: gdb-multiarch -x gba-debug.gdb ${PROJECT_DIR}/stranded.elf"
        else
            echo "ERROR: mGBA GDB server not responding on port 2345" >&2
            exit 1
        fi
        # Keep script alive so VS Code doesn't kill the background task
        wait_mgba
        ;;

    cli)
        echo "Launching mGBA with CLI debugger..."
        echo "Commands: break, continue, next, print, x (examine memory), watch, trace"
        echo "Type 'help' in the debugger for full command list."
        if [[ "$IS_WSL" == true ]]; then
            launch_mgba -d
            wait_mgba
        else
            "$MGBA" -d "$ROM"
        fi
        ;;

    log)
        # Log level bitmask: 1=FATAL 2=ERROR 4=WARN 8=INFO 16=DEBUG 32=STUB 64=GAME_ERROR 128=SWI
        # 255 = all levels
        echo "Launching mGBA with verbose logging (all levels)..."
        echo "Log levels: FATAL|ERROR|WARN|INFO|DEBUG|STUB|GAME_ERROR|SWI"
        if [[ "$IS_WSL" == true ]]; then
            launch_mgba -l 255
            wait_mgba
        else
            "$MGBA" -l 255 "$ROM" 2>&1 | tee "$PROJECT_DIR/tmp/mgba_log.txt"
        fi
        ;;

    lua)
        LUA_SCRIPT="$SCRIPT_DIR/gba_debug.lua"
        if [[ ! -f "$LUA_SCRIPT" ]]; then
            echo "ERROR: Lua debug script not found at $LUA_SCRIPT" >&2
            exit 1
        fi
        echo "Launching mGBA with Lua debug script..."
        echo "Load the script manually: Tools > Scripting > File > Load: scripts/gba_debug.lua"
        echo "(mGBA CLI does not support --script flag; use the GUI Scripting window)"
        launch_mgba
        echo "mGBA ready"
        wait_mgba
        ;;

    *)
        echo "Usage: $0 [gdb|cli|log|lua]" >&2
        echo "  gdb  - GDB server on port 2345"
        echo "  cli  - Built-in command-line debugger"
        echo "  log  - Verbose logging (all levels)"
        echo "  lua  - Launch with Lua debug script instructions"
        exit 1
        ;;
esac
