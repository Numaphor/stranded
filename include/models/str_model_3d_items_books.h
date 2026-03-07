#ifndef STR_MODEL_3D_ITEMS_BOOKS_H
#define STR_MODEL_3D_ITEMS_BOOKS_H

#include "fr_model_3d_item.h"

namespace str::model_3d_items
{
    constexpr inline fr::vertex_3d books_vertices[] = {
        fr::vertex_3d(-0.977904, 0.504, -4.156092),  // 0
        fr::vertex_3d(-0.977904, 3.78, -4.156092),  // 1
        fr::vertex_3d(-1.955807, 0.504, -3.911615),  // 2
        fr::vertex_3d(-1.955807, 3.78, -3.911615),  // 3
        fr::vertex_3d(-1.985904, 3.78, 0.0),  // 4
        fr::vertex_3d(-1.985904, 0.252, 0.0),  // 5
        fr::vertex_3d(-6.017904, 3.78, 0.0),  // 6
        fr::vertex_3d(-2.993904, 0.756, 0.0),  // 7
        fr::vertex_3d(-4.001904, 0.756, 0.0),  // 8
        fr::vertex_3d(-4.001904, 0.252, 0.0),  // 9
        fr::vertex_3d(-5.009904, 0.252, 0.0),  // 10
        fr::vertex_3d(-5.009904, 0.0, 0.0),  // 11
        fr::vertex_3d(-6.017904, 0.0, 0.0),  // 12
        fr::vertex_3d(-2.993904, 0.252, 0.0),  // 13
        fr::vertex_3d(-0.977904, 3.78, -0.0),  // 14
        fr::vertex_3d(0.0, 3.78, -0.244476),  // 15
        fr::vertex_3d(-5.009904, 0.0, -3.78),  // 16
        fr::vertex_3d(-5.009904, 0.252, -3.78),  // 17
        fr::vertex_3d(0.0, 0.504, -0.244476),  // 18
        fr::vertex_3d(-0.977904, 0.504, -0.0),  // 19
        fr::vertex_3d(-6.017904, 0.0, -3.78),  // 20
        fr::vertex_3d(-6.017904, 3.78, -3.78),  // 21
        fr::vertex_3d(-5.009904, 3.78, -3.78),  // 22
        fr::vertex_3d(-1.985904, 3.78, -4.032),  // 23
        fr::vertex_3d(-2.993904, 3.78, -3.528),  // 24
        fr::vertex_3d(-2.993904, 3.78, -4.032),  // 25
        fr::vertex_3d(-4.001904, 3.78, -3.528),  // 26
        fr::vertex_3d(-4.001904, 3.78, -4.032),  // 27
        fr::vertex_3d(-5.009904, 3.78, -4.032),  // 28
        fr::vertex_3d(-4.001904, 0.756, -3.528),  // 29
        fr::vertex_3d(-2.993904, 0.756, -3.528),  // 30
        fr::vertex_3d(-4.001904, 0.252, -4.032),  // 31
        fr::vertex_3d(-5.009904, 0.252, -4.032),  // 32
        fr::vertex_3d(-2.993904, 0.252, -4.032),  // 33
        fr::vertex_3d(-1.985904, 0.252, -4.032)  // 34
    };

    constexpr inline fr::face_3d books_faces[] = {
        fr::face_3d(books_vertices, fr::vertex_3d(-0.242536, 0.0, -0.970143), 0, 2, 3, 1, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 4, 6, 7, 5, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 7, 6, 9, 8, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 9, 6, 11, 10, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 6, 12, 11, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 5, 7, 13, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 14, 15, 1, 3, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 4, 23, 24, 6, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 23, 25, 24, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 24, 26, 21, 6, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 21, 26, 27, 22, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 27, 28, 22, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 16, 17, 10, 11, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.242536, 0.0, 0.970143), 15, 14, 19, 18, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 19, 2, 0, 18, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 12, 20, 16, 11, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 12, 6, 21, 20, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 16, 20, 22, 17, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 20, 21, 22, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.970143, 0.0, -0.242536), 0, 1, 15, 18, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(-0.970143, 0.0, 0.242536), 19, 14, 3, 2, 9, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 8, 29, 30, 7, 8, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 30, 29, 26, 24, 8, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 9, 31, 27, 29, 6, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 27, 26, 29, 6, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 9, 29, 8, 6, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 10, 17, 31, 9, 6, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 17, 32, 31, 6, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 31, 32, 28, 27, 6, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 17, 22, 28, 32, 6, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 13, 7, 30, 33, 2, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 33, 30, 24, 25, 2, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 34, 23, 4, 5, 2, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 13, 33, 34, 5, 2, -1),
        fr::face_3d(books_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 34, 33, 25, 23, 2, -1)
    };

    constexpr inline fr::model_3d_item books(books_vertices, books_faces);
}

#endif


