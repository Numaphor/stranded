#ifndef STR_MODEL_3D_ITEMS_TABLE_H
#define STR_MODEL_3D_ITEMS_TABLE_H

#include "fr_model_3d_item.h"

namespace str::model_3d_items
{
    constexpr inline fr::vertex_3d table_vertices[] = {
        fr::vertex_3d(-12.0, -8.0, -20.0),
        fr::vertex_3d(12.0, -8.0, -20.0),
        fr::vertex_3d(12.0, -8.0, -18.4),
        fr::vertex_3d(-12.0, -8.0, -18.4),
        fr::vertex_3d(-12.0, 8.0, -20.0),
        fr::vertex_3d(12.0, 8.0, -20.0),
        fr::vertex_3d(12.0, 8.0, -18.4),
        fr::vertex_3d(-12.0, 8.0, -18.4),
        fr::vertex_3d(-11.2, -7.2, -18.4),
        fr::vertex_3d(-9.6, -7.2, -18.4),
        fr::vertex_3d(-9.6, -7.2, 0.0),
        fr::vertex_3d(-11.2, -7.2, 0.0),
        fr::vertex_3d(-11.2, -5.6, -18.4),
        fr::vertex_3d(-9.6, -5.6, -18.4),
        fr::vertex_3d(-9.6, -5.6, 0.0),
        fr::vertex_3d(-11.2, -5.6, 0.0),
        fr::vertex_3d(-11.2, 5.6, -18.4),
        fr::vertex_3d(-9.6, 5.6, -18.4),
        fr::vertex_3d(-9.6, 5.6, 0.0),
        fr::vertex_3d(-11.2, 5.6, 0.0),
        fr::vertex_3d(-11.2, 7.2, -18.4),
        fr::vertex_3d(-9.6, 7.2, -18.4),
        fr::vertex_3d(-9.6, 7.2, 0.0),
        fr::vertex_3d(-11.2, 7.2, 0.0),
        fr::vertex_3d(9.6, -7.2, -18.4),
        fr::vertex_3d(11.2, -7.2, -18.4),
        fr::vertex_3d(11.2, -7.2, 0.0),
        fr::vertex_3d(9.6, -7.2, 0.0),
        fr::vertex_3d(9.6, -5.6, -18.4),
        fr::vertex_3d(11.2, -5.6, -18.4),
        fr::vertex_3d(11.2, -5.6, 0.0),
        fr::vertex_3d(9.6, -5.6, 0.0),
        fr::vertex_3d(9.6, 5.6, -18.4),
        fr::vertex_3d(11.2, 5.6, -18.4),
        fr::vertex_3d(11.2, 5.6, 0.0),
        fr::vertex_3d(9.6, 5.6, 0.0),
        fr::vertex_3d(9.6, 7.2, -18.4),
        fr::vertex_3d(11.2, 7.2, -18.4),
        fr::vertex_3d(11.2, 7.2, 0.0),
        fr::vertex_3d(9.6, 7.2, 0.0)
    };

    constexpr inline fr::face_3d table_faces[] = {
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 0, 1, 2, 3, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 7, 6, 5, 4, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 4, 5, 1, 0, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 6, 7, 3, 2, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 7, 4, 0, 3, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 5, 6, 2, 1, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 8, 9, 10, 11, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 15, 14, 13, 12, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 12, 13, 9, 8, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 14, 15, 11, 10, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 15, 12, 8, 11, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 13, 14, 10, 9, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 16, 17, 18, 19, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 23, 22, 21, 20, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 20, 21, 17, 16, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 22, 23, 19, 18, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 23, 20, 16, 19, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 21, 22, 18, 17, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 24, 25, 26, 27, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 31, 30, 29, 28, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 28, 29, 25, 24, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 30, 31, 27, 26, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 31, 28, 24, 27, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 29, 30, 26, 25, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 32, 33, 34, 35, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 39, 38, 37, 36, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 36, 37, 33, 32, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 38, 39, 35, 34, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 39, 36, 32, 35, 6, -1),
        fr::face_3d(table_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 37, 38, 34, 33, 6, -1)
    };

    constexpr inline fr::model_3d_item table(table_vertices, table_faces);
}

#endif
