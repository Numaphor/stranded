#!/usr/bin/env python3
"""Shared utilities for OBJ/MTL parsing and C++ header generation.

Used by both obj_to_header.py (standalone models) and
generate_level_headers.py (multi-object level pipeline).
"""

import math
from collections import defaultdict, OrderedDict


# ---------------------------------------------------------------------------
# MTL parsing
# ---------------------------------------------------------------------------

def parse_mtl(path):
    """Parse MTL file, return dict of material_name -> (r, g, b) in 0-1 float."""
    materials = {}
    current = None
    with open(path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0] == 'newmtl':
                current = parts[1]
            elif parts[0] == 'Kd' and current:
                r, g, b = float(parts[1]), float(parts[2]), float(parts[3])
                materials[current] = (r, g, b)
    return materials


# ---------------------------------------------------------------------------
# OBJ parsing
# ---------------------------------------------------------------------------

def parse_obj(path):
    """Parse single-object OBJ file. Returns (vertices, normals, face_groups).

    face_groups: list of (material_name, [(vertex_indices, normal_index), ...])
    Vertex indices are 0-based.
    """
    vertices = []
    normals = []
    face_groups = []
    current_mtl = None
    current_faces = []

    with open(path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0] == 'v':
                vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == 'vn':
                normals.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == 'usemtl':
                if current_faces:
                    face_groups.append((current_mtl, current_faces))
                current_mtl = parts[1]
                current_faces = []
            elif parts[0] == 'f':
                face_verts = []
                face_normal = None
                for vert in parts[1:]:
                    indices = vert.split('/')
                    v_idx = int(indices[0]) - 1  # OBJ is 1-indexed
                    if len(indices) >= 3 and indices[2]:
                        n_idx = int(indices[2]) - 1
                        face_normal = n_idx
                    face_verts.append(v_idx)
                current_faces.append((face_verts, face_normal))

    if current_faces:
        face_groups.append((current_mtl, current_faces))

    return vertices, normals, face_groups


