# mGBA Debug Guide for Stranded

## Quick Reference

| Task | Command / Action |
|------|-----------------|
| **Build + attach GDB** | VS Code: Run `Attach debugger (mGBA GDB server)` launch config |
| **CLI debugger** | Task: `mGBA: cli debugger` or `bash scripts/launch_debug.sh cli` |
| **Verbose logging** | Task: `mGBA: verbose logging` or `bash scripts/launch_debug.sh log` |
| **Lua inspector** | mGBA > Tools > Scripting > Load `scripts/gba_debug.lua` |
| **GDB from terminal** | `gdb-multiarch -x gba-debug.gdb stranded.elf` |

---

## 1. GDB Server Integration (Breakpoints, Stepping, Inspection)

### VS Code Workflow

1. **Build with `make -j8`** (or use the `make` task).
2. Run launch configuration **"Attach debugger (mGBA GDB server)"**.
   - This builds, starts mGBA with `-g` (GDB server on port 2345), then attaches the VS Code C/C++ debugger.
3. Set breakpoints in source files, use the Variables/Watch/Call Stack panels.
4. Use the **Debug Console** for GDB commands (prefix with `-exec`):
   ```
   -exec help-gba
   -exec oam-visible
   -exec affine-all
   -exec gba-status
   ```

### Terminal Workflow

```bash
# Terminal 1: launch mGBA with GDB server
bash scripts/launch_debug.sh gdb

# Terminal 2: attach GDB
gdb-multiarch -x gba-debug.gdb stranded.elf
```

### Custom GDB Commands (from gba-debug.gdb)

| Command | Description |
|---------|-------------|
| `help-gba` | List all custom GBA commands |
| `oam-raw` | Dump raw OAM memory (hex halfwords) |
| `oam-entry N` | Decode OAM sprite entry N (0-127) |
| `oam-visible` | List all non-hidden sprite entries |
| `affine-all` | Dump all active affine matrices with determinants |
| `affine-entry N` | Inspect affine matrix N (0-31) with det check |
| `gba-dispcnt` | Decode DISPCNT register |
| `gba-dispstat` | Decode DISPSTAT register |
| `gba-vcount` | Current scanline |
| `gba-status` | Quick hardware overview |
| `palette-bg` | BG palette first 16 entries |
| `palette-obj` | OBJ palette first 16 entries |
| `dump-iwram` | First 256 bytes of IWRAM |
| `dump-ewram` | First 256 bytes of EWRAM |

### Debug Build

For better debugging with `-Og` (no LTO), use the **"make (debug)"** task or:
```bash
make -j8 USERFLAGS=-Og USERLDFLAGS=
```
This disables LTO and uses debug-friendly optimization, so breakpoints and variable inspection work reliably.

---

## 2. Command Line Debugger (`-d`)

mGBA's built-in debugger provides a terminal-based interface:

```bash
bash scripts/launch_debug.sh cli
```

Key commands:
| Command | Description |
|---------|-------------|
| `b <addr>` | Set breakpoint at address |
| `c` | Continue execution |
| `n` | Step over |
| `s` | Step into |
| `p <addr>` | Print memory at address |
| `x <addr> <count>` | Examine memory |
| `w <addr>` | Set watchpoint on address |
| `trace` | Toggle instruction trace |
| `events` | List pending hardware events |
| `help` | Full command list |

---

## 3. Memory & Watchpoints

### GDB Hardware Watchpoints

In GDB (VS Code Debug Console with `-exec` prefix):
```gdb
# Watch for writes to a specific address (e.g., player HP in IWRAM)
watch *(unsigned short*)0x03000100

# Read watchpoint
rwatch *(unsigned int*)0x03000200

# Access watchpoint (read or write)
awatch *(unsigned int*)0x03000200
```

### Lua Memory Watches

In the mGBA Scripting console after loading `gba_debug.lua`:
```lua
-- Watch an IWRAM address (32-bit)
watch_add(0x03000000, 32, "IWRAM start")

-- Watch OAM entry 0 attr0 (16-bit)
watch_add(0x07000000, 16, "OBJ0 attr0")

-- Watch DISPCNT register (16-bit)
watch_add(0x04000000, 16, "DISPCNT")
```

The "Memory Watches" buffer auto-updates every N frames and flags changes.

---

## 4. Viewers & Inspectors

### mGBA Built-in Viewers (GUI)

Access via mGBA menu **Tools**:

| Viewer | What it shows |
|--------|--------------|
| **Tile Viewer** | All tiles in VRAM, with palette selection |
| **Map Viewer** | BG layer map data, scroll position, tile layout |
| **Sprite Viewer** | All 128 OAM entries with coordinates, tiles, attributes |
| **Palette Viewer** | All 512 colors (256 BG + 256 OBJ) |
| **I/O Register Viewer** | Live view of all GBA I/O registers |
| **Memory Viewer** | Raw hex editor for any memory region |
| **Frame Inspector** | Step through rendering pipeline |

