#!/usr/bin/env python3
"""Generate 3D room models as separate OBJ + MTL files.

Creates three separate models:
  - room_shell: walls, floor, baseboards, window
  - table: table top + 4 legs (origin at table center)
  - chair: seat + legs + back (origin at chair center)

All models share the same MTL file so their color indices
are compatible when loaded into the engine.

Output is compatible with obj_to_butano.py for conversion to
Butano engine C++ header format.

Usage:
    python generate_room_obj.py [--output-dir DIR]
"""

import os
import argparse


class ObjBuilder:
    """Builds OBJ geometry with material groups."""

    def __init__(self):
        self.vertices = []      # (x, y, z)
        self.normals = []       # (nx, ny, nz)
        self.faces = []         # (material, [v_indices], normal_index)
        self._normal_map = {}   # (nx,ny,nz) -> index

    def add_vertex(self, x, y, z):
        """Add vertex, return 1-based index."""
        self.vertices.append((x, y, z))
        return len(self.vertices)

    def _get_normal(self, nx, ny, nz):
        """Get or create normal, return 1-based index."""
        key = (round(nx, 6), round(ny, 6), round(nz, 6))
        if key not in self._normal_map:
            self.normals.append(key)
            self._normal_map[key] = len(self.normals)
        return self._normal_map[key]

    def add_quad(self, material, v1, v2, v3, v4, nx, ny, nz):
        """Add a quad face (4 vertices, CCW winding from outside).
        v1-v4 are 1-based vertex indices."""
        ni = self._get_normal(nx, ny, nz)
        self.faces.append((material, [v1, v2, v3, v4], ni))

    def add_box(self, material, x1, y1, z1, x2, y2, z2):
        """Add an axis-aligned box. (x1,y1,z1) is min corner, (x2,y2,z2) is max.
        Generates 6 quads with outward-facing normals (CCW from outside)."""
        v = [
            self.add_vertex(x1, y1, z1),  # 0: min corner
            self.add_vertex(x2, y1, z1),  # 1
            self.add_vertex(x2, y2, z1),  # 2
            self.add_vertex(x1, y2, z1),  # 3
            self.add_vertex(x1, y1, z2),  # 4: max Z
            self.add_vertex(x2, y1, z2),  # 5
            self.add_vertex(x2, y2, z2),  # 6
            self.add_vertex(x1, y2, z2),  # 7
        ]
        self.add_quad(material, v[3], v[2], v[1], v[0], 0, 0, -1)
        self.add_quad(material, v[4], v[5], v[6], v[7], 0, 0, 1)
        self.add_quad(material, v[0], v[1], v[5], v[4], 0, -1, 0)
        self.add_quad(material, v[2], v[3], v[7], v[6], 0, 1, 0)
        self.add_quad(material, v[3], v[0], v[4], v[7], -1, 0, 0)
        self.add_quad(material, v[1], v[2], v[6], v[5], 1, 0, 0)

    def write_obj(self, path, mtl_name, object_name="Object"):
        """Write OBJ file."""
        with open(path, 'w') as f:
            f.write(f"# Generated model: {object_name}\n")
            f.write(f"mtllib {mtl_name}\n")
            f.write(f"o {object_name}\n")

            for x, y, z in self.vertices:
                f.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")

            for nx, ny, nz in self.normals:
                f.write(f"vn {nx:.6f} {ny:.6f} {nz:.6f}\n")

            current_mtl = None
            for material, verts, ni in self.faces:
                if material != current_mtl:
                    f.write(f"usemtl {material}\n")
                    f.write("s off\n")
                    current_mtl = material
                vert_str = " ".join(f"{v}//{ni}" for v in verts)
                f.write(f"f {vert_str}\n")


ALL_MATERIALS = {
    "floor_light":      (0.71, 0.52, 0.26),
    "floor_dark":       (0.52, 0.32, 0.13),
    "wall":             (0.90, 0.84, 0.71),
    "wall_shadow":      (0.71, 0.65, 0.52),
    "baseboard":        (0.39, 0.26, 0.13),
    "table_wood":       (0.58, 0.39, 0.19),
    "chair_frame":      (0.65, 0.45, 0.26),
    "chair_fabric":     (0.26, 0.39, 0.58),
    "window_glass":     (0.52, 0.71, 0.90),
    "window_frame":     (0.32, 0.26, 0.19),
}

