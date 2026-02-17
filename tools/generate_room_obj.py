#!/usr/bin/env python3
"""Generate a 3D interior room as OBJ + MTL files.

Creates a cutaway diorama room with 2 walls (back + left), floor,
baseboards, window, table, and chair. All geometry uses blocky
low-poly shapes matching the varooom-3d model style.

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
        # 8 vertices of the box
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
        # Bottom face (Z=z1, normal -Z) CCW from outside (looking from -Z)
        self.add_quad(material, v[3], v[2], v[1], v[0], 0, 0, -1)
        # Top face (Z=z2, normal +Z) CCW from outside (looking from +Z)
        self.add_quad(material, v[4], v[5], v[6], v[7], 0, 0, 1)
        # Front face (Y=y1, normal -Y)
        self.add_quad(material, v[0], v[1], v[5], v[4], 0, -1, 0)
        # Back face (Y=y2, normal +Y)
        self.add_quad(material, v[2], v[3], v[7], v[6], 0, 1, 0)
        # Left face (X=x1, normal -X)
        self.add_quad(material, v[3], v[0], v[4], v[7], -1, 0, 0)
        # Right face (X=x2, normal +X)
        self.add_quad(material, v[1], v[2], v[6], v[5], 1, 0, 0)

    def write_obj(self, path, mtl_name):
        """Write OBJ file."""
        with open(path, 'w') as f:
            f.write(f"# Generated room model\n")
            f.write(f"mtllib {mtl_name}\n")
            f.write(f"o Room\n")

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


def build_room():
    """Build the room geometry and return ObjBuilder + materials dict."""
    obj = ObjBuilder()

    # Materials (muted colors, will be converted to GBA 5-bit RGB)
    materials = {
        "floor_light":      (0.71, 0.52, 0.26),   # warm medium brown
        "floor_dark":       (0.52, 0.32, 0.13),   # darker wood plank
        "wall":             (0.90, 0.84, 0.71),   # off-white/cream
        "wall_shadow":      (0.71, 0.65, 0.52),   # darker cream
        "baseboard":        (0.39, 0.26, 0.13),   # dark wood trim
        "table_wood":       (0.58, 0.39, 0.19),   # medium-dark wood
        "chair_frame":      (0.65, 0.45, 0.26),   # lighter wood
        "chair_fabric":     (0.26, 0.39, 0.58),   # muted blue-grey
        "window_glass":     (0.52, 0.71, 0.90),   # pale blue
        "window_frame":     (0.32, 0.26, 0.19),   # dark frame
    }

    # Room dimensions (OBJ coordinates: Y-up, Z-forward)
    # After axis swap in engine: X stays, Y becomes depth, Z becomes height
    # IMPORTANT: In the engine, +Z projects DOWNWARD on screen.
    # So we use NEGATIVE Y for "up" so that walls appear above the floor.
    # Floor at Y=0, walls extend to Y=-WALL_HEIGHT (negative = up on screen).
    FLOOR_SIZE = 30.0     # half-size, so floor is 60x60
    WALL_HEIGHT = 25.0
    WALL_THICK = 1.5

    # ---- FLOOR ----
    # Floor planks at Y=0, each overlapping neighbors by OVERLAP to prevent gaps
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

    # ---- BACK WALL (at Z = -FLOOR_SIZE, facing +Z) ----
    # Wall with window cutout. Walls go from Y=0 down to Y=-WALL_HEIGHT.
    WIN_LEFT = -8.0
    WIN_RIGHT = 8.0
    WIN_BOTTOM = -8.0     # negative Y = up on screen
    WIN_TOP = -18.0       # more negative = higher on screen
    WIN_DEPTH = 1.0       # inset depth

    # Back wall outer face - left of window
    v1 = obj.add_vertex(-FLOOR_SIZE, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_LEFT, 0, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_LEFT, -WALL_HEIGHT, -FLOOR_SIZE)
    v4 = obj.add_vertex(-FLOOR_SIZE, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v4, v3, v2, v1, 0, 0, 1)

    # Back wall - right of window
    v1 = obj.add_vertex(WIN_RIGHT, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(FLOOR_SIZE, 0, -FLOOR_SIZE)
    v3 = obj.add_vertex(FLOOR_SIZE, -WALL_HEIGHT, -FLOOR_SIZE)
    v4 = obj.add_vertex(WIN_RIGHT, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v4, v3, v2, v1, 0, 0, 1)

    # Back wall - below window (between floor and window bottom)
    v1 = obj.add_vertex(WIN_LEFT, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, 0, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE)
    v4 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE)
    obj.add_quad("wall_shadow", v4, v3, v2, v1, 0, 0, 1)

    # Back wall - above window
    v1 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, -WALL_HEIGHT, -FLOOR_SIZE)
    v4 = obj.add_vertex(WIN_LEFT, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v4, v3, v2, v1, 0, 0, 1)

    # Window inset - recessed glass pane
    v1 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_glass", v4, v3, v2, v1, 0, 0, 1)

    # Window frame - sill (bottom edge, faces downward in OBJ = upward on screen)
    v1 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_frame", v1, v2, v3, v4, 0, 1, 0)

    # Window frame - top (faces upward in OBJ = downward on screen, visible from above)
    v1 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_frame", v4, v3, v2, v1, 0, -1, 0)

    # Window frame - left side
    v1 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_LEFT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_LEFT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_frame", v1, v2, v3, v4, 1, 0, 0)

    # Window frame - right side
    v1 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE)
    v2 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE)
    v3 = obj.add_vertex(WIN_RIGHT, WIN_TOP, -FLOOR_SIZE - WIN_DEPTH)
    v4 = obj.add_vertex(WIN_RIGHT, WIN_BOTTOM, -FLOOR_SIZE - WIN_DEPTH)
    obj.add_quad("window_frame", v4, v3, v2, v1, -1, 0, 0)

    # ---- LEFT WALL (at X = -FLOOR_SIZE, facing +X) ----
    v1 = obj.add_vertex(-FLOOR_SIZE, 0, -FLOOR_SIZE)
    v2 = obj.add_vertex(-FLOOR_SIZE, 0, FLOOR_SIZE)
    v3 = obj.add_vertex(-FLOOR_SIZE, -WALL_HEIGHT, FLOOR_SIZE)
    v4 = obj.add_vertex(-FLOOR_SIZE, -WALL_HEIGHT, -FLOOR_SIZE)
    obj.add_quad("wall", v1, v2, v3, v4, 1, 0, 0)

    # ---- TABLE ----
    # Table centered around (5, 0, 0) in OBJ coords.
    # Height goes NEGATIVE (up on screen).
    TABLE_X = 5.0
    TABLE_Z = 0.0
    TABLE_TOP_H = 10.0    # distance from floor (positive value, used as negative Y)
    TABLE_W = 12.0         # width (X)
    TABLE_D = 8.0          # depth (Z)
    TABLE_TOP_THICK = 0.8
    LEG_SIZE = 0.8

    # Table top (Y from -TABLE_TOP_H to -(TABLE_TOP_H - THICK))
    obj.add_box("table_wood",
                TABLE_X - TABLE_W/2, -TABLE_TOP_H, TABLE_Z - TABLE_D/2,
                TABLE_X + TABLE_W/2, -(TABLE_TOP_H - TABLE_TOP_THICK), TABLE_Z + TABLE_D/2)

    # Table legs (4 corners, from floor Y=0 down to table top)
    for dx in [-1, 1]:
        for dz in [-1, 1]:
            lx = TABLE_X + dx * (TABLE_W/2 - LEG_SIZE)
            lz = TABLE_Z + dz * (TABLE_D/2 - LEG_SIZE)
            obj.add_box("table_wood",
                        lx - LEG_SIZE/2, -(TABLE_TOP_H - TABLE_TOP_THICK), lz - LEG_SIZE/2,
                        lx + LEG_SIZE/2, 0, lz + LEG_SIZE/2)

    # ---- CHAIR ----
    # Chair positioned near the table, facing it (back away from table)
    CHAIR_X = 5.0
    CHAIR_Z = 8.0       # in front of table (OBJ +Z = engine +Y = closer to camera)
    SEAT_H = 7.0        # seat height from floor
    SEAT_W = 6.0
    SEAT_D = 6.0
    SEAT_THICK = 0.6
    CHAIR_LEG = 0.6
    BACK_H = 14.0       # top of chair back from floor
    BACK_THICK = 0.6

    # Chair seat
    obj.add_box("chair_fabric",
                CHAIR_X - SEAT_W/2, -SEAT_H, CHAIR_Z - SEAT_D/2,
                CHAIR_X + SEAT_W/2, -(SEAT_H - SEAT_THICK), CHAIR_Z + SEAT_D/2)

    # Chair legs
    for dx in [-1, 1]:
        for dz in [-1, 1]:
            lx = CHAIR_X + dx * (SEAT_W/2 - CHAIR_LEG)
            lz = CHAIR_Z + dz * (SEAT_D/2 - CHAIR_LEG)
            obj.add_box("chair_frame",
                        lx - CHAIR_LEG/2, -(SEAT_H - SEAT_THICK), lz - CHAIR_LEG/2,
                        lx + CHAIR_LEG/2, 0, lz + CHAIR_LEG/2)

    # Chair back (on the +Z side = away from table, person sitting faces -Z toward table)
    obj.add_box("chair_fabric",
                CHAIR_X - SEAT_W/2, -BACK_H, CHAIR_Z + SEAT_D/2 - BACK_THICK,
                CHAIR_X + SEAT_W/2, -SEAT_H, CHAIR_Z + SEAT_D/2)

    return obj, materials


def main():
    parser = argparse.ArgumentParser(description='Generate room OBJ + MTL files')
    parser.add_argument('--output-dir', default='.', help='Output directory')
    args = parser.parse_args()

    obj, materials = build_room()

    obj_path = os.path.join(args.output_dir, 'room.obj')
    mtl_path = os.path.join(args.output_dir, 'room.mtl')

    os.makedirs(args.output_dir, exist_ok=True)
    write_mtl(mtl_path, materials)
    obj.write_obj(obj_path, 'room.mtl')

    # Budget report
    total_faces = len(obj.faces)
    total_verts = len(obj.vertices)
    quads = sum(1 for _, v, _ in obj.faces if len(v) == 4)
    tris = total_faces - quads

    print(f"Room model generated:")
    print(f"  Vertices: {total_verts} / 256 max")
    print(f"  Faces: {total_faces} ({quads} quads, {tris} tris) / 300 max")
    print(f"  Materials: {len(materials)} / 10 max")
    print(f"  Estimated visible faces (after backface culling): ~{total_faces // 2}")
    print(f"  Written: {obj_path}, {mtl_path}")

    if total_verts > 256:
        print(f"  WARNING: Vertex count {total_verts} exceeds engine limit 256!")
    if total_faces > 300:
        print(f"  WARNING: Face count {total_faces} exceeds engine limit 300!")
    if len(materials) > 10:
        print(f"  WARNING: Material count {len(materials)} exceeds engine limit 10!")


if __name__ == '__main__':
    main()
