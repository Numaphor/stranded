#ifndef STR_MODEL_3D_ITEMS_POTTED_PLANT_H
#define STR_MODEL_3D_ITEMS_POTTED_PLANT_H
#include "fr_model_3d_item.h"
namespace str::model_3d_items
{
    constexpr inline fr::vertex_3d potted_plant_vertices[] = {
        fr::vertex_3d(-3.431539, -1.9812, -9.7216),  // 0
        fr::vertex_3d(-3.431539, 1.9812, -9.7216),  // 1
        fr::vertex_3d(-2.701999, -1.56, 0.0),  // 2
        fr::vertex_3d(-2.701999, 1.56, 0.0),  // 3
        fr::vertex_3d(-2.581966, -1.490699, -9.7216),  // 4
        fr::vertex_3d(-2.581966, -1.490699, -8.7296),  // 5
        fr::vertex_3d(-2.581966, 1.490699, -9.7216),  // 6
        fr::vertex_3d(-2.581966, 1.490699, -8.7296),  // 7
        fr::vertex_3d(0.0, 2.981398, -8.7296),  // 8
        fr::vertex_3d(0.0, 2.981398, -9.7216),  // 9
        fr::vertex_3d(-0.0, -2.981398, -8.7296),  // 10
        fr::vertex_3d(-0.0, -2.981398, -9.7216),  // 11
        fr::vertex_3d(2.581966, 1.490699, -8.7296),  // 12
        fr::vertex_3d(2.581966, 1.490699, -9.7216),  // 13
        fr::vertex_3d(2.581966, -1.490699, -9.7216),  // 14
        fr::vertex_3d(3.431539, -1.9812, -9.7216),  // 15
        fr::vertex_3d(-0.0, -3.9624, -9.7216),  // 16
        fr::vertex_3d(3.431539, 1.9812, -9.7216),  // 17
        fr::vertex_3d(0.0, 3.9624, -9.7216),  // 18
        fr::vertex_3d(0.0, 3.12, 0.0),  // 19
        fr::vertex_3d(-0.0, -3.12, 0.0),  // 20
        fr::vertex_3d(2.701999, -1.56, 0.0),  // 21
        fr::vertex_3d(2.581966, -1.490699, -8.7296),  // 22
        fr::vertex_3d(2.701999, 1.56, 0.0),  // 23
        fr::vertex_3d(4.241184, 0.0, -26.160672),  // 24
        fr::vertex_3d(0.0, 0.0, -4.7296),  // 25
        fr::vertex_3d(0.913705, 1.959552, -14.117032),  // 26
        fr::vertex_3d(-2.390653, 1.040393, -11.683252),  // 27
        fr::vertex_3d(0.0, 4.82924, -20.604468),  // 28
        fr::vertex_3d(2.390654, -1.040392, -11.683252),  // 29
        fr::vertex_3d(0.0, -4.82924, -20.604468),  // 30
        fr::vertex_3d(-0.913704, -1.959552, -14.117032),  // 31
        fr::vertex_3d(-4.241184, 0.0, -26.160672),  // 32
        fr::vertex_3d(-2.390653, -1.040392, -11.683252),  // 33
        fr::vertex_3d(-0.913704, 1.959552, -14.117032),  // 34
        fr::vertex_3d(0.913705, -1.959552, -14.117032),  // 35
        fr::vertex_3d(2.390654, 1.040393, -11.683252)  // 36
    };
    constexpr inline fr::face_3d potted_plant_faces[] = {
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.997196, 0.0, 0.074833), 0, 2, 3, 1, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 4, 6, 7, 5, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.5, -0.866025, 0.0), 7, 6, 9, 8, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.5, 0.866025, 0.0), 10, 11, 4, 5, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.5, -0.866025, 0.0), 8, 9, 13, 12, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 13, 15, 16, 14, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 14, 16, 4, 11, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 16, 0, 1, 4, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 15, 13, 18, 17, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 13, 9, 1, 18, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 9, 6, 4, 1, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.498598, 0.863597, 0.074833), 19, 18, 1, 3, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.498598, -0.863597, 0.074833), 20, 16, 15, 21, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 22, 12, 13, 14, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 23, 19, 20, 21, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 19, 3, 2, 20, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.498598, -0.863597, 0.074833), 2, 0, 16, 20, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.5, 0.866025, 0.0), 22, 14, 11, 10, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.498598, 0.863597, 0.074833), 23, 17, 18, 19, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.997196, 0.0, 0.074833), 15, 17, 23, 21, 9, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 12, 22, 10, 8, 8, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 8, 10, 5, 7, 8, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.886914, 0.42729, 0.175519), 25, 24, 26, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.886914, -0.42729, -0.175519), 25, 26, 24, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.395171, 0.878843, 0.26735), 27, 25, 28, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.395171, -0.878843, -0.26735), 27, 28, 25, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.395171, -0.878843, 0.26735), 29, 25, 30, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.395171, 0.878843, -0.26735), 29, 30, 25, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.886914, -0.42729, 0.175519), 31, 25, 32, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.886914, 0.42729, -0.175519), 31, 32, 25, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.395171, -0.878843, 0.26735), 25, 33, 30, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.395171, 0.878843, -0.26735), 25, 30, 33, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.886914, 0.42729, 0.175519), 32, 25, 34, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.886914, -0.42729, -0.175519), 32, 34, 25, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.886914, -0.42729, 0.175519), 35, 24, 25, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.886914, 0.42729, -0.175519), 35, 25, 24, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(0.395171, 0.878843, 0.26735), 25, 36, 28, 2, -1),
        fr::face_3d(potted_plant_vertices, fr::vertex_3d(-0.395171, -0.878843, -0.26735), 25, 28, 36, 2, -1)
    };
    constexpr inline fr::model_3d_item potted_plant(potted_plant_vertices, potted_plant_faces);
}
#endif
