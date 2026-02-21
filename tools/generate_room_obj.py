#!/usr/bin/env python3
"""Split a combined room OBJ into separate OBJ files.

Edit obj/room.obj and obj/room.mtl as the single source of truth. Run this
script to generate room_shell.obj, table.obj, chair.obj.

To regenerate C++ headers too, run tools/regenerate_room_models.py (it uses a
shared palette and correct scales; RoomShell is written with flip_y so depth
matches the engine).

Splitting is done in two ways (in order):
  1. By object: each "o Name" in room.obj becomes a file (e.g. "o RoomShell" -> room_shell.obj).
  2. By material: if there is only one object, faces are split by usemtl:
     - room_shell: floor_light, floor_dark, wall, wall_shadow, window_glass, window_frame, baseboard
     - table: table_wood
     - chair: chair_fabric, chair_frame
  So a single "o Room" with usemtl tags still produces room_shell.obj, table.obj, chair.obj
  (under 256 vertices total for the game).

All output OBJs share the same MTL so color indices stay compatible for
obj_to_butano.py and the engine.

Usage:
    python generate_room_obj.py [--input-obj PATH] [--input-mtl PATH] [--output-dir DIR]

Example:
    # From repo root, default: obj/room.obj, obj/room.mtl -> obj/*.obj
    python tools/generate_room_obj.py

    # Custom paths
    python tools/generate_room_obj.py --input-obj obj/room.obj --output-dir obj
"""

import os
import argparse
import shutil


# When splitting by material (single object), map usemtl names to output object.
# Order defines output order. Unlisted materials go to room_shell.
MATERIAL_TO_OBJECT = {
    'room_shell': [
        'floor_light', 'floor_dark', 'wall', 'wall_shadow',
        'window_glass', 'window_frame', 'baseboard',
    ],
    'table': ['table_wood'],
    'chair': ['chair_fabric', 'chair_frame'],
}


def object_name_to_filename(name):
    """Convert object name to OBJ filename (no extension)."""
    s = name.strip().lower().replace(' ', '_').replace('-', '_')
    # RoomShell -> room_shell for expected filenames
    if s == 'roomshell':
        return 'room_shell'
    return s


def split_faces_by_material(faces):
    """Group (material, face) list into (object_name, faces) by MATERIAL_TO_OBJECT.
    object_name is PascalCase for the OBJ 'o' tag (RoomShell, Table, Chair).
    """
    mtl_to_obj = {}
    for obj_key, mtls in MATERIAL_TO_OBJECT.items():
        for m in mtls:
            mtl_to_obj[m] = obj_key
    # PascalCase for display
    key_to_name = {'room_shell': 'RoomShell', 'table': 'Table', 'chair': 'Chair'}

    groups = {}  # obj_key -> list of (material, face)
    for material, face in faces:
        obj_key = mtl_to_obj.get(material, 'room_shell')
        groups.setdefault(obj_key, []).append((material, face))

    return [(key_to_name[k], g) for k in ('room_shell', 'table', 'chair') if k in groups for g in [groups[k]]]


