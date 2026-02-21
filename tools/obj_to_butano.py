#!/usr/bin/env python3
"""Convert OBJ + MTL files to Butano engine C++ header format.

Reads a Wavefront OBJ file and its material library, merges coplanar
adjacent triangle pairs into quads, and outputs a constexpr C++ header
compatible with varooom-3d's fr::model_3d_item format.

Usage:
    python obj_to_butano.py <input.obj> <input.mtl> <output.h> [--scale N] [--name NAME]
"""

import sys
import os
import math
import argparse
from collections import defaultdict


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


def parse_obj(path):
    """Parse OBJ file, return vertices, normals, and face groups."""
    vertices = []   # list of (x, y, z)
    normals = []    # list of (nx, ny, nz)
    face_groups = []  # list of (material_name, [(v_indices, n_index), ...])
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


def merge_triangles_to_quads(faces_with_normals, normals_list):
    """Merge coplanar adjacent triangle pairs into quads.

    For voxel models, each rectangular face is exported as 2 triangles
    sharing a hypotenuse. This function finds those pairs and merges them.

    Args:
        faces_with_normals: list of (vertex_indices, normal_index)
        normals_list: list of (nx, ny, nz)

    Returns:
        list of (vertex_indices, normal_index) where vertex_indices has
        3 elements (triangle) or 4 elements (quad)
    """
    result = []
    used = set()

    # Group triangles by normal index for faster matching
    by_normal = defaultdict(list)
    for i, (verts, n_idx) in enumerate(faces_with_normals):
        if len(verts) == 3:
            by_normal[n_idx].append(i)

    for n_idx, tri_indices in by_normal.items():
        # Try to pair triangles with same normal that share an edge
        for i_pos, i in enumerate(tri_indices):
            if i in used:
                continue
            verts_i = set(faces_with_normals[i][0])

            matched = False
            for j in tri_indices[i_pos+1:]:
                if j in used:
                    continue
                verts_j = set(faces_with_normals[j][0])

                # Two triangles sharing an edge have exactly 2 vertices in common
                shared = verts_i & verts_j
                if len(shared) == 2:
                    # Found a pair! Merge into quad
                    # The unique vertices from each triangle are the opposite corners
                    unique_i = (verts_i - shared).pop()
                    unique_j = (verts_j - shared).pop()
                    shared_list = sorted(shared)

                    # Build quad with correct winding order
                    # For the merged quad: unique_i, shared_a, unique_j, shared_b
                    # We need to maintain consistent winding (CCW)
                    tri_i_verts = faces_with_normals[i][0]
                    tri_j_verts = faces_with_normals[j][0]

                    # Find the winding order from triangle i
                    # Triangle i has vertices in order. Find positions of shared verts
                    # relative to unique_i
                    idx_unique = tri_i_verts.index(unique_i)
                    # The next vertex in winding after unique_i
                    next_v = tri_i_verts[(idx_unique + 1) % 3]
                    prev_v = tri_i_verts[(idx_unique + 2) % 3]

                    # Quad winding: unique_i -> next_v -> unique_j -> prev_v
                    quad = [unique_i, next_v, unique_j, prev_v]
                    result.append((quad, n_idx))
                    used.add(i)
                    used.add(j)
                    matched = True
                    break

            if not matched:
                result.append((faces_with_normals[i][0], n_idx))
                used.add(i)

    # Add any non-triangle faces that weren't processed
    for i, (verts, n_idx) in enumerate(faces_with_normals):
        if i not in used:
            result.append((verts, n_idx))

    return result


def transform_vertex(v, scale):
    """Transform OBJ vertex to engine coordinates.

    OBJ: Y-up, Z-forward
    Engine: Y-depth (camera looks along -Y), Z-up
    Transform: engine_x = obj_x, engine_y = obj_z, engine_z = obj_y
    """
    x, y, z = v
    return (x * scale, z * scale, y * scale)


def transform_normal(n):
    """Transform OBJ normal to engine coordinates (same axis swap, no scale)."""
    x, y, z = n
    return normalize((x, z, y))


def float_to_gba_color(r, g, b):
    """Convert float RGB (0-1) to GBA 5-bit RGB."""
    return (round(r * 31), round(g * 31), round(b * 31))


def format_fixed(value):
    """Format a float as a string suitable for bn::fixed constexpr."""
    # Round to reasonable precision
    if value == int(value):
        return f"{int(value)}.0"
    return f"{value:.6f}".rstrip('0').rstrip('.')
    # Make sure there's always a decimal
    s = f"{value:.6f}".rstrip('0')
    if s.endswith('.'):
        s += '0'
    return s


