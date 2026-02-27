#!/usr/bin/env python3
"""Generate level model C++ headers from canonical obj/level.obj.

Input contract:
- level.obj is the single source of truth and contains:
  Room0..Room5
- Room0..Room5 are explicit room meshes (including decor in source OBJ).
- Table/Chair runtime headers are extracted from level.obj room decor geometry.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys

# Allow importing model_utils from same directory
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from model_utils import (
    center_vertices,
    deduplicate_vertices,
    generate_color_lines,
    generate_face_lines,
    generate_header_wrapper,
    generate_vertex_lines,
    parse_mtl,
    parse_obj_multi,
    prepare_faces,
)


REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MODELS_INCLUDE = os.path.join(REPO_ROOT, "include", "models")
TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
PLAYER_CAR_HEADER_SOURCE = os.path.join(
    REPO_ROOT,
    "butano",
    "games",
    "varooom-3d",
    "include",
    "models",
    "fr_model_3d_items_player_car.h",
)

ROOM_SCALE = 2.0
BUILDING_SCALE = 2.0
FURNITURE_SCALE = 3.0

ROOM_PALETTE = [
    "floor_light", "floor_dark", "wall", "wall_shadow",
    "reserved", "door_frame",
    "table_wood", "chair_fabric", "chair_frame",
]

# Runtime room-viewer palette contract (indices consumed by room_viewer.cpp):
# 0..5: room-specific floor tint
# 6: wall
# 7: window (reserved)
# 8: door_frame
# 9: table_wood (furniture light)
ROOM_VIEWER_PALETTE = [
    "room0_floor", "room1_floor", "room2_floor",
    "room3_floor", "room4_floor", "room5_floor",
    "wall", "reserved", "door_frame", "table_wood",
]

# Runtime room models stay structural-only; furniture is drawn via dynamic models.
ROOM_RUNTIME_MATERIALS = {
    "floor_light", "floor_dark", "wall", "reserved", "door_frame",
}

BUILDING_PALETTE = [
    "room0_floor", "room1_floor", "room2_floor",
    "room3_floor", "room4_floor", "room5_floor",
    "outer_wall", "interior_wall",
]

ROOM_NAMES = ["Room0", "Room1", "Room2", "Room3", "Room4", "Room5"]
ROOM_CPP_NAMES = ["room_0", "room_1", "room_2", "room_3", "room_4", "room_5"]
REQUIRED_OBJECTS = set(ROOM_NAMES)

ROOM_CENTERS = {
    0: (-30.0, -60.0),
    1: (37.5, -67.5),
    2: (-30.0, 0.0),
    3: (30.0, 0.0),
    4: (-30.0, 60.0),
    5: (37.5, 67.5),
}

# Locked decor specification.
ROOM_DECOR_SPEC = {
    0: {"table", "chair"},
    1: {"window"},
    2: {"table"},
    3: {"chair", "window"},
    4: {"table"},
    5: {"chair"},
}

FURNITURE_SPEC = {
    "table": {
        "materials": {"table_wood", "wall_shadow"},
        "rooms": [room_id for room_id, tokens in ROOM_DECOR_SPEC.items() if "table" in tokens],
    },
    "chair": {
        "materials": {"chair_fabric", "chair_frame"},
        "rooms": [room_id for room_id, tokens in ROOM_DECOR_SPEC.items() if "chair" in tokens],
    },
}

WINDOW_WALL_TARGETS = {
    1: "north",
    3: "east",
}

ROOM_DOOR_WALLS = {
    0: {"east", "south"},
    1: {"west", "south"},
    2: {"east", "north", "south"},
    3: {"west", "north", "south"},
    4: {"east", "north"},
    5: {"west", "north"},
}


def remap_room_material_to_runtime_palette(material_name, room_id):
    if material_name in {"floor_light", "floor_dark"}:
        return f"room{room_id}_floor"
    if material_name in {"wall", "wall_shadow"}:
        return "wall"
    if material_name == "reserved":
        return "reserved"
    if material_name == "door_frame":
        return "door_frame"
    if material_name in {"table_wood", "chair_fabric", "chair_frame"}:
        return "table_wood"
    raise ValueError(f"Unsupported room runtime material remap: {material_name}")


def remap_furniture_material_to_runtime_palette(material_name):
    if material_name in {"table_wood", "chair_fabric"}:
        return "table_wood"
    if material_name in {"chair_frame", "wall_shadow", "door_frame"}:
        return "door_frame"
    raise ValueError(f"Unsupported furniture runtime material remap: {material_name}")


def _vertex_key(vertex):
    return (round(vertex[0], 5), round(vertex[1], 5), round(vertex[2], 5))


def dedupe_room_runtime_faces(room_vertices, runtime_faces, normals):
    """Drop duplicate coplanar quads with opposite winding.

    Some source room meshes include both windings for the same polygon. Keep
    one face per (material, vertex set), preferring the normal that points
    toward the room interior center to avoid scanline artifacts in the 3D
    rasterizer.
    """
    if not runtime_faces:
        return runtime_faces

    min_x = min(v[0] for v in room_vertices)
    max_x = max(v[0] for v in room_vertices)
    min_y = min(v[1] for v in room_vertices)
    max_y = max(v[1] for v in room_vertices)
    min_z = min(v[2] for v in room_vertices)
    max_z = max(v[2] for v in room_vertices)
    room_center = (
        (min_x + max_x) / 2,
        (min_y + max_y) / 2,
        (min_z + max_z) / 2,
    )

    grouped = {}
    for material_name, face_vertices, normal_idx in runtime_faces:
        key = (material_name, tuple(sorted(face_vertices)))
        grouped.setdefault(key, []).append((face_vertices, normal_idx))

    deduped = []
    for (material_name, _vertex_set), candidates in grouped.items():
        if len(candidates) == 1:
            face_vertices, normal_idx = candidates[0]
            deduped.append((material_name, face_vertices, normal_idx))
            continue

        best_face_vertices = None
        best_normal_idx = None
        best_score = None

        for face_vertices, normal_idx in candidates:
            cx = sum(room_vertices[v][0] for v in face_vertices) / len(face_vertices)
            cy = sum(room_vertices[v][1] for v in face_vertices) / len(face_vertices)
            cz = sum(room_vertices[v][2] for v in face_vertices) / len(face_vertices)
            to_center = (
                room_center[0] - cx,
                room_center[1] - cy,
                room_center[2] - cz,
            )
            nx, ny, nz = normals[normal_idx]
            score = nx * to_center[0] + ny * to_center[1] + nz * to_center[2]

            if best_score is None or score > best_score:
                best_score = score
                best_face_vertices = face_vertices
                best_normal_idx = normal_idx

        deduped.append((material_name, best_face_vertices, best_normal_idx))

    return deduped


def trim_room_runtime_floor(room_vertices, runtime_faces, floor_material):
    """Clamp floor faces to the interior wall footprint at floor height.

    Source room meshes intentionally include floor underlap outside the inner
    wall shell. In the room viewer's open-front camera, that underlap can
    become visible at the wall edges. Keep the floor within wall bounds.
    """
    if not runtime_faces:
        return room_vertices, runtime_faces

    floor_line_tolerance = 0.001
    structural_materials = {"wall", "door_frame", "reserved"}
    boundary_points = []

    for material_name, face_vertices, _normal_idx in runtime_faces:
        if material_name not in structural_materials:
            continue

        for vertex_index in face_vertices:
            vx, vy, vz = room_vertices[vertex_index]
            if abs(vy) <= floor_line_tolerance:
                boundary_points.append((vx, vz))

    if not boundary_points:
        return room_vertices, runtime_faces

    min_x = min(point[0] for point in boundary_points)
    max_x = max(point[0] for point in boundary_points)
    min_z = min(point[1] for point in boundary_points)
    max_z = max(point[1] for point in boundary_points)

    if min_x >= max_x or min_z >= max_z:
        return room_vertices, runtime_faces

    non_floor_vertices = set()
    for material_name, face_vertices, _normal_idx in runtime_faces:
        if material_name != floor_material:
            non_floor_vertices.update(face_vertices)

    adjusted_vertices = list(room_vertices)
    replacement_map = {}
    adjusted_faces = []
    changed = False

    for material_name, face_vertices, normal_idx in runtime_faces:
        if material_name != floor_material:
            adjusted_faces.append((material_name, face_vertices, normal_idx))
            continue

        adjusted_face_vertices = list(face_vertices)

        for index, vertex_index in enumerate(face_vertices):
            vx, vy, vz = room_vertices[vertex_index]

            if abs(vy) > floor_line_tolerance:
                continue

            clamped_x = max(min_x, min(max_x, vx))
            clamped_z = max(min_z, min(max_z, vz))

            if abs(clamped_x - vx) <= floor_line_tolerance and abs(clamped_z - vz) <= floor_line_tolerance:
                continue

            changed = True
            replacement_key = (
                vertex_index,
                round(clamped_x, 5),
                round(vy, 5),
                round(clamped_z, 5),
            )
            mapped_index = replacement_map.get(replacement_key)

            if mapped_index is None:
                if vertex_index in non_floor_vertices:
                    mapped_index = len(adjusted_vertices)
                    adjusted_vertices.append((clamped_x, vy, clamped_z))
                else:
                    mapped_index = vertex_index
                    adjusted_vertices[mapped_index] = (clamped_x, vy, clamped_z)

                replacement_map[replacement_key] = mapped_index

            adjusted_face_vertices[index] = mapped_index

        adjusted_faces.append((material_name, adjusted_face_vertices, normal_idx))

    if changed:
        return adjusted_vertices, adjusted_faces

    return room_vertices, runtime_faces


def validate_decor_targets():
    for room_id, wall in WINDOW_WALL_TARGETS.items():
        if wall in ROOM_DOOR_WALLS.get(room_id, set()):
            raise ValueError(
                f"Decor config invalid: room {room_id} window wall '{wall}' is a door wall"
            )


def validate_level_objects(objects):
    object_set = set(objects.keys())
    missing = sorted(REQUIRED_OBJECTS - object_set)
    extra = sorted(object_set - REQUIRED_OBJECTS)

    if missing:
        raise ValueError(f"Missing required objects in level.obj: {missing}")
    if extra:
        raise ValueError(f"Unexpected objects in level.obj: {extra}")


def build_room_geometry(vertices, objects):
    room_vertices_by_cpp = {}
    room_face_groups = {}

    for room_id, room_name in enumerate(ROOM_NAMES):
        cpp_name = ROOM_CPP_NAMES[room_id]
        center_x, center_z = ROOM_CENTERS[room_id]
        room_vertices = []
        vertex_index_by_key = {}
        room_material_faces = {material: [] for material in ROOM_PALETTE}

        def get_vertex_index(local_vertex):
            key = _vertex_key(local_vertex)
            existing = vertex_index_by_key.get(key)
            if existing is not None:
                return existing
            new_index = len(room_vertices)
            room_vertices.append(local_vertex)
            vertex_index_by_key[key] = new_index
            return new_index

        for material_name, faces in objects[room_name]:
            if material_name not in ROOM_RUNTIME_MATERIALS:
                continue

            for face_verts, normal_idx in faces:
                local_indices = []
                for v_idx in face_verts:
                    vx, vy, vz = vertices[v_idx]
                    local = (vx - center_x, vy, vz - center_z)
                    local_indices.append(get_vertex_index(local))

                if len(local_indices) < 3:
                    continue
                if len(set(local_indices)) < 3:
                    continue

                final_normal_idx = normal_idx if normal_idx is not None else 0
                room_material_faces[material_name].append((local_indices, final_normal_idx))

        groups = []
        for material_name in ROOM_PALETTE:
            faces = room_material_faces[material_name]
            if faces:
                groups.append((material_name, faces))

        room_vertices_by_cpp[cpp_name] = room_vertices
        room_face_groups[cpp_name] = groups

    return room_vertices_by_cpp, room_face_groups


def validate_room_windows(room_face_groups, room_vertices_by_cpp):
    expected_window_rooms = {room_id for room_id, tokens in ROOM_DECOR_SPEC.items() if "window" in tokens}

    for room_id, cpp_name in enumerate(ROOM_CPP_NAMES):
        materials_present = {material for material, faces in room_face_groups[cpp_name] if faces}
        has_window = "reserved" in materials_present
        expected_has_window = room_id in expected_window_rooms
        if has_window != expected_has_window:
            raise ValueError(
                f"Room {room_id} window presence mismatch: expected={expected_has_window}, got={has_window}"
            )

    # Validate window wall targets via room-local centroids of reserved faces.
    for room_id, target_wall in WINDOW_WALL_TARGETS.items():
        cpp_name = ROOM_CPP_NAMES[room_id]
        room_vertices = room_vertices_by_cpp[cpp_name]
        reserved_faces = []
        for material_name, faces in room_face_groups[cpp_name]:
            if material_name == "reserved":
                reserved_faces.extend(faces)

        if not reserved_faces:
            raise ValueError(f"Room {room_id} expected window faces but none were found")

        centroid_x = 0.0
        centroid_z = 0.0
        count = 0

        for face_indices, _n_idx in reserved_faces:
            for vid in face_indices:
                vx, _vy, vz = room_vertices[vid]
                centroid_x += vx
                centroid_z += vz
                count += 1

        centroid_x /= count
        centroid_z /= count

        if target_wall == "north" and centroid_z > -20.0:
            raise ValueError(
                f"Room {room_id} window expected on north wall, centroid z={centroid_z:.2f}"
            )
        if target_wall == "east" and centroid_x < 20.0:
            raise ValueError(
                f"Room {room_id} window expected on east wall, centroid x={centroid_x:.2f}"
            )


def validate_room_decor_source(objects):
    for room_id, room_name in enumerate(ROOM_NAMES):
        expected = ROOM_DECOR_SPEC[room_id]
        groups = objects[room_name]

        materials_present = {
            material_name
            for material_name, faces in groups
            if faces
        }

        has_table = "table_wood" in materials_present
        has_chair = bool({"chair_fabric", "chair_frame"} & materials_present)
        has_window = "reserved" in materials_present

        if ("table" in expected) != has_table:
            raise ValueError(
                f"Room {room_id} table decor mismatch: expected={'table' in expected}, got={has_table}"
            )
        if ("chair" in expected) != has_chair:
            raise ValueError(
                f"Room {room_id} chair decor mismatch: expected={'chair' in expected}, got={has_chair}"
            )
        if ("window" in expected) != has_window:
            raise ValueError(
                f"Room {room_id} window decor mismatch: expected={'window' in expected}, got={has_window}"
            )


def generate_room_header(vertices, normals, objects, materials_rgb):
    validate_room_decor_source(objects)
    room_vertices_by_cpp, room_face_groups = build_room_geometry(vertices, objects)

    validate_room_windows(room_face_groups, room_vertices_by_cpp)

    room_faces = {}
    for room_id, cpp_name in enumerate(ROOM_CPP_NAMES):
        faces = prepare_faces(room_face_groups[cpp_name], ROOM_PALETTE, normals)
        runtime_faces = [
            (remap_room_material_to_runtime_palette(material_name, room_id), face_vertices, normal_idx)
            for material_name, face_vertices, normal_idx in faces
        ]
        room_vertices, runtime_faces = trim_room_runtime_floor(
            room_vertices_by_cpp[cpp_name],
            runtime_faces,
            floor_material=f"room{room_id}_floor",
        )
        room_vertices_by_cpp[cpp_name] = room_vertices
        runtime_faces = dedupe_room_runtime_faces(room_vertices, runtime_faces, normals)
        room_faces[cpp_name] = runtime_faces
        vertex_count = len(room_vertices_by_cpp[cpp_name])
        print(f"  {cpp_name}: {vertex_count} vertices, {len(runtime_faces)} faces")
        if vertex_count > 100:
            print("    WARNING: exceeds 100 vertex limit")
        if len(runtime_faces) > 176:
            print("    WARNING: exceeds 176 face limit")

    body_lines = []
    body_lines.extend(generate_color_lines("room", ROOM_VIEWER_PALETTE, materials_rgb))
    body_lines.append("")

    for cpp_name in ROOM_CPP_NAMES:
        faces = room_faces[cpp_name]
        body_lines.extend(generate_vertex_lines(cpp_name, room_vertices_by_cpp[cpp_name], ROOM_SCALE, flip_z=True))
        body_lines.append("")
        body_lines.extend(generate_face_lines(cpp_name, cpp_name, faces, normals, ROOM_VIEWER_PALETTE, flip_z=True))
        body_lines.append(f"    constexpr inline fr::model_3d_item {cpp_name}({cpp_name}_vertices, {cpp_name}_faces);")
        body_lines.append("")

    body_lines.append("    constexpr inline fr::model_3d_item room(room_0_vertices, room_0_faces);")
    return generate_header_wrapper("STR_MODEL_3D_ITEMS_ROOM_H", "str", body_lines, emit_colors=True)


def generate_building_header(vertices, normals, objects, materials_rgb):
    building_groups = objects["Building"]
    building_materials = {material for material, _faces in building_groups}
    disallowed = sorted(building_materials - set(BUILDING_PALETTE))
    if disallowed:
        raise ValueError(
            f"Building object has non-structural materials (should be structure only): {disallowed}"
        )

    face_groups = [
        (material_name, faces)
        for material_name, faces in building_groups
        if material_name in BUILDING_PALETTE
    ]

    used = set()
    for _material, faces in face_groups:
        for face_verts, _n_idx in faces:
            used.update(face_verts)

    unique_verts, remap = deduplicate_vertices(vertices, used)
    print(f"  building: {len(used)} used -> {len(unique_verts)} unique vertices")

    remapped_groups = []
    for material_name, faces in face_groups:
        remapped_faces = []
        for face_verts, n_idx in faces:
            remapped_faces.append(([remap[v] for v in face_verts], n_idx))
        remapped_groups.append((material_name, remapped_faces))

    all_faces = prepare_faces(remapped_groups, BUILDING_PALETTE, normals)
    print(f"  building: {len(all_faces)} faces")

    body_lines = []
    body_lines.extend(generate_color_lines("building", BUILDING_PALETTE, materials_rgb))
    body_lines.append("")
    body_lines.extend(generate_vertex_lines("building", unique_verts, BUILDING_SCALE, flip_z=True))
    body_lines.append("")
    body_lines.extend(generate_face_lines("building", "building", all_faces, normals, BUILDING_PALETTE, flip_z=True))
    body_lines.append("")
    body_lines.append("    constexpr inline fr::model_3d_item building(building_vertices, building_faces);")

    return generate_header_wrapper("STR_MODEL_3D_ITEMS_BUILDING_H", "str", body_lines, emit_colors=True)


def generate_furniture_header(vertices, normals, face_groups, model_name, guard):
    face_groups = [
        (material_name, faces)
        for material_name, faces in face_groups
        if material_name in ROOM_PALETTE
    ]
    if not face_groups:
        raise ValueError(f"{model_name} has no faces with recognized room palette materials")

    used = set()
    for _material, faces in face_groups:
        for face_verts, _n_idx in faces:
            used.update(face_verts)

    unique_verts, remap = deduplicate_vertices(vertices, used)
    remapped_groups = []
    for material_name, faces in face_groups:
        remapped_faces = []
        for face_verts, n_idx in faces:
            remapped_faces.append(([remap[v] for v in face_verts], n_idx))
        remapped_groups.append((material_name, remapped_faces))

    all_faces = prepare_faces(remapped_groups, ROOM_PALETTE, normals)
    runtime_faces = [
        (remap_furniture_material_to_runtime_palette(material_name), face_vertices, normal_idx)
        for material_name, face_vertices, normal_idx in all_faces
    ]

    if model_name == "chair":
        # The backrest panel is a thin single surface in source geometry.
        # Mirror only that panel to avoid see-through without doubling whole chair fill cost.
        normal_to_index = {
            (round(nx, 5), round(ny, 5), round(nz, 5)): idx
            for idx, (nx, ny, nz) in enumerate(normals)
        }

        backrest_panel_index = -1
        backrest_vertices = {40, 41, 42, 43}

        for index, (_material_name, face_vertices, _normal_idx) in enumerate(runtime_faces):
            if len(face_vertices) == 4 and set(face_vertices) == backrest_vertices:
                backrest_panel_index = index
                break

        if backrest_panel_index >= 0:
            material_name, face_vertices, normal_idx = runtime_faces[backrest_panel_index]
            nx, ny, nz = normals[normal_idx]
            opposite_idx = normal_to_index.get(
                (round(-nx, 5), round(-ny, 5), round(-nz, 5)),
                normal_idx,
            )
            runtime_faces.append((material_name, list(reversed(face_vertices)), opposite_idx))

    print(f"  {model_name}: {len(unique_verts)} vertices, {len(all_faces)} faces")

    body_lines = []
    body_lines.extend(generate_vertex_lines(model_name, unique_verts, FURNITURE_SCALE, flip_z=True))
    body_lines.append("")
    body_lines.extend(generate_face_lines(
        model_name, model_name, runtime_faces, normals, ROOM_VIEWER_PALETTE, flip_z=True
    ))
    body_lines.append("")
    body_lines.append(f"    constexpr inline fr::model_3d_item {model_name}({model_name}_vertices, {model_name}_faces);")

    return generate_header_wrapper(guard, "str", body_lines, emit_colors=False)


def _furniture_face_signature(face_groups):
    signature = []
    for material_name, faces in face_groups:
        arities = sorted(len(face_verts) for face_verts, _normal_idx in faces)
        signature.append((material_name, len(faces), tuple(arities)))
    return tuple(signature)


def _extract_room_furniture(vertices, objects, room_id, materials):
    room_name = ROOM_NAMES[room_id]
    center_x, center_z = ROOM_CENTERS[room_id]
    room_groups = objects[room_name]

    local_vertices = []
    remap = {}
    by_material = {material_name: [] for material_name in ROOM_PALETTE}

    for material_name, faces in room_groups:
        if material_name not in materials:
            continue

        dst_faces = by_material[material_name]
        for face_verts, normal_idx in faces:
            local_face = []
            for v_idx in face_verts:
                mapped = remap.get(v_idx)
                if mapped is None:
                    vx, vy, vz = vertices[v_idx]
                    mapped = len(local_vertices)
                    local_vertices.append((vx - center_x, vy, vz - center_z))
                    remap[v_idx] = mapped
                local_face.append(mapped)

            if len(local_face) < 3:
                continue
            if len(set(local_face)) < 3:
                continue

            dst_faces.append((local_face, normal_idx if normal_idx is not None else 0))

    face_groups = []
    for material_name in ROOM_PALETTE:
        faces = by_material[material_name]
        if faces:
            face_groups.append((material_name, faces))

    return local_vertices, face_groups


def load_centered_furniture_from_level(vertices, normals, objects, model_name):
    spec = FURNITURE_SPEC[model_name]
    materials = spec["materials"]
    source_rooms = spec["rooms"]

    extracted = []
    missing_rooms = []

    for room_id in source_rooms:
        local_vertices, face_groups = _extract_room_furniture(vertices, objects, room_id, materials)
        if face_groups:
            extracted.append((room_id, local_vertices, face_groups))
        else:
            missing_rooms.append(room_id)

    if missing_rooms:
        raise ValueError(
            f"{model_name}: expected furniture geometry missing in rooms {missing_rooms}"
        )
    if not extracted:
        raise ValueError(
            f"{model_name}: no geometry found in level rooms for materials {sorted(materials)}"
        )

    # Use the first expected room as canonical template and ensure other rooms
    # have equivalent face structure.
    template_room_id, template_vertices, template_groups = extracted[0]
    template_sig = _furniture_face_signature(template_groups)
    for room_id, _verts, groups in extracted[1:]:
        if _furniture_face_signature(groups) != template_sig:
            raise ValueError(
                f"{model_name}: room {room_id} geometry doesn't match template room {template_room_id}"
            )

    used = set()
    for _material_name, faces in template_groups:
        for face_verts, _n_idx in faces:
            used.update(face_verts)

    cx, cy, cz = center_vertices(template_vertices, used)
    centered_vertices = [(vx - cx, vy - cy, vz - cz) for vx, vy, vz in template_vertices]
    print(
        f"  extracted {model_name} from {ROOM_NAMES[template_room_id]} "
        f"(validated {len(extracted)} room instances)"
    )
    return centered_vertices, normals, template_groups


def write_header(path, content):
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "w") as file:
        file.write(content)
    print(f"  Written: {path}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate level model C++ headers from canonical level.obj + level.mtl"
    )
    parser.add_argument(
        "--obj", default=os.path.join(REPO_ROOT, "obj", "level.obj"),
        help="Input level OBJ file (default: obj/level.obj)",
    )
    parser.add_argument(
        "--mtl", default=os.path.join(REPO_ROOT, "obj", "level.mtl"),
        help="Input level MTL file (default: obj/level.mtl)",
    )
    parser.add_argument(
        "--table-obj", default=None,
        help="Deprecated (furniture is extracted from level.obj)",
    )
    parser.add_argument(
        "--chair-obj", default=None,
        help="Deprecated (furniture is extracted from level.obj)",
    )
    parser.add_argument(
        "--output-dir", default=MODELS_INCLUDE,
        help="Output directory for headers (default: include/models)",
    )
    parser.add_argument(
        "--blaster-obj", default=os.path.join(REPO_ROOT, "obj", "blaster.obj"),
        help="Input blaster OBJ file for standalone header generation",
    )
    parser.add_argument(
        "--blaster-mtl", default=os.path.join(REPO_ROOT, "obj", "blaster.mtl"),
        help="Input blaster MTL file for standalone header generation",
    )
    parser.add_argument(
        "--player-car-header", default=PLAYER_CAR_HEADER_SOURCE,
        help="Source fr_model_3d_items_player_car.h to copy into output dir",
    )
    parser.add_argument(
        "--skip-standalone", action="store_true",
        help="Skip standalone headers (blaster/player_car) and generate only level-derived headers",
    )
    args = parser.parse_args()

    validate_decor_targets()

    if not os.path.isfile(args.obj):
        print(f"Error: {args.obj} not found")
        return 1
    if not os.path.isfile(args.mtl):
        print(f"Error: {args.mtl} not found")
        return 1

    if args.table_obj:
        print("  NOTE: --table-obj is ignored; table mesh is extracted from level.obj")
    if args.chair_obj:
        print("  NOTE: --chair-obj is ignored; chair mesh is extracted from level.obj")

    print("=== Parsing level.obj ===")
    vertices, normals, objects, _mtl_file = parse_obj_multi(args.obj)
    materials_rgb = parse_mtl(args.mtl)

    print(f"  {len(vertices)} vertices, {len(normals)} normals, {len(objects)} objects")
    print(f"  Objects: {', '.join(objects.keys())}")
    validate_level_objects(objects)

    print("\n=== Room header (from explicit Room0..Room5) ===")
    room_header = generate_room_header(vertices, normals, objects, materials_rgb)
    write_header(os.path.join(args.output_dir, "str_model_3d_items_room.h"), room_header)

    print("\n=== Table header ===")
    table_vertices, table_normals, table_groups = load_centered_furniture_from_level(
        vertices,
        normals,
        objects,
        "table",
    )
    table_header = generate_furniture_header(
        table_vertices,
        table_normals,
        table_groups,
        model_name="table",
        guard="STR_MODEL_3D_ITEMS_TABLE_H",
    )
    write_header(os.path.join(args.output_dir, "str_model_3d_items_table.h"), table_header)

    print("\n=== Chair header ===")
    chair_vertices, chair_normals, chair_groups = load_centered_furniture_from_level(
        vertices,
        normals,
        objects,
        "chair",
    )
    chair_header = generate_furniture_header(
        chair_vertices,
        chair_normals,
        chair_groups,
        model_name="chair",
        guard="STR_MODEL_3D_ITEMS_CHAIR_H",
    )
    write_header(os.path.join(args.output_dir, "str_model_3d_items_chair.h"), chair_header)

    if not args.skip_standalone:
        print("\n=== Blaster header ===")
        sys.stdout.flush()
        blaster_out = os.path.join(args.output_dir, "str_model_3d_items_blaster.h")
        blaster_cmd = [
            sys.executable,
            os.path.join(TOOLS_DIR, "obj_to_header.py"),
            args.blaster_obj,
            args.blaster_mtl,
            blaster_out,
            "--scale",
            "4.0",
            "--name",
            "blaster",
            "--namespace",
            "str",
        ]
        subprocess.run(blaster_cmd, check=True)

        print("\n=== Player Car header ===")
        player_car_out = os.path.join(args.output_dir, "fr_model_3d_items_player_car.h")
        if os.path.isfile(args.player_car_header):
            shutil.copyfile(args.player_car_header, player_car_out)
            print(f"  Copied: {player_car_out}")
        else:
            print(
                f"  WARNING: player car source header not found: {args.player_car_header}"
            )

    print("\n=== Summary ===")
    print(f"  Room palette: {len(ROOM_PALETTE)} colors")
    if args.skip_standalone:
        print("  Generated: room, table, chair headers")
    else:
        print("  Generated: room, table, chair, blaster, player_car headers")
    print("  Furniture source: extracted from level.obj room meshes")
    print("\nLevel headers generated. Rebuild the project.")
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