def parse_obj(path):
    """Parse OBJ file. Returns (vertices, normals, objects).

    vertices: list of (x, y, z)
    normals: list of (nx, ny, nz)
    objects: list of (object_name, faces) where each face is
             (material_name, [ (v_idx_1based, n_idx_1based), ... ])
    """
    vertices = []
    normals = []
    objects = []
    current_obj_name = None
    current_obj_faces = []
    current_mtl = None
    mtl_name = "room.mtl"

    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            if not parts:
                continue

            if parts[0] == 'mtllib':
                mtl_name = parts[1] if len(parts) > 1 else "room.mtl"
            elif parts[0] == 'v':
                x, y, z = float(parts[1]), float(parts[2]), float(parts[3])
                vertices.append((x, y, z))
            elif parts[0] == 'vn':
                nx, ny, nz = float(parts[1]), float(parts[2]), float(parts[3])
                normals.append((nx, ny, nz))
            elif parts[0] == 'o':
                if current_obj_name is not None and current_obj_faces:
                    objects.append((current_obj_name, current_obj_faces))
                current_obj_name = parts[1] if len(parts) > 1 else "Object"
                current_obj_faces = []
            elif parts[0] == 'usemtl':
                current_mtl = parts[1] if len(parts) > 1 else None
            elif parts[0] == 'f':
                face_verts = []
                for vert_spec in parts[1:]:
                    # Support v, v/vt, v/vt/vn, v//vn
                    indices = vert_spec.split('/')
                    v_idx = int(indices[0])  # 1-based
                    n_idx = int(indices[2]) if len(indices) >= 3 and indices[2] else 0
                    face_verts.append((v_idx, n_idx))
                if face_verts and current_mtl is not None:
                    current_obj_faces.append((current_mtl, face_verts))
                elif face_verts:
                    current_obj_faces.append(("default", face_verts))

        if current_obj_name is not None and current_obj_faces:
            objects.append((current_obj_name, current_obj_faces))

    # If no "o" was ever seen, collect all faces then split by material
    if not objects and (vertices or normals):
        current_mtl = None
        all_faces = []
        with open(path, 'r', encoding='utf-8') as f:
            for line in f:
                parts = line.strip().split()
                if not parts:
                    continue
                if parts[0] == 'usemtl':
                    current_mtl = parts[1] if len(parts) > 1 else None
                elif parts[0] == 'f':
                    face_verts = []
                    for vert_spec in parts[1:]:
                        indices = vert_spec.split('/')
                        v_idx = int(indices[0])
                        n_idx = int(indices[2]) if len(indices) >= 3 and indices[2] else 0
                        face_verts.append((v_idx, n_idx))
                    if face_verts:
                        all_faces.append((current_mtl or "default", face_verts))
        if all_faces:
            # Split by usemtl -> room_shell / table / chair
            objects = split_faces_by_material(all_faces)
            if not objects:
                objects = [("Room", all_faces)]

    # If exactly one object and it's "Room", also split by material so we get 3 files
    if len(objects) == 1 and objects[0][0] == "Room":
        objects = split_faces_by_material(objects[0][1])
        if not objects:
            pass  # keep single Room

    return vertices, normals, objects, mtl_name


def write_object_obj(path, object_name, vertices, normals, faces, mtl_name, center_at_origin=False, flip_y=False):
    """Write one OBJ file with only used vertices/normals, renumbered.
    If center_at_origin True, subtract centroid from vertices so mesh is in local space (for table/chair).
    If flip_y True, negate Y (and reverse face winding) so engine gets correct depth sign (engine_z = obj_y*scale).
    """
    used_v = set()
    used_n = set()
    for _mtl, face in faces:
        for v_idx, n_idx in face:
            used_v.add(v_idx)
            if n_idx > 0:
                used_n.add(n_idx)

    v_list = sorted(used_v)
    n_list = sorted(used_n)

    # Optional: recenter at origin and/or flip Y for engine convention (engine_z = obj_y * scale)
    tx, ty, tz = 0.0, 0.0, 0.0
    if center_at_origin and v_list:
        cx = sum(vertices[i - 1][0] for i in v_list) / len(v_list)
        cy = sum(vertices[i - 1][1] for i in v_list) / len(v_list)
        cz = sum(vertices[i - 1][2] for i in v_list) / len(v_list)
        tx, ty, tz = -cx, -cy, -cz
        flip_y = True  # table/chair: flip so they render right-side up
    # RoomShell: flip_y only (no center) so depth becomes negative engine_z

    v_old_to_new = {old: i for i, old in enumerate(v_list, start=1)}
    n_old_to_new = {old: i for i, old in enumerate(n_list, start=1)}

    with open(path, 'w', encoding='utf-8') as f:
        f.write(f"# Generated from room.obj: {object_name}\n")
        f.write(f"mtllib {mtl_name}\n")
        f.write(f"o {object_name}\n")
        for i in v_list:
            x, y, z = vertices[i - 1]
            y_out = -(y + ty) if flip_y else (y + ty)
            f.write(f"v {x + tx:.6f} {y_out:.6f} {z + tz:.6f}\n")
        for i in n_list:
            nx, ny, nz = normals[i - 1]
            ny_out = -ny if flip_y else ny
            f.write(f"vn {nx:.6f} {ny_out:.6f} {nz:.6f}\n")

        current_mtl = None
        for material, face in faces:
            if material != current_mtl:
                f.write(f"usemtl {material}\n")
                f.write("s off\n")
                current_mtl = material
            order = reversed(face) if flip_y else face
            vert_str = " ".join(
                f"{v_old_to_new[v]}//{n_old_to_new.get(n, n_list[0] if n_list else 1)}"
                for v, n in order
            )
            f.write(f"f {vert_str}\n")


