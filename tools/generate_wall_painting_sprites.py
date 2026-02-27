#!/usr/bin/env python3
"""
Generate textured-triangle wall painting sprites for room_viewer.cpp.

Each full painting source image is converted into two indexed sprites:
 - *_wall_bottom.bmp  (triangle in x + y >= size - 1 from direct sample)
 - *_wall_top.bmp     (triangle in x + y >= size - 1 from mirrored sample)

Palette index 0 is always reserved for transparency, per Butano import rules.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import json
import sys

from PIL import Image, ImageEnhance


PROJECT_ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = PROJECT_ROOT / "graphics/sprite/decor"


@dataclass(frozen=True)
class SourceSpec:
    name: str
    source_path: Path
    output_size: tuple[int, int]
    bpp_mode: str | None = None
    crop_box: tuple[float, float, float, float] | None = None
    contrast: float = 1.0
    color: float = 1.0
    sharpness: float = 1.0


SOURCES = (
    SourceSpec(
        "escaping_criticism",
        PROJECT_ROOT / "butano/examples/sp_direct_bitmap_bgs/graphics/escaping_criticism.bmp",
        (32, 32),
    ),
    SourceSpec(
        "mr_and_mrs_andrews",
        PROJECT_ROOT / "butano/examples/sp_direct_bitmap_bgs/graphics/mr_and_mrs_andrews.bmp",
        (32, 32),
        # Focus on the seated couple so the 32x32 downscale remains readable.
        crop_box=(0.08, 0.08, 0.72, 0.98),
        contrast=1.2,
        color=1.15,
        sharpness=1.4,
    ),
)


@dataclass(frozen=True)
class PaintingSprites:
    name: str
    source_path: Path
    output_size: tuple[int, int]
    bpp_mode: str | None
    crop_box: tuple[float, float, float, float] | None
    contrast: float
    color: float
    sharpness: float
    top_bmp: Path
    top_json: Path
    bottom_bmp: Path
    bottom_json: Path


def _choose_transparent_color(used_colors: set[tuple[int, int, int]]) -> tuple[int, int, int]:
    candidates = [
        (0, 255, 0),
        (255, 0, 255),
        (0, 255, 255),
        (255, 255, 0),
        (255, 0, 0),
        (0, 0, 255),
    ]

    for candidate in candidates:
        if candidate not in used_colors:
            return candidate

    # Guaranteed fallback: scan RGB space until we find a free color.
    for r in range(256):
        for g in range(256):
            for b in range(256):
                color = (r, g, b)
                if color not in used_colors:
                    return color

    raise RuntimeError("Unable to select transparent color not present in quantized palette")


def _flatten_palette_256(colors: list[tuple[int, int, int]]) -> list[int]:
    palette = list(colors)

    if len(palette) > 256:
        raise ValueError(f"Expected <= 256 palette entries, got {len(palette)}")

    # BMP/Pillow palette is 256 entries (768 bytes).
    while len(palette) < 256:
        palette.append((0, 0, 0))

    flat: list[int] = []
    for r, g, b in palette:
        flat.extend((r, g, b))

    return flat


def _pixel_data(image: Image.Image) -> list[int]:
    if hasattr(image, "get_flattened_data"):
        return list(image.get_flattened_data())

    return list(image.getdata())


def _build_base_index_image(
    source: Path,
    output_size: tuple[int, int],
    bpp_mode: str | None,
    crop_box: tuple[float, float, float, float] | None,
    contrast: float,
    color: float,
    sharpness: float,
) -> tuple[Image.Image, list[tuple[int, int, int]]]:
    src = Image.open(source).convert("RGB")

    if crop_box:
        left, top, right, bottom = crop_box
        width, height = src.size
        crop_pixels = (
            int(width * left),
            int(height * top),
            int(width * right),
            int(height * bottom),
        )
        src = src.crop(crop_pixels)

    quantized_colors = 255 if bpp_mode == "bpp_8" else 15
    resized = src.resize(output_size, Image.Resampling.LANCZOS)

    if contrast != 1.0:
        resized = ImageEnhance.Contrast(resized).enhance(contrast)

    if color != 1.0:
        resized = ImageEnhance.Color(resized).enhance(color)

    if sharpness != 1.0:
        resized = ImageEnhance.Sharpness(resized).enhance(sharpness)

    quantized = resized.quantize(
        colors=quantized_colors,
        method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.NONE,
    )

    quantized_palette = quantized.getpalette()
    if not quantized_palette:
        raise RuntimeError(f"Quantized image has no palette: {source}")

    data = _pixel_data(quantized)
    used_indices = sorted(set(data))
    used_colors: list[tuple[int, int, int]] = []

    for index in used_indices:
        offset = index * 3
        used_colors.append(
            (
                quantized_palette[offset],
                quantized_palette[offset + 1],
                quantized_palette[offset + 2],
            )
        )

    transparent_color = _choose_transparent_color(set(used_colors))
    final_colors16 = [transparent_color] + used_colors
    final_palette = _flatten_palette_256(final_colors16)

    remap = {src_index: dst_index + 1 for dst_index, src_index in enumerate(used_indices)}
    base_index_data = [remap[index] for index in data]

    base = Image.new("P", output_size)
    base.putpalette(final_palette)
    base.putdata(base_index_data)
    return base, final_colors16


def _triangular_variant(base: Image.Image, mirrored: bool) -> Image.Image:
    width, height = base.size
    opaque_mask_sum = width - 1
    base_pixels = _pixel_data(base)
    out_pixels = [0] * (width * height)

    for y in range(height):
        for x in range(width):
            out_index = y * width + x
            if x + y >= opaque_mask_sum:
                sx = width - 1 - x if mirrored else x
                sy = height - 1 - y if mirrored else y
                sample_index = sy * width + sx
                out_pixels[out_index] = base_pixels[sample_index]
            else:
                out_pixels[out_index] = 0

    out = Image.new("P", base.size)
    out.putpalette(base.getpalette())
    out.putdata(out_pixels)
    return out


def _validate_outputs(top: Image.Image, bottom: Image.Image, output_size: tuple[int, int]) -> None:
    if top.size != output_size or bottom.size != output_size:
        raise AssertionError("Unexpected output size")

    if output_size[0] != output_size[1]:
        raise AssertionError("Only square output sizes are supported")

    top_palette = top.getpalette()
    bottom_palette = bottom.getpalette()
    if top_palette != bottom_palette:
        raise AssertionError("Top and bottom palettes differ")

    top_data = _pixel_data(top)
    bottom_data = _pixel_data(bottom)
    width, height = output_size
    opaque_mask_sum = width - 1
    saw_transparent = False

    for y in range(height):
        for x in range(width):
            idx = y * width + x
            in_opaque_region = x + y >= opaque_mask_sum
            top_pixel = top_data[idx]
            bottom_pixel = bottom_data[idx]

            if in_opaque_region:
                if top_pixel == 0 or bottom_pixel == 0:
                    raise AssertionError("Opaque region uses transparent index 0")
            else:
                saw_transparent = True
                if top_pixel != 0 or bottom_pixel != 0:
                    raise AssertionError("Transparent region uses non-zero index")

    if not saw_transparent:
        raise AssertionError("No transparent region detected")


def _write_sprite_json(path: Path, output_size: tuple[int, int], bpp_mode: str | None) -> None:
    payload = {"type": "sprite", "width": output_size[0], "height": output_size[1]}

    if bpp_mode:
        payload["bpp_mode"] = bpp_mode

    path.write_text(json.dumps(payload, indent=4) + "\n", encoding="utf-8")


def _build_paths(source: SourceSpec) -> PaintingSprites:
    return PaintingSprites(
        name=source.name,
        source_path=source.source_path,
        output_size=source.output_size,
        bpp_mode=source.bpp_mode,
        crop_box=source.crop_box,
        contrast=source.contrast,
        color=source.color,
        sharpness=source.sharpness,
        top_bmp=OUTPUT_DIR / f"{source.name}_wall_top.bmp",
        top_json=OUTPUT_DIR / f"{source.name}_wall_top.json",
        bottom_bmp=OUTPUT_DIR / f"{source.name}_wall_bottom.bmp",
        bottom_json=OUTPUT_DIR / f"{source.name}_wall_bottom.json",
    )


def _generate_one(config: PaintingSprites) -> None:
    if not config.source_path.is_file():
        raise FileNotFoundError(f"Missing source painting: {config.source_path}")

    base, _ = _build_base_index_image(
        config.source_path,
        config.output_size,
        config.bpp_mode,
        config.crop_box,
        config.contrast,
        config.color,
        config.sharpness,
    )
    bottom = _triangular_variant(base, mirrored=False)
    top = _triangular_variant(base, mirrored=True)
    _validate_outputs(top, bottom, config.output_size)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    bottom.save(config.bottom_bmp, format="BMP")
    top.save(config.top_bmp, format="BMP")
    _write_sprite_json(config.bottom_json, config.output_size, config.bpp_mode)
    _write_sprite_json(config.top_json, config.output_size, config.bpp_mode)

    print(f"Generated {config.bottom_bmp.relative_to(PROJECT_ROOT)}")
    print(f"Generated {config.top_bmp.relative_to(PROJECT_ROOT)}")


def main() -> int:
    try:
        for source in SOURCES:
            _generate_one(_build_paths(source))
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
