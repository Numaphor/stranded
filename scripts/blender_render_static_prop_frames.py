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
    parser.add_argument("--cell-size", type=int, required=True)
    parser.add_argument("--yaw-offset-degrees", type=float, default=0.0)
    return parser.parse_args(argv)


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def relink_missing_images(search_dir: Path):
    png_files = list(search_dir.glob("*.png"))
    png_by_stem = {path.stem.lower(): path for path in png_files}

    for image in bpy.data.images:
        candidate = Path(image.filepath_from_user())
        if candidate.exists():
            continue

        stem = candidate.stem.lower()
        target = png_by_stem.get(stem)

        if target is None:
            target = next(
                (
                    path
                    for path in png_files
                    if stem in path.stem.lower() or path.stem.lower() in stem
                ),
                None,
            )

        if target is not None:
            image.filepath = str(target)
            image.reload()


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


def import_obj(input_path: Path):
    before_names = set(bpy.data.objects.keys())

    try:
        bpy.ops.wm.obj_import(filepath=str(input_path))
    except Exception:
        bpy.ops.import_scene.obj(filepath=str(input_path))

    imported_objects = [bpy.data.objects[name] for name in bpy.data.objects.keys() if name not in before_names]
    return imported_objects


def mesh_world_bounds(mesh_objects):
    min_corner = Vector((float("inf"), float("inf"), float("inf")))
    max_corner = Vector((float("-inf"), float("-inf"), float("-inf")))
    depsgraph = bpy.context.evaluated_depsgraph_get()

    for mesh_obj in mesh_objects:
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


def recenter_roots_to_ground(roots, min_corner: Vector, max_corner: Vector):
    offset = Vector((
        (min_corner.x + max_corner.x) * 0.5,
        (min_corner.y + max_corner.y) * 0.5,
        min_corner.z,
    ))

    for root in roots:
        world = root.matrix_world.copy()
        world.translation = world.translation - offset
        root.matrix_world = world

    bpy.context.view_layer.update()
    return max_corner.z - min_corner.z


def create_light(name: str, energy: float, rotation_euler, use_shadow: bool = True):
    light_data = bpy.data.lights.new(name, type="SUN")
    light_data.energy = energy
    light_data.use_shadow = use_shadow
    light_object = bpy.data.objects.new(name, light_data)
    bpy.context.scene.collection.objects.link(light_object)
    light_object.rotation_euler = rotation_euler
    return light_object


def create_camera():
    camera_data = bpy.data.cameras.new("PreviewCamera")
    camera_data.type = "ORTHO"
    camera_object = bpy.data.objects.new("PreviewCamera", camera_data)
    bpy.context.scene.collection.objects.link(camera_object)
    bpy.context.scene.camera = camera_object
    return camera_object


def camera_direction(angle_degrees: float, yaw_offset_degrees: float):
    yaw_radians = math.radians(angle_degrees + yaw_offset_degrees)
    pitch_radians = math.radians(CAMERA_PITCH_DEGREES)
    cos_pitch = math.cos(pitch_radians)
    return Vector((
        math.sin(yaw_radians) * cos_pitch,
        -math.cos(yaw_radians) * cos_pitch,
        math.sin(pitch_radians),
    ))


def aim_camera(camera_object, angle_degrees: float, focus_height: float, yaw_offset_degrees: float):
    direction = camera_direction(angle_degrees, yaw_offset_degrees)
    target = Vector((0.0, 0.0, focus_height))
    camera_object.location = target + direction * CAMERA_DISTANCE
    camera_object.rotation_euler = (target - camera_object.location).to_track_quat("-Z", "Y").to_euler()
    bpy.context.view_layer.update()


def frame_vertices(mesh_objects):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    vertices = []

    for mesh_obj in mesh_objects:
        obj_eval = mesh_obj.evaluated_get(depsgraph)
        mesh = obj_eval.to_mesh()
        try:
            vertices.extend(obj_eval.matrix_world @ vertex.co for vertex in mesh.vertices)
        finally:
            obj_eval.to_mesh_clear()

    return vertices


def projected_span(camera_object, mesh_objects, focus_height: float):
    min_x = float("inf")
    max_x = float("-inf")
    min_y = float("inf")
    max_y = float("-inf")
    camera_matrix = camera_object.matrix_world.inverted()

    for vertex in frame_vertices(mesh_objects):
        local = camera_matrix @ vertex
        min_x = min(min_x, local.x)
        max_x = max(max_x, local.x)
        min_y = min(min_y, local.y)
        max_y = max(max_y, local.y)

    width = max_x - min_x
    height = max_y - min_y
    return max(width, height)


def global_ortho_scale(camera_object, mesh_objects, focus_height: float, yaw_offset_degrees: float):
    max_span = 0.0

    for angle in ANGLE_ORDER:
        aim_camera(camera_object, angle, focus_height, yaw_offset_degrees)
        max_span = max(max_span, projected_span(camera_object, mesh_objects, focus_height))

    return max_span / FRAME_FILL_RATIO


def render_frames(scene, camera_object, camera_fill_light, output_dir: Path, focus_height: float,
                  yaw_offset_degrees: float):
    for angle_index, angle in enumerate(ANGLE_ORDER):
        aim_camera(camera_object, angle, focus_height, yaw_offset_degrees)
        camera_fill_light.rotation_euler = camera_object.rotation_euler
        bpy.context.view_layer.update()

        output_path = output_dir / f"angle_{angle_index:02d}.png"
        scene.render.filepath = str(output_path)
        bpy.ops.render.render(write_still=True)

    manifest = {
        "angle_count": len(ANGLE_ORDER),
        "angle_order": ANGLE_ORDER,
        "cell_size": scene.render.resolution_x,
        "yaw_offset_degrees": yaw_offset_degrees,
    }
    (output_dir / "render_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main():
    args = parse_args()
    input_path = Path(args.input)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    reset_scene()
    imported_objects = import_obj(input_path)
    relink_missing_images(input_path.parent)

    mesh_objects = [obj for obj in imported_objects if obj.type == "MESH"]
    root_objects = [obj for obj in imported_objects if obj.parent is None]
    if not mesh_objects:
        raise RuntimeError("No mesh objects were imported from the OBJ")

    scene = configure_scene(args.cell_size)
    create_light("PreviewKeyLight", 3.0, (math.radians(40), 0.0, math.radians(35)))
    create_light("PreviewFillLight", 1.2, (math.radians(55), 0.0, math.radians(-145)))
    camera_fill_light = create_light("PreviewCameraFillLight", 1.35, (0.0, 0.0, 0.0), use_shadow=False)

    min_corner, max_corner = mesh_world_bounds(mesh_objects)
    height = recenter_roots_to_ground(root_objects, min_corner, max_corner)
    camera_object = create_camera()
    focus_height = height * FOCUS_HEIGHT_RATIO
    camera_object.data.ortho_scale = global_ortho_scale(camera_object, mesh_objects, focus_height,
                                                        args.yaw_offset_degrees)
    render_frames(scene, camera_object, camera_fill_light, output_dir, focus_height, args.yaw_offset_degrees)


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise
