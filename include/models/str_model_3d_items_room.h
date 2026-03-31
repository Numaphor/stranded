#ifndef STR_MODEL_3D_ITEMS_ROOM_H
#define STR_MODEL_3D_ITEMS_ROOM_H

#include "fr_model_3d_item.h"
#include "bn_color.h"

namespace str::model_3d_items
{
    constexpr inline bn::color room_model_colors[] = {
        bn::color(28, 20, 10),  // 0  floor_light_a
        bn::color(27, 19, 10),  // 1  floor_light_b
        bn::color(26, 18, 9),  // 2  floor_mid_a
        bn::color(25, 18, 9),  // 3  floor_mid_b
        bn::color(24, 17, 8),  // 4  floor_shadow_a
        bn::color(23, 16, 8),  // 5  floor_shadow_b
        bn::color(28, 22, 13),  // 6  wall
        bn::color(19, 12, 6),  // 7  wainscot
        bn::color(12, 8, 4),  // 8  trim
        bn::color(18, 12, 6)  // 9  unused
    };

    constexpr inline fr::vertex_3d room_0_vertices[] = {
        fr::vertex_3d(-90.0, 60.0, 0.0),  // 0
        fr::vertex_3d(-67.5, 60.0, 0.0),  // 1
        fr::vertex_3d(-67.5, 45.0, 0.0),  // 2
        fr::vertex_3d(-90.0, 45.0, 0.0),  // 3
        fr::vertex_3d(-45.0, 60.0, 0.0),  // 4
        fr::vertex_3d(-45.0, 45.0, 0.0),  // 5
        fr::vertex_3d(-22.5, 60.0, 0.0),  // 6
        fr::vertex_3d(-22.5, 45.0, 0.0),  // 7
        fr::vertex_3d(0.0, 60.0, 0.0),  // 8
        fr::vertex_3d(0.0, 45.0, 0.0),  // 9
        fr::vertex_3d(22.5, 60.0, 0.0),  // 10
        fr::vertex_3d(22.5, 45.0, 0.0),  // 11
        fr::vertex_3d(45.0, 60.0, 0.0),  // 12
        fr::vertex_3d(45.0, 45.0, 0.0),  // 13
        fr::vertex_3d(67.5, 60.0, 0.0),  // 14
        fr::vertex_3d(67.5, 45.0, 0.0),  // 15
        fr::vertex_3d(90.0, 60.0, 0.0),  // 16
        fr::vertex_3d(90.0, 45.0, 0.0),  // 17
        fr::vertex_3d(-67.5, 30.0, 0.0),  // 18
        fr::vertex_3d(-90.0, 30.0, 0.0),  // 19
        fr::vertex_3d(-45.0, 30.0, 0.0),  // 20
        fr::vertex_3d(-22.5, 30.0, 0.0),  // 21
        fr::vertex_3d(0.0, 30.0, 0.0),  // 22
        fr::vertex_3d(22.5, 30.0, 0.0),  // 23
        fr::vertex_3d(45.0, 30.0, 0.0),  // 24
        fr::vertex_3d(67.5, 30.0, 0.0),  // 25
        fr::vertex_3d(90.0, 30.0, 0.0),  // 26
        fr::vertex_3d(-67.5, 15.0, 0.0),  // 27
        fr::vertex_3d(-90.0, 15.0, 0.0),  // 28
        fr::vertex_3d(-45.0, 15.0, 0.0),  // 29
        fr::vertex_3d(-22.5, 15.0, 0.0),  // 30
        fr::vertex_3d(0.0, 15.0, 0.0),  // 31
        fr::vertex_3d(22.5, 15.0, 0.0),  // 32
        fr::vertex_3d(45.0, 15.0, 0.0),  // 33
        fr::vertex_3d(67.5, 15.0, 0.0),  // 34
        fr::vertex_3d(90.0, 15.0, 0.0),  // 35
        fr::vertex_3d(-67.5, 0.0, 0.0),  // 36
        fr::vertex_3d(-90.0, 0.0, 0.0),  // 37
        fr::vertex_3d(-45.0, 0.0, 0.0),  // 38
        fr::vertex_3d(-22.5, 0.0, 0.0),  // 39
        fr::vertex_3d(0.0, 0.0, 0.0),  // 40
        fr::vertex_3d(22.5, 0.0, 0.0),  // 41
        fr::vertex_3d(45.0, 0.0, 0.0),  // 42
        fr::vertex_3d(67.5, 0.0, 0.0),  // 43
        fr::vertex_3d(90.0, 0.0, 0.0),  // 44
        fr::vertex_3d(-67.5, -15.0, 0.0),  // 45
        fr::vertex_3d(-90.0, -15.0, 0.0),  // 46
        fr::vertex_3d(-45.0, -15.0, 0.0),  // 47
        fr::vertex_3d(-22.5, -15.0, 0.0),  // 48
        fr::vertex_3d(0.0, -15.0, 0.0),  // 49
        fr::vertex_3d(22.5, -15.0, 0.0),  // 50
        fr::vertex_3d(45.0, -15.0, 0.0),  // 51
        fr::vertex_3d(67.5, -15.0, 0.0),  // 52
        fr::vertex_3d(90.0, -15.0, 0.0),  // 53
        fr::vertex_3d(-67.5, -30.0, 0.0),  // 54
        fr::vertex_3d(-90.0, -30.0, 0.0),  // 55
        fr::vertex_3d(-45.0, -30.0, 0.0),  // 56
        fr::vertex_3d(-22.5, -30.0, 0.0),  // 57
        fr::vertex_3d(0.0, -30.0, 0.0),  // 58
        fr::vertex_3d(22.5, -30.0, 0.0),  // 59
        fr::vertex_3d(45.0, -30.0, 0.0),  // 60
        fr::vertex_3d(67.5, -30.0, 0.0),  // 61
        fr::vertex_3d(90.0, -30.0, 0.0),  // 62
        fr::vertex_3d(-67.5, -45.0, 0.0),  // 63
        fr::vertex_3d(-90.0, -45.0, 0.0),  // 64
        fr::vertex_3d(-45.0, -45.0, 0.0),  // 65
        fr::vertex_3d(-22.5, -45.0, 0.0),  // 66
        fr::vertex_3d(0.0, -45.0, 0.0),  // 67
        fr::vertex_3d(22.5, -45.0, 0.0),  // 68
        fr::vertex_3d(45.0, -45.0, 0.0),  // 69
        fr::vertex_3d(67.5, -45.0, 0.0),  // 70
        fr::vertex_3d(90.0, -45.0, 0.0),  // 71
        fr::vertex_3d(-67.5, -60.0, 0.0),  // 72
        fr::vertex_3d(-90.0, -60.0, 0.0),  // 73
        fr::vertex_3d(-45.0, -60.0, 0.0),  // 74
        fr::vertex_3d(-22.5, -60.0, 0.0),  // 75
        fr::vertex_3d(0.0, -60.0, 0.0),  // 76
        fr::vertex_3d(22.5, -60.0, 0.0),  // 77
        fr::vertex_3d(45.0, -60.0, 0.0),  // 78
        fr::vertex_3d(67.5, -60.0, 0.0),  // 79
        fr::vertex_3d(90.0, -60.0, 0.0),  // 80
        fr::vertex_3d(90.0, -60.0, -14.0),  // 81
        fr::vertex_3d(-90.0, -60.0, -14.0),  // 82
        fr::vertex_3d(90.0, -60.0, -16.0),  // 83
        fr::vertex_3d(-90.0, -60.0, -16.0),  // 84
        fr::vertex_3d(90.0, -60.0, -50.0),  // 85
        fr::vertex_3d(-90.0, -60.0, -50.0),  // 86
        fr::vertex_3d(35.0, 60.0, 0.0),  // 87
        fr::vertex_3d(35.0, 60.0, -14.0),  // 88
        fr::vertex_3d(-90.0, 60.0, -14.0),  // 89
        fr::vertex_3d(35.0, 60.0, -16.0),  // 90
        fr::vertex_3d(-90.0, 60.0, -16.0),  // 91
        fr::vertex_3d(35.0, 60.0, -50.0),  // 92
        fr::vertex_3d(-90.0, 60.0, -50.0),  // 93
        fr::vertex_3d(55.0, 60.0, 0.0),  // 94
        fr::vertex_3d(90.0, 60.0, -14.0),  // 95
        fr::vertex_3d(55.0, 60.0, -14.0),  // 96
        fr::vertex_3d(90.0, 60.0, -16.0),  // 97
        fr::vertex_3d(55.0, 60.0, -16.0),  // 98
        fr::vertex_3d(90.0, 60.0, -50.0),  // 99
        fr::vertex_3d(55.0, 60.0, -50.0),  // 100
        fr::vertex_3d(35.0, 60.0, -38.0),  // 101
        fr::vertex_3d(55.0, 60.0, -38.0),  // 102
        fr::vertex_3d(35.0, 60.0, -36.0),  // 103
        fr::vertex_3d(55.0, 60.0, -36.0),  // 104
        fr::vertex_3d(37.0, 60.0, 0.0),  // 105
        fr::vertex_3d(37.0, 60.0, -36.0),  // 106
        fr::vertex_3d(53.0, 60.0, 0.0),  // 107
        fr::vertex_3d(53.0, 60.0, -36.0)  // 108
    };

