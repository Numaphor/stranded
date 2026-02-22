#!/usr/bin/env python3
"""Generate canonical obj/level.obj as the single source of truth.

Output object contract:
- Building: structural shell only (floors + walls + doors)
- Room0..Room5: explicit per-room shell meshes with decor instances baked in
"""

from __future__ import annotations

import os


REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJ_DIR = os.path.join(REPO_ROOT, "obj")

ROOM_SCALE = 2.0
BUILDING_SCALE = 2.0

ROOM_NAMES = ["Room0", "Room1", "Room2", "Room3", "Room4", "Room5"]

# 2x3 building layout in room ids.
ROOM_CENTERS_OBJ = {
    0: (-30.0, -60.0),
    1: (30.0, -60.0),
    2: (-30.0, 0.0),
    3: (30.0, 0.0),
    4: (-30.0, 60.0),
    5: (30.0, 60.0),
}

# R0 TC, R1 W, R2 T, R3 CW, R4 T, R5 C
ROOM_DECOR = {
    0: {"table", "chair"},
    1: {"window"},
    2: {"table"},
    3: {"chair", "window"},
    4: {"table"},
    5: {"chair"},
}

WINDOW_WALL_BY_ROOM = {
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

# Building vertices from building.h (engine coordinates)
BUILDING_ENGINE_VERTS = [
    (-120.0, -180.0, 0.0), (0.0, -180.0, 0.0), (120.0, -180.0, 0.0),
    (-120.0, -60.0, 0.0), (0.0, -60.0, 0.0), (120.0, -60.0, 0.0),
    (-120.0, 60.0, 0.0), (0.0, 60.0, 0.0), (120.0, 60.0, 0.0),
    (-120.0, 180.0, 0.0), (0.0, 180.0, 0.0), (120.0, 180.0, 0.0),
    (-120.0, -180.0, -50.0), (0.0, -180.0, -50.0), (120.0, -180.0, -50.0),
    (-120.0, -60.0, -50.0), (0.0, -60.0, -50.0), (120.0, -60.0, -50.0),
    (-120.0, 60.0, -50.0), (0.0, 60.0, -50.0), (120.0, 60.0, -50.0),
    (-120.0, 180.0, -50.0), (0.0, 180.0, -50.0), (120.0, 180.0, -50.0),
    (0.0, -130.0, 0.0), (0.0, -110.0, 0.0), (0.0, -10.0, 0.0), (0.0, 10.0, 0.0),
    (0.0, 110.0, 0.0), (0.0, 130.0, 0.0),
    (-70.0, -60.0, 0.0), (-50.0, -60.0, 0.0), (50.0, -60.0, 0.0), (70.0, -60.0, 0.0),
    (-70.0, 60.0, 0.0), (-50.0, 60.0, 0.0), (50.0, 60.0, 0.0), (70.0, 60.0, 0.0),
    (0.0, -130.0, -50.0), (0.0, -110.0, -50.0), (0.0, -10.0, -50.0), (0.0, 10.0, -50.0),
    (0.0, 110.0, -50.0), (0.0, 130.0, -50.0),
    (-70.0, -60.0, -50.0), (-50.0, -60.0, -50.0), (50.0, -60.0, -50.0), (70.0, -60.0, -50.0),
    (-70.0, 60.0, -50.0), (-50.0, 60.0, -50.0), (50.0, 60.0, -50.0), (70.0, 60.0, -50.0),
]

# (material, engine_normal, vertex_indices_0based)
BUILDING_FACES = [
    ("room0_floor", (0, 0, -1), [3, 4, 1, 0]),
    ("room1_floor", (0, 0, -1), [4, 5, 2, 1]),
    ("room2_floor", (0, 0, -1), [6, 7, 4, 3]),
    ("room3_floor", (0, 0, -1), [7, 8, 5, 4]),
    ("room4_floor", (0, 0, -1), [9, 10, 7, 6]),
    ("room5_floor", (0, 0, -1), [10, 11, 8, 7]),
    ("outer_wall", (0, 1, 0), [0, 2, 14, 12]),
    ("outer_wall", (-1, 0, 0), [2, 11, 23, 14]),
    ("outer_wall", (0, -1, 0), [9, 11, 23, 21]),
    ("outer_wall", (1, 0, 0), [12, 21, 9, 0]),
    ("interior_wall", (-1, 0, 0), [1, 24, 38, 13]),
    ("interior_wall", (1, 0, 0), [13, 38, 24, 1]),
    ("interior_wall", (-1, 0, 0), [25, 4, 16, 39]),
    ("interior_wall", (1, 0, 0), [39, 16, 4, 25]),
    ("interior_wall", (-1, 0, 0), [4, 26, 40, 16]),
    ("interior_wall", (1, 0, 0), [16, 40, 26, 4]),
    ("interior_wall", (-1, 0, 0), [27, 7, 19, 41]),
    ("interior_wall", (1, 0, 0), [41, 19, 7, 27]),
    ("interior_wall", (-1, 0, 0), [7, 28, 42, 19]),
    ("interior_wall", (1, 0, 0), [19, 42, 28, 7]),
    ("interior_wall", (-1, 0, 0), [29, 10, 22, 43]),
    ("interior_wall", (1, 0, 0), [43, 22, 10, 29]),
    ("interior_wall", (0, 1, 0), [3, 30, 44, 15]),
    ("interior_wall", (0, -1, 0), [30, 3, 15, 44]),
    ("interior_wall", (0, 1, 0), [31, 4, 16, 45]),
    ("interior_wall", (0, -1, 0), [4, 31, 45, 16]),
    ("interior_wall", (0, 1, 0), [4, 32, 46, 16]),
    ("interior_wall", (0, -1, 0), [32, 4, 16, 46]),
    ("interior_wall", (0, 1, 0), [33, 5, 17, 47]),
    ("interior_wall", (0, -1, 0), [5, 33, 47, 17]),
    ("interior_wall", (0, 1, 0), [6, 34, 48, 18]),
    ("interior_wall", (0, -1, 0), [34, 6, 18, 48]),
    ("interior_wall", (0, 1, 0), [35, 7, 19, 49]),
    ("interior_wall", (0, -1, 0), [7, 35, 49, 19]),
    ("interior_wall", (0, 1, 0), [7, 36, 50, 19]),
    ("interior_wall", (0, -1, 0), [36, 7, 19, 50]),
    ("interior_wall", (0, 1, 0), [37, 8, 20, 51]),
    ("interior_wall", (0, -1, 0), [8, 37, 51, 20]),
]

OBJ_NORMAL_TO_ENGINE = {
    1: (0, 0, -1),
    2: (0, 1, 0),
    3: (0, 0, 1),
    4: (1, 0, 0),
    5: (-1, 0, 0),
    6: (0, -1, 0),
}

# Room shell dimensions in canonical OBJ space.
ROOM_HALF = 30.0
ROOM_WALL_TOP = 25.0
DOOR_HALF = 5.0
DOOR_TOP = 17.5

WINDOW_BOTTOM = 10.0
WINDOW_TOP = 18.0
WINDOW_HALF_SPAN = 8.0
WINDOW_INSET = 0.08

# Runtime baseline placements are in room-local engine XY.
TABLE_LOCAL_OBJ = (10.0 / ROOM_SCALE, 0.0, 0.0 / ROOM_SCALE)
CHAIR_LOCAL_OBJ = (10.0 / ROOM_SCALE, 0.0, 16.0 / ROOM_SCALE)


def engine_to_obj(ex: float, ey: float, ez: float, scale: float) -> tuple[float, float, float]:
    """Inverse of transform_vertex with flip_z=True."""
    return (ex / scale, -ez / scale, ey / scale)


def engine_normal_to_obj(enx: float, eny: float, enz: float) -> tuple[float, float, float]:
    return (enx, -enz, eny)


def read_existing_obj(path: str):
    vertices = []
    faces = []  # (material, [(v_idx_1based, n_idx_1based), ...])
    current_mtl = None

    with open(path, "r") as file:
        for line in file:
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0] == "v":
                vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == "usemtl":
                current_mtl = parts[1]
            elif parts[0] == "f":
                face_verts = []
                for vert in parts[1:]:
                    indices = vert.split("/")
                    v_idx = int(indices[0])
                    n_idx = int(indices[2]) if len(indices) >= 3 and indices[2] else 1
                    face_verts.append((v_idx, n_idx))
                faces.append((current_mtl, face_verts))

    return vertices, faces


