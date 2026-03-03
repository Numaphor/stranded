#!/usr/bin/env python3
"""
Convert AxulArt Small-8-Direction-Characters sprite sheet PNG to Butano 4bpp BMP.

Input:  Small-8-Direction-Characters_by_AxulArt.png (128x288, palette-indexed PNG)
Output: graphics/sprite/npc/villager.bmp (32x640, 4bpp indexed BMP, 20 frames)

Frame layout in output BMP (5 directions x 4 frames each):
  Frames  0-3:  Down      (idle: standing pose, all same frame)
  Frames  4-7:  Down_Side (SE)
  Frames  8-11: Side      (E/right)
  Frames 12-15: Up_Side   (NE)
  Frames 16-19: Up        (N)

Directions 5-7 (SW, W, NW) are mirrors handled by horizontal_flip at runtime.
"""

import struct
import zlib
import os
import sys

# --- Configuration ---
CHAR_BLOCK = 1          # 0=template, 1=blue hat, 2=orange hat
CELL_W = 16             # pixels per cell width
CELL_H = 24             # pixels per cell height (24px rows, ~17px actual content)
OUT_W = 32              # output frame width (padded)
OUT_H = 32              # output frame height (padded)
NUM_DIRS = 5            # Butano directions (down, down_side, side, up_side, up)
FRAMES_PER_DIR = 4      # idle bounce: F1, F2, F3, F2
TOTAL_FRAMES = NUM_DIRS * FRAMES_PER_DIR  # 20

# Source column -> Butano direction mapping
# PNG columns: 0=N, 1=NE, 2=E, 3=SE, 4=S, 5=SW, 6=W, 7=NW
# Butano dirs: 0=Down(S), 1=Down_Side(SE), 2=Side(E), 3=Up_Side(NE), 4=Up(N)
SRC_COL_FOR_DIR = [4, 3, 2, 1, 0]  # Down=col4(S), DownSide=col3(SE), Side=col2(E), UpSide=col1(NE), Up=col0(N)

# Source rows within a character block (0=arrows skip, 1-3=anim frames)
SRC_FRAME_ROWS = [1, 2, 3]  # 3 source frames
# Idle pose: all frames use row 2 (standing, feet together)
BOUNCE = [1, 1, 1, 1]


