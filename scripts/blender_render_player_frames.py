import argparse
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector


ANGLE_ORDER = [0, 45, 90, 135, 180, 225, 270, 315]
YAW_OFFSET_DEGREES = 0.0
CAMERA_PITCH_DEGREES = 35.0
FRAME_FILL_RATIO = 0.9
FOCUS_HEIGHT_RATIO = 0.55
CAMERA_DISTANCE = 10.0


def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output-dir", required=True)
    parser.add_argument("--action", required=True)
    parser.add_argument("--fps", type=float, default=10.0)
    parser.add_argument("--cell-size", type=int, required=True)
    return parser.parse_args(argv)


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def relink_missing_images(search_dir: Path):
    png_files = list(search_dir.glob("*.png"))
    png_by_stem = {path.stem.lower(): path for path in png_files}
    aliases = {
        "outsider": "voxel character texture",
        "character": "voxel character texture",
    }

    for image in bpy.data.images:
        candidate = Path(image.filepath_from_user())
        if candidate.exists():
            continue

        stem = candidate.stem.lower()
        mapped_stem = aliases.get(stem, stem)
        target = png_by_stem.get(mapped_stem)

        if target is None:
            target = next(
                (
                    path
                    for path in png_files
                    if mapped_stem in path.stem.lower() or path.stem.lower() in mapped_stem
                ),
                None,
            )

        if target is not None:
            image.filepath = str(target)
            image.reload()


def set_active_action(imported_objects, action_name: str):
    action = bpy.data.actions.get(action_name)
    if action is None:
        raise RuntimeError(f"Action '{action_name}' was not found")

    for obj in imported_objects:
        if obj.type == "ARMATURE":
            obj.animation_data_create()
            obj.animation_data.action = action

    return action


def find_animation_range(scene):
    action = None
    for obj in bpy.data.objects:
        if obj.animation_data and obj.animation_data.action:
            action = obj.animation_data.action
            break

    if action is not None:
        frame_start, frame_end = action.frame_range
    else:
        frame_start = scene.frame_start
        frame_end = scene.frame_end

    frame_start = float(frame_start)
    frame_end = float(frame_end)
    if frame_end <= frame_start:
        frame_end = frame_start + 1.0

    return frame_start, frame_end


def source_fps(scene):
    return scene.render.fps / scene.render.fps_base


def build_sample_positions(frame_start, frame_end, src_fps, target_fps):
    duration_frames = frame_end - frame_start
    duration_seconds = max(duration_frames / src_fps, 1.0 / src_fps)
    sample_count = max(1, int(round(duration_seconds * target_fps)))
    step = duration_frames / sample_count
    return [frame_start + step * index for index in range(sample_count)]


def set_scene_frame(scene, frame_value: float):
    base_frame = int(math.floor(frame_value))
    subframe = frame_value - base_frame
    scene.frame_set(base_frame, subframe=subframe)
    bpy.context.view_layer.update()


def mesh_world_bounds(mesh_obj, sample_positions):
    scene = bpy.context.scene
    depsgraph = bpy.context.evaluated_depsgraph_get()
    min_corner = Vector((float("inf"), float("inf"), float("inf")))
    max_corner = Vector((float("-inf"), float("-inf"), float("-inf")))

    for frame_value in sample_positions:
        set_scene_frame(scene, frame_value)
        obj_eval = mesh_obj.evaluated_get(depsgraph)
        mesh = obj_eval.to_mesh()
        try:
            for vertex in mesh.vertices:
                point = obj_eval.matrix_world @ vertex.co
                min_corner.x = min(min_corner.x, point.x)
                min_corner.y = min(min_corner.y, point.y)
                min_corner.z = min(min_corner.z, point.z)
                max_corner.x = max(max_corner.x, point.x)
                max_corner.y = max(max_corner.y, point.y)
                max_corner.z = max(max_corner.z, point.z)
        finally:
            obj_eval.to_mesh_clear()

    return min_corner, max_corner


