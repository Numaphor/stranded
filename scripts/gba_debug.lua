-- gba_debug.lua — mGBA Lua debug script for Stranded
-- Load via: mGBA > Tools > Scripting > File > Load
--
-- Features:
--   1. OAM sprite inspector (live TextBuffer)
--   2. Affine matrix monitor with degeneration alerts
--   3. Per-frame memory watchpoints
--   4. I/O register display
--   5. Automated screenshot on anomaly detection

-- ============================================================
-- Configuration
-- ============================================================
local CFG = {
    -- How often to refresh the debug display (in frames)
    refresh_interval = 10,
    -- Affine determinant thresholds (8.8 fixed * 8.8 fixed = 16.16)
    -- Identity det = 256*256 = 65536. Warn if outside this range:
    affine_det_min = 16384,   -- 0.25x scale
    affine_det_max = 262144,  -- 4.0x scale
    -- Max sprites before warning
    sprite_warn_threshold = 120,
    -- Enable per-frame auto-screenshot on anomaly
    auto_screenshot = false,
    -- Screenshot output path (relative to ROM directory)
    screenshot_dir = "tmp/",
}

-- ============================================================
-- State
-- ============================================================
local frame_count = 0
local buf_oam = nil    -- TextBuffer for OAM display
local buf_affine = nil -- TextBuffer for affine display
local buf_io = nil     -- TextBuffer for I/O registers
local buf_watch = nil  -- TextBuffer for memory watches

local watched_addresses = {}  -- { {addr=, size=, name=, last_value=}, ... }

-- ============================================================
-- Helpers
-- ============================================================

local function hex16(v)
    return string.format("0x%04X", v)
end

local function signed16(v)
    if v >= 0x8000 then return v - 0x10000 end
    return v
end

local function fixed88_to_float(v)
    return signed16(v) / 256.0
end

-- ============================================================
-- OAM Inspector
-- ============================================================

local function refresh_oam()
    if not buf_oam then
        buf_oam = console:createBuffer("OAM Sprites")
        buf_oam:setSize(80, 40)
    end
    buf_oam:clear()
    buf_oam:moveCursor(0, 0)

    local oam = emu.memory.oam
    local visible = 0
    local affine_count = 0
    local lines = {}

    for i = 0, 127 do
        local off = i * 8  -- 8 bytes per OBJ_ATTR
        local attr0 = oam:read16(off)
        local attr1 = oam:read16(off + 2)
        local attr2 = oam:read16(off + 4)

        local y = attr0 & 0xFF
        local mode = (attr0 >> 8) & 0x3
        local shape = (attr0 >> 14) & 0x3
        local x = attr1 & 0x1FF
        local size = (attr1 >> 14) & 0x3

        if mode ~= 2 then  -- not hidden
            visible = visible + 1
            if mode == 1 or mode == 3 then
                affine_count = affine_count + 1
            end
            local affine_idx = ""
            if mode == 1 or mode == 3 then
                affine_idx = string.format(" Aff=%d", (attr1 >> 9) & 0x1F)
            end
            table.insert(lines, string.format(
                "OBJ[%3d] X=%3d Y=%3d M=%d Sh=%d Sz=%d T=%3d P=%d%s",
                i, x, y, mode, shape, size,
                attr2 & 0x3FF, (attr2 >> 10) & 0x3, affine_idx
            ))
        end
    end

    buf_oam:print(string.format("=== OAM: %d visible, %d affine (frame %d) ===\n",
        visible, affine_count, frame_count))

    if visible >= CFG.sprite_warn_threshold then
        buf_oam:print(string.format("!! WARNING: %d sprites near HW limit (128) !!\n", visible))
    end

    for _, line in ipairs(lines) do
        buf_oam:print(line .. "\n")
    end

    return visible, affine_count
end

-- ============================================================
-- Affine Matrix Monitor
-- ============================================================

local function refresh_affine()
    if not buf_affine then
        buf_affine = console:createBuffer("Affine Matrices")
        buf_affine:setSize(80, 40)
    end
    buf_affine:clear()
    buf_affine:moveCursor(0, 0)

    local oam = emu.memory.oam
    local anomalies = 0

    buf_affine:print(string.format("=== Affine Matrices (frame %d) ===\n", frame_count))
    buf_affine:print("Idx |    pa    |    pb    |    pc    |    pd    |   det\n")
    buf_affine:print("----|----------|----------|----------|----------|--------\n")

    for m = 0, 31 do
        -- Affine params are interleaved in OAM at offsets 6, 14, 22, 30 within
        -- each group of 4 OBJ_ATTRs (32 bytes per affine group)
        local base = m * 32
        local pa = signed16(oam:read16(base + 6))
        local pb = signed16(oam:read16(base + 14))
        local pc = signed16(oam:read16(base + 22))
        local pd = signed16(oam:read16(base + 30))

        if pa ~= 0 or pb ~= 0 or pc ~= 0 or pd ~= 0 then
            local det = pa * pd - pb * pc
            local det_f = det / 65536.0
            local flag = ""

            if det < CFG.affine_det_min or det > CFG.affine_det_max then
                flag = " !!"
                anomalies = anomalies + 1
            end

            buf_affine:print(string.format(
                "%3d | %6d | %6d | %6d | %6d | %8.3f%s\n",
                m, pa, pb, pc, pd, det_f, flag
            ))
        end
    end

    if anomalies > 0 then
        buf_affine:print(string.format(
            "\n!! %d matrices with abnormal determinant (outside %.1f - %.1f) !!\n",
            anomalies,
            CFG.affine_det_min / 65536.0,
            CFG.affine_det_max / 65536.0
        ))
    end

    return anomalies
