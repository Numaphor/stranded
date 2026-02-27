#ifndef STR_MODEL_3D_ITEMS_TABLE_H
#define STR_MODEL_3D_ITEMS_TABLE_H

#include "fr_model_3d_item.h"

namespace str::model_3d_items
{
    constexpr inline fr::vertex_3d table_vertices[] = {
        fr::vertex_3d(-12.6, -8.4, -6.16),  // 0
        fr::vertex_3d(12.6, -8.4, -6.16),  // 1
        fr::vertex_3d(12.6, 8.4, -6.16),  // 2
        fr::vertex_3d(-12.6, 8.4, -6.16),  // 3
        fr::vertex_3d(-12.6, 8.4, -7.84),  // 4
        fr::vertex_3d(12.6, 8.4, -7.84),  // 5
        fr::vertex_3d(12.6, -8.4, -7.84),  // 6
        fr::vertex_3d(-12.6, -8.4, -7.84),  // 7
        fr::vertex_3d(-11.76, -7.56, 13.16),  // 8
        fr::vertex_3d(-10.08, -7.56, 13.16),  // 9
        fr::vertex_3d(-10.08, 7.56, 13.16),  // 10
        fr::vertex_3d(-11.76, 7.56, 13.16),  // 11
        fr::vertex_3d(-11.76, -7.56, -6.16),  // 12
        fr::vertex_3d(-10.08, -7.56, -6.16),  // 13
        fr::vertex_3d(-10.08, 7.56, -6.16),  // 14
        fr::vertex_3d(-11.76, 7.56, -6.16),  // 15
        fr::vertex_3d(10.08, -7.56, 13.16),  // 16
        fr::vertex_3d(11.76, -7.56, 13.16),  // 17
        fr::vertex_3d(11.76, 7.56, 13.16),  // 18
        fr::vertex_3d(10.08, 7.56, 13.16),  // 19
        fr::vertex_3d(10.08, -7.56, -6.16),  // 20
        fr::vertex_3d(11.76, -7.56, -6.16),  // 21
        fr::vertex_3d(11.76, 7.56, -6.16),  // 22
        fr::vertex_3d(10.08, 7.56, -6.16)  // 23
    };

    constexpr inline fr::face_3d table_faces[] = {
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 8, 9, 10, 11, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 13, 9, 8, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 14, 15, 11, 10, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 15, 12, 8, 11, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 13, 14, 10, 9, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 16, 17, 18, 19, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 20, 21, 17, 16, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 22, 23, 19, 18, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 23, 20, 16, 19, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 21, 22, 18, 17, 8, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 0, 1, 2, 3, 9, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 5, 6, 7, 9, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 7, 6, 1, 0, 9, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 5, 4, 3, 2, 9, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 4, 7, 0, 3, 9, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 6, 5, 2, 1, 9, -1)
    };

    constexpr inline fr::model_3d_item table(table_vertices, table_faces);
}

#endif
