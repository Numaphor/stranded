import argparse
import json
import math
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw


ANGLE_ORDER = [0, 45, 90, 135, 180, 225, 270, 315]
BASE_INTERMEDIATE_CELL_SIZE = 128
BASE_SIDE_PADDING = 8
BASE_TOP_PADDING = 10
BASE_BOTTOM_PADDING = 4
YAW_OFFSET_DEGREES = 0.0
TOP_SHADE = 1.05
POS_X_SHADE = 0.94
NEG_X_SHADE = 0.78
POS_Y_SHADE = 0.88
NEG_Y_SHADE = 0.72
AMBIENT_LIGHT = 0.22
PITCH_DEGREES = 35.0
FOCUS_HEIGHT_RATIO = 0.55
ACTION_CONFIGS = (
    ("player_idle", "Armature|Armature|idle"),
    ("player_walk", "Armature|Armature|walk"),
)
PREVIEW_FILE_BASES = {
    "player_idle": "armature_armature_idle",
    "player_walk": "armature_armature_walk",
}
PREVIEW_CELL_SIZE_FALLBACKS = (64, 32)


def parse_args():
    repo_root = Path(__file__).resolve().parents[1]
    default_fbx = Path(r"C:\Users\numap\Downloads\Free Essential Animation CC0\Voxel character.fbx")
    default_blender = Path(r"C:\Program Files\Blender Foundation\Blender 4.2\blender.exe")
    default_preview_dir = repo_root / "scripts" / "assets" / "player_preview"

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", default=str(default_fbx))
    parser.add_argument("--blender-exe", default=str(default_blender))
    parser.add_argument("--fps", type=float, default=10.0)
    parser.add_argument("--voxel-size", type=float, default=0.03)
    parser.add_argument("--surface-shell-factor", type=float, default=1.25)
    parser.add_argument("--temp-dir", default=str(repo_root / "tmp" / "voxel_player"))
    parser.add_argument("--output-dir", default=str(repo_root / "graphics" / "sprite" / "player"))
    parser.add_argument("--preview-dir", default=str(default_preview_dir))
    parser.add_argument("--final-cell-size", type=int, default=64)
    parser.add_argument("--render-scale", type=int, default=6)
    parser.add_argument("--asset-source", choices=("voxel", "preview", "auto"), default="preview")
    parser.add_argument("--force-blender", action="store_true")
    return parser.parse_args()


def run_blender_export(blender_exe: Path, helper_script: Path, input_fbx: Path, output_dir: Path,
                       action_name: str, fps: float, voxel_size: float, surface_shell_factor: float):
    output_dir.mkdir(parents=True, exist_ok=True)
    command = [
        str(blender_exe),
        "--background",
        "--factory-startup",
        "--python",
        str(helper_script),
        "--",
        "--input",
        str(input_fbx),
        "--output-dir",
        str(output_dir),
        "--action",
        action_name,
        "--fps",
        str(fps),
        "--voxel-size",
        str(voxel_size),
        "--surface-shell-factor",
        str(surface_shell_factor),
    ]
    subprocess.run(command, check=True)


def load_action_frames(action_dir: Path):
    manifest = json.loads((action_dir / "frame_manifest.json").read_text(encoding="utf-8"))
    frames = []
    for frame_entry in manifest["frames"]:
        frame_data = json.loads((action_dir / frame_entry["filename"]).read_text(encoding="utf-8"))
        origin = frame_data["origin"]
        voxel_size = manifest["voxel_size"]
        voxels = []
        occupancy = set()

        for voxel in frame_data["voxels"]:
            x_index, y_index, z_index, red, green, blue = voxel
            occupancy.add((x_index, y_index, z_index))
            voxels.append(
                {
                    "index": (x_index, y_index, z_index),
                    "base": (
                        origin[0] + x_index * voxel_size,
                        origin[1] + y_index * voxel_size,
                        origin[2] + z_index * voxel_size,
                    ),
                    "color": (red, green, blue),
                }
            )

        frames.append(
            {
                "voxels": voxels,
                "occupancy": occupancy,
            }
        )

    return manifest, frames


