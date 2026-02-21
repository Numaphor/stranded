#ifndef STR_MODEL_3D_ITEMS_BUILDING_H
#define STR_MODEL_3D_ITEMS_BUILDING_H

#include "fr_model_3d_item.h"
#include "bn_color.h"

namespace str::model_3d_items
{
    // 6 floor colors + 2 wall colors = 8 total (max 10)
    constexpr inline bn::color building_model_colors[] = {
        bn::color(22, 16, 8),   // 0: Room 0 floor - warm brown
        bn::color(12, 14, 22),  // 1: Room 1 floor - blue-gray
        bn::color(10, 20, 10),  // 2: Room 2 floor - green
        bn::color(22, 10, 8),   // 3: Room 3 floor - terra cotta
        bn::color(16, 10, 20),  // 4: Room 4 floor - purple
        bn::color(8, 18, 20),   // 5: Room 5 floor - teal
        bn::color(28, 26, 22),  // 6: outer wall - off-white
        bn::color(24, 22, 18)   // 7: interior wall - beige
    };

    // Building layout: 2 columns x 3 rows of rooms
    //
    //        x=-60    x=0     x=60
    // y=-90  +--------+--------+
    //        | Room 0 | Room 1 |
    // y=-30  +---||---+---||---+
    //        | Room 2 | Room 3 |
    // y= 30  +---||---+---||---+
    //        | Room 4 | Room 5 |
    // y= 90  +--------+--------+
    //
    // Each room is 60x60 units. Wall height is 35 units.
    // Doors (width 20) connect all adjacent rooms.

    constexpr inline fr::vertex_3d building_vertices[] = {
        // Floor grid (z=0), 3 columns x 4 rows = 12 vertices
        fr::vertex_3d(-60.0, -90.0, 0.0),   //  0
        fr::vertex_3d(  0.0, -90.0, 0.0),   //  1
        fr::vertex_3d( 60.0, -90.0, 0.0),   //  2
        fr::vertex_3d(-60.0, -30.0, 0.0),   //  3
        fr::vertex_3d(  0.0, -30.0, 0.0),   //  4
        fr::vertex_3d( 60.0, -30.0, 0.0),   //  5
        fr::vertex_3d(-60.0,  30.0, 0.0),   //  6
        fr::vertex_3d(  0.0,  30.0, 0.0),   //  7
        fr::vertex_3d( 60.0,  30.0, 0.0),   //  8
        fr::vertex_3d(-60.0,  90.0, 0.0),   //  9
        fr::vertex_3d(  0.0,  90.0, 0.0),   // 10
        fr::vertex_3d( 60.0,  90.0, 0.0),   // 11

        // Wall top grid (z=-35), 3 columns x 4 rows = 12 vertices
        fr::vertex_3d(-60.0, -90.0, -35.0),  // 12
        fr::vertex_3d(  0.0, -90.0, -35.0),  // 13
        fr::vertex_3d( 60.0, -90.0, -35.0),  // 14
        fr::vertex_3d(-60.0, -30.0, -35.0),  // 15
        fr::vertex_3d(  0.0, -30.0, -35.0),  // 16
        fr::vertex_3d( 60.0, -30.0, -35.0),  // 17
        fr::vertex_3d(-60.0,  30.0, -35.0),  // 18
        fr::vertex_3d(  0.0,  30.0, -35.0),  // 19
        fr::vertex_3d( 60.0,  30.0, -35.0),  // 20
        fr::vertex_3d(-60.0,  90.0, -35.0),  // 21
        fr::vertex_3d(  0.0,  90.0, -35.0),  // 22
        fr::vertex_3d( 60.0,  90.0, -35.0),  // 23

        // Center wall (x=0) door edges at z=0
        fr::vertex_3d(0.0, -70.0, 0.0),      // 24: door 0-1 edge
        fr::vertex_3d(0.0, -50.0, 0.0),      // 25: door 0-1 edge
        fr::vertex_3d(0.0, -10.0, 0.0),      // 26: door 2-3 edge
        fr::vertex_3d(0.0,  10.0, 0.0),      // 27: door 2-3 edge
        fr::vertex_3d(0.0,  50.0, 0.0),      // 28: door 4-5 edge
        fr::vertex_3d(0.0,  70.0, 0.0),      // 29: door 4-5 edge

        // y=-30 wall door edges at z=0
        fr::vertex_3d(-40.0, -30.0, 0.0),    // 30: door 0-2 edge
        fr::vertex_3d(-20.0, -30.0, 0.0),    // 31: door 0-2 edge
        fr::vertex_3d( 20.0, -30.0, 0.0),    // 32: door 1-3 edge
        fr::vertex_3d( 40.0, -30.0, 0.0),    // 33: door 1-3 edge

        // y=30 wall door edges at z=0
        fr::vertex_3d(-40.0,  30.0, 0.0),    // 34: door 2-4 edge
        fr::vertex_3d(-20.0,  30.0, 0.0),    // 35: door 2-4 edge
        fr::vertex_3d( 20.0,  30.0, 0.0),    // 36: door 3-5 edge
        fr::vertex_3d( 40.0,  30.0, 0.0),    // 37: door 3-5 edge

        // Center wall door edges at z=-35
        fr::vertex_3d(0.0, -70.0, -35.0),    // 38
        fr::vertex_3d(0.0, -50.0, -35.0),    // 39
        fr::vertex_3d(0.0, -10.0, -35.0),    // 40
        fr::vertex_3d(0.0,  10.0, -35.0),    // 41
        fr::vertex_3d(0.0,  50.0, -35.0),    // 42
        fr::vertex_3d(0.0,  70.0, -35.0),    // 43

        // y=-30 wall door edges at z=-35
        fr::vertex_3d(-40.0, -30.0, -35.0),  // 44
        fr::vertex_3d(-20.0, -30.0, -35.0),  // 45
        fr::vertex_3d( 20.0, -30.0, -35.0),  // 46
        fr::vertex_3d( 40.0, -30.0, -35.0),  // 47

        // y=30 wall door edges at z=-35
        fr::vertex_3d(-40.0,  30.0, -35.0),  // 48
        fr::vertex_3d(-20.0,  30.0, -35.0),  // 49
        fr::vertex_3d( 20.0,  30.0, -35.0),  // 50
        fr::vertex_3d( 40.0,  30.0, -35.0)   // 51
    };