def generate_header(name, vertices, faces, normals_list, material_order, materials_rgb, scale,
                    emit_colors=True, namespace='str'):
    """Generate C++ header content. namespace is 'str' or 'fr' for guard/namespace."""
    ns_upper = namespace.upper()
    # Transform vertices
    transformed_verts = [transform_vertex(v, scale) for v in vertices]

    # Transform normals
    transformed_normals = {}
    for n_idx, n in enumerate(normals_list):
        transformed_normals[n_idx] = transform_normal(n)

    # Build header guard (STR_MODEL_3D_ITEMS_* or FR_MODEL_3D_ITEMS_*)
    guard = f"{ns_upper}_MODEL_3D_ITEMS_{name.upper()}_H"

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

    if emit_colors:
        lines.append(f"    constexpr inline bn::color {name}_model_colors[] = {{")
        for i, mtl_name in enumerate(material_order):
            r, g, b = materials_rgb[mtl_name]
            r5, g5, b5 = float_to_gba_color(r, g, b)
            comma = "," if i < len(material_order) - 1 else ""
            lines.append(f"        bn::color({r5}, {g5}, {b5}){comma}  // {mtl_name}: rgb({r:.3f}, {g:.3f}, {b:.3f})")
        lines.append("    };")
        lines.append("")

    # Vertices
    lines.append(f"    constexpr inline fr::vertex_3d {name}_vertices[] = {{")
    for i, (x, y, z) in enumerate(transformed_verts):
        comma = "," if i < len(transformed_verts) - 1 else ""
        lines.append(f"        fr::vertex_3d({format_fixed(x)}, {format_fixed(y)}, {format_fixed(z)}){comma}")
    lines.append("    };")
    lines.append("")

    # Faces
    lines.append(f"    constexpr inline fr::face_3d {name}_faces[] = {{")
    for i, (mtl_name, verts, n_idx) in enumerate(faces):
        color_idx = material_order.index(mtl_name)
        normal = transformed_normals.get(n_idx, (0, 0, 1))
        nx, ny, nz = normal
        comma = "," if i < len(faces) - 1 else ""

        # Reverse vertex order: OBJ CCW + axis swap → need CW for engine rasterizer
        if len(verts) == 4:
            rv = list(reversed(verts))
            lines.append(
                f"        fr::face_3d({name}_vertices, fr::vertex_3d({format_fixed(nx)}, {format_fixed(ny)}, {format_fixed(nz)}), "
                f"{rv[0]}, {rv[1]}, {rv[2]}, {rv[3]}, {color_idx}, -1){comma}"
            )
        else:
            rv = list(reversed(verts))
            lines.append(
                f"        fr::face_3d({name}_vertices, fr::vertex_3d({format_fixed(nx)}, {format_fixed(ny)}, {format_fixed(nz)}), "
                f"{rv[0]}, {rv[1]}, {rv[2]}, {color_idx}, -1){comma}"
            )
    lines.append("    };")
    lines.append("")

    # Model item
    lines.append(f"    constexpr inline fr::model_3d_item {name}({name}_vertices, {name}_faces);")

    lines.append("}")
    lines.append("")
    lines.append(f"#endif")
    lines.append("")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description='Convert OBJ+MTL to Butano engine header')
    parser.add_argument('obj', help='Input OBJ file')
    parser.add_argument('mtl', help='Input MTL file')
    parser.add_argument('output', help='Output C++ header file')
    parser.add_argument('--scale', type=float, default=4.0, help='Coordinate scale factor (default: 4.0)')
    parser.add_argument('--name', type=str, default=None, help='Model name (default: derived from filename)')
    parser.add_argument('--palette', type=str, default=None,
                        help='Comma-separated material names defining the shared color palette order. '
                             'Face color_index values will reference this palette.')
    parser.add_argument('--no-colors', action='store_true',
                        help='Do not emit the color array in the header (use when sharing a palette from another header)')
    parser.add_argument('--namespace', type=str, default='str', choices=('str', 'fr'),
                        help='C++ namespace and header guard prefix (default: str)')
    args = parser.parse_args()

    if args.name is None:
        args.name = os.path.splitext(os.path.basename(args.obj))[0].replace(' ', '_').replace('-', '_').lower()

    # Parse input files
    materials_rgb = parse_mtl(args.mtl)
    vertices, normals, face_groups = parse_obj(args.obj)

    print(f"Parsed {len(vertices)} vertices, {len(normals)} normals, {len(materials_rgb)} materials")

    # Establish material order (color indices 0-9)
    if args.palette:
        material_order = [m.strip() for m in args.palette.split(',')]
        print(f"Using shared palette: {material_order}")
    else:
        material_order = []
        for mtl_name, _ in face_groups:
            if mtl_name not in material_order:
                material_order.append(mtl_name)
        print(f"Material order: {material_order}")

    if len(material_order) > 10:
        print(f"WARNING: {len(material_order)} materials exceeds max_colors=10, truncating")
        material_order = material_order[:10]

    # Collect all faces with their material, then merge triangles into quads per-material
    total_tris = 0
    all_faces = []  # (material_name, vertex_indices, normal_index)

    for mtl_name, faces in face_groups:
        if mtl_name not in material_order:
            continue
        total_tris += len(faces)

        # Merge triangles in this material group
        merged = merge_triangles_to_quads(faces, normals)

        for verts, n_idx in merged:
            all_faces.append((mtl_name, verts, n_idx))

    quads = sum(1 for _, v, _ in all_faces if len(v) == 4)
    tris = sum(1 for _, v, _ in all_faces if len(v) == 3)
    print(f"Input: {total_tris} triangles")
    print(f"Output: {len(all_faces)} faces ({quads} quads, {tris} triangles)")
    print(f"Vertices: {len(vertices)}")

    if len(all_faces) > 300:
        print(f"WARNING: {len(all_faces)} faces exceeds engine max_faces=300")
    if len(vertices) > 256:
        print(f"WARNING: {len(vertices)} vertices exceeds engine max_vertices=256")

    # Generate header
    header = generate_header(args.name, vertices, all_faces, normals, material_order, materials_rgb, args.scale,
                             emit_colors=not args.no_colors, namespace=args.namespace)

    # Write output
    os.makedirs(os.path.dirname(args.output) or '.', exist_ok=True)
    with open(args.output, 'w') as f:
        f.write(header)

    print(f"Written to {args.output}")


if __name__ == '__main__':
    main()
