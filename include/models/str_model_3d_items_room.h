#ifndef STR_MODEL_3D_ITEMS_ROOM_H
#define STR_MODEL_3D_ITEMS_ROOM_H

#include "fr_model_3d_item.h"
#include "bn_color.h"

namespace str::model_3d_items
{
    constexpr inline bn::color room_model_colors[] = {
        bn::color(22, 16, 8),  // 0  floor_light
        bn::color(16, 10, 4),  // 1  floor_dark
        bn::color(28, 26, 22),  // 2  wall
        bn::color(22, 20, 16),  // 3  wall_shadow
        bn::color(16, 22, 28),  // 4  reserved
        bn::color(10, 8, 6),  // 5  door_frame
        bn::color(18, 12, 6),  // 6  table_wood
        bn::color(8, 12, 18),  // 7  chair_fabric
        bn::color(20, 14, 8)  // 8  chair_frame
    };

    constexpr inline fr::vertex_3d room_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(-20.0, 60.0, 0.0),  // 1
        fr::vertex_3d(-20.0, -60.0, 0.0),  // 2
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 3
        fr::vertex_3d(20.0, 60.0, 0.0),  // 4
        fr::vertex_3d(20.0, -60.0, 0.0),  // 5
        fr::vertex_3d(60.0, 60.0, 0.0),  // 6
        fr::vertex_3d(60.0, -60.0, 0.0),  // 7
        fr::vertex_3d(60.0, -60.0, -50.0),  // 8
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 9
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 10
        fr::vertex_3d(-10.0, 60.0, -50.0),  // 11
        fr::vertex_3d(-10.0, 60.0, 0.0),  // 12
        fr::vertex_3d(10.0, 60.0, -50.0),  // 13
        fr::vertex_3d(60.0, 60.0, -50.0),  // 14
        fr::vertex_3d(10.0, 60.0, 0.0),  // 15
        fr::vertex_3d(10.0, 60.0, -35.0),  // 16
        fr::vertex_3d(-10.0, 60.0, -35.0),  // 17
        fr::vertex_3d(60.0, -10.0, 0.0),  // 18
        fr::vertex_3d(60.0, -10.0, -50.0),  // 19
        fr::vertex_3d(60.0, 10.0, 0.0),  // 20
        fr::vertex_3d(60.0, 10.0, -50.0),  // 21
        fr::vertex_3d(60.0, -10.0, -35.0),  // 22
        fr::vertex_3d(60.0, 10.0, -35.0),  // 23
        fr::vertex_3d(-60.0, -10.0, -50.0),  // 24
        fr::vertex_3d(-60.0, -10.0, 0.0),  // 25
        fr::vertex_3d(-60.0, 10.0, -50.0),  // 26
        fr::vertex_3d(-60.0, 10.0, 0.0),  // 27
        fr::vertex_3d(-60.0, 10.0, -35.0),  // 28
        fr::vertex_3d(-60.0, -10.0, -35.0),  // 29
        fr::vertex_3d(-16.0, -59.84, -20.0),  // 30
        fr::vertex_3d(16.0, -59.84, -20.0),  // 31
        fr::vertex_3d(16.0, -59.84, -36.0),  // 32
        fr::vertex_3d(-16.0, -59.84, -36.0),  // 33
        fr::vertex_3d(-10.0, -60.0, 0.0),  // 34
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 35
        fr::vertex_3d(10.0, -60.0, 0.0),  // 36
        fr::vertex_3d(10.0, -60.0, -50.0),  // 37
        fr::vertex_3d(-10.0, -60.0, -35.0),  // 38
        fr::vertex_3d(10.0, -60.0, -35.0),  // 39
        fr::vertex_3d(59.84, -16.0, -20.0),  // 40
        fr::vertex_3d(59.84, 16.0, -20.0),  // 41
        fr::vertex_3d(59.84, 16.0, -36.0),  // 42
        fr::vertex_3d(59.84, -16.0, -36.0)  // 43
    };

    constexpr inline fr::face_3d room_0_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 6, 7, 5, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 1, 4, 5, 2, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 7, 8, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 8, 7, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 10, 11, 12, 0, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 0, 12, 11, 10, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 13, 14, 6, 15, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 15, 6, 14, 13, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 9, 10, 0, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 3, 0, 10, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 18, 19, 8, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 8, 19, 18, 7, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 20, 6, 14, 21, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 21, 14, 6, 20, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 11, 13, 16, 17, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 17, 16, 13, 11, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 22, 23, 21, 19, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 19, 21, 23, 22, 5, -1)
    };
    constexpr inline fr::model_3d_item room_0(room_vertices, room_0_faces);

    constexpr inline fr::face_3d room_1_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 6, 7, 5, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 1, 4, 5, 2, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 7, 8, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 8, 7, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 10, 11, 12, 0, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 0, 12, 11, 10, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 13, 14, 6, 15, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 15, 6, 14, 13, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 9, 24, 25, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 3, 25, 24, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 26, 10, 0, 27, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 27, 0, 10, 26, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 6, 14, 8, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 8, 14, 6, 7, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 30, 31, 32, 33, 4, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 33, 32, 31, 30, 4, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 11, 13, 16, 17, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 17, 16, 13, 11, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 24, 26, 28, 29, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 29, 28, 26, 24, 5, -1)
    };
    constexpr inline fr::model_3d_item room_1(room_vertices, room_1_faces);

    constexpr inline fr::face_3d room_2_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 6, 7, 5, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 1, 4, 5, 2, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 34, 35, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 35, 34, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 36, 7, 8, 37, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 37, 8, 7, 36, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 10, 11, 12, 0, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 0, 12, 11, 10, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 13, 14, 6, 15, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 15, 6, 14, 13, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 9, 10, 0, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 3, 0, 10, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 18, 19, 8, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 8, 19, 18, 7, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 20, 6, 14, 21, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 21, 14, 6, 20, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 38, 39, 37, 35, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 35, 37, 39, 38, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 11, 13, 16, 17, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 17, 16, 13, 11, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 22, 23, 21, 19, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 19, 21, 23, 22, 5, -1)
    };
    constexpr inline fr::model_3d_item room_2(room_vertices, room_2_faces);

    constexpr inline fr::face_3d room_3_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 6, 7, 5, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 1, 4, 5, 2, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 34, 35, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 35, 34, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 36, 7, 8, 37, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 37, 8, 7, 36, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 10, 11, 12, 0, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 0, 12, 11, 10, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 13, 14, 6, 15, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 15, 6, 14, 13, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 9, 24, 25, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 3, 25, 24, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 26, 10, 0, 27, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 27, 0, 10, 26, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 6, 14, 8, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 8, 14, 6, 7, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 40, 41, 42, 43, 4, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 43, 42, 41, 40, 4, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 38, 39, 37, 35, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 35, 37, 39, 38, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 11, 13, 16, 17, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 17, 16, 13, 11, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 24, 26, 28, 29, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 29, 28, 26, 24, 5, -1)
    };
    constexpr inline fr::model_3d_item room_3(room_vertices, room_3_faces);

    constexpr inline fr::face_3d room_4_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 6, 7, 5, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 1, 4, 5, 2, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 34, 35, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 35, 34, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 36, 7, 8, 37, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 37, 8, 7, 36, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 10, 14, 6, 0, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 0, 6, 14, 10, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 9, 10, 0, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 3, 0, 10, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 18, 19, 8, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 8, 19, 18, 7, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 20, 6, 14, 21, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 21, 14, 6, 20, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 38, 39, 37, 35, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 35, 37, 39, 38, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 22, 23, 21, 19, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 19, 21, 23, 22, 5, -1)
    };
    constexpr inline fr::model_3d_item room_4(room_vertices, room_4_faces);

    constexpr inline fr::face_3d room_5_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 6, 7, 5, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 1, 4, 5, 2, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 34, 35, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 35, 34, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 36, 7, 8, 37, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 37, 8, 7, 36, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 10, 14, 6, 0, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 0, 6, 14, 10, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 9, 24, 25, 3, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 3, 25, 24, 9, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 26, 10, 0, 27, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 27, 0, 10, 26, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 6, 14, 8, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 8, 14, 6, 7, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 38, 39, 37, 35, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 35, 37, 39, 38, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 24, 26, 28, 29, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 29, 28, 26, 24, 5, -1)
    };
    constexpr inline fr::model_3d_item room_5(room_vertices, room_5_faces);

    constexpr inline fr::model_3d_item room(room_vertices, room_0_faces);
}

#endif