def center_vertices(vertices, faces):
    used = sorted({v_idx - 1 for _mtl, fv in faces for v_idx, _n_idx in fv})
    if not used:
        return list(vertices)

    cx = sum(vertices[i][0] for i in used) / len(used)
    cy = sum(vertices[i][1] for i in used) / len(used)
    cz = sum(vertices[i][2] for i in used) / len(used)
    return [(x - cx, y - cy, z - cz) for x, y, z in vertices]


def validate_decor_spec():
    allowed_tokens = {"table", "chair", "window"}
    for room_id in range(6):
        if room_id not in ROOM_DECOR:
            raise ValueError(f"Missing decor spec for room {room_id}")
        unknown = ROOM_DECOR[room_id] - allowed_tokens
        if unknown:
            raise ValueError(f"Unknown decor tokens for room {room_id}: {sorted(unknown)}")

    for room_id, wall in WINDOW_WALL_BY_ROOM.items():
        if "window" not in ROOM_DECOR.get(room_id, set()):
            raise ValueError(f"Window wall specified for room {room_id} but room has no window token")
        if wall in ROOM_DOOR_WALLS.get(room_id, set()):
            raise ValueError(
                f"Invalid window placement: room {room_id} wall '{wall}' has a door opening"
            )


def _add_quad(local_vertices, local_faces, material, normal, quad):
    base = len(local_vertices)
    local_vertices.extend(quad)
    local_faces.append((material, normal, [base, base + 1, base + 2, base + 3]))