FLOOR_SIZE = 30.0
WALL_HEIGHT = 25.0

TABLE_X = 5.0
TABLE_Z = 0.0
TABLE_TOP_H = 10.0
TABLE_W = 12.0
TABLE_D = 8.0
TABLE_TOP_THICK = 0.8
LEG_SIZE = 0.8

CHAIR_X = 5.0
CHAIR_Z = 8.0
SEAT_H = 7.0
SEAT_W = 6.0
SEAT_D = 6.0
SEAT_THICK = 0.6
CHAIR_LEG = 0.6
BACK_H = 14.0
BACK_THICK = 0.6


def write_mtl(path, materials):
    """Write MTL file. materials is dict of name -> (r, g, b) in 0-1 float."""
    with open(path, 'w') as f:
        f.write("# Generated room materials\n")
        for name, (r, g, b) in materials.items():
            f.write(f"\nnewmtl {name}\n")
            f.write(f"Kd {r:.3f} {g:.3f} {b:.3f}\n")
            f.write(f"Ka 0.1 0.1 0.1\n")
            f.write(f"d 1.0\n")
            f.write(f"illum 1\n")


def build_room_shell():
    """Build the room shell: walls, floor, window. No furniture."""
    obj = ObjBuilder()

    PLANK_COUNT = 3
    plank_width = (FLOOR_SIZE * 2) / PLANK_COUNT
    for i in range(PLANK_COUNT):
        mtl = "floor_light" if i % 2 == 0 else "floor_dark"
        x1 = -FLOOR_SIZE + i * plank_width
        x2 = x1 + plank_width
        v1 = obj.add_vertex(x1, 0, -FLOOR_SIZE)
        v2 = obj.add_vertex(x2, 0, -FLOOR_SIZE)
        v3 = obj.add_vertex(x2, 0, FLOOR_SIZE)
        v4 = obj.add_vertex(x1, 0, FLOOR_SIZE)
        obj.add_quad(mtl, v1, v2, v3, v4, 0, -1, 0)

    WIN_LEFT = -8.0
    WIN_RIGHT = 8.0
    WIN_BOTTOM = -8.0
    WIN_TOP = -18.0
    WIN_DEPTH = 1.0

    v1 = obj.add_vertex(-FLOOR_SIZE, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_LEFT, 0, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_LEFT, -WALL_HEIGHT, -FLOOR_SIZE)
    v4 = obj.add_vertex(-FLOOR_SIZE, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v4, v3, v2, v1, 0, 0, 1)

    v1 = obj.add_vertex(WIN_RIGHT, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(FLOOR_SIZE, 0, -FLOOR_SIZE)
    v3 = obj.add_vertex(FLOOR_SIZE, -WALL_HEIGHT, -FLOOR_SIZE)
    v4 = obj.add_vertex(WIN_RIGHT, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v4, v3, v2, v1, 0, 0, 1)

    v1 = obj.add_vertex(WIN_LEFT, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, 0, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE)
    v4 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE)
    obj.add_quad("wall_shadow", v4, v3, v2, v1, 0, 0, 1)

    v1 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, -WALL_HEIGHT, -FLOOR_SIZE)
    v4 = obj.add_vertex(WIN_LEFT, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v4, v3, v2, v1, 0, 0, 1)

    v1 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_glass", v4, v3, v2, v1, 0, 0, 1)

    v1 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_frame", v1, v2, v3, v4, 0, 1, 0)

    v1 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_frame", v4, v3, v2, v1, 0, -1, 0)

    v1 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_frame", v1, v2, v3, v4, 1, 0, 0)

    v1 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_frame", v4, v3, v2, v1, -1, 0, 0)

    v1 = obj.add_vertex(-FLOOR_SIZE, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(-FLOOR_SIZE, 0, FLOOR_SIZE)
    v3 = obj.add_vertex(-FLOOR_SIZE, -WALL_HEIGHT, FLOOR_SIZE)
    v4 = obj.add_vertex(-FLOOR_SIZE, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v1, v2, v3, v4, 1, 0, 0)

    # ---- RIGHT WALL (at X = +FLOOR_SIZE, facing -X) ----
    v1 = obj.add_vertex(FLOOR_SIZE, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(FLOOR_SIZE, 0, FLOOR_SIZE)
    v3 = obj.add_vertex(FLOOR_SIZE, -WALL_HEIGHT, FLOOR_SIZE)
    v4 = obj.add_vertex(FLOOR_SIZE, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v4, v3, v2, v1, -1, 0, 0)

    # ---- FRONT WALL (at Z = +FLOOR_SIZE, facing -Z) ----
    v1 = obj.add_vertex(-FLOOR_SIZE, 0, FLOOR_SIZE)
    v2 = obj.add_vertex(FLOOR_SIZE, 0, FLOOR_SIZE)
    v3 = obj.add_vertex(FLOOR_SIZE, -WALL_HEIGHT, FLOOR_SIZE)
    v4 = obj.add_vertex(-FLOOR_SIZE, -WALL_HEIGHT, FLOOR_SIZE)
    obj.add_quad("wall", v1, v2, v3, v4, 0, 0, -1)

    return obj


def build_table():
    """Build the table model, centered at its own origin."""
    obj = ObjBuilder()

    obj.add_box("table_wood",
                -TABLE_W/2, -TABLE_TOP_H, -TABLE_D/2,
                TABLE_W/2, -(TABLE_TOP_H - TABLE_TOP_THICK), TABLE_D/2)

    for dx in [-1, 1]:
        for dz in [-1, 1]:
            lx = dx * (TABLE_W/2 - LEG_SIZE)
            lz = dz * (TABLE_D/2 - LEG_SIZE)
            obj.add_box("table_wood",
                        lx - LEG_SIZE/2, -(TABLE_TOP_H - TABLE_TOP_THICK), lz - LEG_SIZE/2,
                        lx + LEG_SIZE/2, 0, lz + LEG_SIZE/2)

    return obj


def build_chair():
    """Build the chair model, centered at its own origin."""
    obj = ObjBuilder()

    obj.add_box("chair_fabric",
                -SEAT_W/2, -SEAT_H, -SEAT_D/2,
                SEAT_W/2, -(SEAT_H - SEAT_THICK), SEAT_D/2)

    for dx in [-1, 1]:
        for dz in [-1, 1]:
            lx = dx * (SEAT_W/2 - CHAIR_LEG)
            lz = dz * (SEAT_D/2 - CHAIR_LEG)
            obj.add_box("chair_frame",
                        lx - CHAIR_LEG/2, -(SEAT_H - SEAT_THICK), lz - CHAIR_LEG/2,
                        lx + CHAIR_LEG/2, 0, lz + CHAIR_LEG/2)

    obj.add_box("chair_fabric",
                -SEAT_W/2, -BACK_H, SEAT_D/2 - BACK_THICK,
                SEAT_W/2, -SEAT_H, SEAT_D/2)

    return obj


def report(name, obj):
    total_faces = len(obj.faces)
    total_verts = len(obj.vertices)
    quads = sum(1 for _, v, _ in obj.faces if len(v) == 4)
    tris = total_faces - quads
    print(f"  {name}: {total_verts} verts, {total_faces} faces ({quads} quads, {tris} tris)")
    return total_verts, total_faces


def main():
    parser = argparse.ArgumentParser(description='Generate room OBJ + MTL files')
    parser.add_argument('--output-dir', default='.', help='Output directory')
    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    mtl_name = 'room.mtl'
    mtl_path = os.path.join(args.output_dir, mtl_name)
    write_mtl(mtl_path, ALL_MATERIALS)

    models = {
        'room_shell': (build_room_shell(), 'RoomShell'),
        'table':      (build_table(), 'Table'),
        'chair':      (build_chair(), 'Chair'),
    }

    total_v = 0
    total_f = 0
    print("Room models generated:")
    for filename, (obj, obj_name) in models.items():
        obj_path = os.path.join(args.output_dir, f'{filename}.obj')
        obj.write_obj(obj_path, mtl_name, obj_name)
        v, f = report(filename, obj)
        total_v += v
        total_f += f
        print(f"    Written: {obj_path}")

    print(f"  TOTAL: {total_v} verts, {total_f} faces")
    print(f"  Materials: {len(ALL_MATERIALS)} / 10 max")
    print(f"  MTL: {mtl_path}")

    if total_v > 256:
        print(f"  WARNING: Combined vertex count {total_v} exceeds engine limit 256!")
    if total_f > 176:
        print(f"  WARNING: Combined face count {total_f} exceeds engine limit 176!")


if __name__ == '__main__':
    main()
