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
        fr::vertex_3d(60.0, -60.0, -50.0),  // 4
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 5
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 6
        fr::vertex_3d(-10.0, 60.0, -50.0),  // 7
        fr::vertex_3d(-10.0, 60.0, 0.0),  // 8
        fr::vertex_3d(10.0, 60.0, -50.0),  // 9
        fr::vertex_3d(60.0, 60.0, -50.0),  // 10
        fr::vertex_3d(10.0, 60.0, 0.0),  // 11
        fr::vertex_3d(10.0, 60.0, -35.0),  // 12
        fr::vertex_3d(-10.0, 60.0, -35.0),  // 13
        fr::vertex_3d(60.0, -17.5, 0.0),  // 14
        fr::vertex_3d(60.0, -17.5, -50.0),  // 15
        fr::vertex_3d(60.0, 2.5, 0.0),  // 16
        fr::vertex_3d(60.0, 2.5, -50.0),  // 17
        fr::vertex_3d(60.0, -17.5, -35.0),  // 18
        fr::vertex_3d(60.0, 2.5, -35.0)  // 19
    };

    constexpr inline fr::face_3d room_0_faces[] = {
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 2, 4, 5, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 6, 7, 8, 0, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 10, 1, 11, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 5, 6, 0, 3, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 2, 14, 15, 4, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 16, 1, 10, 17, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 7, 9, 12, 13, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 18, 19, 17, 15, 8, -1)
    };
    constexpr inline fr::model_3d_item room_0(room_0_vertices, room_0_faces);

    constexpr inline fr::vertex_3d room_1_vertices[] = {
        fr::vertex_3d(-75.0, 75.0, 0.0),  // 0
        fr::vertex_3d(75.0, 75.0, 0.0),  // 1
        fr::vertex_3d(75.0, -75.0, 0.0),  // 2
        fr::vertex_3d(-75.0, -75.0, 0.0),  // 3
        fr::vertex_3d(75.0, -75.0, -62.5),  // 4
        fr::vertex_3d(-75.0, -75.0, -62.5),  // 5
        fr::vertex_3d(-75.0, 75.0, -62.5),  // 6
        fr::vertex_3d(-17.5, 75.0, -62.5),  // 7
        fr::vertex_3d(-17.5, 75.0, 0.0),  // 8
        fr::vertex_3d(2.5, 75.0, -62.5),  // 9
        fr::vertex_3d(75.0, 75.0, -62.5),  // 10
        fr::vertex_3d(2.5, 75.0, 0.0),  // 11
        fr::vertex_3d(2.5, 75.0, -43.75),  // 12
        fr::vertex_3d(-17.5, 75.0, -43.75),  // 13
        fr::vertex_3d(-75.0, -2.5, -62.5),  // 14
        fr::vertex_3d(-75.0, -2.5, 0.0),  // 15
        fr::vertex_3d(-75.0, 17.5, -62.5),  // 16
        fr::vertex_3d(-75.0, 17.5, 0.0),  // 17
        fr::vertex_3d(-75.0, 17.5, -43.75),  // 18
        fr::vertex_3d(-75.0, -2.5, -43.75),  // 19
        fr::vertex_3d(-20.0, -74.8, -25.0),  // 20
        fr::vertex_3d(20.0, -74.8, -25.0),  // 21
        fr::vertex_3d(20.0, -74.8, -45.0),  // 22
        fr::vertex_3d(-20.0, -74.8, -45.0)  // 23
    };

    constexpr inline fr::face_3d room_1_faces[] = {
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 1, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 2, 4, 5, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 6, 7, 8, 0, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 9, 10, 1, 11, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 5, 14, 15, 3, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 16, 6, 0, 17, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 2, 1, 10, 4, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 7, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 7, 9, 12, 13, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 14, 16, 18, 19, 8, -1)
    };
    constexpr inline fr::model_3d_item room_1(room_1_vertices, room_1_faces);

    constexpr inline fr::vertex_3d room_2_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(60.0, 60.0, 0.0),  // 1
        fr::vertex_3d(60.0, -60.0, 0.0),  // 2
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 3
        fr::vertex_3d(-10.0, -60.0, 0.0),  // 4
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 5
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 6
        fr::vertex_3d(10.0, -60.0, 0.0),  // 7
        fr::vertex_3d(60.0, -60.0, -50.0),  // 8
        fr::vertex_3d(10.0, -60.0, -50.0),  // 9
        fr::vertex_3d(-10.0, -60.0, -35.0),  // 10
        fr::vertex_3d(10.0, -60.0, -35.0),  // 11
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 12
        fr::vertex_3d(-10.0, 60.0, -50.0),  // 13
        fr::vertex_3d(-10.0, 60.0, 0.0),  // 14
        fr::vertex_3d(10.0, 60.0, -50.0),  // 15
        fr::vertex_3d(60.0, 60.0, -50.0),  // 16
        fr::vertex_3d(10.0, 60.0, 0.0),  // 17
        fr::vertex_3d(10.0, 60.0, -35.0),  // 18
        fr::vertex_3d(-10.0, 60.0, -35.0),  // 19
        fr::vertex_3d(60.0, -10.0, 0.0),  // 20
        fr::vertex_3d(60.0, -10.0, -50.0),  // 21
        fr::vertex_3d(60.0, 10.0, 0.0),  // 22
        fr::vertex_3d(60.0, 10.0, -50.0),  // 23
        fr::vertex_3d(60.0, -10.0, -35.0),  // 24
        fr::vertex_3d(60.0, 10.0, -35.0)  // 25
    };

    constexpr inline fr::face_3d room_2_faces[] = {
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 2, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 4, 5, 6, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 7, 2, 8, 9, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 13, 14, 0, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 15, 16, 1, 17, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 6, 12, 0, 3, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 2, 20, 21, 8, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 22, 1, 16, 23, 6, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 10, 11, 9, 5, 8, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 13, 15, 18, 19, 8, -1),
        fr::face_3d(room_2_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 24, 25, 23, 21, 8, -1)
    };
    constexpr inline fr::model_3d_item room_2(room_2_vertices, room_2_faces);

    constexpr inline fr::vertex_3d room_3_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(60.0, 60.0, 0.0),  // 1
        fr::vertex_3d(60.0, -60.0, 0.0),  // 2
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 3
        fr::vertex_3d(-2.5, -60.0, 0.0),  // 4
        fr::vertex_3d(-2.5, -60.0, -50.0),  // 5
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 6
        fr::vertex_3d(17.5, -60.0, 0.0),  // 7
        fr::vertex_3d(60.0, -60.0, -50.0),  // 8
        fr::vertex_3d(17.5, -60.0, -50.0),  // 9
        fr::vertex_3d(-2.5, -60.0, -35.0),  // 10
        fr::vertex_3d(17.5, -60.0, -35.0),  // 11
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 12
        fr::vertex_3d(-2.5, 60.0, -50.0),  // 13
        fr::vertex_3d(-2.5, 60.0, 0.0),  // 14
        fr::vertex_3d(17.5, 60.0, -50.0),  // 15
        fr::vertex_3d(60.0, 60.0, -50.0),  // 16
        fr::vertex_3d(17.5, 60.0, 0.0),  // 17
        fr::vertex_3d(17.5, 60.0, -35.0),  // 18
        fr::vertex_3d(-2.5, 60.0, -35.0),  // 19
        fr::vertex_3d(-60.0, -10.0, -50.0),  // 20
        fr::vertex_3d(-60.0, -10.0, 0.0),  // 21
        fr::vertex_3d(-60.0, 10.0, -50.0),  // 22
        fr::vertex_3d(-60.0, 10.0, 0.0),  // 23
        fr::vertex_3d(-60.0, 10.0, -35.0),  // 24
        fr::vertex_3d(-60.0, -10.0, -35.0),  // 25
        fr::vertex_3d(59.84, -16.0, -20.0),  // 26
        fr::vertex_3d(59.84, 16.0, -20.0),  // 27
        fr::vertex_3d(59.84, 16.0, -36.0),  // 28
        fr::vertex_3d(59.84, -16.0, -36.0)  // 29
    };

    constexpr inline fr::face_3d room_3_faces[] = {
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 3, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 4, 5, 6, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 7, 2, 8, 9, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 13, 14, 0, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 15, 16, 1, 17, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 6, 20, 21, 3, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 22, 12, 0, 23, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 2, 1, 16, 8, 6, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 26, 27, 28, 29, 7, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 10, 11, 9, 5, 8, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 13, 15, 18, 19, 8, -1),
        fr::face_3d(room_3_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 20, 22, 24, 25, 8, -1)
    };
    constexpr inline fr::model_3d_item room_3(room_3_vertices, room_3_faces);

    constexpr inline fr::vertex_3d room_4_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(60.0, 60.0, 0.0),  // 1
        fr::vertex_3d(60.0, -60.0, 0.0),  // 2
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 3
        fr::vertex_3d(-10.0, -60.0, 0.0),  // 4
        fr::vertex_3d(-10.0, -60.0, -50.0),  // 5
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 6
        fr::vertex_3d(10.0, -60.0, 0.0),  // 7
        fr::vertex_3d(60.0, -60.0, -50.0),  // 8
        fr::vertex_3d(10.0, -60.0, -50.0),  // 9
        fr::vertex_3d(-10.0, -60.0, -35.0),  // 10
        fr::vertex_3d(10.0, -60.0, -35.0),  // 11
        fr::vertex_3d(-60.0, 60.0, -50.0),  // 12
        fr::vertex_3d(60.0, 60.0, -50.0),  // 13
        fr::vertex_3d(60.0, -2.5, 0.0),  // 14
        fr::vertex_3d(60.0, -2.5, -50.0),  // 15
        fr::vertex_3d(60.0, 17.5, 0.0),  // 16
        fr::vertex_3d(60.0, 17.5, -50.0),  // 17
        fr::vertex_3d(60.0, -2.5, -35.0),  // 18
        fr::vertex_3d(60.0, 17.5, -35.0)  // 19
    };

    constexpr inline fr::face_3d room_4_faces[] = {
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 4, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 4, 5, 6, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 7, 2, 8, 9, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 13, 1, 0, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 6, 12, 0, 3, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 2, 14, 15, 8, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 16, 1, 13, 17, 6, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 10, 11, 9, 5, 8, -1),
        fr::face_3d(room_4_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 18, 19, 17, 15, 8, -1)
    };
    constexpr inline fr::model_3d_item room_4(room_4_vertices, room_4_faces);

    constexpr inline fr::vertex_3d room_5_vertices[] = {
        fr::vertex_3d(-75.0, 75.0, 0.0),  // 0
        fr::vertex_3d(75.0, 75.0, 0.0),  // 1
        fr::vertex_3d(75.0, -75.0, 0.0),  // 2
        fr::vertex_3d(-75.0, -75.0, 0.0),  // 3
        fr::vertex_3d(-17.5, -75.0, 0.0),  // 4
        fr::vertex_3d(-17.5, -75.0, -62.5),  // 5
        fr::vertex_3d(-75.0, -75.0, -62.5),  // 6
        fr::vertex_3d(2.5, -75.0, 0.0),  // 7
        fr::vertex_3d(75.0, -75.0, -62.5),  // 8
        fr::vertex_3d(2.5, -75.0, -62.5),  // 9
        fr::vertex_3d(-17.5, -75.0, -43.75),  // 10
        fr::vertex_3d(2.5, -75.0, -43.75),  // 11
        fr::vertex_3d(-75.0, 75.0, -62.5),  // 12
        fr::vertex_3d(75.0, 75.0, -62.5),  // 13
        fr::vertex_3d(-75.0, -17.5, -62.5),  // 14
        fr::vertex_3d(-75.0, -17.5, 0.0),  // 15
        fr::vertex_3d(-75.0, 2.5, -62.5),  // 16
        fr::vertex_3d(-75.0, 2.5, 0.0),  // 17
        fr::vertex_3d(-75.0, 2.5, -43.75),  // 18
        fr::vertex_3d(-75.0, -17.5, -43.75)  // 19
    };

    constexpr inline fr::face_3d room_5_faces[] = {
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 5, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 3, 4, 5, 6, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 7, 2, 8, 9, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 13, 1, 0, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 6, 14, 15, 3, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 16, 12, 0, 17, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 2, 1, 13, 8, 6, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 10, 11, 9, 5, 8, -1),
        fr::face_3d(room_5_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 14, 16, 18, 19, 8, -1)
    };
    constexpr inline fr::model_3d_item room_5(room_5_vertices, room_5_faces);

    constexpr inline fr::model_3d_item room(room_0_vertices, room_0_faces);
}

#endif