def _add_quad_double_sided(local_vertices, local_faces, material, normal, quad):
    _add_quad(local_vertices, local_faces, material, normal, quad)
    inv = (-normal[0], -normal[1], -normal[2])
    _add_quad(local_vertices, local_faces, material, inv, [quad[3], quad[2], quad[1], quad[0]])


def _add_wall(local_vertices, local_faces, wall_name, has_door):
    x0, x1 = -ROOM_HALF, ROOM_HALF
    z0, z1 = -ROOM_HALF, ROOM_HALF
    y0, y1 = 0.0, ROOM_WALL_TOP
    ds0, ds1 = -DOOR_HALF, DOOR_HALF

    if wall_name == "north":
        normal = (0, 1, 0)
        if has_door:
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x0, y0, z0), (ds0, y0, z0), (ds0, y1, z0), (x0, y1, z0)])
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(ds1, y0, z0), (x1, y0, z0), (x1, y1, z0), (ds1, y1, z0)])
            _add_quad_double_sided(local_vertices, local_faces, "door_frame", normal, [(ds0, DOOR_TOP, z0), (ds1, DOOR_TOP, z0), (ds1, y1, z0), (ds0, y1, z0)])
        else:
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)])
    elif wall_name == "south":
        normal = (0, -1, 0)
        if has_door:
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x0, y1, z1), (ds0, y1, z1), (ds0, y0, z1), (x0, y0, z1)])
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(ds1, y1, z1), (x1, y1, z1), (x1, y0, z1), (ds1, y0, z1)])
            _add_quad_double_sided(local_vertices, local_faces, "door_frame", normal, [(ds0, y1, z1), (ds1, y1, z1), (ds1, DOOR_TOP, z1), (ds0, DOOR_TOP, z1)])
        else:
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x0, y1, z1), (x1, y1, z1), (x1, y0, z1), (x0, y0, z1)])
    elif wall_name == "west":
        normal = (1, 0, 0)
        if has_door:
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x0, y1, z0), (x0, y1, ds0), (x0, y0, ds0), (x0, y0, z0)])
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x0, y1, ds1), (x0, y1, z1), (x0, y0, z1), (x0, y0, ds1)])
            _add_quad_double_sided(local_vertices, local_faces, "door_frame", normal, [(x0, y1, ds0), (x0, y1, ds1), (x0, DOOR_TOP, ds1), (x0, DOOR_TOP, ds0)])
        else:
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x0, y1, z0), (x0, y1, z1), (x0, y0, z1), (x0, y0, z0)])
    elif wall_name == "east":
        normal = (-1, 0, 0)
        if has_door:
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x1, y0, z0), (x1, y0, ds0), (x1, y1, ds0), (x1, y1, z0)])
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x1, y0, ds1), (x1, y0, z1), (x1, y1, z1), (x1, y1, ds1)])
            _add_quad_double_sided(local_vertices, local_faces, "door_frame", normal, [(x1, DOOR_TOP, ds0), (x1, DOOR_TOP, ds1), (x1, y1, ds1), (x1, y1, ds0)])
        else:
            _add_quad_double_sided(local_vertices, local_faces, "wall", normal, [(x1, y0, z0), (x1, y0, z1), (x1, y1, z1), (x1, y1, z0)])
    else:
        raise ValueError(f"Unknown wall '{wall_name}'")