def combined_mesh_bounds(mesh_objects, sample_positions):
    min_corner = Vector((float("inf"), float("inf"), float("inf")))
    max_corner = Vector((float("-inf"), float("-inf"), float("-inf")))

    for mesh_obj in mesh_objects:
        mesh_min, mesh_max = mesh_world_bounds(mesh_obj, sample_positions)
        min_corner.x = min(min_corner.x, mesh_min.x)
        min_corner.y = min(min_corner.y, mesh_min.y)
        min_corner.z = min(min_corner.z, mesh_min.z)
        max_corner.x = max(max_corner.x, mesh_max.x)
        max_corner.y = max(max_corner.y, mesh_max.y)
        max_corner.z = max(max_corner.z, mesh_max.z)

    return min_corner, max_corner


def recenter_roots_to_ground(roots, min_corner: Vector, max_corner: Vector):
    offset = Vector((
        (min_corner.x + max_corner.x) / 2,
        (min_corner.y + max_corner.y) / 2,
        min_corner.z,
    ))

    for root in roots:
        world = root.matrix_world.copy()
        world.translation = world.translation - offset
        root.matrix_world = world

    bpy.context.view_layer.update()

    return max_corner.z - min_corner.z


def frame_vertices(mesh_objects, frame_value):
    scene = bpy.context.scene
    depsgraph = bpy.context.evaluated_depsgraph_get()
    set_scene_frame(scene, frame_value)
    vertices = []

    for mesh_obj in mesh_objects:
        obj_eval = mesh_obj.evaluated_get(depsgraph)
        mesh = obj_eval.to_mesh()
        try:
            vertices.extend(obj_eval.matrix_world @ vertex.co for vertex in mesh.vertices)
        finally:
            obj_eval.to_mesh_clear()

    return vertices


def create_light(name: str, energy: float, rotation_euler, use_shadow: bool = True):
    light_data = bpy.data.lights.new(name, type="SUN")
    light_data.energy = energy
    light_data.use_shadow = use_shadow
    light_object = bpy.data.objects.new(name, light_data)
    bpy.context.scene.collection.objects.link(light_object)
    light_object.rotation_euler = rotation_euler
    return light_object


def configure_scene(cell_size: int):
    scene = bpy.context.scene

    if "BLENDER_EEVEE_NEXT" in bpy.types.RenderSettings.bl_rna.properties["engine"].enum_items.keys():
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    else:
        scene.render.engine = "BLENDER_EEVEE"

    scene.render.film_transparent = True
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.resolution_percentage = 100
    scene.render.resolution_x = cell_size
    scene.render.resolution_y = cell_size
    scene.render.pixel_aspect_x = 1
    scene.render.pixel_aspect_y = 1
    scene.render.filter_size = 0.01
    scene.render.use_compositing = False
    scene.render.use_sequencer = False
    scene.render.dither_intensity = 0.0
    scene.display_settings.display_device = "sRGB"
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.look = "None"
    scene.view_settings.exposure = 0.0
    scene.view_settings.gamma = 1.0
    if scene.world is None:
        scene.world = bpy.data.worlds.new("PreviewWorld")
    scene.world.color = (0.0, 0.0, 0.0)

    eevee = scene.eevee
    eevee.taa_render_samples = 1
    eevee.taa_samples = 1
    eevee.use_gtao = False
    eevee.use_bloom = False

    return scene


def create_camera():
    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera_data.type = "ORTHO"
    camera_object = bpy.data.objects.new("PreviewCamera", camera_data)
    bpy.context.scene.collection.objects.link(camera_object)
    bpy.context.scene.camera = camera_object
    return camera_object


def camera_direction(angle_degrees: float):
    yaw_radians = math.radians(angle_degrees + YAW_OFFSET_DEGREES)
    pitch_radians = math.radians(CAMERA_PITCH_DEGREES)
    cos_pitch = math.cos(pitch_radians)
    return Vector((
        math.sin(yaw_radians) * cos_pitch,
        -math.cos(yaw_radians) * cos_pitch,
        math.sin(pitch_radians),
    ))


