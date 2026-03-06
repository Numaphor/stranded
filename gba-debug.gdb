# GBA Debug Init for Stranded (mGBA GDB server)
# Usage: gdb-multiarch -x gba-debug.gdb stranded.elf

set architecture arm
set pagination off
set confirm off
set print pretty on
set output-radix 16

target remote localhost:2345

# ============================================================
# GBA Memory Map Quick Reference
# ============================================================
# 0x02000000  EWRAM  (256 KB, 16-bit bus)
# 0x03000000  IWRAM  (32 KB, 32-bit bus)
# 0x04000000  I/O Registers
# 0x05000000  Palette RAM  (1 KB)
# 0x06000000  VRAM  (96 KB)
# 0x07000000  OAM   (1 KB, 128 OBJ_ATTR entries)
# 0x08000000  ROM   (up to 32 MB)
# ============================================================

# --- OAM Inspection ---

define oam-raw
    echo \n=== Raw OAM (first 32 entries as u16 words) ===\n
    x/192hx 0x07000000
end
document oam-raw
Dump raw OAM memory (first 32 sprite entries as hex halfwords).
Each OBJ_ATTR is 4 halfwords (attr0, attr1, attr2, padding/affine).
end

define oam-entry
    echo \n=== OBJ_ATTR entry ===\n
    set $__oa = (unsigned short*)0x07000000
    set $__i = $arg0
    set $__base = $__i * 4
    printf "OBJ[%d]  attr0=0x%04x  attr1=0x%04x  attr2=0x%04x\n", $__i, $__oa[$__base], $__oa[$__base+1], $__oa[$__base+2]
    printf "  Y=%d  Mode=%d  Shape=%d\n", ($__oa[$__base] & 0xff), (($__oa[$__base] >> 8) & 0x3), (($__oa[$__base] >> 14) & 0x3)
    printf "  X=%d  Size=%d", ($__oa[$__base+1] & 0x1ff), (($__oa[$__base+1] >> 14) & 0x3)
    if (($__oa[$__base] >> 8) & 0x3) == 1 || (($__oa[$__base] >> 8) & 0x3) == 3
        printf "  AffineIdx=%d", (($__oa[$__base+1] >> 9) & 0x1f)
    end
    printf "\n"
    printf "  TileIdx=%d  Priority=%d  Palette=%d\n", ($__oa[$__base+2] & 0x3ff), (($__oa[$__base+2] >> 10) & 0x3), (($__oa[$__base+2] >> 12) & 0xf)
end
document oam-entry
Decode a single OAM sprite entry: oam-entry <index 0-127>
Shows attr0/1/2, decoded Y, X, mode, shape, size, affine index, tile, priority, palette.
end

define oam-visible
    echo \n=== Visible OAM entries (not disabled) ===\n
    set $__oa = (unsigned short*)0x07000000
    set $__n = 0
    while $__n < 128
        set $__base = $__n * 4
        set $__mode = ($__oa[$__base] >> 8) & 0x3
        if $__mode != 2
            printf "OBJ[%3d] attr0=%04x attr1=%04x attr2=%04x  Y=%3d X=%3d Mode=%d\n", $__n, $__oa[$__base], $__oa[$__base+1], $__oa[$__base+2], ($__oa[$__base] & 0xff), ($__oa[$__base+1] & 0x1ff), $__mode
        end
        set $__n = $__n + 1
    end
end
document oam-visible
List all OAM entries that are not in hidden/disabled mode (mode != 2).
end

# --- Affine Matrix Inspection ---

define affine-all
    echo \n=== Affine Matrices (all 32, 8.8 fixed-point) ===\n
    set $__oa = (unsigned short*)0x07000000
    set $__m = 0
    while $__m < 32
        set $__off = $__m * 16
        set $__pa = (short)$__oa[$__off + 3]
        set $__pb = (short)$__oa[$__off + 7]
        set $__pc = (short)$__oa[$__off + 11]
        set $__pd = (short)$__oa[$__off + 15]
        if $__pa != 0 || $__pb != 0 || $__pc != 0 || $__pd != 0
            printf "Affine[%2d]  pa=%6d (%5.2f)  pb=%6d (%5.2f)  pc=%6d (%5.2f)  pd=%6d (%5.2f)\n", $__m, $__pa, $__pa/256.0, $__pb, $__pb/256.0, $__pc, $__pc/256.0, $__pd, $__pd/256.0
        end
        set $__m = $__m + 1
    end
end
document affine-all
Dump all 32 OAM affine matrices. Shows raw 8.8 fixed-point values and float equivalents.
Only non-zero matrices are printed. Identity = pa=256 pb=0 pc=0 pd=256.
end