def _add_window_faces(local_vertices, local_faces, wall):
    if wall == "north":
        z = -ROOM_HALF + WINDOW_INSET
        v0 = (-WINDOW_HALF_SPAN, WINDOW_BOTTOM, z)
        v1 = (WINDOW_HALF_SPAN, WINDOW_BOTTOM, z)
        v2 = (WINDOW_HALF_SPAN, WINDOW_TOP, z)
        v3 = (-WINDOW_HALF_SPAN, WINDOW_TOP, z)
        _add_quad(local_vertices, local_faces, "reserved", (0, 1, 0), [v0, v1, v2, v3])
        _add_quad(local_vertices, local_faces, "reserved", (0, -1, 0), [v3, v2, v1, v0])
    elif wall == "east":
        x = ROOM_HALF - WINDOW_INSET
        v0 = (x, WINDOW_BOTTOM, -WINDOW_HALF_SPAN)
        v1 = (x, WINDOW_BOTTOM, WINDOW_HALF_SPAN)
        v2 = (x, WINDOW_TOP, WINDOW_HALF_SPAN)
        v3 = (x, WINDOW_TOP, -WINDOW_HALF_SPAN)
        _add_quad(local_vertices, local_faces, "reserved", (-1, 0, 0), [v0, v1, v2, v3])
        _add_quad(local_vertices, local_faces, "reserved", (1, 0, 0), [v3, v2, v1, v0])
    else:
        raise ValueError(f"Unsupported window wall '{wall}'")


def _add_instance_faces(local_vertices, local_faces, source_vertices, source_faces, local_offset):
    ox, oy, oz = local_offset
    base = len(local_vertices)
    local_vertices.extend((vx + ox, vy + oy, vz + oz) for vx, vy, vz in source_vertices)

    for material_name, face_verts in source_faces:
        if len(face_verts) < 3:
            continue

        n_local = face_verts[0][1] if face_verts else 1
        eng_normal = OBJ_NORMAL_TO_ENGINE.get(n_local, (0, 0, -1))
        indices = [base + (v_local - 1) for v_local, _n_local in face_verts]

        if len(set(indices)) < 3:
            continue

        local_faces.append((material_name, eng_normal, indices))


def _build_room_mesh(room_id, table_vertices, table_faces, chair_vertices, chair_faces):
    local_vertices = []
    local_faces = []

    # Three floor strips so floor_dark can remain the center strip.
    _add_quad(local_vertices, local_faces, "floor_light", (0, 0, -1), [(-30, 0, 30), (-10, 0, 30), (-10, 0, -30), (-30, 0, -30)])
    _add_quad(local_vertices, local_faces, "floor_dark", (0, 0, -1), [(-10, 0, 30), (10, 0, 30), (10, 0, -30), (-10, 0, -30)])
    _add_quad(local_vertices, local_faces, "floor_light", (0, 0, -1), [(10, 0, 30), (30, 0, 30), (30, 0, -30), (10, 0, -30)])

    door_walls = ROOM_DOOR_WALLS[room_id]
    _add_wall(local_vertices, local_faces, "north", "north" in door_walls)
    _add_wall(local_vertices, local_faces, "south", "south" in door_walls)
    _add_wall(local_vertices, local_faces, "west", "west" in door_walls)
    _add_wall(local_vertices, local_faces, "east", "east" in door_walls)

    if "window" in ROOM_DECOR[room_id]:
        window_wall = WINDOW_WALL_BY_ROOM.get(room_id)
        if not window_wall:
            raise ValueError(f"Room {room_id} has window decor but no wall target")
        _add_window_faces(local_vertices, local_faces, window_wall)

    if "table" in ROOM_DECOR[room_id]:
        _add_instance_faces(local_vertices, local_faces, table_vertices, table_faces, TABLE_LOCAL_OBJ)

    if "chair" in ROOM_DECOR[room_id]:
        _add_instance_faces(local_vertices, local_faces, chair_vertices, chair_faces, CHAIR_LOCAL_OBJ)

    return local_vertices, local_faces