def aim_camera(camera_object, angle_degrees: float, focus_height: float):
    direction = camera_direction(angle_degrees)
    target = Vector((0.0, 0.0, focus_height))
    camera_object.location = target + direction * CAMERA_DISTANCE
    camera_object.rotation_euler = (target - camera_object.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.view_layer.update()


def projected_span(camera_object, sample_positions, mesh_objects, focus_height: float):
    min_x = float("inf")
    max_x = float("-inf")
    min_y = float("inf")
    max_y = float("-inf")
    camera_matrix = camera_object.matrix_world.inverted()

    for frame_value in sample_positions:
        for vertex in frame_vertices(mesh_objects, frame_value):
            local = camera_matrix @ vertex
            min_x = min(min_x, local.x)
            max_x = max(max_x, local.x)
            min_y = min(min_y, local.y)
            max_y = max(max_y, local.y)

    width = max_x - min_x
    height = max_y - min_y
    return max(width, height)


def global_ortho_scale(camera_object, sample_positions, mesh_objects, focus_height: float):
    max_span = 0.0
    for angle in ANGLE_ORDER:
        aim_camera(camera_object, angle, focus_height)
        max_span = max(max_span, projected_span(camera_object, sample_positions, mesh_objects, focus_height))
    return max_span / FRAME_FILL_RATIO


def render_frames(scene, camera_object, camera_fill_light, output_dir: Path, sample_positions, action_name: str,
                  focus_height: float):
    for angle_index, angle in enumerate(ANGLE_ORDER):
        aim_camera(camera_object, angle, focus_height)
        camera_fill_light.rotation_euler = camera_object.rotation_euler
        bpy.context.view_layer.update()

        for frame_index, frame_value in enumerate(sample_positions):
            set_scene_frame(scene, frame_value)
            output_path = output_dir / f"frame_{frame_index:03d}_angle_{angle_index:02d}.png"
            scene.render.filepath = str(output_path)
            bpy.ops.render.render(write_still=True)

    manifest = {
        "action_name": action_name,
        "frames_per_angle": len(sample_positions),
        "angle_count": len(ANGLE_ORDER),
        "angle_order": ANGLE_ORDER,
        "cell_size": scene.render.resolution_x,
    }
    (output_dir / "render_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main():
    args = parse_args()
    input_path = Path(args.input)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    reset_scene()

    before_names = set(bpy.data.objects.keys())
    bpy.ops.import_scene.fbx(filepath=str(input_path))
    imported_objects = [bpy.data.objects[name] for name in bpy.data.objects.keys() if name not in before_names]

    relink_missing_images(input_path.parent)
    set_active_action(imported_objects, args.action)

    mesh_objects = [obj for obj in imported_objects if obj.type == "MESH"]
    root_objects = [obj for obj in imported_objects if obj.parent is None]

    if not mesh_objects:
        raise RuntimeError("No mesh objects were imported from the FBX")

    scene = configure_scene(args.cell_size)
    create_light("PreviewKeyLight", 3.0, (math.radians(40), 0.0, math.radians(35)))
    create_light("PreviewFillLight", 1.2, (math.radians(55), 0.0, math.radians(-145)))
    camera_fill_light = create_light("PreviewCameraFillLight", 1.35, (0.0, 0.0, 0.0), use_shadow=False)

    frame_start, frame_end = find_animation_range(scene)
    sample_positions = build_sample_positions(frame_start, frame_end, source_fps(scene), args.fps)
    min_corner, max_corner = combined_mesh_bounds(mesh_objects, sample_positions)
    height = recenter_roots_to_ground(root_objects, min_corner, max_corner)
    camera_object = create_camera()
    focus_height = height * FOCUS_HEIGHT_RATIO
    camera_object.data.ortho_scale = global_ortho_scale(camera_object, sample_positions, mesh_objects, focus_height)
    render_frames(scene, camera_object, camera_fill_light, output_dir, sample_positions, args.action, focus_height)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise
