from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class RoomSpec:
    room_id: int
    name: str
    half_w: float
    half_d: float


HEADER_PATH = Path(__file__).resolve().parents[1] / "include" / "models" / "str_model_3d_items_room.h"

ROOM_MODEL_COLORS = [
    ("floor_light_a", (28, 20, 10)),
    ("floor_light_b", (27, 19, 10)),
    ("floor_mid_a", (26, 18, 9)),
    ("floor_mid_b", (25, 18, 9)),
    ("floor_shadow_a", (24, 17, 8)),
    ("floor_shadow_b", (23, 16, 8)),
    ("wall", (28, 22, 13)),
    ("wainscot", (19, 12, 6)),
    ("trim", (12, 8, 4)),
    ("unused", (18, 12, 6)),
]

ROOM_SPECS = [
    RoomSpec(0, "room_0", 60.0, 60.0),
    RoomSpec(1, "room_1", 75.0, 75.0),
    RoomSpec(2, "room_2", 60.0, 60.0),
    RoomSpec(3, "room_3", 60.0, 60.0),
    RoomSpec(4, "room_4", 60.0, 60.0),
    RoomSpec(5, "room_5", 75.0, 75.0),
]

ROOM_CENTER_X = (-60.0, 75.0, -60.0, 60.0, -60.0, 75.0)
ROOM_CENTER_Y = (-120.0, -135.0, 0.0, 0.0, 120.0, 135.0)
DOOR_HALF_WIDTH = 10.0
DOOR_FRAME_SIDE_WIDTH = 2.0
DOOR_FRAME_TOP_HEIGHT = 2.0
DOOR_TOP = -36.0


class MeshBuilder:
    def __init__(self):
        self.vertices: list[tuple[float, float, float]] = []
        self.vertex_ids: dict[tuple[float, float, float], int] = {}
        self.faces: list[tuple[str, int, int, int, int, int]] = []

    def add_vertex(self, x: float, y: float, z: float) -> int:
        key = (round(x, 4), round(y, 4), round(z, 4))
        index = self.vertex_ids.get(key)
        if index is None:
            index = len(self.vertices)
            self.vertex_ids[key] = index
            self.vertices.append((x, y, z))
        return index

    def add_quad(self, p0, p1, p2, p3, color: int, normal: tuple[float, float, float]):
        i0 = self.add_vertex(*p0)
        i1 = self.add_vertex(*p1)
        i2 = self.add_vertex(*p2)
        i3 = self.add_vertex(*p3)
        self.faces.append((normal_to_string(normal), i0, i1, i2, i3, color))


def normal_to_string(normal: tuple[float, float, float]) -> str:
    x, y, z = normal
    return f"fr::vertex_3d({x:.1f}, {y:.1f}, {z:.1f})"


def interval_segments(bounds: float, openings: list[tuple[float, float]]) -> list[tuple[float, float]]:
    positions = [-bounds]
    for start, end in sorted(openings):
        positions.extend([start, end])
    positions.append(bounds)

    segments = []
    for left, right in zip(positions, positions[1:]):
        if right - left > 0.01:
            if any(left >= start and right <= end for start, end in openings):
                continue
            segments.append((left, right))
    return segments


def room_col(room_id: int) -> int:
    return room_id % 2


def room_row(room_id: int) -> int:
    return room_id // 2


def room_id_from_grid(col: int, row: int) -> int:
    return row * 2 + col


def neighbor_room_for_door(room_id: int, direction: str) -> int:
    col = room_col(room_id)
    row = room_row(room_id)

    if direction == "east":
        return room_id_from_grid(col + 1, row) if col < 1 else -1
    if direction == "west":
        return room_id_from_grid(col - 1, row) if col > 0 else -1
    if direction == "south":
        return room_id_from_grid(col, row + 1) if row < 2 else -1
    if direction == "north":
        return room_id_from_grid(col, row - 1) if row > 0 else -1

    raise ValueError(direction)


def aligned_door_center_offset(room_id: int, direction: str) -> float:
    neighbor_room = neighbor_room_for_door(room_id, direction)

    if neighbor_room < 0:
        return 0.0

    if direction in ("east", "west"):
        shared_world_y = (ROOM_CENTER_Y[room_id] + ROOM_CENTER_Y[neighbor_room]) / 2.0
        return shared_world_y - ROOM_CENTER_Y[room_id]

    shared_world_x = (ROOM_CENTER_X[room_id] + ROOM_CENTER_X[neighbor_room]) / 2.0
    return shared_world_x - ROOM_CENTER_X[room_id]


def room_openings(spec: RoomSpec) -> dict[str, list[tuple[float, float]]]:
    openings: dict[str, list[tuple[float, float]]] = {}

    for direction in ("north", "south", "east", "west"):
        if neighbor_room_for_door(spec.room_id, direction) < 0:
            continue

        center = aligned_door_center_offset(spec.room_id, direction)
        openings[direction] = [(center - DOOR_HALF_WIDTH, center + DOOR_HALF_WIDTH)]

    return openings


