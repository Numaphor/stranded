#ifndef STR_MODEL_3D_ITEMS_CHAIR_H
#define STR_MODEL_3D_ITEMS_CHAIR_H

#include "fr_model_3d_item.h"

namespace str::model_3d_items
{
    constexpr inline fr::vertex_3d chair_vertices[] = {
        fr::vertex_3d(-6.3, -7.245, -4.2),  // 0
        fr::vertex_3d(6.3, -7.245, -4.2),  // 1
        fr::vertex_3d(6.3, -7.245, -2.94),  // 2
        fr::vertex_3d(-6.3, -7.245, -2.94),  // 3
        fr::vertex_3d(-6.3, 5.355, -2.94),  // 4
        fr::vertex_3d(6.3, 5.355, -2.94),  // 5
        fr::vertex_3d(6.3, 5.355, -4.2),  // 6
        fr::vertex_3d(-6.3, 5.355, -4.2),  // 7
        fr::vertex_3d(-5.67, -6.615, -2.94),  // 8
        fr::vertex_3d(-4.41, -6.615, -2.94),  // 9
        fr::vertex_3d(-4.41, -6.615, 10.5),  // 10
        fr::vertex_3d(-5.67, -6.615, 10.5),  // 11
        fr::vertex_3d(-5.67, -5.355, 10.5),  // 12
        fr::vertex_3d(-4.41, -5.355, 10.5),  // 13
        fr::vertex_3d(-4.41, -5.355, -2.94),  // 14
        fr::vertex_3d(-5.67, -5.355, -2.94),  // 15
        fr::vertex_3d(-5.67, 3.465, -2.94),  // 16
        fr::vertex_3d(-4.41, 3.465, -2.94),  // 17
        fr::vertex_3d(-4.41, 3.465, 10.5),  // 18
        fr::vertex_3d(-5.67, 3.465, 10.5),  // 19
        fr::vertex_3d(-5.67, 4.725, 10.5),  // 20
        fr::vertex_3d(-4.41, 4.725, 10.5),  // 21
        fr::vertex_3d(-4.41, 4.725, -2.94),  // 22
        fr::vertex_3d(-5.67, 4.725, -2.94),  // 23
        fr::vertex_3d(4.41, -6.615, -2.94),  // 24
        fr::vertex_3d(5.67, -6.615, -2.94),  // 25
        fr::vertex_3d(5.67, -6.615, 10.5),  // 26
        fr::vertex_3d(4.41, -6.615, 10.5),  // 27
        fr::vertex_3d(4.41, -5.355, 10.5),  // 28
        fr::vertex_3d(5.67, -5.355, 10.5),  // 29
        fr::vertex_3d(5.67, -5.355, -2.94),  // 30
        fr::vertex_3d(4.41, -5.355, -2.94),  // 31
        fr::vertex_3d(4.41, 3.465, -2.94),  // 32
        fr::vertex_3d(5.67, 3.465, -2.94),  // 33
        fr::vertex_3d(5.67, 3.465, 10.5),  // 34
        fr::vertex_3d(4.41, 3.465, 10.5),  // 35
        fr::vertex_3d(4.41, 4.725, 10.5),  // 36
        fr::vertex_3d(5.67, 4.725, 10.5),  // 37
        fr::vertex_3d(5.67, 4.725, -2.94),  // 38
        fr::vertex_3d(4.41, 4.725, -2.94),  // 39
        fr::vertex_3d(-6.3, 4.095, -18.9),  // 40
        fr::vertex_3d(6.3, 4.095, -18.9),  // 41
        fr::vertex_3d(6.3, 4.095, -4.2),  // 42
        fr::vertex_3d(-6.3, 4.095, -4.2),  // 43
        fr::vertex_3d(6.3, 5.355, -18.9),  // 44
        fr::vertex_3d(-6.3, 5.355, -18.9)  // 45
    };

    constexpr inline fr::face_3d chair_faces[] = {
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 0, 1, 2, 3, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 5, 6, 7, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 1, 0, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 5, 4, 3, 2, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 4, 7, 0, 3, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 6, 5, 2, 1, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 40, 41, 42, 43, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 7, 6, 44, 45, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 45, 44, 41, 40, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 6, 7, 43, 42, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 45, 40, 43, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 44, 6, 42, 41, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 8, 9, 10, 11, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 14, 15, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 15, 14, 9, 8, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 13, 12, 11, 10, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 12, 15, 8, 11, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 14, 13, 10, 9, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 16, 17, 18, 19, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 23, 22, 17, 16, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 21, 20, 19, 18, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 20, 23, 16, 19, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 22, 21, 18, 17, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 24, 25, 26, 27, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 28, 29, 30, 31, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 31, 30, 25, 24, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 29, 28, 27, 26, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 28, 31, 24, 27, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 30, 29, 26, 25, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 32, 33, 34, 35, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 36, 37, 38, 39, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 39, 38, 33, 32, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 37, 36, 35, 34, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 36, 39, 32, 35, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 38, 37, 34, 33, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 43, 42, 41, 40, 9, -1)
    };

    constexpr inline fr::model_3d_item chair(chair_vertices, chair_faces);
}

#endif
