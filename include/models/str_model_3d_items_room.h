#ifndef STR_MODEL_3D_ITEMS_ROOM_H
#define STR_MODEL_3D_ITEMS_ROOM_H

#include "fr_model_3d_item.h"
#include "bn_color.h"

namespace str::model_3d_items
{
    constexpr inline bn::color room_model_colors[] = {
        bn::color(22, 16, 8),  // floor_light: rgb(0.710, 0.520, 0.260)
        bn::color(16, 10, 4),  // floor_dark: rgb(0.520, 0.320, 0.130)
        bn::color(28, 26, 22),  // wall: rgb(0.900, 0.840, 0.710)
        bn::color(22, 20, 16),  // wall_shadow: rgb(0.710, 0.650, 0.520)
        bn::color(16, 22, 28),  // window_glass: rgb(0.520, 0.710, 0.900)
        bn::color(10, 8, 6),  // window_frame: rgb(0.320, 0.260, 0.190)
        bn::color(18, 12, 6),  // table_wood: rgb(0.580, 0.390, 0.190)
        bn::color(8, 12, 18),  // chair_fabric: rgb(0.260, 0.390, 0.580)
        bn::color(20, 14, 8)  // chair_frame: rgb(0.650, 0.450, 0.260)
    };

    constexpr inline fr::vertex_3d room_vertices[] = {
        fr::vertex_3d(-60.0, -60.0, 0.0),
        fr::vertex_3d(-20.0, -60.0, 0.0),
        fr::vertex_3d(-20.0, 60.0, 0.0),
        fr::vertex_3d(-60.0, 60.0, 0.0),
        fr::vertex_3d(-20.0, -60.0, 0.0),
        fr::vertex_3d(20.0, -60.0, 0.0),
        fr::vertex_3d(20.0, 60.0, 0.0),
        fr::vertex_3d(-20.0, 60.0, 0.0),
        fr::vertex_3d(20.0, -60.0, 0.0),
        fr::vertex_3d(60.0, -60.0, 0.0),
        fr::vertex_3d(60.0, 60.0, 0.0),
        fr::vertex_3d(20.0, 60.0, 0.0),
        fr::vertex_3d(-60.0, -60.0, 0.0),
        fr::vertex_3d(-11.2, -60.0, 0.0),
        fr::vertex_3d(-11.2, -60.0, -50.0),
        fr::vertex_3d(-60.0, -60.0, -50.0),
        fr::vertex_3d(11.2, -60.0, 0.0),
        fr::vertex_3d(60.0, -60.0, 0.0),
        fr::vertex_3d(60.0, -60.0, -50.0),
        fr::vertex_3d(11.2, -60.0, -50.0),
        fr::vertex_3d(-11.2, -60.0, 0.0),
        fr::vertex_3d(11.2, -60.0, 0.0),
        fr::vertex_3d(11.2, -60.0, -19.0),
        fr::vertex_3d(-11.2, -60.0, -19.0),
        fr::vertex_3d(-11.2, -60.0, -33.0),
        fr::vertex_3d(11.2, -60.0, -33.0),
        fr::vertex_3d(11.2, -60.0, -50.0),
        fr::vertex_3d(-11.2, -60.0, -50.0),
        fr::vertex_3d(-11.2, -62.0, -19.0),
        fr::vertex_3d(11.2, -62.0, -19.0),
        fr::vertex_3d(11.2, -62.0, -33.0),
        fr::vertex_3d(-11.2, -62.0, -33.0),
        fr::vertex_3d(-11.2, -60.0, -19.0),
        fr::vertex_3d(11.2, -60.0, -19.0),
        fr::vertex_3d(11.2, -62.0, -19.0),
        fr::vertex_3d(-11.2, -62.0, -19.0),
        fr::vertex_3d(-11.2, -60.0, -33.0),
        fr::vertex_3d(11.2, -60.0, -33.0),
        fr::vertex_3d(11.2, -62.0, -33.0),
        fr::vertex_3d(-11.2, -62.0, -33.0),
        fr::vertex_3d(-11.2, -60.0, -19.0),
        fr::vertex_3d(-11.2, -60.0, -33.0),
        fr::vertex_3d(-11.2, -62.0, -33.0),
        fr::vertex_3d(-11.2, -62.0, -19.0),
        fr::vertex_3d(11.2, -60.0, -19.0),
        fr::vertex_3d(11.2, -60.0, -33.0),
        fr::vertex_3d(11.2, -62.0, -33.0),
        fr::vertex_3d(11.2, -62.0, -19.0),
        fr::vertex_3d(-60.0, -60.0, 0.0),
        fr::vertex_3d(-60.0, 60.0, 0.0),
        fr::vertex_3d(-60.0, 60.0, -50.0),
        fr::vertex_3d(-60.0, -60.0, -50.0),
        fr::vertex_3d(60.0, 60.0, -50.0)
    };

    constexpr inline fr::face_3d room_faces[] = {
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 3, 2, 1, 0, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 7, 6, 5, 4, 1, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 11, 10, 9, 8, 0, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 12, 13, 14, 15, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 16, 17, 18, 19, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 20, 21, 22, 23, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 24, 25, 26, 27, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 1.0, 0.0), 28, 29, 30, 31, 4, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, 1.0), 35, 34, 33, 32, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, 0.0, -1.0), 36, 37, 38, 39, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 43, 42, 41, 40, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 44, 45, 46, 47, 5, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(1.0, 0.0, 0.0), 51, 50, 49, 48, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(-1.0, 0.0, 0.0), 9, 10, 52, 18, 2, -1),
        fr::face_3d(room_vertices, fr::vertex_3d(0.0, -1.0, 0.0), 3, 10, 52, 50, 2, -1)
    };

    constexpr inline fr::model_3d_item room(room_vertices, room_faces);
}

#endif