def decode_png(path):
    """Decode palette-indexed PNG using pure Python (struct + zlib)."""
    with open(path, 'rb') as f:
        sig = f.read(8)
        assert sig == b'\x89PNG\r\n\x1a\n', "Not a valid PNG"

        chunks = {}
        idat_data = b''
        while True:
            raw = f.read(4)
            if len(raw) < 4:
                break
            length = struct.unpack('>I', raw)[0]
            chunk_type = f.read(4).decode('ascii')
            data = f.read(length)
            _crc = f.read(4)
            if chunk_type == 'IDAT':
                idat_data += data
            else:
                chunks[chunk_type] = data

    # Parse IHDR
    ihdr = chunks['IHDR']
    w, h = struct.unpack('>II', ihdr[:8])
    bit_depth = ihdr[8]
    color_type = ihdr[9]
    assert color_type == 3, f"Expected palette-indexed PNG (color_type=3), got {color_type}"
    assert bit_depth == 8, f"Expected 8-bit depth, got {bit_depth}"

    # Parse PLTE
    plte_data = chunks['PLTE']
    num_colors = len(plte_data) // 3
    palette = []
    for i in range(num_colors):
        r, g, b = plte_data[i*3], plte_data[i*3+1], plte_data[i*3+2]
        palette.append((r, g, b))

    # Parse tRNS (transparency)
    trans_index = None
    if 'tRNS' in chunks:
        trns = chunks['tRNS']
        for i, a in enumerate(trns):
            if a == 0:
                trans_index = i
                break

    # Decompress IDAT
    raw_data = zlib.decompress(idat_data)

    # Unfilter scanlines
    stride = w  # 8bpp = 1 byte per pixel
    pixels = []
    pos = 0
    prev_row = bytes(stride)
    for _y in range(h):
        filter_type = raw_data[pos]
        pos += 1
        row = bytearray(raw_data[pos:pos + stride])
        pos += stride

        if filter_type == 0:
            pass
        elif filter_type == 1:  # Sub
            for x in range(1, stride):
                row[x] = (row[x] + row[x - 1]) & 0xFF
        elif filter_type == 2:  # Up
            for x in range(stride):
                row[x] = (row[x] + prev_row[x]) & 0xFF
        elif filter_type == 3:  # Average
            for x in range(stride):
                a = row[x - 1] if x > 0 else 0
                b = prev_row[x]
                row[x] = (row[x] + (a + b) // 2) & 0xFF
        elif filter_type == 4:  # Paeth
            for x in range(stride):
                a = row[x - 1] if x > 0 else 0
                b = prev_row[x]
                c = prev_row[x - 1] if x > 0 else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                if pa <= pb and pa <= pc:
                    pred = a
                elif pb <= pc:
                    pred = b
                else:
                    pred = c
                row[x] = (row[x] + pred) & 0xFF

        pixels.append(bytes(row))
        prev_row = row

    return w, h, pixels, palette, trans_index


def extract_cell(pixels, col, row_in_block, block_index, cell_w, cell_h):
    """Extract a cell_w x cell_h region from the pixel grid."""
    y_start = block_index * (4 * cell_h) + row_in_block * cell_h
    x_start = col * cell_w
    cell = []
    for y in range(y_start, y_start + cell_h):
        cell.append(pixels[y][x_start:x_start + cell_w])
    return cell


def pad_frame(cell, cell_w, cell_h, out_w, out_h, trans_idx):
    """Pad a cell_w x cell_h frame to out_w x out_h, centered, using trans_idx."""
    pad_left = (out_w - cell_w) // 2
    pad_right = out_w - cell_w - pad_left
    pad_top = (out_h - cell_h) // 2
    pad_bottom = out_h - cell_h - pad_top

    frame = []
    # Top padding
    for _ in range(pad_top):
        frame.append(bytes([trans_idx] * out_w))
    # Content rows with left/right padding
    for row in cell:
        padded = bytes([trans_idx] * pad_left) + row + bytes([trans_idx] * pad_right)
        frame.append(padded)
    # Bottom padding
    for _ in range(pad_bottom):
        frame.append(bytes([trans_idx] * out_w))

    return frame


def remap_palette(frames, src_palette, trans_index):
    """
    Remap pixel indices to a compact <=16 color palette.
    Returns (remapped_frames, new_palette) where new_palette[0] = transparent.
    """
    # Collect all unique indices used
    used_indices = set()
    for frame in frames:
        for row in frame:
            for px in row:
                used_indices.add(px)

    # Build new palette: index 0 = transparent (black), rest = used colors
    used_indices.discard(trans_index)
    used_sorted = sorted(used_indices)

    if len(used_sorted) + 1 > 16:
        print(f"WARNING: {len(used_sorted)+1} colors used, exceeds 4bpp limit of 16!")
        # Truncate (shouldn't happen with this sprite)
        used_sorted = used_sorted[:15]

    # Map: old index -> new index
    remap = {}
    if trans_index is not None:
        remap[trans_index] = 0
    new_palette = [(0, 0, 0)]  # index 0 = black (transparent)

    for new_idx, old_idx in enumerate(used_sorted, start=1):
        remap[old_idx] = new_idx
        new_palette.append(src_palette[old_idx])

    # Pad to 16 entries
    while len(new_palette) < 16:
        new_palette.append((0, 0, 0))

    # Remap all frames
    remapped = []
    for frame in frames:
        new_frame = []
        for row in frame:
            new_row = bytes([remap.get(px, 0) for px in row])
            new_frame.append(new_row)
        remapped.append(new_frame)

    # Print palette mapping for reference
    print("Palette mapping (old -> new):")
    for old_idx in [trans_index] + used_sorted:
        if old_idx is not None:
            new_idx = remap[old_idx]
            r, g, b = new_palette[new_idx]
            label = " (transparent)" if new_idx == 0 else ""
            print(f"  [{old_idx:2d}] -> [{new_idx:2d}] RGB({r:3d},{g:3d},{b:3d}){label}")

    return remapped, new_palette, remap


def write_4bpp_bmp(path, width, height, palette_rgb, frames):
    """
    Write a 4bpp indexed BMP. Frames are laid out vertically (strip).
    BMP is bottom-up by default (row 0 = bottom of image).
    """
    total_height = height * len(frames)

    # Compose full image (top-down order first)
    full_image = []
    for frame in frames:
        full_image.extend(frame)

    # BMP row stride for 4bpp: each row = width/2 bytes, padded to 4-byte boundary
    row_bytes = (width + 1) // 2  # 2 pixels per byte
    row_stride = (row_bytes + 3) & ~3  # pad to 4 bytes

    # Pixel data (bottom-up: reverse row order)
    pixel_data = bytearray()
    for y in range(total_height - 1, -1, -1):
        row = full_image[y]
        packed = bytearray(row_stride)
        for x in range(width):
            byte_idx = x // 2
            if x % 2 == 0:
                packed[byte_idx] |= (row[x] & 0x0F) << 4
            else:
                packed[byte_idx] |= row[x] & 0x0F
        pixel_data.extend(packed)

    # Palette: 16 entries, 4 bytes each (BGRA)
    palette_data = bytearray()
    for r, g, b in palette_rgb:
        palette_data.extend(struct.pack('BBBB', b, g, r, 0))

    # Headers
    header_size = 14 + 40  # file header + DIB header
    palette_size = 16 * 4
    data_offset = header_size + palette_size
    file_size = data_offset + len(pixel_data)

    with open(path, 'wb') as f:
        # BITMAPFILEHEADER (14 bytes)
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(struct.pack('<HH', 0, 0))  # reserved
        f.write(struct.pack('<I', data_offset))

        # BITMAPINFOHEADER (40 bytes)
        f.write(struct.pack('<I', 40))  # header size
        f.write(struct.pack('<i', width))
        f.write(struct.pack('<i', total_height))  # positive = bottom-up
        f.write(struct.pack('<HH', 1, 4))  # planes=1, bpp=4
        f.write(struct.pack('<I', 0))  # compression=none
        f.write(struct.pack('<I', len(pixel_data)))
        f.write(struct.pack('<ii', 0, 0))  # pixels per meter
        f.write(struct.pack('<II', 16, 16))  # colors used, colors important

        # Palette
        f.write(palette_data)

        # Pixel data
        f.write(pixel_data)

    print(f"Wrote {path}: {width}x{total_height}, 4bpp, {len(frames)} frames, {file_size} bytes")


def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    input_path = os.path.join(project_root, 'Small-8-Direction-Characters_by_AxulArt.png')
    output_path = os.path.join(project_root, 'graphics', 'sprite', 'npc', 'villager.bmp')

    if not os.path.exists(input_path):
        print(f"ERROR: Input not found: {input_path}")
        sys.exit(1)

    print(f"Reading {input_path}...")
    w, h, pixels, palette, trans_index = decode_png(input_path)
    print(f"PNG: {w}x{h}, {len(palette)} colors, transparent index={trans_index}")

    # Extract frames for Character 2 (blue hat, block index 1)
    print(f"\nExtracting Character {CHAR_BLOCK + 1} (block {CHAR_BLOCK})...")
    frames = []
    for dir_idx in range(NUM_DIRS):
        src_col = SRC_COL_FOR_DIR[dir_idx]
        for bounce_idx in BOUNCE:
            src_row = SRC_FRAME_ROWS[bounce_idx]  # 1, 2, or 3
            cell = extract_cell(pixels, src_col, src_row, CHAR_BLOCK, CELL_W, CELL_H)
            padded = pad_frame(cell, CELL_W, CELL_H, OUT_W, OUT_H, trans_index if trans_index is not None else 0)
            frames.append(padded)

    print(f"Extracted {len(frames)} frames ({NUM_DIRS} dirs x {FRAMES_PER_DIR} frames)")

    # Remap palette to compact 16-color
    print("\nRemapping palette...")
    trans_for_remap = trans_index if trans_index is not None else 0
    frames, new_palette, remap = remap_palette(frames, palette, trans_for_remap)

    # Write BMP
    print(f"\nWriting BMP...")
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    write_4bpp_bmp(output_path, OUT_W, OUT_H, new_palette, frames)

    # Print recolor guide
    print("\n--- Palette Recolor Guide ---")
    print("To change the NPC's hat/clothing color at runtime, modify these palette indices:")
    hat_indices = []
    for old_idx in [23, 24, 25]:  # blue hat colors in source
        if old_idx in remap:
            hat_indices.append(remap[old_idx])
            r, g, b = new_palette[remap[old_idx]]
            print(f"  New index {remap[old_idx]}: RGB({r},{g},{b}) <- was source index {old_idx}")
    print(f"Hat/clothing indices to recolor: {hat_indices}")

    print("\nDone!")


if __name__ == '__main__':
    main()