### Key Memory Regions for Inspection

| Address | Name | Size | Contents |
|---------|------|------|----------|
| `0x02000000` | EWRAM | 256KB | Large data, heap |
| `0x03000000` | IWRAM | 32KB | Stack, fast variables |
| `0x04000000` | I/O | 1KB | Hardware registers |
| `0x05000000` | Palette | 1KB | 256 BG + 256 OBJ colors |
| `0x06000000` | VRAM | 96KB | Tiles and maps |
| `0x07000000` | OAM | 1KB | 128 sprites + 32 affine matrices |

---

## 5. Log Control (`-l`)

```bash
# Launch with specific log levels
bash scripts/launch_debug.sh log
```

Log level bitmask values:

| Bit | Value | Level |
|-----|-------|-------|
| 0 | 1 | FATAL |
| 1 | 2 | ERROR |
| 2 | 4 | WARN |
| 3 | 8 | INFO |
| 4 | 16 | DEBUG |
| 5 | 32 | STUB (unimplemented HW) |
| 6 | 64 | GAME_ERROR |
| 7 | 128 | SWI (software interrupts) |

Examples:
```bash
# Errors and warnings only
mgba-qt -l 7 stranded.gba

# Everything including SWI
mgba-qt -l 255 stranded.gba 2>&1 | tee tmp/mgba_log.txt

# Game errors + SWI (useful for Butano issues)
mgba-qt -l 192 stranded.gba
```

Butano's `BN_LOG()` output appears in the SWI category.

---

## 6. Lua Scripting

### Loading the Debug Script

1. Launch mGBA: `bash scripts/launch_debug.sh lua` (or any mode)
2. In mGBA: **Tools > Scripting**
3. In the Scripting window: **File > Load Script**
4. Select `scripts/gba_debug.lua`

### What the Script Provides

Four auto-refreshing TextBuffer windows:

- **OAM Sprites**: Visible sprite count, coordinates, attributes, affine indices. Warns when approaching 128-sprite hardware limit.
- **Affine Matrices**: All active affine matrices with 8.8 fixed-point and float values. Flags degenerate determinants.
- **GBA I/O Registers**: DISPCNT, DISPSTAT, VCOUNT, BG control, pressed keys.
- **Memory Watches**: User-defined address watchpoints with change detection.

### Scripting Console Commands

```lua
-- Adjust refresh rate (default: every 10 frames)
CFG.refresh_interval = 1       -- every frame (slower)
CFG.refresh_interval = 60      -- once per second

-- Enable auto-screenshot on anomaly detection
CFG.auto_screenshot = true

-- Add memory watches
watch_add(0x03000100, 16, "player_hp")
watch_add(0x07000000, 16, "OBJ0_attr0")

-- Direct memory reads
emu:read16(0x07000000)         -- read OAM entry 0 attr0
emu:read32(0x04000000)         -- read DISPCNT + DISPSTAT

-- Access memory domains
emu.memory.oam:read16(0)       -- OAM offset 0
emu.memory.iwram:read32(0)     -- IWRAM offset 0
emu.memory.vram:read16(0)      -- VRAM offset 0

-- Read CPU registers
emu:readRegister("r0")
emu:readRegister("sp")
emu:readRegister("pc")
emu:readRegister("cpsr")

-- Save state / screenshot
emu:screenshot("tmp/debug_capture.png")
emu:saveStateSlot(1)
emu:loadStateSlot(1)

-- Frame control
emu:currentFrame()
emu:step()                     -- single instruction
emu:runFrame()                 -- run one full frame
```

### Writing Custom Callbacks

```lua
-- Example: log when a specific sprite becomes visible
callbacks:add("frame", function()
    local attr0 = emu.memory.oam:read16(10 * 8)  -- sprite 10
    if (attr0 >> 8) & 0x3 ~= 2 then
        console:log("Sprite 10 visible at frame " .. emu:currentFrame())
    end
end)
```

---

## Workflow: Diagnosing Rendering Artifacts

1. **Build**: `make -j8` (or `make -j8 USERFLAGS=-Og USERLDFLAGS=` for debug)
2. **Launch with GDB**: Run `Attach debugger (mGBA GDB server)` in VS Code
3. **Load Lua script**: Tools > Scripting > Load `scripts/gba_debug.lua`
4. **Reproduce the issue** in-game
5. **Pause** in VS Code debugger (or press Pause button)
6. **Inspect** using:
   - VS Code Debug Console: `-exec oam-visible`, `-exec affine-all`
   - mGBA Sprite Viewer: Tools > Sprite Viewer
   - Lua buffers: check OAM Sprites and Affine Matrices windows
7. **Cross-reference** with room viewer overlay diagnostics (`Door`, `Df`, `HL`)

See also: [ROOM_VIEWER_OAM_AFFINE_DEBUG_PLAYBOOK.md](ROOM_VIEWER_OAM_AFFINE_DEBUG_PLAYBOOK.md) for artifact-specific diagnosis.