def parse_obj_multi(path):
    """Parse multi-object OBJ file separated by 'o' tags.

    Returns (vertices, normals, objects_dict, mtl_filename) where:
        vertices: list of (x, y, z) -- global vertex pool (0-indexed)
        normals: list of (nx, ny, nz) -- global normal pool (0-indexed)
        objects_dict: OrderedDict[obj_name -> face_groups]
            face_groups: list of (material_name, [(vertex_indices, normal_index), ...])
            vertex_indices are 0-based into the global vertex pool
        mtl_filename: string from mtllib directive
    """
    vertices = []
    normals = []
    objects = OrderedDict()
    current_obj = None
    current_mtl = None
    current_faces = []
    mtl_filename = 'level.mtl'

    def flush_faces():
        nonlocal current_faces
        if current_obj is not None and current_faces:
            objects.setdefault(current_obj, []).append((current_mtl, current_faces))
            current_faces = []

    with open(path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if not parts or parts[0] == '#':
                continue

            if parts[0] == 'mtllib':
                mtl_filename = parts[1] if len(parts) > 1 else 'level.mtl'
            elif parts[0] == 'v':
                vertices.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == 'vn':
                normals.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == 'o':
                flush_faces()
                current_obj = parts[1] if len(parts) > 1 else 'Object'
                current_mtl = None
                current_faces = []
                if current_obj not in objects:
                    objects[current_obj] = []
            elif parts[0] == 'usemtl':
                flush_faces()
                current_mtl = parts[1] if len(parts) > 1 else None
                current_faces = []
            elif parts[0] == 'f':
                face_verts = []
                face_normal = None
                for vert in parts[1:]:
                    indices = vert.split('/')
                    v_idx = int(indices[0]) - 1  # 0-based
                    if len(indices) >= 3 and indices[2]:
                        face_normal = int(indices[2]) - 1
                    face_verts.append(v_idx)
                current_faces.append((face_verts, face_normal))

    flush_faces()

    return vertices, normals, objects, mtl_filename


# ---------------------------------------------------------------------------
# Vector math
# ---------------------------------------------------------------------------

def normalize(v):
    """Normalize a 3D vector."""
    length = math.sqrt(v[0]**2 + v[1]**2 + v[2]**2)
    if length < 1e-10:
        return (0.0, 0.0, 0.0)
    return (v[0]/length, v[1]/length, v[2]/length)


def normals_equal(n1, n2, tol=0.01):
    """Check if two normals are approximately equal."""
    return (abs(n1[0]-n2[0]) < tol and
            abs(n1[1]-n2[1]) < tol and
            abs(n1[2]-n2[2]) < tol)


# ---------------------------------------------------------------------------
# Triangle -> quad merging
# ---------------------------------------------------------------------------

def merge_triangles_to_quads(faces_with_normals, normals_list):
    """Merge coplanar adjacent triangle pairs into quads.

    Args:
        faces_with_normals: list of (vertex_indices, normal_index)
        normals_list: list of (nx, ny, nz)

    Returns:
        list of (vertex_indices, normal_index) where vertex_indices has
        3 elements (triangle) or 4 elements (quad)
    """
    result = []
    used = set()

    by_normal = defaultdict(list)
    for i, (verts, n_idx) in enumerate(faces_with_normals):
        if len(verts) == 3:
            by_normal[n_idx].append(i)

    for n_idx, tri_indices in by_normal.items():
        for i_pos, i in enumerate(tri_indices):
            if i in used:
                continue
            verts_i = set(faces_with_normals[i][0])

            matched = False
            for j in tri_indices[i_pos+1:]:
                if j in used:
                    continue
                verts_j = set(faces_with_normals[j][0])

                shared = verts_i & verts_j
                if len(shared) == 2:
                    unique_i = (verts_i - shared).pop()
                    unique_j = (verts_j - shared).pop()

                    tri_i_verts = faces_with_normals[i][0]
                    idx_unique = tri_i_verts.index(unique_i)
                    next_v = tri_i_verts[(idx_unique + 1) % 3]
                    prev_v = tri_i_verts[(idx_unique + 2) % 3]

                    quad = [unique_i, next_v, unique_j, prev_v]
                    result.append((quad, n_idx))
                    used.add(i)
                    used.add(j)
                    matched = True
                    break

            if not matched:
                result.append((faces_with_normals[i][0], n_idx))
                used.add(i)

    for i, (verts, n_idx) in enumerate(faces_with_normals):
        if i not in used:
            result.append((verts, n_idx))

    return result


# ---------------------------------------------------------------------------
# Coordinate transforms
# ---------------------------------------------------------------------------

def transform_vertex(v, scale, flip_z=False):
    """Transform OBJ vertex to engine coordinates.

    OBJ: Y-up, Z-forward
    Engine: Y-depth (camera looks along -Y), Z-up
    Transform: engine_x = obj_x, engine_y = obj_z, engine_z = obj_y (or -obj_y)
    """
    x, y, z = v
    z_out = (-y if flip_z else y) * scale
    return (x * scale, z * scale, z_out)


def transform_normal(n, flip_z=False):
    """Transform OBJ normal to engine coordinates (same axis swap, no scale)."""
    x, y, z = n
    ny_out = -y if flip_z else y
    return normalize((x, z, ny_out))


# ---------------------------------------------------------------------------
# Color conversion
# ---------------------------------------------------------------------------

def float_to_gba_color(r, g, b):
    """Convert float RGB (0-1) to GBA 5-bit RGB."""
    return (round(r * 31), round(g * 31), round(b * 31))


# ---------------------------------------------------------------------------
# Formatting
# ---------------------------------------------------------------------------

def format_fixed(value):
    """Format a float as a string suitable for bn::fixed constexpr."""
    if value == int(value):
        return f"{int(value)}.0"
    s = f"{value:.6f}".rstrip('0')
    if s.endswith('.'):
        s += '0'
    return s


# ---------------------------------------------------------------------------
# Face preparation
# ---------------------------------------------------------------------------

def prepare_faces(face_groups, material_order, normals):
    """Merge triangles into quads and filter by palette.

    Args:
        face_groups: list of (material_name, [(vertex_indices, normal_index), ...])
        material_order: list of material names in palette order
        normals: list of (nx, ny, nz)

    Returns:
        list of (material_name, vertex_indices, normal_index)
    """
    all_faces = []
    for mtl_name, faces in face_groups:
        if mtl_name not in material_order:
            continue
        merged = merge_triangles_to_quads(faces, normals)
        for verts, n_idx in merged:
            all_faces.append((mtl_name, verts, n_idx))
    return all_faces


# ---------------------------------------------------------------------------
# Vertex utilities
# ---------------------------------------------------------------------------

def deduplicate_vertices(vertices, used_indices, tolerance=0.001):
    """Collapse vertices at the same position into one index.

    Args:
        vertices: list of (x, y, z) -- full vertex pool
        used_indices: set of vertex indices actually referenced
        tolerance: max distance to consider vertices identical

    Returns:
        (unique_verts, remap) where
            unique_verts: list of (x, y, z) -- deduplicated
            remap: dict mapping old index -> new index
    """
    sorted_used = sorted(used_indices)
    unique_verts = []
    remap = {}

    for old_idx in sorted_used:
        v = vertices[old_idx]
        found = False
        for new_idx, uv in enumerate(unique_verts):
            if (abs(v[0] - uv[0]) < tolerance and
                abs(v[1] - uv[1]) < tolerance and
                abs(v[2] - uv[2]) < tolerance):
                remap[old_idx] = new_idx
                found = True
                break
        if not found:
            remap[old_idx] = len(unique_verts)
            unique_verts.append(v)

    return unique_verts, remap


def center_vertices(vertices, used_indices):
    """Compute centroid of used vertices and return offset to subtract.

    Args:
        vertices: list of (x, y, z)
        used_indices: set of vertex indices

    Returns:
        (cx, cy, cz) centroid to subtract from each vertex
    """
    if not used_indices:
        return (0.0, 0.0, 0.0)
    used = sorted(used_indices)
    cx = sum(vertices[i][0] for i in used) / len(used)
    cy = sum(vertices[i][1] for i in used) / len(used)
    cz = sum(vertices[i][2] for i in used) / len(used)
    return (cx, cy, cz)


# ---------------------------------------------------------------------------
# C++ code generation helpers
# ---------------------------------------------------------------------------

def generate_color_lines(name, material_order, materials_rgb):
    """Generate color array C++ lines (indented, no namespace/guards).

    Returns list of C++ lines for the constexpr color array declaration.
    """
    lines = []
    lines.append(f"    constexpr inline bn::color {name}_model_colors[] = {{")
    for i, mtl_name in enumerate(material_order):
        r, g, b = materials_rgb[mtl_name]
        r5, g5, b5 = float_to_gba_color(r, g, b)
        comma = "," if i < len(material_order) - 1 else ""
        lines.append(f"        bn::color({r5}, {g5}, {b5}){comma}  // {i}  {mtl_name}")
    lines.append("    };")
    return lines


def generate_vertex_lines(name, vertices, scale, flip_z=False):
    """Generate vertex array C++ lines.

    Args:
        name: array name prefix (e.g. 'room' for room_vertices[])
        vertices: list of (x,y,z) in OBJ coordinates
        scale: coordinate scale
        flip_z: negate OBJ Y

    Returns list of indented C++ lines.
    """
    transformed = [transform_vertex(v, scale, flip_z=flip_z) for v in vertices]
    lines = []
    lines.append(f"    constexpr inline fr::vertex_3d {name}_vertices[] = {{")
    for i, (x, y, z) in enumerate(transformed):
        comma = "," if i < len(transformed) - 1 else ""
        lines.append(f"        fr::vertex_3d({format_fixed(x)}, {format_fixed(y)}, {format_fixed(z)}){comma}  // {i}")
    lines.append("    };")
    return lines


def generate_face_lines(name, vertices_name, faces, normals_list, material_order,
                        flip_z=False):
    """Generate face array C++ lines.

    Args:
        name: array name prefix (e.g. 'room_0' for room_0_faces[])
        vertices_name: name of vertex array to reference (e.g. 'room')
        faces: list of (material_name, vertex_indices, normal_index)
        normals_list: list of (nx, ny, nz)
        material_order: list of material names in palette order
        flip_z: affects winding order

    Returns list of indented C++ lines.
    """
    transformed_normals = {}
    for n_idx, n in enumerate(normals_list):
        transformed_normals[n_idx] = transform_normal(n, flip_z=flip_z)

    lines = []
    lines.append(f"    constexpr inline fr::face_3d {name}_faces[] = {{")
    for i, (mtl_name, verts, n_idx) in enumerate(faces):
        color_idx = material_order.index(mtl_name)
        normal = transformed_normals.get(n_idx, (0, 0, 1))
        nx, ny, nz = normal
        comma = "," if i < len(faces) - 1 else ""

        # Y<->Z swap flips handedness -> reverse winding.
        # flip_z adds a second reflection -> keep original winding.
        if flip_z:
            fv = list(verts)
        else:
            fv = list(reversed(verts))

        nstr = f"fr::vertex_3d({format_fixed(nx)}, {format_fixed(ny)}, {format_fixed(nz)})"
        if len(fv) == 4:
            lines.append(
                f"        fr::face_3d({vertices_name}_vertices, {nstr}, "
                f"{fv[0]}, {fv[1]}, {fv[2]}, {fv[3]}, {color_idx}, -1){comma}"
            )
        else:
            lines.append(
                f"        fr::face_3d({vertices_name}_vertices, {nstr}, "
                f"{fv[0]}, {fv[1]}, {fv[2]}, {color_idx}, -1){comma}"
            )
    lines.append("    };")
    return lines


def generate_model_lines(name, vertices, faces, normals_list, material_order,
                         scale, flip_z=False):
    """Generate vertex array + face array + model_3d_item C++ lines.

    For standalone models (not shared-vertex). Used by obj_to_header.py.
    Returns list of indented C++ lines.
    """
    lines = []
    lines.extend(generate_vertex_lines(name, vertices, scale, flip_z))
    lines.append("")
    lines.extend(generate_face_lines(name, name, faces, normals_list, material_order, flip_z))
    lines.append("")
    lines.append(f"    constexpr inline fr::model_3d_item {name}({name}_vertices, {name}_faces);")
    return lines


def generate_header_wrapper(guard, namespace, body_lines, emit_colors=True):
    """Wrap model/color lines in header guards and namespace.

    Returns complete header string.
    """
    lines = []
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append('#include "fr_model_3d_item.h"')
    if emit_colors:
        lines.append('#include "bn_color.h"')
    lines.append("")
    lines.append(f"namespace {namespace}::model_3d_items")
    lines.append("{")
    lines.extend(body_lines)
    lines.append("}")
    lines.append("")
    lines.append("#endif")
    lines.append("")
    return "\n".join(lines)