    constexpr inline fr::face_3d room_0_faces[] = {
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 1, 4, 5, 2, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 6, 7, 5, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 6, 8, 9, 7, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 8, 10, 11, 9, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 10, 12, 13, 11, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 12, 14, 15, 13, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 14, 16, 17, 15, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 18, 19, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 2, 5, 20, 18, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 5, 7, 21, 20, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 9, 22, 21, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 9, 11, 23, 22, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 13, 24, 23, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 13, 15, 25, 24, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 15, 17, 26, 25, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 19, 18, 27, 28, 4, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 18, 20, 29, 27, 1, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 20, 21, 30, 29, 4, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 21, 22, 31, 30, 1, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 22, 23, 32, 31, 4, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 23, 24, 33, 32, 1, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 24, 25, 34, 33, 4, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 25, 26, 35, 34, 1, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 28, 27, 36, 37, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 27, 29, 38, 36, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 29, 30, 39, 38, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 30, 31, 40, 39, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 31, 32, 41, 40, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 32, 33, 42, 41, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 33, 34, 43, 42, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 34, 35, 44, 43, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 37, 36, 45, 46, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 36, 38, 47, 45, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 38, 39, 48, 47, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 39, 40, 49, 48, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 40, 41, 50, 49, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 41, 42, 51, 50, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 42, 43, 52, 51, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 43, 44, 53, 52, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 46, 45, 54, 55, 4, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 45, 47, 56, 54, 1, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 47, 48, 57, 56, 4, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 48, 49, 58, 57, 1, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 49, 50, 59, 58, 4, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 50, 51, 60, 59, 1, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 51, 52, 61, 60, 4, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 52, 53, 62, 61, 1, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 55, 54, 63, 64, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 54, 56, 65, 63, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 56, 57, 66, 65, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 57, 58, 67, 66, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 58, 59, 68, 67, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 59, 60, 69, 68, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 60, 61, 70, 69, 0, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 61, 62, 71, 70, 3, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 64, 63, 72, 73, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 63, 65, 74, 72, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 65, 66, 75, 74, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 66, 67, 76, 75, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 67, 68, 77, 76, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 68, 69, 78, 77, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 69, 70, 79, 78, 2, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 70, 71, 80, 79, 5, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 73, 80, 81, 82, 7, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 82, 81, 83, 84, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 84, 83, 85, 86, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 0, 87, 88, 89, 7, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 89, 88, 90, 91, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 91, 90, 92, 93, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 94, 16, 95, 96, 7, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 96, 95, 97, 98, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 98, 97, 99, 100, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 101, 102, 100, 92, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 103, 104, 102, 101, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 87, 105, 106, 103, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 107, 94, 104, 108, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 80, 16, 95, 81, 7, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 81, 95, 97, 83, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 83, 97, 99, 85, 6, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 73, 0, 89, 82, 7, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 82, 89, 91, 84, 8, -1),
        fr::face_3d(room_0_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 84, 91, 93, 86, 6, -1)
    };
    constexpr inline fr::model_3d_item room_0(room_0_vertices, room_0_faces);

    constexpr inline fr::vertex_3d room_1_vertices[] = {
        fr::vertex_3d(-60.0, 60.0, 0.0),  // 0
        fr::vertex_3d(-42.9, 60.0, 0.0),  // 1
        fr::vertex_3d(-42.9, 42.9, 0.0),  // 2
        fr::vertex_3d(-60.0, 42.9, 0.0),  // 3
        fr::vertex_3d(-25.7, 60.0, 0.0),  // 4
        fr::vertex_3d(-25.7, 42.9, 0.0),  // 5
        fr::vertex_3d(-8.6, 60.0, 0.0),  // 6
        fr::vertex_3d(-8.6, 42.9, 0.0),  // 7
        fr::vertex_3d(8.6, 60.0, 0.0),  // 8
        fr::vertex_3d(8.6, 42.9, 0.0),  // 9
        fr::vertex_3d(25.7, 60.0, 0.0),  // 10
        fr::vertex_3d(25.7, 42.9, 0.0),  // 11
        fr::vertex_3d(42.9, 60.0, 0.0),  // 12
        fr::vertex_3d(42.9, 42.9, 0.0),  // 13
        fr::vertex_3d(60.0, 60.0, 0.0),  // 14
        fr::vertex_3d(60.0, 42.9, 0.0),  // 15
        fr::vertex_3d(-42.9, 25.7, 0.0),  // 16
        fr::vertex_3d(-60.0, 25.7, 0.0),  // 17
        fr::vertex_3d(-25.7, 25.7, 0.0),  // 18
        fr::vertex_3d(-8.6, 25.7, 0.0),  // 19
        fr::vertex_3d(8.6, 25.7, 0.0),  // 20
        fr::vertex_3d(25.7, 25.7, 0.0),  // 21
        fr::vertex_3d(42.9, 25.7, 0.0),  // 22
        fr::vertex_3d(60.0, 25.7, 0.0),  // 23
        fr::vertex_3d(-42.9, 8.6, 0.0),  // 24
        fr::vertex_3d(-60.0, 8.6, 0.0),  // 25
        fr::vertex_3d(-25.7, 8.6, 0.0),  // 26
        fr::vertex_3d(-8.6, 8.6, 0.0),  // 27
        fr::vertex_3d(8.6, 8.6, 0.0),  // 28
        fr::vertex_3d(25.7, 8.6, 0.0),  // 29
        fr::vertex_3d(42.9, 8.6, 0.0),  // 30
        fr::vertex_3d(60.0, 8.6, 0.0),  // 31
        fr::vertex_3d(-42.9, -8.6, 0.0),  // 32
        fr::vertex_3d(-60.0, -8.6, 0.0),  // 33
        fr::vertex_3d(-25.7, -8.6, 0.0),  // 34
        fr::vertex_3d(-8.6, -8.6, 0.0),  // 35
        fr::vertex_3d(8.6, -8.6, 0.0),  // 36
        fr::vertex_3d(25.7, -8.6, 0.0),  // 37
        fr::vertex_3d(42.9, -8.6, 0.0),  // 38
        fr::vertex_3d(60.0, -8.6, 0.0),  // 39
        fr::vertex_3d(-42.9, -25.7, 0.0),  // 40
        fr::vertex_3d(-60.0, -25.7, 0.0),  // 41
        fr::vertex_3d(-25.7, -25.7, 0.0),  // 42
        fr::vertex_3d(-8.6, -25.7, 0.0),  // 43
        fr::vertex_3d(8.6, -25.7, 0.0),  // 44
        fr::vertex_3d(25.7, -25.7, 0.0),  // 45
        fr::vertex_3d(42.9, -25.7, 0.0),  // 46
        fr::vertex_3d(60.0, -25.7, 0.0),  // 47
        fr::vertex_3d(-42.9, -42.9, 0.0),  // 48
        fr::vertex_3d(-60.0, -42.9, 0.0),  // 49
        fr::vertex_3d(-25.7, -42.9, 0.0),  // 50
        fr::vertex_3d(-8.6, -42.9, 0.0),  // 51
        fr::vertex_3d(8.6, -42.9, 0.0),  // 52
        fr::vertex_3d(25.7, -42.9, 0.0),  // 53
        fr::vertex_3d(42.9, -42.9, 0.0),  // 54
        fr::vertex_3d(60.0, -42.9, 0.0),  // 55
        fr::vertex_3d(-42.9, -60.0, 0.0),  // 56
        fr::vertex_3d(-60.0, -60.0, 0.0),  // 57
        fr::vertex_3d(-25.7, -60.0, 0.0),  // 58
        fr::vertex_3d(-8.6, -60.0, 0.0),  // 59
        fr::vertex_3d(8.6, -60.0, 0.0),  // 60
        fr::vertex_3d(25.7, -60.0, 0.0),  // 61
        fr::vertex_3d(42.9, -60.0, 0.0),  // 62
        fr::vertex_3d(60.0, -60.0, 0.0),  // 63
        fr::vertex_3d(-25.0, -60.0, 0.0),  // 64
        fr::vertex_3d(-25.0, -60.0, -14.0),  // 65
        fr::vertex_3d(-60.0, -60.0, -14.0),  // 66
        fr::vertex_3d(-25.0, -60.0, -16.0),  // 67
        fr::vertex_3d(-60.0, -60.0, -16.0),  // 68
        fr::vertex_3d(-25.0, -60.0, -50.0),  // 69
        fr::vertex_3d(-60.0, -60.0, -50.0),  // 70
        fr::vertex_3d(-5.0, -60.0, 0.0),  // 71
        fr::vertex_3d(60.0, -60.0, -14.0),  // 72
        fr::vertex_3d(-5.0, -60.0, -14.0),  // 73
        fr::vertex_3d(60.0, -60.0, -16.0),  // 74
        fr::vertex_3d(-5.0, -60.0, -16.0),  // 75
        fr::vertex_3d(60.0, -60.0, -50.0),  // 76
        fr::vertex_3d(-5.0, -60.0, -50.0),  // 77
        fr::vertex_3d(-25.0, -60.0, -38.0),  // 78
        fr::vertex_3d(-5.0, -60.0, -38.0),  // 79
        fr::vertex_3d(-25.0, -60.0, -36.0),  // 80
        fr::vertex_3d(-5.0, -60.0, -36.0),  // 81
        fr::vertex_3d(-23.0, -60.0, 0.0),  // 82
        fr::vertex_3d(-23.0, -60.0, -36.0),  // 83
        fr::vertex_3d(-7.0, -60.0, 0.0),  // 84
        fr::vertex_3d(-7.0, -60.0, -36.0),  // 85
        fr::vertex_3d(60.0, 60.0, -14.0),  // 86
        fr::vertex_3d(-60.0, 60.0, -14.0),  // 87
        fr::vertex_3d(60.0, 60.0, -16.0),  // 88
        fr::vertex_3d(-60.0, 60.0, -16.0),  // 89
        fr::vertex_3d(60.0, 60.0, -50.0),  // 90
        fr::vertex_3d(-60.0, 60.0, -50.0)  // 91
    };

    constexpr inline fr::face_3d room_1_faces[] = {
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 0, 1, 2, 3, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 1, 4, 5, 2, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 6, 7, 5, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 6, 8, 9, 7, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 8, 10, 11, 9, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 10, 12, 13, 11, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 12, 14, 15, 13, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 16, 17, 2, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 2, 5, 18, 16, 5, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 5, 7, 19, 18, 2, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 9, 20, 19, 5, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 9, 11, 21, 20, 2, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 13, 22, 21, 5, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 13, 15, 23, 22, 2, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 17, 16, 24, 25, 4, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 16, 18, 26, 24, 1, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 18, 19, 27, 26, 4, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 19, 20, 28, 27, 1, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 20, 21, 29, 28, 4, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 21, 22, 30, 29, 1, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 22, 23, 31, 30, 4, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 25, 24, 32, 33, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 24, 26, 34, 32, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 26, 27, 35, 34, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 27, 28, 36, 35, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 28, 29, 37, 36, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 29, 30, 38, 37, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 30, 31, 39, 38, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 33, 32, 40, 41, 2, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 32, 34, 42, 40, 5, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 34, 35, 43, 42, 2, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 35, 36, 44, 43, 5, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 36, 37, 45, 44, 2, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 37, 38, 46, 45, 5, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 38, 39, 47, 46, 2, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 41, 40, 48, 49, 4, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 40, 42, 50, 48, 1, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 42, 43, 51, 50, 4, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 43, 44, 52, 51, 1, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 44, 45, 53, 52, 4, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 45, 46, 54, 53, 1, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 46, 47, 55, 54, 4, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 49, 48, 56, 57, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 48, 50, 58, 56, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 50, 51, 59, 58, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 51, 52, 60, 59, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 52, 53, 61, 60, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 53, 54, 62, 61, 3, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 54, 55, 63, 62, 0, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 57, 64, 65, 66, 7, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 66, 65, 67, 68, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 68, 67, 69, 70, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 71, 63, 72, 73, 7, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 73, 72, 74, 75, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 75, 74, 76, 77, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 78, 79, 77, 69, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 80, 81, 79, 78, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 64, 82, 83, 80, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 84, 71, 81, 85, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 0, 14, 86, 87, 7, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 87, 86, 88, 89, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 89, 88, 90, 91, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 63, 14, 86, 72, 7, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 72, 86, 88, 74, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 74, 88, 90, 76, 6, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 57, 0, 87, 66, 7, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 66, 87, 89, 68, 8, -1),
        fr::face_3d(room_1_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 68, 89, 91, 70, 6, -1)
    };
    constexpr inline fr::model_3d_item room_1(room_1_vertices, room_1_faces);

    constexpr inline fr::model_3d_item room(room_0_vertices, room_0_faces);
}

#endif
