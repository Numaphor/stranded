import argparse
import json
import subprocess
from pathlib import Path

from PIL import Image

PROP_CONFIGS = (
    {
        "input": r"tmp/apartment_assets/interior_obj/3D Voxel Interior Furniture Pack/Chairs/Interior_Furniture_Couch_01.obj",
        "output_name": "interior_prop_couch_01",
        "cell_size": 32,
        "yaw_offset_degrees": 0.0,
    },
    {
        "input": r"tmp/apartment_assets/interior_obj/3D Voxel Interior Furniture Pack/Chairs/Interior_Furniture_Chair_01.obj",
        "output_name": "interior_prop_chair_01",
        "cell_size": 32,
        "yaw_offset_degrees": 0.0,
    },
    {
        "input": r"tmp/apartment_assets/interior_obj/3D Voxel Interior Furniture Pack/Tables/Interior_Furniture_Table_Coffee_Type01_01.obj",
        "output_name": "interior_prop_coffee_table_01",
        "cell_size": 32,
        "yaw_offset_degrees": 0.0,
    },
    {
        "input": r"tmp/apartment_assets/interior_obj/3D Voxel Interior Furniture Pack/Misc/Interior_Furniture_Misc_Shelf_01.obj",
        "output_name": "interior_prop_shelf_01",
        "cell_size": 32,
        "yaw_offset_degrees": 0.0,
    },
    {
        "input": r"tmp/apartment_assets/interior_obj/3D Voxel Interior Furniture Pack/Misc/Plants/Interior_Furniture_Misc_Plant_01.obj",
        "output_name": "interior_prop_plant_01",
        "cell_size": 32,
        "yaw_offset_degrees": 0.0,
    },
    {
        "input": r"tmp/apartment_assets/interior_obj/3D Voxel Interior Furniture Pack/Misc/Interior_Furniture_Misc_Clock_01.obj",
        "output_name": "interior_prop_clock_01",
        "cell_size": 32,
        "yaw_offset_degrees": 0.0,
    },
)


def parse_args():
    repo_root = Path(__file__).resolve().parents[1]
    default_blender = Path(r"C:\Program Files\Blender Foundation\Blender 4.2\blender.exe")
    default_output_dir = repo_root / "graphics" / "sprite" / "interior_props"

    parser = argparse.ArgumentParser()
    parser.add_argument("--blender-exe", default=str(default_blender))
    parser.add_argument("--output-dir", default=str(default_output_dir))
    parser.add_argument("--temp-dir", default=str(repo_root / "tmp" / "interior_prop_bakes"))
    return parser.parse_args()


def run_blender_render(blender_exe: Path, helper_script: Path, input_path: Path, output_dir: Path,
                       cell_size: int, yaw_offset_degrees: float):
    output_dir.mkdir(parents=True, exist_ok=True)
    command = [
        str(blender_exe),
        "--background",
        "--factory-startup",
        "--python",
        str(helper_script),
        "--",
        "--input",
        str(input_path),
        "--output-dir",
        str(output_dir),
        "--cell-size",
        str(cell_size),
        "--yaw-offset-degrees",
        str(yaw_offset_degrees),
    ]
    subprocess.run(command, check=True)


def assemble_sheet(render_dir: Path):
    manifest = json.loads((render_dir / "render_manifest.json").read_text(encoding="utf-8"))
    cell_size = int(manifest["cell_size"])
    angle_count = int(manifest["angle_count"])
    sheet = Image.new("RGBA", (cell_size, cell_size * angle_count), (0, 0, 0, 0))

    for angle_index in range(angle_count):
        frame_path = render_dir / f"angle_{angle_index:02d}.png"
        frame_image = Image.open(frame_path).convert("RGBA")
        sheet.alpha_composite(frame_image, (0, angle_index * cell_size))

    return sheet


def build_shared_palette(images: list[Image.Image], colors: int):
    transparent_rgb = (255, 0, 255)
    alpha_threshold = 96
    opaque_pixels = []

    for image in images:
        alpha_values = image.getchannel("A").tobytes()
        rgb_bytes = image.convert("RGB").tobytes()
        rgb_values = [tuple(rgb_bytes[index:index + 3]) for index in range(0, len(rgb_bytes), 3)]
        opaque_pixels.extend(rgb for rgb, alpha in zip(rgb_values, alpha_values) if alpha >= alpha_threshold)

    palette = [transparent_rgb]

    if not opaque_pixels:
        return palette

    opaque_strip = Image.new("RGB", (len(opaque_pixels), 1))
    opaque_strip.putdata(opaque_pixels)
    quantized_strip = opaque_strip.quantize(
        colors=colors - 1,
        method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.NONE,
    )

    source_palette = quantized_strip.getpalette()
    used_indexes = sorted(set(quantized_strip.tobytes()))

    for source_index in used_indexes:
        source_offset = source_index * 3
        source_rgb = tuple(source_palette[source_offset:source_offset + 3])
        if source_rgb == transparent_rgb:
            raise ValueError("Opaque pixels quantized to the transparent color")
        palette.append(source_rgb)

    return palette


