#ifndef STR_MODEL_3D_ITEMS_ROOM_H
#define STR_MODEL_3D_ITEMS_ROOM_H

#include "fr_model_3d_item.h"
#include "bn_color.h"

namespace str::model_3d_items
{
    constexpr inline bn::color room_model_colors[] = {
        bn::color(22, 16, 8),  // 0  room0_floor
        bn::color(12, 14, 22),  // 1  room1_floor
        bn::color(10, 20, 10),  // 2  room2_floor
        bn::color(22, 10, 8),  // 3  room3_floor
        bn::color(16, 10, 20),  // 4  room4_floor
        bn::color(8, 18, 20),  // 5  room5_floor
        bn::color(28, 26, 22),  // 6  wall
        bn::color(16, 22, 28),  // 7  reserved
        bn::color(10, 8, 6),  // 8  door_frame
        bn::color(18, 12, 6)  // 9  table_wood
    };

    constexpr inline fr::vertex_3d room_0_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(60.0, 60.0, 0.0),  // 1
        fr::vertex_3d(60.0, -60.0, 0.0),  // 2
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 3
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 4
        fr::vertex_3d(60.0, -60.0, 0.0),  // 5
        fr::vertex_3d(60.0, -60.0, -50.0),  // 6
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 7
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 8
        fr::vertex_3d(-10.0, 60.0, -50.0),  // 9
        fr::vertex_3d(-10.0, 60.0, 0.0),  // 10
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 11
        fr::vertex_3d(10.0, 60.0, -50.0),  // 12
        fr::vertex_3d(60.0, 60.0, -50.0),  // 13
        fr::vertex_3d(60.0, 60.0, 0.0),  // 14
        fr::vertex_3d(10.0, 60.0, 0.0),  // 15
        fr::vertex_3d(10.0, 60.0, -35.0),  // 16
        fr::vertex_3d(-10.0, 60.0, -35.0),  // 17
        fr::vertex_3d(60.0, -17.5, 0.0),  // 18
        fr::vertex_3d(60.0, -17.5, -50.0),  // 19
        fr::vertex_3d(60.0, 2.5, 0.0),  // 20
        fr::vertex_3d(60.0, 2.5, -50.0),  // 21
        fr::vertex_3d(60.0, -17.5, -35.0),  // 22
        fr::vertex_3d(60.0, 2.5, -35.0)  // 23
    };

    constexpr inline fr::face_3d room_0_faces[] = {
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 5, 6, 7, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 8, 9, 10, 11, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 13, 14, 15, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 7, 8, 11, 4, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 5, 18, 19, 6, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 20, 14, 13, 21, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 12, 16, 17, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 22, 23, 21, 19, 8, -1)
    };
    constexpr inline fr::model_3d_item room_0(room_0_vertices, room_0_faces);

    constexpr inline fr::vertex_3d room_1_vertices[] = {
        fr::vertex_3d(-75.0, 75.0, 0.0),  // 0
        fr::vertex_3d(75.0, 75.0, 0.0),  // 1
        fr::vertex_3d(75.0, -75.0, 0.0),  // 2
        fr::vertex_3d(-75.0, -75.0, 0.0),  // 3
        fr::vertex_3d(-75.0, -75.0, 0.0),  // 4
        fr::vertex_3d(75.0, -75.0, 0.0),  // 5
        fr::vertex_3d(75.0, -75.0, -62.5),  // 6
        fr::vertex_3d(-75.0, -75.0, -62.5),  // 7
        fr::vertex_3d(-75.0, 75.0, -62.5),  // 8
        fr::vertex_3d(-17.5, 75.0, -62.5),  // 9
        fr::vertex_3d(-17.5, 75.0, 0.0),  // 10
        fr::vertex_3d(-75.0, 75.0, 0.0),  // 11
        fr::vertex_3d(2.5, 75.0, -62.5),  // 12
        fr::vertex_3d(75.0, 75.0, -62.5),  // 13
        fr::vertex_3d(75.0, 75.0, 0.0),  // 14
        fr::vertex_3d(2.5, 75.0, 0.0),  // 15
        fr::vertex_3d(2.5, 75.0, -35.0),  // 16
        fr::vertex_3d(-17.5, 75.0, -35.0),  // 17
        fr::vertex_3d(-75.0, -2.5, -62.5),  // 18
        fr::vertex_3d(-75.0, -2.5, 0.0),  // 19
        fr::vertex_3d(-75.0, 17.5, -62.5),  // 20
        fr::vertex_3d(-75.0, 17.5, 0.0),  // 21
        fr::vertex_3d(-75.0, 17.5, -35.0),  // 22
        fr::vertex_3d(-75.0, -2.5, -35.0),  // 23
        fr::vertex_3d(-20.0, -74.8, -25.0),  // 24
        fr::vertex_3d(20.0, -74.8, -25.0),  // 25
        fr::vertex_3d(20.0, -74.8, -45.0),  // 26
        fr::vertex_3d(-20.0, -74.8, -45.0)  // 27
    };

    constexpr inline fr::face_3d room_1_faces[] = {
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 1, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 5, 6, 7, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 8, 9, 10, 11, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 13, 14, 15, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 7, 18, 19, 4, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 20, 8, 11, 21, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 5, 14, 13, 6, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 24, 25, 26, 27, 7, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 12, 16, 17, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 18, 20, 22, 23, 8, -1)
    };
    constexpr inline fr::model_3d_item room_1(room_1_vertices, room_1_faces);

    constexpr inline fr::vertex_3d room_2_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(60.0, 60.0, 0.0),  // 1
        fr::vertex_3d(60.0, -60.0, 0.0),  // 2
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 3
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 4
        fr::vertex_3d(-10.0, -60.0, 0.0),  // 5
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 6
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 7
        fr::vertex_3d(10.0, -60.0, 0.0),  // 8
        fr::vertex_3d(60.0, -60.0, 0.0),  // 9
        fr::vertex_3d(60.0, -60.0, -50.0),  // 10
        fr::vertex_3d(10.0, -60.0, -50.0),  // 11
        fr::vertex_3d(-10.0, -60.0, -35.0),  // 12
        fr::vertex_3d(10.0, -60.0, -35.0),  // 13
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 14
        fr::vertex_3d(-10.0, 60.0, -50.0),  // 15
        fr::vertex_3d(-10.0, 60.0, 0.0),  // 16
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 17
        fr::vertex_3d(10.0, 60.0, -50.0),  // 18
        fr::vertex_3d(60.0, 60.0, -50.0),  // 19
        fr::vertex_3d(60.0, 60.0, 0.0),  // 20
        fr::vertex_3d(10.0, 60.0, 0.0),  // 21
        fr::vertex_3d(10.0, 60.0, -35.0),  // 22
        fr::vertex_3d(-10.0, 60.0, -35.0),  // 23
        fr::vertex_3d(60.0, -10.0, 0.0),  // 24
        fr::vertex_3d(60.0, -10.0, -50.0),  // 25
        fr::vertex_3d(60.0, 10.0, 0.0),  // 26
        fr::vertex_3d(60.0, 10.0, -50.0),  // 27
        fr::vertex_3d(60.0, -10.0, -35.0),  // 28
        fr::vertex_3d(60.0, 10.0, -35.0)  // 29
    };

    constexpr inline fr::face_3d room_2_faces[] = {
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 2, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 5, 6, 7, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 8, 9, 10, 11, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 14, 15, 16, 17, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 18, 19, 20, 21, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 7, 14, 17, 4, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 9, 24, 25, 10, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 26, 20, 19, 27, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 11, 6, 8, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 15, 18, 22, 23, 8, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 28, 29, 27, 25, 8, -1)
    };
    constexpr inline fr::model_3d_item room_2(room_2_vertices, room_2_faces);

    constexpr inline fr::vertex_3d room_3_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(60.0, 60.0, 0.0),  // 1
        fr::vertex_3d(60.0, -60.0, 0.0),  // 2
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 3
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 4
        fr::vertex_3d(-2.5, -60.0, 0.0),  // 5
        fr::vertex_3d(-2.5, -60.0, -50.0),  // 6
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 7
        fr::vertex_3d(17.5, -60.0, 0.0),  // 8
        fr::vertex_3d(60.0, -60.0, 0.0),  // 9
        fr::vertex_3d(60.0, -60.0, -50.0),  // 10
        fr::vertex_3d(17.5, -60.0, -50.0),  // 11
        fr::vertex_3d(-2.5, -60.0, -35.0),  // 12
        fr::vertex_3d(17.5, -60.0, -35.0),  // 13
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 14
        fr::vertex_3d(-2.5, 60.0, -50.0),  // 15
        fr::vertex_3d(-2.5, 60.0, 0.0),  // 16
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 17
        fr::vertex_3d(17.5, 60.0, -50.0),  // 18
        fr::vertex_3d(60.0, 60.0, -50.0),  // 19
        fr::vertex_3d(60.0, 60.0, 0.0),  // 20
        fr::vertex_3d(17.5, 60.0, 0.0),  // 21
        fr::vertex_3d(17.5, 60.0, -35.0),  // 22
        fr::vertex_3d(-2.5, 60.0, -35.0),  // 23
        fr::vertex_3d(-60.0, -10.0, -50.0),  // 24
        fr::vertex_3d(-60.0, -10.0, 0.0),  // 25
        fr::vertex_3d(-60.0, 10.0, -50.0),  // 26
        fr::vertex_3d(-60.0, 10.0, 0.0),  // 27
        fr::vertex_3d(-60.0, 10.0, -35.0),  // 28
        fr::vertex_3d(-60.0, -10.0, -35.0),  // 29
        fr::vertex_3d(59.84, -16.0, -20.0),  // 30
        fr::vertex_3d(59.84, 16.0, -20.0),  // 31
        fr::vertex_3d(59.84, 16.0, -36.0),  // 32
        fr::vertex_3d(59.84, -16.0, -36.0)  // 33
    };

    constexpr inline fr::face_3d room_3_faces[] = {
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 3, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 5, 6, 7, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 8, 9, 10, 11, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 14, 15, 16, 17, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 18, 19, 20, 21, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 7, 24, 25, 4, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 26, 14, 17, 27, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 9, 20, 19, 10, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 30, 31, 32, 33, 7, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 11, 6, 8, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 15, 18, 22, 23, 8, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 24, 26, 28, 29, 8, -1)
    };
    constexpr inline fr::model_3d_item room_3(room_3_vertices, room_3_faces);

    constexpr inline fr::vertex_3d room_4_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(60.0, 60.0, 0.0),  // 1
        fr::vertex_3d(60.0, -60.0, 0.0),  // 2
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 3
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 4
        fr::vertex_3d(-10.0, -60.0, 0.0),  // 5
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 6
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 7
        fr::vertex_3d(10.0, -60.0, 0.0),  // 8
        fr::vertex_3d(60.0, -60.0, 0.0),  // 9
        fr::vertex_3d(60.0, -60.0, -50.0),  // 10
        fr::vertex_3d(10.0, -60.0, -50.0),  // 11
        fr::vertex_3d(-10.0, -60.0, -35.0),  // 12
        fr::vertex_3d(10.0, -60.0, -35.0),  // 13
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 14
        fr::vertex_3d(60.0, 60.0, -50.0),  // 15
        fr::vertex_3d(60.0, 60.0, 0.0),  // 16
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 17
        fr::vertex_3d(60.0, -2.5, 0.0),  // 18
        fr::vertex_3d(60.0, -2.5, -50.0),  // 19
        fr::vertex_3d(60.0, 17.5, 0.0),  // 20
        fr::vertex_3d(60.0, 17.5, -50.0),  // 21
        fr::vertex_3d(60.0, -2.5, -35.0),  // 22
        fr::vertex_3d(60.0, 17.5, -35.0)  // 23
    };

    constexpr inline fr::face_3d room_4_faces[] = {
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 4, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 5, 6, 7, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 8, 9, 10, 11, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 14, 15, 16, 17, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 7, 14, 17, 4, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 9, 18, 19, 10, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 20, 16, 15, 21, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 11, 6, 8, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 22, 23, 21, 19, 8, -1)
    };
    constexpr inline fr::model_3d_item room_4(room_4_vertices, room_4_faces);

    constexpr inline fr::vertex_3d room_5_vertices[] = {
        fr::vertex_3d(-75.0, 75.0, 0.0),  // 0
        fr::vertex_3d(75.0, 75.0, 0.0),  // 1
        fr::vertex_3d(75.0, -75.0, 0.0),  // 2
        fr::vertex_3d(-75.0, -75.0, 0.0),  // 3
        fr::vertex_3d(-75.0, -75.0, 0.0),  // 4
        fr::vertex_3d(-17.5, -75.0, 0.0),  // 5
        fr::vertex_3d(-17.5, -75.0, -62.5),  // 6
        fr::vertex_3d(-75.0, -75.0, -62.5),  // 7
        fr::vertex_3d(2.5, -75.0, 0.0),  // 8
        fr::vertex_3d(75.0, -75.0, 0.0),  // 9
        fr::vertex_3d(75.0, -75.0, -62.5),  // 10
        fr::vertex_3d(2.5, -75.0, -62.5),  // 11
        fr::vertex_3d(-17.5, -75.0, -35.0),  // 12
        fr::vertex_3d(2.5, -75.0, -35.0),  // 13
        fr::vertex_3d(-75.0, 75.0, -62.5),  // 14
        fr::vertex_3d(75.0, 75.0, -62.5),  // 15
        fr::vertex_3d(75.0, 75.0, 0.0),  // 16
        fr::vertex_3d(-75.0, 75.0, 0.0),  // 17
        fr::vertex_3d(-75.0, -17.5, -62.5),  // 18
        fr::vertex_3d(-75.0, -17.5, 0.0),  // 19
        fr::vertex_3d(-75.0, 2.5, -62.5),  // 20
        fr::vertex_3d(-75.0, 2.5, 0.0),  // 21
        fr::vertex_3d(-75.0, 2.5, -35.0),  // 22
        fr::vertex_3d(-75.0, -17.5, -35.0)  // 23
    };

    constexpr inline fr::face_3d room_5_faces[] = {
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 5, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 5, 6, 7, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 8, 9, 10, 11, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 14, 15, 16, 17, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 7, 18, 19, 4, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 20, 14, 17, 21, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 9, 16, 15, 10, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 11, 6, 8, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 18, 20, 22, 23, 8, -1)
    };
    constexpr inline fr::model_3d_item room_5(room_5_vertices, room_5_faces);

    // Shared door panel model — paneled door filling doorway opening (20 wide × 35 tall)
    // Three horizontal panels: top rail (dark brown), center panel (lighter wood), bottom rail (dark brown)
    // Color 8 = door_frame dark brown, Color 9 = furniture/wood medium brown
    constexpr inline fr::vertex_3d door_panel_vertices[] = {
        // Bottom rail (z=0 to z=-5)
        fr::vertex_3d(-10.0, 0.0, 0.0),       // 0: bottom-left floor
        fr::vertex_3d(10.0, 0.0, 0.0),        // 1: bottom-right floor
        fr::vertex_3d(10.0, 0.0, -5.0),       // 2: bottom-right top-of-rail
        fr::vertex_3d(-10.0, 0.0, -5.0),      // 3: bottom-left top-of-rail
        // Center panel (z=-5 to z=-30) — shared vertices 2,3 with bottom rail
        fr::vertex_3d(10.0, 0.0, -30.0),      // 4: center-right top
        fr::vertex_3d(-10.0, 0.0, -30.0),     // 5: center-left top
        // Top rail (z=-30 to z=-35) — shared vertices 4,5 with center panel
        fr::vertex_3d(10.0, 0.0, -35.0),      // 6: top-right
        fr::vertex_3d(-10.0, 0.0, -35.0)      // 7: top-left
    };
    constexpr inline fr::face_3d door_panel_faces[] = {
        // Bottom rail — dark brown (matches door frame)
        fr::face_3d(door_panel_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 0, 1, 2, 3, 8, -1),
        // Center panel — lighter wood (furniture brown)
        fr::face_3d(door_panel_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 3, 2, 4, 5, 9, -1),
        // Top rail — dark brown (matches door frame/lintel)
        fr::face_3d(door_panel_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 5, 4, 6, 7, 8, -1)
    };
    constexpr inline fr::model_3d_item door_panel(door_panel_vertices, door_panel_faces);

    // Legacy aliases — all point to the shared door_panel
    constexpr inline const fr::model_3d_item& door_peek_0 = door_panel;
    constexpr inline const fr::model_3d_item& door_peek_1 = door_panel;
    constexpr inline const fr::model_3d_item& door_peek_2 = door_panel;
    constexpr inline const fr::model_3d_item& door_peek_3 = door_panel;
    constexpr inline const fr::model_3d_item& door_peek_4 = door_panel;
    constexpr inline const fr::model_3d_item& door_peek_5 = door_panel;

    constexpr inline fr::model_3d_item room(room_0_vertices, room_0_faces);
}

#endif
