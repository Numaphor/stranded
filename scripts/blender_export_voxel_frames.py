import argparse
import json
import math
import sys
from pathlib import Path

import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree


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
    parser.add_argument("--voxel-size", type=float, default=0.03)
    parser.add_argument("--surface-shell-factor", type=float, default=1.25)
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
    return [frame_start + step * i for i in range(sample_count)]


def set_scene_frame(scene, frame_value: float):
    base = int(math.floor(frame_value))
    subframe = frame_value - base
    scene.frame_set(base, subframe=subframe)
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
        (min_corner.x + max_corner.x) * 0.5,
        (min_corner.y + max_corner.y) * 0.5,
        min_corner.z,
    ))

    for root in roots:
        world = root.matrix_world.copy()
        world.translation = world.translation - offset
        root.matrix_world = world

    return max_corner.z - min_corner.z


def find_material_images(mesh_obj):
    material_images = {}

    for material_index, slot in enumerate(mesh_obj.material_slots):
        material = slot.material
        if material and material.use_nodes:
            for node in material.node_tree.nodes:
                if node.type == "TEX_IMAGE" and getattr(node, "image", None):
                    material_images[material_index] = node.image
                    break

    if not material_images:
        raise RuntimeError(f"No texture image found for mesh '{mesh_obj.name}'")

    return material_images


def barycentric(point, a, b, c):
    v0 = b - a
    v1 = c - a
    v2 = point - a
    d00 = v0.dot(v0)
    d01 = v0.dot(v1)
    d11 = v1.dot(v1)
    d20 = v2.dot(v0)
    d21 = v2.dot(v1)
    denom = d00 * d11 - d01 * d01

    if abs(denom) < 1e-8:
        return 1.0, 0.0, 0.0

    v = (d11 * d20 - d01 * d21) / denom
    w = (d00 * d21 - d01 * d20) / denom
    u = 1.0 - v - w
    return u, v, w


def sample_image_rgba(image, uv):
    width, height = image.size
    px = max(0, min(width - 1, int(uv.x * (width - 1))))
    py = max(0, min(height - 1, int((1.0 - uv.y) * (height - 1))))
    index = (py * width + px) * 4
    pixels = image.pixels
    return tuple(int(round(pixels[index + component] * 255)) for component in range(4))


def is_inside(bvh, point, min_x, max_x, voxel_size):
    axis = Vector((1.0, 0.0, 0.0))
    count = 0
    current = point + axis * 1e-4
    max_distance = (max_x - min_x) + voxel_size * 4

    while True:
        location, normal, index, distance = bvh.ray_cast(current, axis, max_distance)
        if location is None:
            break
        count += 1
        current = location + axis * 1e-4

    return count % 2 == 1


