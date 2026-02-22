#ifndef STR_MODEL_3D_ITEMS_TABLE_H
#define STR_MODEL_3D_ITEMS_TABLE_H

#include "fr_model_3d_item.h"

namespace str::model_3d_items
{
    constexpr inline fr::vertex_3d table_vertices[] = {
        fr::vertex_3d(-12.6, -8.4, -9.24),  // 0
        fr::vertex_3d(12.6, -8.4, -9.24),  // 1
        fr::vertex_3d(12.6, -8.4, -7.56),  // 2
        fr::vertex_3d(-12.6, -8.4, -7.56),  // 3
        fr::vertex_3d(-12.6, 8.4, -7.56),  // 4
        fr::vertex_3d(12.6, 8.4, -7.56),  // 5
        fr::vertex_3d(12.6, 8.4, -9.24),  // 6
        fr::vertex_3d(-12.6, 8.4, -9.24),  // 7
        fr::vertex_3d(-11.76, -7.56, -7.56),  // 8
        fr::vertex_3d(-10.08, -7.56, -7.56),  // 9
        fr::vertex_3d(-10.08, -7.56, 11.76),  // 10
        fr::vertex_3d(-11.76, -7.56, 11.76),  // 11
        fr::vertex_3d(-11.76, -5.88, 11.76),  // 12
        fr::vertex_3d(-10.08, -5.88, 11.76),  // 13
        fr::vertex_3d(-10.08, -5.88, -7.56),  // 14
        fr::vertex_3d(-11.76, -5.88, -7.56),  // 15
        fr::vertex_3d(-11.76, 5.88, -7.56),  // 16
        fr::vertex_3d(-10.08, 5.88, -7.56),  // 17
        fr::vertex_3d(-10.08, 5.88, 11.76),  // 18
        fr::vertex_3d(-11.76, 5.88, 11.76),  // 19
        fr::vertex_3d(-11.76, 7.56, 11.76),  // 20
        fr::vertex_3d(-10.08, 7.56, 11.76),  // 21
        fr::vertex_3d(-10.08, 7.56, -7.56),  // 22
        fr::vertex_3d(-11.76, 7.56, -7.56),  // 23
        fr::vertex_3d(10.08, -7.56, -7.56),  // 24
        fr::vertex_3d(11.76, -7.56, -7.56),  // 25
        fr::vertex_3d(11.76, -7.56, 11.76),  // 26
        fr::vertex_3d(10.08, -7.56, 11.76),  // 27
        fr::vertex_3d(10.08, -5.88, 11.76),  // 28
        fr::vertex_3d(11.76, -5.88, 11.76),  // 29
        fr::vertex_3d(11.76, -5.88, -7.56),  // 30
        fr::vertex_3d(10.08, -5.88, -7.56),  // 31
        fr::vertex_3d(10.08, 5.88, -7.56),  // 32
        fr::vertex_3d(11.76, 5.88, -7.56),  // 33
        fr::vertex_3d(11.76, 5.88, 11.76),  // 34
        fr::vertex_3d(10.08, 5.88, 11.76),  // 35
        fr::vertex_3d(10.08, 7.56, 11.76),  // 36
        fr::vertex_3d(11.76, 7.56, 11.76),  // 37
        fr::vertex_3d(11.76, 7.56, -7.56),  // 38
        fr::vertex_3d(10.08, 7.56, -7.56)  // 39
    };

    constexpr inline fr::face_3d table_faces[] = {
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 0, 1, 2, 3, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 5, 6, 7, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 1, 0, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 5, 4, 3, 2, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 4, 7, 0, 3, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 6, 5, 2, 1, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 8, 9, 10, 11, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 14, 15, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 15, 14, 9, 8, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 13, 12, 11, 10, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 12, 15, 8, 11, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 14, 13, 10, 9, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 16, 17, 18, 19, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 23, 22, 17, 16, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 21, 20, 19, 18, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 20, 23, 16, 19, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 22, 21, 18, 17, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 24, 25, 26, 27, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 28, 29, 30, 31, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 31, 30, 25, 24, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 29, 28, 27, 26, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 28, 31, 24, 27, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 30, 29, 26, 25, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 33, 34, 35, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 36, 37, 38, 39, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 39, 38, 33, 32, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 37, 36, 35, 34, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 36, 39, 32, 35, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 38, 37, 34, 33, 6, -1)
    };

    constexpr inline fr::model_3d_item table(table_vertices, table_faces);
}

#endif
