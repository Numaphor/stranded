#!/usr/bin/env python3
"""Convert OBJ + MTL files to Butano engine C++ header format.

Reads a Wavefront OBJ file and its material library, merges coplanar
adjacent triangle pairs into quads, and outputs a constexpr C++ header
compatible with varooom-3d's fr::model_3d_item format.

Usage:
    python obj_to_header.py <input.obj> <input.mtl> <output.h> [--scale N] [--name NAME]
"""

import sys
import os
import argparse

# Allow importing model_utils from same directory
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from model_utils import (
    parse_mtl, parse_obj, prepare_faces,
    generate_model_lines, generate_color_lines, generate_header_wrapper,
)


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
    parser.add_argument('--flip-z', action='store_true',
                        help='Negate OBJ Y in transform (engine Z) so model renders right-side up when engine Z is down')
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
    total_tris = sum(len(faces) for _, faces in face_groups)
    all_faces = prepare_faces(face_groups, material_order, normals)

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
    ns_upper = args.namespace.upper()
    guard = f"{ns_upper}_MODEL_3D_ITEMS_{args.name.upper()}_H"
    emit_colors = not args.no_colors

    body_lines = []
    if emit_colors:
        body_lines.extend(generate_color_lines(args.name, material_order, materials_rgb))
        body_lines.append("")
    body_lines.extend(generate_model_lines(args.name, vertices, all_faces, normals,
                                           material_order, args.scale, args.flip_z))

    header = generate_header_wrapper(guard, args.namespace, body_lines, emit_colors)

    # Write output
    os.makedirs(os.path.dirname(args.output) or '.', exist_ok=True)
    with open(args.output, 'w') as f:
        f.write(header)

    print(f"Written to {args.output}")


if __name__ == '__main__':
    main()