def export_frame_voxels(mesh_objects, voxel_size, frame_value, surface_shell_factor):
    scene = bpy.context.scene
    depsgraph = bpy.context.evaluated_depsgraph_get()
    set_scene_frame(scene, frame_value)
    mesh_sources = []
    min_x = float("inf")
    max_x = float("-inf")
    min_y = float("inf")
    max_y = float("-inf")
    min_z = float("inf")
    max_z = float("-inf")

    for mesh_obj in mesh_objects:
        material_images = find_material_images(mesh_obj)
        obj_eval = mesh_obj.evaluated_get(depsgraph)
        mesh = obj_eval.to_mesh()
        mesh.calc_loop_triangles()

        uv_layer = mesh.uv_layers.active.data
        world_vertices = [obj_eval.matrix_world @ vertex.co for vertex in mesh.vertices]
        triangles = []
        triangle_uvs = []
        triangle_images = []
        triangle_indices = []

        for triangle in mesh.loop_triangles:
            triangles.append([world_vertices[index] for index in triangle.vertices])
            triangle_uvs.append([uv_layer[loop_index].uv.copy() for loop_index in triangle.loops])
            triangle_images.append(material_images.get(triangle.material_index))
            triangle_indices.append(tuple(triangle.vertices[:]))

        if not triangles:
            obj_eval.to_mesh_clear()
            continue

        mesh_min_x = min(vertex.x for vertex in world_vertices)
        mesh_max_x = max(vertex.x for vertex in world_vertices)
        min_x = min(min_x, mesh_min_x)
        max_x = max(max_x, mesh_max_x)
        min_y = min(min_y, min(vertex.y for vertex in world_vertices))
        max_y = max(max_y, max(vertex.y for vertex in world_vertices))
        min_z = min(min_z, min(vertex.z for vertex in world_vertices))
        max_z = max(max_z, max(vertex.z for vertex in world_vertices))

        mesh_sources.append(
            {
                "obj_eval": obj_eval,
                "bvh": BVHTree.FromPolygons(world_vertices, triangle_indices, all_triangles=True),
                "min_x": mesh_min_x,
                "max_x": mesh_max_x,
                "triangles": triangles,
                "triangle_uvs": triangle_uvs,
                "triangle_images": triangle_images,
            }
        )

    try:
        margin = voxel_size * 0.5
        origin_x = min_x - margin
        origin_y = min_y - margin
        origin_z = min_z - margin
        count_x = int((max_x - min_x + margin * 2) / voxel_size) + 1
        count_y = int((max_y - min_y + margin * 2) / voxel_size) + 1
        count_z = int((max_z - min_z + margin * 2) / voxel_size) + 1

        voxels = []

        for y_index in range(count_y):
            center_y = origin_y + (y_index + 0.5) * voxel_size

            for z_index in range(count_z):
                center_z = origin_z + (z_index + 0.5) * voxel_size

                for x_index in range(count_x):
                    center_x = origin_x + (x_index + 0.5) * voxel_size
                    point = Vector((center_x, center_y, center_z))

                    inside_any_mesh = False
                    nearest_location = None
                    nearest_distance = None
                    nearest_triangle_index = None
                    nearest_source = None

                    for source in mesh_sources:
                        if is_inside(source["bvh"], point, source["min_x"], source["max_x"], voxel_size):
                            inside_any_mesh = True

                        location, normal, triangle_index, distance = source["bvh"].find_nearest(point)
                        if location is not None and triangle_index is not None:
                            if nearest_distance is None or distance < nearest_distance:
                                nearest_location = location
                                nearest_distance = distance
                                nearest_triangle_index = triangle_index
                                nearest_source = source

                    if not inside_any_mesh or nearest_source is None or nearest_distance is None:
                        continue

                    if nearest_distance > voxel_size * surface_shell_factor:
                        continue

                    image = nearest_source["triangle_images"][nearest_triangle_index]
                    if image is None:
                        continue

                    triangle = nearest_source["triangles"][nearest_triangle_index]
                    uvs = nearest_source["triangle_uvs"][nearest_triangle_index]
                    u, v, w = barycentric(nearest_location, triangle[0], triangle[1], triangle[2])
                    uv = uvs[0] * u + uvs[1] * v + uvs[2] * w
                    color = sample_image_rgba(image, uv)

                    if color[3] == 0:
                        continue

                    voxels.append([x_index, y_index, z_index, color[0], color[1], color[2]])

        return {
            "origin": [origin_x, origin_y, origin_z],
            "dims": [count_x, count_y, count_z],
            "voxels": voxels,
        }
    finally:
        for source in mesh_sources:
            source["obj_eval"].to_mesh_clear()


def main():
    args = parse_args()
    reset_scene()
    bpy.ops.import_scene.fbx(filepath=args.input, use_anim=True, automatic_bone_orientation=True)
    relink_missing_images(Path(args.input).resolve().parent)

    imported_objects = list(bpy.data.objects)
    roots = [obj for obj in imported_objects if obj.parent is None]
    mesh_objects = [obj for obj in imported_objects if obj.type == "MESH"]
    if not mesh_objects:
        raise RuntimeError("No mesh objects were imported from the FBX")

    action = set_active_action(imported_objects, args.action)

    scene = bpy.context.scene
    frame_start, frame_end = find_animation_range(scene)
    src_fps = source_fps(scene)
    target_fps = src_fps if 8.0 <= src_fps <= 12.0 else args.fps
    sample_positions = build_sample_positions(frame_start, frame_end, src_fps, target_fps)

    min_corner, max_corner = combined_mesh_bounds(mesh_objects, sample_positions)
    height = recenter_roots_to_ground(roots, min_corner, max_corner)
    bpy.context.view_layer.update()

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    frame_entries = []
    for frame_index, frame_value in enumerate(sample_positions):
        frame_data = export_frame_voxels(mesh_objects, args.voxel_size, frame_value, args.surface_shell_factor)
        filename = f"frame_{frame_index:03d}.json"
        (output_dir / filename).write_text(json.dumps(frame_data), encoding="utf-8")
        frame_entries.append(
            {
                "frame_index": frame_index,
                "source_frame": frame_value,
                "filename": filename,
            }
        )

    manifest = {
        "input_file": str(Path(args.input).resolve()),
        "action_name": action.name,
        "animation_fps": target_fps,
        "frame_start": frame_start,
        "frame_end": frame_end,
        "frames_per_angle": len(sample_positions),
        "angle_count": 8,
        "angle_order": [0, 45, 90, 135, 180, 225, 270, 315],
        "voxel_size": args.voxel_size,
        "height": height,
        "frames": frame_entries,
    }
    (output_dir / "frame_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