def build_room(spec: RoomSpec) -> MeshBuilder:
    mesh = MeshBuilder()
    wall_top = -50.0
    trim_top = -16.0
    wainscot_top = -14.0
    openings_by_side = room_openings(spec)
    floor_tiles = 7 if spec.half_w <= 60.0 else 8
    tile_w = (spec.half_w * 2.0) / floor_tiles
    tile_d = (spec.half_d * 2.0) / floor_tiles

    for row in range(floor_tiles):
        y0 = spec.half_d - row * tile_d
        y1 = y0 - tile_d

        for col in range(floor_tiles):
            x0 = -spec.half_w + col * tile_w
            x1 = x0 + tile_w
            floor_color = (row * 2 + col * 3) % 6
            mesh.add_quad(
                (x0, y0, 0.0),
                (x1, y0, 0.0),
                (x1, y1, 0.0),
                (x0, y1, 0.0),
                floor_color,
                (0.0, 0.0, -1.0),
            )

    def add_wall_quad(start: float, end: float, z_bottom: float, z_top: float, color: int,
                      axis: str, side: float, normal: tuple[float, float, float]):
        if axis == "x":
            mesh.add_quad(
                (side, start, z_bottom),
                (side, end, z_bottom),
                (side, end, z_top),
                (side, start, z_top),
                color,
                normal,
            )
        else:
            mesh.add_quad(
                (start, side, z_bottom),
                (end, side, z_bottom),
                (end, side, z_top),
                (start, side, z_top),
                color,
                normal,
            )

    def add_wall_side(axis: str, side: float, openings: list[tuple[float, float]], normal: tuple[float, float, float]):
        segments = interval_segments(spec.half_w if axis == "x" else spec.half_d, openings)

        for start, end in segments:
            add_wall_quad(start, end, 0.0, wainscot_top, 7, axis, side, normal)
            add_wall_quad(start, end, wainscot_top, trim_top, 8, axis, side, normal)
            add_wall_quad(start, end, trim_top, wall_top, 6, axis, side, normal)

    def add_opening_frame(axis: str, side: float, opening: tuple[float, float], normal: tuple[float, float, float]):
        start, end = opening
        frame_bottom = 0.0
        door_trim_top = DOOR_TOP - DOOR_FRAME_TOP_HEIGHT

        add_wall_quad(start, end, door_trim_top, wall_top, 6, axis, side, normal)
        add_wall_quad(start, end, DOOR_TOP, door_trim_top, 8, axis, side, normal)
        add_wall_quad(start, start + DOOR_FRAME_SIDE_WIDTH, frame_bottom, DOOR_TOP, 8, axis, side, normal)
        add_wall_quad(end - DOOR_FRAME_SIDE_WIDTH, end, frame_bottom, DOOR_TOP, 8, axis, side, normal)

    side_map = {
        "north": ("y", -spec.half_d, (0.0, 1.0, 0.0)),
        "south": ("y", spec.half_d, (0.0, -1.0, 0.0)),
        "east": ("x", spec.half_w, (-1.0, 0.0, 0.0)),
        "west": ("x", -spec.half_w, (1.0, 0.0, 0.0)),
    }

    for side_name, (axis, side, normal) in side_map.items():
        openings = openings_by_side.get(side_name, [])
        add_wall_side(axis, side, openings, normal)
        for opening in openings:
            add_opening_frame(axis, side, opening, normal)

    return mesh


def write_header():
    lines = []
    lines.append("#ifndef STR_MODEL_3D_ITEMS_ROOM_H")
    lines.append("#define STR_MODEL_3D_ITEMS_ROOM_H")
    lines.append("")
    lines.append('#include "fr_model_3d_item.h"')
    lines.append('#include "bn_color.h"')
    lines.append("")
    lines.append("namespace str::model_3d_items")
    lines.append("{")
    lines.append("    constexpr inline bn::color room_model_colors[] = {")
    for index, (name, rgb) in enumerate(ROOM_MODEL_COLORS):
        suffix = "," if index < len(ROOM_MODEL_COLORS) - 1 else ""
        lines.append(f"        bn::color({rgb[0]}, {rgb[1]}, {rgb[2]}){suffix}  // {index}  {name}")
    lines.append("    };")
    lines.append("")

    for room in ROOM_SPECS:
        mesh = build_room(room)
        lines.append(f"    constexpr inline fr::vertex_3d {room.name}_vertices[] = {{")
        for index, (x, y, z) in enumerate(mesh.vertices):
            comma = "," if index < len(mesh.vertices) - 1 else ""
            lines.append(f"        fr::vertex_3d({x:.1f}, {y:.1f}, {z:.1f}){comma}  // {index}")
        lines.append("    };")
        lines.append("")
        lines.append(f"    constexpr inline fr::face_3d {room.name}_faces[] = {{")
        for index, (normal, i0, i1, i2, i3, color) in enumerate(mesh.faces):
            comma = "," if index < len(mesh.faces) - 1 else ""
            lines.append(
                f"        fr::face_3d({room.name}_vertices, {normal}, {i0}, {i1}, {i2}, {i3}, {color}, -1){comma}"
            )
        lines.append("    };")
        lines.append(f"    constexpr inline fr::model_3d_item {room.name}({room.name}_vertices, {room.name}_faces);")
        lines.append("")

    lines.append("    constexpr inline fr::model_3d_item room(room_0_vertices, room_0_faces);")
    lines.append("}")
    lines.append("")
    lines.append("#endif")
    lines.append("")

    HEADER_PATH.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    write_header()
