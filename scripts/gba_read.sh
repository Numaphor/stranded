#!/usr/bin/env bash
# Read GBA memory via mGBA GDB server (batch mode).
# Usage: gba_read.sh <gdb_commands>
# Example: gba_read.sh "x/8hx 0x07000000"
#          gba_read.sh "info registers"

set -euo pipefail

GDB="${GDB:-gdb-multiarch}"
ELF="${ELF:-/home/numa/repos/stranded/stranded.elf}"
ADDR="${GDB_ADDR:-localhost:2345}"

# Build GDB command sequence
CMDS=$(cat <<'INIT'
set architecture arm
set pagination off
set confirm off
set print pretty on
INIT
)

# Add target connection
CMDS="$CMDS
target remote $ADDR"

# Add user commands (each arg is one command)
for cmd in "$@"; do
    CMDS="$CMDS
$cmd"
done

CMDS="$CMDS
detach
quit"

echo "$CMDS" | "$GDB" -batch -x /dev/stdin "$ELF" 2>/dev/null | grep -v "^Reading symbols\|^Remote debugging\|^warning:\|^\[Inferior\|^Detaching"