def main():
    parser = argparse.ArgumentParser(
        description='Split room.obj into separate OBJ files by object name'
    )
    parser.add_argument(
        '--input-obj',
        default='obj/room.obj',
        help='Input combined OBJ (default: obj/room.obj)',
    )
    parser.add_argument(
        '--input-mtl',
        default=None,
        help='Input MTL file (default: same dir as input-obj, room.mtl)',
    )
    parser.add_argument(
        '--output-dir',
        default=None,
        help='Output directory (default: same as input-obj directory)',
    )
    args = parser.parse_args()

    input_obj = os.path.normpath(args.input_obj)
    input_dir = os.path.dirname(input_obj)
    if args.input_mtl is not None:
        input_mtl = os.path.normpath(args.input_mtl)
    else:
        input_mtl = os.path.join(input_dir, 'room.mtl')
    output_dir = os.path.normpath(args.output_dir) if args.output_dir else input_dir

    if not os.path.isfile(input_obj):
        print(f"Error: input OBJ not found: {input_obj}")
        return 1

    vertices, normals, objects, mtl_name = parse_obj(input_obj)
    if not objects:
        print("Error: no objects found in OBJ (no 'o Name' groups or faces)")
        return 1

    os.makedirs(output_dir, exist_ok=True)

    # Copy MTL to output dir so mtllib works (skip if same path, e.g. file open in editor)
    mtl_basename = os.path.basename(mtl_name)
    dest_mtl = os.path.join(output_dir, mtl_basename)
    if os.path.isfile(input_mtl):
        if os.path.abspath(input_mtl) != os.path.abspath(dest_mtl):
            shutil.copy2(input_mtl, dest_mtl)
        print(f"  MTL: {dest_mtl}")
    else:
        print(f"  Warning: MTL not found ({input_mtl}), OBJs will reference {mtl_basename}")

    total_v = 0
    total_f = 0
    input_abs = os.path.abspath(input_obj)
    print("Split room models:")
    for obj_name, faces in objects:
        fname = object_name_to_filename(obj_name) + '.obj'
        out_path = os.path.join(output_dir, fname)
        if os.path.abspath(out_path) == input_abs:
            out_path = os.path.join(output_dir, object_name_to_filename(obj_name) + '_out.obj')
            print(f"  Warning: single object would overwrite input; writing to {os.path.basename(out_path)} instead.")
            print("  To get room_shell.obj, table.obj, chair.obj (for game, under 256 verts):")
            print('    Edit room.obj in your 3D editor and split into objects "RoomShell", "Table", "Chair".')
        n_verts = len(vertices)
        n_faces = sum(1 for _ in faces)
        center = obj_name in ('Table', 'Chair')
        # RoomShell: flip Y so depth is negative in engine (engine_z = obj_y*scale). Table/Chair: center + flip.
        flip_y_room = obj_name == 'RoomShell'
        write_object_obj(out_path, obj_name, vertices, normals, faces, mtl_basename,
                         center_at_origin=center, flip_y=flip_y_room)
        # Count actual verts used by this object
        used_v = set()
        for _mtl, face in faces:
            for v_idx, _ in face:
                used_v.add(v_idx)
        total_v += len(used_v)
        total_f += n_faces
        print(f"    {fname}: {len(used_v)} verts, {n_faces} faces")
        print(f"    Written: {out_path}")

    print(f"  Total: {len(objects)} objects, {total_v} verts (sum per object), {total_f} faces")
    if total_v > 256:
        print("  WARNING: Combined vertex count may exceed engine limit 256")
    if total_f > 176:
        print("  WARNING: Combined face count may exceed engine limit 176")

    return 0


if __name__ == '__main__':
    exit(main() or 0)
