#ifndef STR_MODEL_3D_ITEMS_ROOM_H
#define STR_MODEL_3D_ITEMS_ROOM_H

#include "fr_model_3d_item.h"
#include "bn_color.h"

namespace str::model_3d_items
{
    // Color palette indices:
    //   0 = floor_light     1 = floor_dark
    //   2 = wall             3 = wall_shadow (reserved)
    //   4 = (reserved)       5 = door_frame
    //   6 = table_wood       7 = chair_fabric
    //   8 = chair_frame
    constexpr inline bn::color room_model_colors[] = {
        bn::color(22, 16, 8),   // 0  floor_light
        bn::color(16, 10, 4),   // 1  floor_dark
        bn::color(28, 26, 22),  // 2  wall
        bn::color(22, 20, 16),  // 3  wall_shadow (reserved)
        bn::color(16, 22, 28),  // 4  (reserved)
        bn::color(10, 8, 6),    // 5  door_frame
        bn::color(18, 12, 6),   // 6  table_wood
        bn::color(8, 12, 18),   // 7  chair_fabric
        bn::color(20, 14, 8)    // 8  chair_frame
    };

    // Room: 120x120 floor, 50-unit walls, door openings (20 wide, 35 tall) at center of each wall.
    //
    // Door openings: x or y from -10 to +10, z from 0 (floor) to -35.
    // Lintel above each door: z from -35 to -50.
    constexpr inline fr::vertex_3d room_vertices[] = {
        // Floor tiles (3 strips)
        fr::vertex_3d(-60.0, -60.0, 0.0),   // 0
        fr::vertex_3d(-20.0, -60.0, 0.0),   // 1
        fr::vertex_3d(-20.0,  60.0, 0.0),   // 2
        fr::vertex_3d(-60.0,  60.0, 0.0),   // 3
        fr::vertex_3d(-20.0, -60.0, 0.0),   // 4
        fr::vertex_3d( 20.0, -60.0, 0.0),   // 5
        fr::vertex_3d( 20.0,  60.0, 0.0),   // 6
        fr::vertex_3d(-20.0,  60.0, 0.0),   // 7
        fr::vertex_3d( 20.0, -60.0, 0.0),   // 8
        fr::vertex_3d( 60.0, -60.0, 0.0),   // 9
        fr::vertex_3d( 60.0,  60.0, 0.0),   // 10
        fr::vertex_3d( 20.0,  60.0, 0.0),   // 11

        // North wall (y=-60, normal 0,1,0) — left section
        fr::vertex_3d(-60.0, -60.0,   0.0),  // 12
        fr::vertex_3d(-10.0, -60.0,   0.0),  // 13
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 14
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 15
        // North wall — right section
        fr::vertex_3d( 10.0, -60.0,   0.0),  // 16
        fr::vertex_3d( 60.0, -60.0,   0.0),  // 17
        fr::vertex_3d( 60.0, -60.0, -50.0),  // 18
        fr::vertex_3d( 10.0, -60.0, -50.0),  // 19
        // North wall — door lintel
        fr::vertex_3d(-10.0, -60.0, -35.0),  // 20
        fr::vertex_3d( 10.0, -60.0, -35.0),  // 21
        fr::vertex_3d( 10.0, -60.0, -50.0),  // 22
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 23

        // South wall (y=60, normal 0,-1,0) — left section
        fr::vertex_3d(-60.0,  60.0,   0.0),  // 24
        fr::vertex_3d(-10.0,  60.0,   0.0),  // 25
        fr::vertex_3d(-10.0,  60.0, -50.0),  // 26
        fr::vertex_3d(-60.0,  60.0, -50.0),  // 27
        // South wall — right section
        fr::vertex_3d( 10.0,  60.0,   0.0),  // 28
        fr::vertex_3d( 60.0,  60.0,   0.0),  // 29
        fr::vertex_3d( 60.0,  60.0, -50.0),  // 30
        fr::vertex_3d( 10.0,  60.0, -50.0),  // 31
        // South wall — door lintel
        fr::vertex_3d(-10.0,  60.0, -35.0),  // 32
        fr::vertex_3d( 10.0,  60.0, -35.0),  // 33
        fr::vertex_3d( 10.0,  60.0, -50.0),  // 34
        fr::vertex_3d(-10.0,  60.0, -50.0),  // 35

        // West wall (x=-60, normal 1,0,0) — left section (ceiling-first order)
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 36
        fr::vertex_3d(-60.0, -10.0, -50.0),  // 37
        fr::vertex_3d(-60.0, -10.0,   0.0),  // 38
        fr::vertex_3d(-60.0, -60.0,   0.0),  // 39
        // West wall — right section
        fr::vertex_3d(-60.0,  10.0, -50.0),  // 40
        fr::vertex_3d(-60.0,  60.0, -50.0),  // 41
        fr::vertex_3d(-60.0,  60.0,   0.0),  // 42
        fr::vertex_3d(-60.0,  10.0,   0.0),  // 43
        // West wall — door lintel
        fr::vertex_3d(-60.0, -10.0, -50.0),  // 44
        fr::vertex_3d(-60.0,  10.0, -50.0),  // 45
        fr::vertex_3d(-60.0,  10.0, -35.0),  // 46
        fr::vertex_3d(-60.0, -10.0, -35.0),  // 47

        // East wall (x=60, normal -1,0,0) — left section
        fr::vertex_3d( 60.0, -60.0,   0.0),  // 48
        fr::vertex_3d( 60.0, -10.0,   0.0),  // 49
        fr::vertex_3d( 60.0, -10.0, -50.0),  // 50
        fr::vertex_3d( 60.0, -60.0, -50.0),  // 51
        // East wall — right section
        fr::vertex_3d( 60.0,  10.0,   0.0),  // 52
        fr::vertex_3d( 60.0,  60.0,   0.0),  // 53
        fr::vertex_3d( 60.0,  60.0, -50.0),  // 54
        fr::vertex_3d( 60.0,  10.0, -50.0),  // 55
        // East wall — door lintel
        fr::vertex_3d( 60.0, -10.0, -35.0),  // 56
        fr::vertex_3d( 60.0,  10.0, -35.0),  // 57
        fr::vertex_3d( 60.0,  10.0, -50.0),  // 58
        fr::vertex_3d( 60.0, -10.0, -50.0),  // 59
    };

    constexpr inline fr::face_3d room_faces[] = {
        // Floor (3 strips, normal 0,0,-1)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 1, 0, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 5, 4, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 10, 9, 8, 0, -1),

        // North wall (y=-60): left, right, lintel
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 14, 15, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 16, 17, 18, 19, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 5, -1),

        // South wall (y=60): left, right, lintel
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 24, 25, 26, 27, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 28, 29, 30, 31, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 33, 34, 35, 5, -1),

        // West wall (x=-60): left, right, lintel
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 36, 37, 38, 39, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 40, 41, 42, 43, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 44, 45, 46, 47, 5, -1),

        // East wall (x=60): left, right, lintel
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 48, 49, 50, 51, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 52, 53, 54, 55, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 56, 57, 58, 59, 5, -1),
    };

    constexpr inline fr::model_3d_item room(room_vertices, room_faces);
}

#endif