def main():
    validate_decor_spec()
    out_path = os.path.join(OBJ_DIR, "level.obj")

    normal_map = {}

    def get_normal_idx(engine_normal):
        key = tuple(engine_normal)
        if key not in normal_map:
            normal_map[key] = len(normal_map) + 1
        return normal_map[key]

    for axis_normal in [(0, 0, -1), (0, 1, 0), (0, -1, 0), (1, 0, 0), (-1, 0, 0), (0, 0, 1)]:
        get_normal_idx(axis_normal)

    all_vertices = []
    object_faces = {
        "Building": [],
        "Room0": [],
        "Room1": [],
        "Room2": [],
        "Room3": [],
        "Room4": [],
        "Room5": [],
    }

    # Building (structure only).
    building_obj_verts = [engine_to_obj(*v, BUILDING_SCALE) for v in BUILDING_ENGINE_VERTS]
    building_offset = len(all_vertices)
    all_vertices.extend(building_obj_verts)
    for mtl, eng_normal, vert_indices in BUILDING_FACES:
        object_faces["Building"].append((mtl, eng_normal, [building_offset + i for i in vert_indices]))
        get_normal_idx(eng_normal)

    # Template furniture meshes (centered around local origin).
    table_verts_raw, table_faces = read_existing_obj(os.path.join(OBJ_DIR, "table.obj"))
    chair_verts_raw, chair_faces = read_existing_obj(os.path.join(OBJ_DIR, "chair.obj"))
    table_centered = center_vertices(table_verts_raw, table_faces)
    chair_centered = center_vertices(chair_verts_raw, chair_faces)

    # Explicit room meshes.
    for room_id, room_name in enumerate(ROOM_NAMES):
        local_vertices, local_faces = _build_room_mesh(
            room_id,
            table_centered,
            table_faces,
            chair_centered,
            chair_faces,
        )
        cx, cz = ROOM_CENTERS_OBJ[room_id]
        world_vertices = [(x + cx, y, z + cz) for x, y, z in local_vertices]
        base = len(all_vertices)
        all_vertices.extend(world_vertices)

        for mtl, eng_normal, local_indices in local_faces:
            object_faces[room_name].append((mtl, eng_normal, [base + i for i in local_indices]))
            get_normal_idx(eng_normal)

    ordered_objects = ["Building", *ROOM_NAMES]

    with open(out_path, "w") as output:
        output.write("# Level model - canonical single source\n")
        output.write("# Generated by create_starter_level_obj.py\n")
        output.write("mtllib level.mtl\n\n")

        output.write("# Global vertex pool\n")
        for x, y, z in all_vertices:
            output.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")

        output.write("\n# Normals\n")
        for en_tuple in sorted(normal_map.keys(), key=lambda key: normal_map[key]):
            ox, oy, oz = engine_normal_to_obj(*en_tuple)
            output.write(f"vn {ox:.6f} {oy:.6f} {oz:.6f}\n")

        for object_name in ordered_objects:
            output.write(f"\no {object_name}\n")
            current_mtl = None
            for mtl, eng_normal, face_indices in object_faces[object_name]:
                if mtl != current_mtl:
                    output.write(f"usemtl {mtl}\n")
                    output.write("s off\n")
                    current_mtl = mtl
                normal_idx = get_normal_idx(eng_normal)
                face_tokens = " ".join(f"{idx + 1}//{normal_idx}" for idx in face_indices)
                output.write(f"f {face_tokens}\n")

    missing = [name for name in ordered_objects if not object_faces[name]]
    if missing:
        raise ValueError(f"Generated level has empty required objects: {missing}")

    print(f"Written {out_path}")
    print(f"  Vertices: {len(all_vertices)}")
    print(f"  Normals: {len(normal_map)}")
    print(f"  Objects: {ordered_objects}")
    print(f"  Decor map: {ROOM_DECOR}")
    print(f"  Window walls: {WINDOW_WALL_BY_ROOM}")


if __name__ == "__main__":
    main()
