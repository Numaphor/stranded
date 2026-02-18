"""
Convert Eris Esra 16x16 character template PNGs to Butano GBA BMP format.

Converts each sheet individually (Idle, Walk) without combining them.
Replaces transparent pixels with magenta, builds exact palette, saves as
4bpp indexed BMP (matching Butano's expected format).

Output:
  graphics/sprite/player/eris_idle.bmp
  graphics/sprite/player/eris_walk.bmp
"""

from PIL import Image
import struct
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
SOURCE_DIR = r"C:\Users\numap\Downloads\Eris Esra's Character Template 4.0\16x16"
OUTPUT_DIR = os.path.join(PROJECT_DIR, "graphics", "sprite", "player")

MAGENTA = (255, 0, 255)
FRAME = 16


def save_4bpp_bmp(path, width, height, palette_colors, pixel_indices):
    """Save a 4bpp indexed BMP file (matching Butano's expected format)."""
    # BMP row stride: each row is (width/2) bytes, padded to 4-byte boundary
    row_bytes = (width + 1) // 2  # 2 pixels per byte for 4bpp
    row_stride = (row_bytes + 3) & ~3  # pad to 4-byte boundary

    # Palette: 16 entries * 4 bytes (BGRA)
    palette_size = 16 * 4
    pixel_data_size = row_stride * height
    pixel_offset = 14 + 40 + palette_size  # BM header + DIB header + palette
    file_size = pixel_offset + pixel_data_size

    with open(path, 'wb') as f:
        # BM file header (14 bytes)
        f.write(b'BM')
        f.write(struct.pack('<I', file_size))
        f.write(struct.pack('<HH', 0, 0))  # reserved
        f.write(struct.pack('<I', pixel_offset))

        # DIB header (BITMAPINFOHEADER, 40 bytes)
        f.write(struct.pack('<I', 40))  # header size
        f.write(struct.pack('<i', width))
        f.write(struct.pack('<i', height))  # positive = bottom-up
        f.write(struct.pack('<HH', 1, 4))  # planes=1, bpp=4
        f.write(struct.pack('<I', 0))  # compression=0 (BI_RGB)
        f.write(struct.pack('<I', pixel_data_size))
        f.write(struct.pack('<ii', 3780, 3780))  # pixels per meter
        f.write(struct.pack('<II', 16, 16))  # colors used, important

        # Palette (16 entries, BGRA format)
        for i in range(16):
            if i < len(palette_colors):
                r, g, b = palette_colors[i]
            else:
                r, g, b = 0, 0, 0
            f.write(struct.pack('BBBB', b, g, r, 0))  # BGRA

        # Pixel data (bottom-up row order, 4bpp packed)
        for y in range(height - 1, -1, -1):
            row = bytearray(row_stride)
            for x in range(width):
                idx = pixel_indices[y * width + x]
                byte_pos = x // 2
                if x % 2 == 0:
                    row[byte_pos] |= (idx << 4)  # high nibble
                else:
                    row[byte_pos] |= idx  # low nibble
            f.write(row)


def convert_sheet(src_path, out_path):
    """Convert a single RGBA PNG sprite sheet to 4bpp BMP."""
    img = Image.open(src_path).convert("RGBA")
    print(f"  Source: {img.size} mode={img.mode}")

    # Flatten: replace transparent pixels with magenta
    rgb = Image.new("RGB", img.size, MAGENTA)
    rgb.paste(img, mask=img.split()[3])

    # Crop to exact tile grid (remove bottom padding)
    cols = rgb.width // FRAME
    rows = rgb.height // FRAME
    cropped = rgb.crop((0, 0, cols * FRAME, rows * FRAME))
    print(f"  Cropped: {cropped.size} ({cols}x{rows} = {cols * rows} frames)")

    # Collect unique colors (exact, no quantization)
    pixels = list(cropped.getdata())
    unique_colors = set(pixels)
    print(f"  Unique colors: {len(unique_colors)}")

    # Build palette with magenta at index 0
    palette_colors = [MAGENTA]
    for c in sorted(unique_colors):
        if c != MAGENTA:
            palette_colors.append(c)

    if len(palette_colors) > 16:
        print(f"  WARNING: {len(palette_colors)} colors exceeds 16-color limit!")

    # Create color -> index mapping
    color_to_idx = {c: i for i, c in enumerate(palette_colors)}

    # Map pixels to indices
    idx_pixels = [color_to_idx[p] for p in pixels]

    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    save_4bpp_bmp(out_path, cropped.width, cropped.height, palette_colors, idx_pixels)
    print(f"  Saved: {out_path}")

    # Verify
    verify = Image.open(out_path)
    print(f"  Verify: {verify.size}, mode={verify.mode}")
    vpal = verify.getpalette()
    print(f"  Palette[0]: ({vpal[0]}, {vpal[1]}, {vpal[2]})")
    vdata = list(verify.getdata())
    print(f"  Unique indices used: {len(set(vdata))}")

    # Verify pixel data matches
    mismatches = 0
    for i, (orig, read) in enumerate(zip(idx_pixels, vdata)):
        if orig != read:
            mismatches += 1
            if mismatches <= 5:
                y, x = divmod(i, cropped.width)
                print(f"  MISMATCH at ({x},{y}): wrote {orig}, read {read}")
    if mismatches:
        print(f"  Total mismatches: {mismatches}")
    else:
        print(f"  All pixels verified OK")


def main():
    sheets = {
        "eris_idle": "16x16 Idle-Sheet.png",
        "eris_walk": "16x16 Walk-Sheet.png",
    }

    for name, filename in sheets.items():
        src = os.path.join(SOURCE_DIR, filename)
        dst = os.path.join(OUTPUT_DIR, f"{name}.bmp")
        print(f"\nConverting {filename} -> {name}.bmp")
        convert_sheet(src, dst)


if __name__ == "__main__":
    main()
