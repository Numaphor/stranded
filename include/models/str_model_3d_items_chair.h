#ifndef STR_MODEL_3D_ITEMS_CHAIR_H
#define STR_MODEL_3D_ITEMS_CHAIR_H

#include "fr_model_3d_item.h"

namespace str::model_3d_items
{
    constexpr inline fr::vertex_3d chair_vertices[] = {
        fr::vertex_3d(-6.3, -7.02, -2.43),  // 0
        fr::vertex_3d(6.3, -7.02, -2.43),  // 1
        fr::vertex_3d(6.3, 5.58, -2.43),  // 2
        fr::vertex_3d(-6.3, 5.58, -2.43),  // 3
        fr::vertex_3d(-6.3, 5.58, -3.69),  // 4
        fr::vertex_3d(6.3, 5.58, -3.69),  // 5
        fr::vertex_3d(6.3, -7.02, -3.69),  // 6
        fr::vertex_3d(-6.3, -7.02, -3.69),  // 7
        fr::vertex_3d(-5.67, -6.39, 11.01),  // 8
        fr::vertex_3d(-4.41, -6.39, 11.01),  // 9
        fr::vertex_3d(-4.41, 4.95, 11.01),  // 10
        fr::vertex_3d(-5.67, 4.95, 11.01),  // 11
        fr::vertex_3d(-5.67, -6.39, -2.43),  // 12
        fr::vertex_3d(-4.41, -6.39, -2.43),  // 13
        fr::vertex_3d(-4.41, 4.95, -2.43),  // 14
        fr::vertex_3d(-5.67, 4.95, -2.43),  // 15
        fr::vertex_3d(4.41, -6.39, 11.01),  // 16
        fr::vertex_3d(5.67, -6.39, 11.01),  // 17
        fr::vertex_3d(5.67, 4.95, 11.01),  // 18
        fr::vertex_3d(4.41, 4.95, 11.01),  // 19
        fr::vertex_3d(4.41, -6.39, -2.43),  // 20
        fr::vertex_3d(5.67, -6.39, -2.43),  // 21
        fr::vertex_3d(5.67, 4.95, -2.43),  // 22
        fr::vertex_3d(4.41, 4.95, -2.43),  // 23
        fr::vertex_3d(-6.3, 4.32, -3.69),  // 24
        fr::vertex_3d(6.3, 4.32, -3.69),  // 25
        fr::vertex_3d(6.3, 4.32, -18.39),  // 26
        fr::vertex_3d(-6.3, 4.32, -18.39)  // 27
    };

    constexpr inline fr::face_3d chair_faces[] = {
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 0, 1, 2, 3, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 5, 6, 7, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 7, 6, 1, 0, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 5, 4, 3, 2, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 4, 7, 0, 3, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 6, 5, 2, 1, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 24, 25, 26, 27, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 27, 26, 25, 24, 9, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 8, 9, 10, 11, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 13, 9, 8, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 14, 15, 11, 10, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 15, 12, 8, 11, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 13, 14, 10, 9, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 16, 17, 18, 19, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 20, 21, 17, 16, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 22, 23, 19, 18, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 23, 20, 16, 19, 8, -1),
        fr::face_3d(chair_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 21, 22, 18, 17, 8, -1)
    };

    constexpr inline fr::model_3d_item chair(chair_vertices, chair_faces);
}

#endif
