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

    // Shared vertex array for all room variants.
    // Contains vertices for floor tiles, plus both solid-wall and door-wall
    // geometry for each of the four walls.
    //
    // Building layout (2 columns × 3 rows):
    //   R0(0,0) R1(1,0)    R0: east+south doors
    //   R2(0,1) R3(1,1)    R1: west+south doors
    //   R4(0,2) R5(1,2)    R2: east+north+south doors
    //                      R3: west+north+south doors
    //                      R4: east+north doors
    //                      R5: west+north doors
    //
    // Door openings: 20 units wide (-10 to +10), 35 tall (z: 0 to -35).
    // Lintel above each door: z from -35 to -50.
    constexpr inline fr::vertex_3d room_vertices[] = {
        // --- Floor tiles (3 strips) ---
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

        // --- North wall (y=-60, inward normal 0,1,0) ---
        // Solid variant
        fr::vertex_3d(-60.0, -60.0,   0.0),  // 12
        fr::vertex_3d( 60.0, -60.0,   0.0),  // 13
        fr::vertex_3d( 60.0, -60.0, -50.0),  // 14
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 15
        // Door variant: left section
        fr::vertex_3d(-60.0, -60.0,   0.0),  // 16
        fr::vertex_3d(-10.0, -60.0,   0.0),  // 17
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 18
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 19
        // Door variant: right section
        fr::vertex_3d( 10.0, -60.0,   0.0),  // 20
        fr::vertex_3d( 60.0, -60.0,   0.0),  // 21
        fr::vertex_3d( 60.0, -60.0, -50.0),  // 22
        fr::vertex_3d( 10.0, -60.0, -50.0),  // 23
        // Door variant: lintel
        fr::vertex_3d(-10.0, -60.0, -35.0),  // 24
        fr::vertex_3d( 10.0, -60.0, -35.0),  // 25
        fr::vertex_3d( 10.0, -60.0, -50.0),  // 26
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 27

        // --- South wall (y=60, inward normal 0,-1,0) ---
        // Solid variant
        fr::vertex_3d(-60.0,  60.0,   0.0),  // 28
        fr::vertex_3d( 60.0,  60.0,   0.0),  // 29
        fr::vertex_3d( 60.0,  60.0, -50.0),  // 30
        fr::vertex_3d(-60.0,  60.0, -50.0),  // 31
        // Door variant: left section
        fr::vertex_3d(-60.0,  60.0,   0.0),  // 32
        fr::vertex_3d(-10.0,  60.0,   0.0),  // 33
        fr::vertex_3d(-10.0,  60.0, -50.0),  // 34
        fr::vertex_3d(-60.0,  60.0, -50.0),  // 35
        // Door variant: right section
        fr::vertex_3d( 10.0,  60.0,   0.0),  // 36
        fr::vertex_3d( 60.0,  60.0,   0.0),  // 37
        fr::vertex_3d( 60.0,  60.0, -50.0),  // 38
        fr::vertex_3d( 10.0,  60.0, -50.0),  // 39
        // Door variant: lintel
        fr::vertex_3d(-10.0,  60.0, -35.0),  // 40
        fr::vertex_3d( 10.0,  60.0, -35.0),  // 41
        fr::vertex_3d( 10.0,  60.0, -50.0),  // 42
        fr::vertex_3d(-10.0,  60.0, -50.0),  // 43

        // --- West wall (x=-60, inward normal 1,0,0) ---
        // Solid variant
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 44
        fr::vertex_3d(-60.0,  60.0, -50.0),  // 45
        fr::vertex_3d(-60.0,  60.0,   0.0),  // 46
        fr::vertex_3d(-60.0, -60.0,   0.0),  // 47
        // Door variant: left section
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 48
        fr::vertex_3d(-60.0, -10.0, -50.0),  // 49
        fr::vertex_3d(-60.0, -10.0,   0.0),  // 50
        fr::vertex_3d(-60.0, -60.0,   0.0),  // 51
        // Door variant: right section
        fr::vertex_3d(-60.0,  10.0, -50.0),  // 52
        fr::vertex_3d(-60.0,  60.0, -50.0),  // 53
        fr::vertex_3d(-60.0,  60.0,   0.0),  // 54
        fr::vertex_3d(-60.0,  10.0,   0.0),  // 55
        // Door variant: lintel
        fr::vertex_3d(-60.0, -10.0, -50.0),  // 56
        fr::vertex_3d(-60.0,  10.0, -50.0),  // 57
        fr::vertex_3d(-60.0,  10.0, -35.0),  // 58
        fr::vertex_3d(-60.0, -10.0, -35.0),  // 59

        // --- East wall (x=60, inward normal -1,0,0) ---
        // Solid variant
        fr::vertex_3d( 60.0, -60.0,   0.0),  // 60
        fr::vertex_3d( 60.0,  60.0,   0.0),  // 61
        fr::vertex_3d( 60.0,  60.0, -50.0),  // 62
        fr::vertex_3d( 60.0, -60.0, -50.0),  // 63
        // Door variant: left section
        fr::vertex_3d( 60.0, -60.0,   0.0),  // 64
        fr::vertex_3d( 60.0, -10.0,   0.0),  // 65
        fr::vertex_3d( 60.0, -10.0, -50.0),  // 66
        fr::vertex_3d( 60.0, -60.0, -50.0),  // 67
        // Door variant: right section
        fr::vertex_3d( 60.0,  10.0,   0.0),  // 68
        fr::vertex_3d( 60.0,  60.0,   0.0),  // 69
        fr::vertex_3d( 60.0,  60.0, -50.0),  // 70
        fr::vertex_3d( 60.0,  10.0, -50.0),  // 71
        // Door variant: lintel
        fr::vertex_3d( 60.0, -10.0, -35.0),  // 72
        fr::vertex_3d( 60.0,  10.0, -35.0),  // 73
        fr::vertex_3d( 60.0,  10.0, -50.0),  // 74
        fr::vertex_3d( 60.0, -10.0, -50.0),  // 75
    };

    // ---------------------------------------------------------------
    // Per-room face arrays.
    // Each room selects solid or door walls based on adjacency.
    // ---------------------------------------------------------------

    // Room 0 (col=0, row=0): east door, south door, north solid, west solid
    // Each wall has inward + outward faces for visibility from all camera angles.
    // Outward face: reversed vertex order + flipped normal.
    constexpr inline fr::face_3d room_0_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 1, 0, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 5, 4, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 10, 9, 8, 0, -1),
        // North solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 14, 15, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 15, 14, 13, 12, 2, -1),
        // South door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 33, 34, 35, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 35, 34, 33, 32, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 36, 37, 38, 39, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 39, 38, 37, 36, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 40, 41, 42, 43, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 43, 42, 41, 40, 5, -1),
        // West solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 44, 45, 46, 47, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 47, 46, 45, 44, 2, -1),
        // East door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 64, 65, 66, 67, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 67, 66, 65, 64, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 68, 69, 70, 71, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 71, 70, 69, 68, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 72, 73, 74, 75, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 75, 74, 73, 72, 5, -1),
    };
    constexpr inline fr::model_3d_item room_0(room_vertices, room_0_faces);

    // Room 1 (col=1, row=0): west door, south door, north solid, east solid
    constexpr inline fr::face_3d room_1_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 1, 0, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 5, 4, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 10, 9, 8, 0, -1),
        // North solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 14, 15, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 15, 14, 13, 12, 2, -1),
        // South door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 33, 34, 35, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 35, 34, 33, 32, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 36, 37, 38, 39, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 39, 38, 37, 36, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 40, 41, 42, 43, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 43, 42, 41, 40, 5, -1),
        // West door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 48, 49, 50, 51, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 51, 50, 49, 48, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 52, 53, 54, 55, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 55, 54, 53, 52, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 56, 57, 58, 59, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 59, 58, 57, 56, 5, -1),
        // East solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 60, 61, 62, 63, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 63, 62, 61, 60, 2, -1),
    };
    constexpr inline fr::model_3d_item room_1(room_vertices, room_1_faces);

    // Room 2 (col=0, row=1): east door, north door, south door, west solid
    constexpr inline fr::face_3d room_2_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 1, 0, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 5, 4, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 10, 9, 8, 0, -1),
        // North door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 16, 17, 18, 19, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 19, 18, 17, 16, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 23, 22, 21, 20, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 24, 25, 26, 27, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 27, 26, 25, 24, 5, -1),
        // South door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 33, 34, 35, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 35, 34, 33, 32, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 36, 37, 38, 39, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 39, 38, 37, 36, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 40, 41, 42, 43, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 43, 42, 41, 40, 5, -1),
        // West solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 44, 45, 46, 47, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 47, 46, 45, 44, 2, -1),
        // East door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 64, 65, 66, 67, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 67, 66, 65, 64, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 68, 69, 70, 71, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 71, 70, 69, 68, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 72, 73, 74, 75, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 75, 74, 73, 72, 5, -1),
    };
    constexpr inline fr::model_3d_item room_2(room_vertices, room_2_faces);

    // Room 3 (col=1, row=1): west door, north door, south door, east solid
    constexpr inline fr::face_3d room_3_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 1, 0, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 5, 4, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 10, 9, 8, 0, -1),
        // North door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 16, 17, 18, 19, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 19, 18, 17, 16, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 23, 22, 21, 20, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 24, 25, 26, 27, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 27, 26, 25, 24, 5, -1),
        // South door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 33, 34, 35, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 35, 34, 33, 32, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 36, 37, 38, 39, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 39, 38, 37, 36, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 40, 41, 42, 43, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 43, 42, 41, 40, 5, -1),
        // West door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 48, 49, 50, 51, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 51, 50, 49, 48, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 52, 53, 54, 55, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 55, 54, 53, 52, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 56, 57, 58, 59, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 59, 58, 57, 56, 5, -1),
        // East solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 60, 61, 62, 63, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 63, 62, 61, 60, 2, -1),
    };
    constexpr inline fr::model_3d_item room_3(room_vertices, room_3_faces);

    // Room 4 (col=0, row=2): east door, north door, south solid, west solid
    constexpr inline fr::face_3d room_4_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 1, 0, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 5, 4, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 10, 9, 8, 0, -1),
        // North door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 16, 17, 18, 19, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 19, 18, 17, 16, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 23, 22, 21, 20, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 24, 25, 26, 27, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 27, 26, 25, 24, 5, -1),
        // South solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 28, 29, 30, 31, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 31, 30, 29, 28, 2, -1),
        // West solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 44, 45, 46, 47, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 47, 46, 45, 44, 2, -1),
        // East door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 64, 65, 66, 67, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 67, 66, 65, 64, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 68, 69, 70, 71, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 71, 70, 69, 68, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 72, 73, 74, 75, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 75, 74, 73, 72, 5, -1),
    };
    constexpr inline fr::model_3d_item room_4(room_vertices, room_4_faces);

    // Room 5 (col=1, row=2): west door, north door, south solid, east solid
    constexpr inline fr::face_3d room_5_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 1, 0, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 5, 4, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 10, 9, 8, 0, -1),
        // North door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 16, 17, 18, 19, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 19, 18, 17, 16, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 23, 22, 21, 20, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 24, 25, 26, 27, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 27, 26, 25, 24, 5, -1),
        // South solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 28, 29, 30, 31, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 31, 30, 29, 28, 2, -1),
        // West door (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 48, 49, 50, 51, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 51, 50, 49, 48, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 52, 53, 54, 55, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 55, 54, 53, 52, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 56, 57, 58, 59, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 59, 58, 57, 56, 5, -1),
        // East solid (inward + outward)
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 60, 61, 62, 63, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 63, 62, 61, 60, 2, -1),
    };
    constexpr inline fr::model_3d_item room_5(room_vertices, room_5_faces);

    // For backward compatibility (default = room 0)
    constexpr inline fr::model_3d_item room(room_vertices, room_0_faces);
}

#endif
