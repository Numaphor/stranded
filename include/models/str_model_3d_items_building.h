#ifndef STR_MODEL_3D_ITEMS_BUILDING_H
#define STR_MODEL_3D_ITEMS_BUILDING_H

#include "fr_model_3d_item.h"
#include "bn_color.h"

namespace str::model_3d_items
{
    constexpr inline bn::color building_model_colors[] = {
        bn::color(22, 16, 8),  // 0  room0_floor
        bn::color(12, 14, 22),  // 1  room1_floor
        bn::color(10, 20, 10),  // 2  room2_floor
        bn::color(22, 10, 8),  // 3  room3_floor
        bn::color(16, 10, 20),  // 4  room4_floor
        bn::color(8, 18, 20),  // 5  room5_floor
        bn::color(28, 26, 22),  // 6  outer_wall
        bn::color(24, 22, 18)  // 7  interior_wall
    };

    constexpr inline fr::vertex_3d building_vertices[] = {
        fr::vertex_3d(-120.0, -180.0, 0.0),  // 0
        fr::vertex_3d(0.0, -180.0, 0.0),  // 1
        fr::vertex_3d(120.0, -180.0, 0.0),  // 2
        fr::vertex_3d(-120.0, -60.0, 0.0),  // 3
        fr::vertex_3d(0.0, -60.0, 0.0),  // 4
        fr::vertex_3d(120.0, -60.0, 0.0),  // 5
        fr::vertex_3d(-120.0, 60.0, 0.0),  // 6
        fr::vertex_3d(0.0, 60.0, 0.0),  // 7
        fr::vertex_3d(120.0, 60.0, 0.0),  // 8
        fr::vertex_3d(-120.0, 180.0, 0.0),  // 9
        fr::vertex_3d(0.0, 180.0, 0.0),  // 10
        fr::vertex_3d(120.0, 180.0, 0.0),  // 11
        fr::vertex_3d(-120.0, -180.0, -50.0),  // 12
        fr::vertex_3d(0.0, -180.0, -50.0),  // 13
        fr::vertex_3d(120.0, -180.0, -50.0),  // 14
        fr::vertex_3d(-120.0, -60.0, -50.0),  // 15
        fr::vertex_3d(0.0, -60.0, -50.0),  // 16
        fr::vertex_3d(120.0, -60.0, -50.0),  // 17
        fr::vertex_3d(-120.0, 60.0, -50.0),  // 18
        fr::vertex_3d(0.0, 60.0, -50.0),  // 19
        fr::vertex_3d(120.0, 60.0, -50.0),  // 20
        fr::vertex_3d(-120.0, 180.0, -50.0),  // 21
        fr::vertex_3d(0.0, 180.0, -50.0),  // 22
        fr::vertex_3d(120.0, 180.0, -50.0),  // 23
        fr::vertex_3d(0.0, -130.0, 0.0),  // 24
        fr::vertex_3d(0.0, -110.0, 0.0),  // 25
        fr::vertex_3d(0.0, -10.0, 0.0),  // 26
        fr::vertex_3d(0.0, 10.0, 0.0),  // 27
        fr::vertex_3d(0.0, 110.0, 0.0),  // 28
        fr::vertex_3d(0.0, 130.0, 0.0),  // 29
        fr::vertex_3d(-70.0, -60.0, 0.0),  // 30
        fr::vertex_3d(-50.0, -60.0, 0.0),  // 31
        fr::vertex_3d(50.0, -60.0, 0.0),  // 32
        fr::vertex_3d(70.0, -60.0, 0.0),  // 33
        fr::vertex_3d(-70.0, 60.0, 0.0),  // 34
        fr::vertex_3d(-50.0, 60.0, 0.0),  // 35
        fr::vertex_3d(50.0, 60.0, 0.0),  // 36
        fr::vertex_3d(70.0, 60.0, 0.0),  // 37
        fr::vertex_3d(0.0, -130.0, -50.0),  // 38
        fr::vertex_3d(0.0, -110.0, -50.0),  // 39
        fr::vertex_3d(0.0, -10.0, -50.0),  // 40
        fr::vertex_3d(0.0, 10.0, -50.0),  // 41
        fr::vertex_3d(0.0, 110.0, -50.0),  // 42
        fr::vertex_3d(0.0, 130.0, -50.0),  // 43
        fr::vertex_3d(-70.0, -60.0, -50.0),  // 44
        fr::vertex_3d(-50.0, -60.0, -50.0),  // 45
        fr::vertex_3d(50.0, -60.0, -50.0),  // 46
        fr::vertex_3d(70.0, -60.0, -50.0),  // 47
        fr::vertex_3d(-70.0, 60.0, -50.0),  // 48
        fr::vertex_3d(-50.0, 60.0, -50.0),  // 49
        fr::vertex_3d(50.0, 60.0, -50.0),  // 50
        fr::vertex_3d(70.0, 60.0, -50.0)  // 51
    };

    constexpr inline fr::face_3d building_faces[] = {
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 4, 1, 0, 0, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 5, 2, 1, 1, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 6, 7, 4, 3, 2, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 8, 5, 4, 3, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 9, 10, 7, 6, 4, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 10, 11, 8, 7, 5, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 0, 2, 14, 12, 6, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 2, 11, 23, 14, 6, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 11, 23, 21, 6, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 12, 21, 9, 0, 6, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 1, 24, 38, 13, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 13, 38, 24, 1, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 25, 4, 16, 39, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 39, 16, 4, 25, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 4, 26, 40, 16, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 16, 40, 26, 4, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 27, 7, 19, 41, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 41, 19, 7, 27, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 28, 42, 19, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 19, 42, 28, 7, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 29, 10, 22, 43, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 43, 22, 10, 29, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 30, 44, 15, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 30, 3, 15, 44, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 31, 4, 16, 45, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 4, 31, 45, 16, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 32, 46, 16, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 4, 16, 46, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 33, 5, 17, 47, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 5, 33, 47, 17, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 6, 34, 48, 18, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 34, 6, 18, 48, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 35, 7, 19, 49, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 7, 35, 49, 19, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 7, 36, 50, 19, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 36, 7, 19, 50, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 37, 8, 20, 51, 7, -1),
        fr::face_3d(building_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 8, 37, 51, 20, 7, -1)
    };

    constexpr inline fr::model_3d_item building(building_vertices, building_faces);
}

#endif