define affine-entry
    echo \n=== Affine Matrix ===\n
    set $__oa = (unsigned short*)0x07000000
    set $__m = $arg0
    set $__off = $__m * 16
    set $__pa = (short)$__oa[$__off + 3]
    set $__pb = (short)$__oa[$__off + 7]
    set $__pc = (short)$__oa[$__off + 11]
    set $__pd = (short)$__oa[$__off + 15]
    printf "Affine[%d]:\n", $__m
    printf "  | pa=%6d (%7.4f)  pb=%6d (%7.4f) |\n", $__pa, $__pa/256.0, $__pb, $__pb/256.0
    printf "  | pc=%6d (%7.4f)  pd=%6d (%7.4f) |\n", $__pc, $__pc/256.0, $__pd, $__pd/256.0
    set $__det = ($__pa * $__pd - $__pb * $__pc)
    printf "  det = %d (%.4f)  [healthy: ~65536 / 256.0]\n", $__det, $__det/65536.0
end
document affine-entry
Inspect a single affine matrix: affine-entry <index 0-31>
Shows pa/pb/pc/pd as raw 8.8 and float, plus determinant for degeneration check.
end

# --- I/O Register Inspection ---

define gba-dispcnt
    set $__v = *(unsigned short*)0x04000000
    printf "DISPCNT = 0x%04x\n", $__v
    printf "  Mode=%d  BG0=%d BG1=%d BG2=%d BG3=%d  OBJ=%d  Win0=%d Win1=%d ObjWin=%d\n", ($__v & 0x7), (($__v >> 8) & 1), (($__v >> 9) & 1), (($__v >> 10) & 1), (($__v >> 11) & 1), (($__v >> 12) & 1), (($__v >> 13) & 1), (($__v >> 14) & 1), (($__v >> 15) & 1)
end
document gba-dispcnt
Read and decode the GBA DISPCNT register (0x04000000).
Shows video mode, enabled BG layers, OBJ enable, and window flags.
end

define gba-dispstat
    set $__v = *(unsigned short*)0x04000002
    printf "DISPSTAT = 0x%04x\n", $__v
    printf "  VBlank=%d  HBlank=%d  VCount=%d  VBI=%d  HBI=%d  VCI=%d  VCountTarget=%d\n", ($__v & 1), (($__v >> 1) & 1), (($__v >> 2) & 1), (($__v >> 3) & 1), (($__v >> 4) & 1), (($__v >> 5) & 1), (($__v >> 8) & 0xff)
end
document gba-dispstat
Read and decode the GBA DISPSTAT register (0x04000002).
end

define gba-vcount
    printf "VCOUNT = %d (scanline)\n", *(unsigned short*)0x04000006
end
document gba-vcount
Read the current scanline from VCOUNT (0x04000006). 0-159=visible, 160-227=vblank.
end

# --- Palette ---

define palette-bg
    echo \n=== BG Palette (first 16 colors) ===\n
    x/16hx 0x05000000
end
document palette-bg
Dump the first 16 entries of BG palette RAM (0x05000000). Values are BGR555.
end

define palette-obj
    echo \n=== OBJ Palette (first 16 colors) ===\n
    x/16hx 0x05000200
end
document palette-obj
Dump the first 16 entries of OBJ palette RAM (0x05000200). Values are BGR555.
end

# --- Memory Region Dumps ---

define dump-iwram
    echo \n=== IWRAM header (first 256 bytes) ===\n
    x/64wx 0x03000000
end
document dump-iwram
Dump first 256 bytes of IWRAM (fast 32KB work RAM at 0x03000000).
end

define dump-ewram
    echo \n=== EWRAM header (first 256 bytes) ===\n
    x/64wx 0x02000000
end
document dump-ewram
Dump first 256 bytes of EWRAM (slow 256KB work RAM at 0x02000000).
end

# --- Convenience ---

define gba-status
    echo \n========== GBA Status ==========\n
    gba-dispcnt
    gba-dispstat
    gba-vcount
    echo \n--- Active Sprites ---\n
    set $__oa = (unsigned short*)0x07000000
    set $__count = 0
    set $__n = 0
    while $__n < 128
        set $__mode = ($__oa[$__n * 4] >> 8) & 0x3
        if $__mode != 2
            set $__count = $__count + 1
        end
        set $__n = $__n + 1
    end
    printf "Visible sprites: %d / 128\n", $__count
    echo ================================\n
end
document gba-status
Quick GBA hardware status: display registers + visible sprite count.
end

define help-gba
    echo \nGBA Debug Commands:\n
    echo   oam-raw          - Dump raw OAM memory\n
    echo   oam-entry N      - Decode OAM entry N (0-127)\n
    echo   oam-visible      - List all visible (non-hidden) sprites\n
    echo   affine-all       - Dump all non-zero affine matrices\n
    echo   affine-entry N   - Inspect affine matrix N (0-31)\n
    echo   gba-dispcnt      - Decode DISPCNT register\n
    echo   gba-dispstat     - Decode DISPSTAT register\n
    echo   gba-vcount       - Read current scanline\n
    echo   gba-status       - Quick hardware overview\n
    echo   palette-bg       - Dump BG palette (first 16)\n
    echo   palette-obj      - Dump OBJ palette (first 16)\n
    echo   dump-iwram       - First 256 bytes of IWRAM\n
    echo   dump-ewram       - First 256 bytes of EWRAM\n
    echo   help-gba         - This help\n
end
document help-gba
List all custom GBA debugging commands.
end

echo \n[gba-debug.gdb] Connected to mGBA. Type 'help-gba' for GBA commands.\n
