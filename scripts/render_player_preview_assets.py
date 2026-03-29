import argparse
import json
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


ACTION_CONFIGS = (
    ("armature_armature_idle", "Armature|Armature|idle"),
    ("armature_armature_walk", "Armature|Armature|walk"),
)


def parse_args():
    repo_root = Path(__file__).resolve().parents[1]
    default_fbx = Path(r"C:\Users\numap\Downloads\Free Essential Animation CC0\Voxel character.fbx")
    default_blender = Path(r"C:\Program Files\Blender Foundation\Blender 4.2\blender.exe")
    default_output_dir = repo_root / "scripts" / "assets" / "player_preview"

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", default=str(default_fbx))
    parser.add_argument("--blender-exe", default=str(default_blender))
    parser.add_argument("--fps", type=float, default=10.0)
    parser.add_argument("--sizes", type=int, nargs="+", default=[32, 64])
    parser.add_argument("--output-dir", default=str(default_output_dir))
    return parser.parse_args()


def run_blender_render(blender_exe: Path, helper_script: Path, input_fbx: Path, output_dir: Path,
                       action_name: str, fps: float, cell_size: int):
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
        "--cell-size",
        str(cell_size),
    ]
    result = subprocess.run(command)
    if result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, command)
    if not (output_dir / "render_manifest.json").exists():
        raise RuntimeError(f"Blender render did not produce a manifest for {action_name} at {cell_size}px")


def assemble_sheet(render_dir: Path, output_path: Path):
    manifest = json.loads((render_dir / "render_manifest.json").read_text(encoding="utf-8"))
    cell_size = int(manifest["cell_size"])
    frames_per_angle = int(manifest["frames_per_angle"])
    angle_count = int(manifest["angle_count"])
    sheet = Image.new("RGBA", (cell_size * frames_per_angle, cell_size * angle_count), (0, 0, 0, 0))

    for angle_index in range(angle_count):
        for frame_index in range(frames_per_angle):
            frame_path = render_dir / f"frame_{frame_index:03d}_angle_{angle_index:02d}.png"
            frame_image = Image.open(frame_path).convert("RGBA")
            sheet.alpha_composite(frame_image, (frame_index * cell_size, angle_index * cell_size))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(output_path)


def main():
    args = parse_args()
    repo_root = Path(__file__).resolve().parents[1]
    helper_script = repo_root / "scripts" / "blender_render_player_frames.py"
    input_fbx = Path(args.input)
    blender_exe = Path(args.blender_exe)
    output_dir = Path(args.output_dir)

    for cell_size in args.sizes:
        with tempfile.TemporaryDirectory(prefix=f"player_preview_{cell_size}_") as temp_dir:
            temp_root = Path(temp_dir)
            for output_name, action_name in ACTION_CONFIGS:
                render_dir = temp_root / output_name
                run_blender_render(blender_exe, helper_script, input_fbx, render_dir, action_name, args.fps, cell_size)
                assemble_sheet(render_dir, output_dir / f"{output_name}_{cell_size}.png")


if __name__ == "__main__":
    main()