def rotate_point(point, yaw_radians, pitch_radians, focus_height):
    cos_yaw = math.cos(yaw_radians)
    sin_yaw = math.sin(yaw_radians)
    cos_pitch = math.cos(pitch_radians)
    sin_pitch = math.sin(pitch_radians)

    direction_x = sin_yaw * cos_pitch
    direction_y = -cos_yaw * cos_pitch
    direction_z = sin_pitch

    forward_x = -direction_x
    forward_y = -direction_y
    forward_z = -direction_z

    right_x = direction_y
    right_y = -direction_x
    right_z = 0.0
    right_length = math.sqrt(right_x * right_x + right_y * right_y)

    if right_length <= 1e-6:
        right_x, right_y, right_z = 1.0, 0.0, 0.0
    else:
        right_x /= right_length
        right_y /= right_length

    up_x = right_y * forward_z - right_z * forward_y
    up_y = right_z * forward_x - right_x * forward_z
    up_z = right_x * forward_y - right_y * forward_x

    world_x, world_y, world_z = point
    world_z -= focus_height
    local_x = world_x * right_x + world_y * right_y + world_z * right_z
    local_y = world_x * up_x + world_y * up_y + world_z * up_z
    local_z = world_x * forward_x + world_y * forward_y + world_z * forward_z

    return local_x, local_y, local_z


def face_descriptors(voxel_size):
    return (
        ((0, 0, 1), ((0, 0, voxel_size), (voxel_size, 0, voxel_size),
                      (voxel_size, voxel_size, voxel_size), (0, voxel_size, voxel_size)), TOP_SHADE),
        ((1, 0, 0), ((voxel_size, 0, 0), (voxel_size, voxel_size, 0),
                      (voxel_size, voxel_size, voxel_size), (voxel_size, 0, voxel_size)), POS_X_SHADE),
        ((-1, 0, 0), ((0, 0, 0), (0, 0, voxel_size), (0, voxel_size, voxel_size), (0, voxel_size, 0)), NEG_X_SHADE),
        ((0, 1, 0), ((0, voxel_size, 0), (0, voxel_size, voxel_size),
                      (voxel_size, voxel_size, voxel_size), (voxel_size, voxel_size, 0)), POS_Y_SHADE),
        ((0, -1, 0), ((0, 0, 0), (voxel_size, 0, 0), (voxel_size, 0, voxel_size), (0, 0, voxel_size)), NEG_Y_SHADE),
    )


def collect_projected_quads(frame, voxel_size, angle_degrees, focus_height):
    yaw_radians = math.radians(-(angle_degrees + YAW_OFFSET_DEGREES))
    pitch_radians = math.radians(PITCH_DEGREES)
    quads = []

    for voxel in frame["voxels"]:
        base_x, base_y, base_z = voxel["base"]

        for normal, corners, shade in face_descriptors(voxel_size):
            neighbor = (
                voxel["index"][0] + normal[0],
                voxel["index"][1] + normal[1],
                voxel["index"][2] + normal[2],
            )
            if neighbor in frame["occupancy"]:
                continue

            rotated_normal = rotate_point(normal, yaw_radians, pitch_radians, 0)
            if rotated_normal[2] >= 0:
                continue

            projected_points = []
            depth_total = 0.0
            for corner in corners:
                projected = rotate_point(
                    (base_x + corner[0], base_y + corner[1], base_z + corner[2]),
                    yaw_radians,
                    pitch_radians,
                    focus_height,
                )
                projected_points.append(projected)
                depth_total += projected[2]

            quads.append(
                {
                    "depth": depth_total / len(projected_points),
                    "points": projected_points,
                    "color": tuple(
                        max(0, min(255, int(channel * (AMBIENT_LIGHT + shade * (1.0 - AMBIENT_LIGHT)))))
                        for channel in voxel["color"]
                    ),
                }
            )

    return quads