    constexpr inline fr::face_3d building_faces[] = {
        // === FLOORS (6 faces) ===
        // Normal (0,0,-1) = floor visible from above
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 4, 1, 0, 0, -1),      // Room 0
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 5, 2, 1, 1, -1),      // Room 1
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 6, 7, 4, 3, 2, -1),      // Room 2
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 8, 5, 4, 3, -1),      // Room 3
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 9, 10, 7, 6, 4, -1),     // Room 4
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 10, 11, 8, 7, 5, -1),    // Room 5

        // === OUTER WALLS (4 faces) ===
        // North wall (y=-90), faces into building
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 0, 2, 14, 12, 6, -1),
        // East wall (x=60), faces into building
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 2, 11, 23, 14, 6, -1),
        // South wall (y=90), faces into building
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 11, 23, 21, 6, -1),
        // West wall (x=-60), faces into building
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 12, 21, 9, 0, 6, -1),

        // === CENTER WALL (x=0) - 6 sections x 2 sides = 12 faces ===
        // Section y=[-90, -70]
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 1, 24, 38, 13, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 13, 38, 24, 1, 7, -1),
        // Section y=[-50, -30]
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 25, 4, 16, 39, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 39, 16, 4, 25, 7, -1),
        // Section y=[-30, -10]
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 4, 26, 40, 16, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 16, 40, 26, 4, 7, -1),
        // Section y=[10, 30]
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 27, 7, 19, 41, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 41, 19, 7, 27, 7, -1),
        // Section y=[30, 50]
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 28, 42, 19, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 19, 42, 28, 7, 7, -1),
        // Section y=[70, 90]
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 29, 10, 22, 43, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 43, 22, 10, 29, 7, -1),

        // === y=-30 WALL - 4 sections x 2 sides = 8 faces ===
        // Section x=[-60, -40]
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 30, 44, 15, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 30, 3, 15, 44, 7, -1),
        // Section x=[-20, 0]
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 31, 4, 16, 45, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 4, 31, 45, 16, 7, -1),
        // Section x=[0, 20]
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 32, 46, 16, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 4, 16, 46, 7, -1),
        // Section x=[40, 60]
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 33, 5, 17, 47, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 5, 33, 47, 17, 7, -1),

        // === y=30 WALL - 4 sections x 2 sides = 8 faces ===
        // Section x=[-60, -40]
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 6, 34, 48, 18, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 34, 6, 18, 48, 7, -1),
        // Section x=[-20, 0]
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 35, 7, 19, 49, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 7, 35, 49, 19, 7, -1),
        // Section x=[0, 20]
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 7, 36, 50, 19, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 36, 7, 19, 50, 7, -1),
        // Section x=[40, 60]
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 37, 8, 20, 51, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 8, 37, 51, 20, 7, -1)
    };

    constexpr inline fr::model_3d_item building(building_vertices, building_faces);
}

#endif