def indexed_bmp(image: Image.Image, shared_palette: list[tuple[int, int, int]]):
    transparent_rgb = shared_palette[0]
    quantize_palette = [0] * 768
    quantize_palette_image = Image.new("P", (1, 1))
    quantize_colors = shared_palette[1:]

    if not quantize_colors:
        quantize_colors = [(0, 0, 0)]

    for index, rgb in enumerate(quantize_colors):
        palette_offset = index * 3
        quantize_palette[palette_offset:palette_offset + 3] = list(rgb)

    quantize_palette_image.putpalette(quantize_palette)

    alpha_values = image.getchannel("A").tobytes()
    rgb_bytes = image.convert("RGB").tobytes()
    rgb_values = [tuple(rgb_bytes[index:index + 3]) for index in range(0, len(rgb_bytes), 3)]
    alpha_threshold = 96
    opaque_pixels = [rgb for rgb, alpha in zip(rgb_values, alpha_values) if alpha >= alpha_threshold]

    indexed = Image.new("P", image.size, 0)
    palette = [0] * 768
    palette[0:3] = list(transparent_rgb)

    if opaque_pixels:
        opaque_strip = Image.new("RGB", (len(opaque_pixels), 1))
        opaque_strip.putdata(opaque_pixels)
        quantized_strip = opaque_strip.quantize(
            palette=quantize_palette_image,
            dither=Image.Dither.NONE,
        )

        indexed_pixels = []
        opaque_index = 0

        for alpha in alpha_values:
            if alpha >= alpha_threshold:
                indexed_pixels.append(quantized_strip.getpixel((opaque_index, 0)) + 1)
                opaque_index += 1
            else:
                indexed_pixels.append(0)

        indexed.putdata(indexed_pixels)

    for palette_index, rgb in enumerate(shared_palette[1:], start=1):
        palette_offset = palette_index * 3
        palette[palette_offset:palette_offset + 3] = list(rgb)

    indexed.putpalette(palette)
    indexed.info.pop("transparency", None)
    return indexed


def save_asset(sheet: Image.Image, output_dir: Path, output_name: str, cell_size: int,
               shared_palette: list[tuple[int, int, int]]):
    indexed = indexed_bmp(sheet, shared_palette)
    indexed.save(output_dir / f"{output_name}.bmp", format="BMP")

    sprite_json = {
        "type": "sprite",
        "width": cell_size,
        "height": cell_size,
        "bpp_mode": "bpp_4",
        "compression": "none",
    }
    (output_dir / f"{output_name}.json").write_text(json.dumps(sprite_json, indent=2) + "\n", encoding="utf-8")


def main():
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    helper_script = repo_root / "scripts" / "blender_render_static_prop_frames.py"
    blender_exe = Path(args.blender_exe)
    output_dir = Path(args.output_dir)
    temp_dir = Path(args.temp_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    rendered_sheets: list[tuple[dict, Image.Image]] = []

    for prop_config in PROP_CONFIGS:
        input_path = repo_root / prop_config["input"]
        if not input_path.exists():
            raise FileNotFoundError(f"Missing prop source: {input_path}")

        render_dir = temp_dir / prop_config["output_name"]
        if render_dir.exists():
            for child in render_dir.iterdir():
                if child.is_file():
                    child.unlink()
        render_dir.mkdir(parents=True, exist_ok=True)

        run_blender_render(
            blender_exe,
            helper_script,
            input_path,
            render_dir,
            prop_config["cell_size"],
            prop_config["yaw_offset_degrees"],
        )
        final_sheet = assemble_sheet(render_dir)
        rendered_sheets.append((prop_config, final_sheet))

    shared_palette = build_shared_palette([sheet for _, sheet in rendered_sheets], 16)

    for prop_config, final_sheet in rendered_sheets:
        save_asset(final_sheet, output_dir, prop_config["output_name"], prop_config["cell_size"], shared_palette)


if __name__ == "__main__":
    main()