def projected_bounds_for_angle(frames, voxel_size, angle, focus_height):
    min_x = float("inf")
    max_x = float("-inf")
    min_y = float("inf")
    max_y = float("-inf")

    for frame in frames:
        for quad in collect_projected_quads(frame, voxel_size, angle, focus_height):
            for point_x, point_y, _ in quad["points"]:
                min_x = min(min_x, point_x)
                max_x = max(max_x, point_x)
                min_y = min(min_y, point_y)
                max_y = max(max_y, point_y)

    return min_x, max_x, min_y, max_y


def render_layout(final_cell_size: int, render_scale: int):
    intermediate_cell_size = max(BASE_INTERMEDIATE_CELL_SIZE, final_cell_size * render_scale)
    scale_factor = intermediate_cell_size / BASE_INTERMEDIATE_CELL_SIZE
    return {
        "intermediate_cell_size": intermediate_cell_size,
        "side_padding": int(round(BASE_SIDE_PADDING * scale_factor)),
        "top_padding": int(round(BASE_TOP_PADDING * scale_factor)),
        "bottom_padding": int(round(BASE_BOTTOM_PADDING * scale_factor)),
    }


def render_cell(quads, min_x, max_x, min_y, max_y, layout):
    intermediate_cell_size = layout["intermediate_cell_size"]
    side_padding = layout["side_padding"]
    top_padding = layout["top_padding"]
    bottom_padding = layout["bottom_padding"]

    image = Image.new("RGBA", (intermediate_cell_size, intermediate_cell_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    width = max_x - min_x
    height = max_y - min_y
    available_width = intermediate_cell_size - side_padding * 2
    available_height = intermediate_cell_size - top_padding - bottom_padding
    scale = min(available_width / width, available_height / height)
    translate_x = side_padding + (available_width - width * scale) / 2 - min_x * scale
    translate_y = intermediate_cell_size - bottom_padding - max_y * scale

    quads.sort(key=lambda quad: quad["depth"], reverse=True)
    for quad in quads:
        polygon = [
            (translate_x + point_x * scale, translate_y + point_y * scale)
            for point_x, point_y, _ in quad["points"]
        ]
        draw.polygon(polygon, fill=quad["color"])

    return image


def build_sheet(frames, voxel_size, layout, focus_height):
    frames_per_angle = len(frames)
    intermediate_cell_size = layout["intermediate_cell_size"]
    sheet = Image.new(
        "RGBA",
        (intermediate_cell_size * frames_per_angle, intermediate_cell_size * len(ANGLE_ORDER)),
        (0, 0, 0, 0),
    )

    for angle_index, angle in enumerate(ANGLE_ORDER):
        min_x, max_x, min_y, max_y = projected_bounds_for_angle(frames, voxel_size, angle, focus_height)
        for frame_index, frame in enumerate(frames):
            quads = collect_projected_quads(frame, voxel_size, angle, focus_height)
            cell = render_cell(quads, min_x, max_x, min_y, max_y, layout)
            sheet.alpha_composite(
                cell,
                (frame_index * intermediate_cell_size, angle_index * intermediate_cell_size),
            )

    return sheet


def indexed_bmp(image: Image.Image, colors: int):
    transparent_rgb = (255, 0, 255)
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
            colors=colors - 1,
            method=Image.Quantize.MEDIANCUT,
            dither=Image.Dither.NONE,
        )

        source_palette = quantized_strip.getpalette()
        used_indexes = set(quantized_strip.tobytes())

        for source_index in sorted(used_indexes):
            palette_offset = (source_index + 1) * 3
            source_offset = source_index * 3
            source_rgb = tuple(source_palette[source_offset:source_offset + 3])
            if source_rgb == transparent_rgb:
                raise ValueError("Opaque pixels quantized to the transparent color")
            palette[palette_offset:palette_offset + 3] = list(source_rgb)

        indexed_pixels = []
        opaque_index = 0

        for alpha in alpha_values:
            if alpha >= alpha_threshold:
                indexed_pixels.append(quantized_strip.getpixel((opaque_index, 0)) + 1)
                opaque_index += 1
            else:
                indexed_pixels.append(0)

        indexed.putdata(indexed_pixels)

    indexed.putpalette(palette)
    indexed.info.pop("transparency", None)
    return indexed


def resolve_preview_path(preview_dir: Path, asset_name: str, requested_cell_size: int):
    preview_file_base = PREVIEW_FILE_BASES.get(asset_name)
    if not preview_file_base:
        return None, None

    candidate_sizes = [requested_cell_size]
    for fallback_size in PREVIEW_CELL_SIZE_FALLBACKS:
        if fallback_size not in candidate_sizes:
            candidate_sizes.append(fallback_size)

    for cell_size in candidate_sizes:
        preview_path = preview_dir / f"{preview_file_base}_{cell_size}.png"
        if preview_path.exists():
            return preview_path, cell_size

    return None, None


def load_preview_sheet(preview_dir: Path, asset_name: str, requested_cell_size: int):
    preview_path, preview_cell_size = resolve_preview_path(preview_dir, asset_name, requested_cell_size)

    if not preview_path or not preview_cell_size:
        return None, None

    preview_sheet = Image.open(preview_path).convert("RGBA")
    return preview_sheet, preview_cell_size


def save_asset(sheet, output_dir: Path, asset_name: str, frames_per_angle: int, final_cell_size: int):
    target_size = (final_cell_size * frames_per_angle, final_cell_size * len(ANGLE_ORDER))
    final_sheet = sheet.resize(
        target_size,
        Image.Resampling.NEAREST,
    )
    indexed = indexed_bmp(final_sheet, 16)
    indexed.save(output_dir / f"{asset_name}.bmp", format="BMP")

    sprite_json = {
        "type": "sprite",
        "width": final_cell_size,
        "height": final_cell_size,
        "bpp_mode": "bpp_4",
        "compression": "none",
    }
    (output_dir / f"{asset_name}.json").write_text(json.dumps(sprite_json, indent=2) + "\n", encoding="utf-8")


def main():
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    helper_script = repo_root / "scripts" / "blender_export_voxel_frames.py"
    input_fbx = Path(args.input)
    blender_exe = Path(args.blender_exe)
    temp_dir = Path(args.temp_dir)
    output_dir = Path(args.output_dir)
    preview_dir = Path(args.preview_dir)
    final_cell_size = args.final_cell_size
    layout = render_layout(final_cell_size, args.render_scale)
    output_dir.mkdir(parents=True, exist_ok=True)
    asset_source = "voxel" if args.force_blender else args.asset_source

    for asset_name, action_name in ACTION_CONFIGS:
        if asset_source == "preview":
            preview_sheet, preview_cell_size = load_preview_sheet(preview_dir, asset_name, final_cell_size)

            if not preview_sheet or not preview_cell_size:
                raise FileNotFoundError(
                    f"No preview sheet found for {asset_name} in {preview_dir} at {final_cell_size}px"
                )

            save_asset(
                preview_sheet,
                output_dir,
                asset_name,
                preview_sheet.width // preview_cell_size,
                final_cell_size,
            )
            continue

        if asset_source == "auto":
            preview_sheet, preview_cell_size = load_preview_sheet(preview_dir, asset_name, final_cell_size)

            if preview_sheet and preview_cell_size:
                save_asset(
                    preview_sheet,
                    output_dir,
                    asset_name,
                    preview_sheet.width // preview_cell_size,
                    final_cell_size,
                )
                continue

        action_dir = temp_dir / asset_name
        run_blender_export(blender_exe, helper_script, input_fbx, action_dir, action_name, args.fps,
                           args.voxel_size, args.surface_shell_factor)
        manifest, frames = load_action_frames(action_dir)
        focus_height = manifest.get("height", 0) * FOCUS_HEIGHT_RATIO
        sheet = build_sheet(frames, manifest["voxel_size"], layout, focus_height)
        save_asset(sheet, output_dir, asset_name, manifest["frames_per_angle"], final_cell_size)


if __name__ == "__main__":
    main()