end

-- ============================================================
-- I/O Register Display
-- ============================================================

local function refresh_io()
    if not buf_io then
        buf_io = console:createBuffer("GBA I/O Registers")
        buf_io:setSize(60, 20)
    end
    buf_io:clear()
    buf_io:moveCursor(0, 0)

    local io = emu.memory.io

    local dispcnt  = io:read16(0x000)
    local dispstat = io:read16(0x002)
    local vcount   = io:read16(0x006)
    local bg0cnt   = io:read16(0x008)
    local bg1cnt   = io:read16(0x00A)
    local bg2cnt   = io:read16(0x00C)
    local bg3cnt   = io:read16(0x00E)
    local keyinput = io:read16(0x130)

    buf_io:print(string.format("=== I/O Registers (frame %d) ===\n", frame_count))
    buf_io:print(string.format("DISPCNT  = %s  Mode=%d OBJ=%d\n",
        hex16(dispcnt), dispcnt & 0x7, (dispcnt >> 12) & 1))
    buf_io:print(string.format("  BG enables: BG0=%d BG1=%d BG2=%d BG3=%d\n",
        (dispcnt >> 8) & 1, (dispcnt >> 9) & 1,
        (dispcnt >> 10) & 1, (dispcnt >> 11) & 1))
    buf_io:print(string.format("DISPSTAT = %s  VBl=%d HBl=%d VCnt=%d\n",
        hex16(dispstat), dispstat & 1, (dispstat >> 1) & 1, (dispstat >> 2) & 1))
    buf_io:print(string.format("VCOUNT   = %d\n", vcount & 0xFF))
    buf_io:print(string.format("BG0CNT=%s BG1CNT=%s\n", hex16(bg0cnt), hex16(bg1cnt)))
    buf_io:print(string.format("BG2CNT=%s BG3CNT=%s\n", hex16(bg2cnt), hex16(bg3cnt)))

    -- Decode key input (active-low: 0=pressed)
    local keys = keyinput ~ 0x3FF  -- invert to get pressed=1
    local key_names = {"A","B","Sel","Start","R","L","U","D","RB","LB"}
    local pressed = {}
    for bit = 0, 9 do
        if (keys >> bit) & 1 == 1 then
            table.insert(pressed, key_names[bit + 1])
        end
    end
    buf_io:print(string.format("Keys: %s\n", #pressed > 0 and table.concat(pressed, "+") or "(none)"))
end

-- ============================================================
-- Memory Watchpoints
-- ============================================================

--- Add a memory address to watch for changes.
-- @param addr  Bus address (e.g., 0x03000000)
-- @param size  Read size: 8, 16, or 32
-- @param name  Display name
function watch_add(addr, size, name)
    local read_fn
    if size == 8 then read_fn = function() return emu:read8(addr) end
    elseif size == 16 then read_fn = function() return emu:read16(addr) end
    else read_fn = function() return emu:read32(addr) end
    end

    table.insert(watched_addresses, {
        addr = addr,
        size = size,
        name = name or string.format("0x%08X", addr),
        read = read_fn,
        last_value = read_fn(),
        changes = 0,
    })
    console:log(string.format("Watching %s at 0x%08X (%d-bit)", name or "addr", addr, size))
end

local function refresh_watches()
    if #watched_addresses == 0 then return end

    if not buf_watch then
        buf_watch = console:createBuffer("Memory Watches")
        buf_watch:setSize(70, 20)
    end
    buf_watch:clear()
    buf_watch:moveCursor(0, 0)

    buf_watch:print(string.format("=== Memory Watches (frame %d) ===\n", frame_count))

    for _, w in ipairs(watched_addresses) do
        local val = w.read()
        local changed = val ~= w.last_value
        if changed then
            w.changes = w.changes + 1
            w.last_value = val
        end

        local fmt = w.size <= 16 and "0x%04X" or "0x%08X"
        buf_watch:print(string.format(
            "%-20s [0x%08X] = " .. fmt .. "  changes=%d%s\n",
            w.name, w.addr, val, w.changes,
            changed and " *CHANGED*" or ""
        ))
    end
end

-- ============================================================
-- Frame Callback
-- ============================================================

local function on_frame()
    frame_count = frame_count + 1

    if frame_count % CFG.refresh_interval ~= 0 then return end

    local sprite_count, _ = refresh_oam()
    local affine_anomalies = refresh_affine()
    refresh_io()
    refresh_watches()

    -- Auto-screenshot on anomaly
    if CFG.auto_screenshot and (affine_anomalies > 0 or sprite_count >= CFG.sprite_warn_threshold) then
        local filename = string.format("%sanomaly_frame_%06d.png", CFG.screenshot_dir, frame_count)
        emu:screenshot(filename)
        console:warn(string.format("Anomaly detected at frame %d — screenshot saved: %s", frame_count, filename))
    end
end

-- ============================================================
-- Initialization
-- ============================================================

callbacks:add("frame", on_frame)

console:log("=== Stranded GBA Debug Script Loaded ===")
console:log("Buffers: OAM Sprites, Affine Matrices, GBA I/O Registers")
console:log(string.format("Refresh every %d frames. Type in scripting console:", CFG.refresh_interval))
console:log("  watch_add(0x03000000, 32, 'IWRAM start')  -- add memory watch")
console:log("  CFG.refresh_interval = 1                   -- every frame")
console:log("  CFG.auto_screenshot = true                 -- screenshot on anomaly")
