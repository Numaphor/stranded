/*
 * Copyright (c) 2020-2026 Gustavo Valiente gustavo.valiente@protonmail.com
 * zlib License, see LICENSE file.
 */

#include "fr_models_3d.h"

#include "bn_profiler.h"
#include "../../butano/butano/hw/include/bn_hw_sprites.h"
#include "../../butano/butano/hw/include/bn_hw_common.h"

#include "fr_div_lut.h"
#include "fr_camera_3d.h"
#include "fr_sprite_3d_item.h"

#if FR_DETAILED_PROFILE
    #define FR_PROFILER_START BN_PROFILER_START
    #define FR_PROFILER_STOP BN_PROFILER_STOP
#else
    #define FR_PROFILER_START(id) \
        do \
        { \
        } while(false)

    #define FR_PROFILER_STOP() \
        do \
        { \
        } while(false)
#endif

#ifndef FR_MAX_RENDER_VISIBLE_FACES
    #define FR_MAX_RENDER_VISIBLE_FACES 0
#endif

namespace fr
{

namespace
{
    constexpr int fixed_precision = 18;
    using fixed = bn::fixed_t<fixed_precision>;
    constexpr int room_back_layer_bias = 1000000;
    constexpr int room_front_layer_bias = -1000000;
    constexpr bn::fixed room_near_wall_cull_normal_y_max = bn::fixed(-0.2);
    constexpr int projected_face_min_area2 = 24;
    constexpr int div_lut_max_index = 1024 * 4 - 1;
}

void models_3d::_process_models(const camera_3d& camera)
{
    constexpr int display_width = bn::display::width();
    constexpr int display_height = bn::display::height();
    constexpr int focal_length_shift = constants_3d::focal_length_shift;
    constexpr int near_plane = 24 * 256 * 16;

    // These arrays use BN_DATA_EWRAM_BSS to avoid IWRAM overflow.
    // Stranded uses more IWRAM static code/data than varooom-3d standalone.
    static BN_DATA_EWRAM_BSS point_2d _projected_vertices[_max_vertices];
    static BN_DATA_EWRAM_BSS bool _projected_vertices_valid[_max_vertices];
    static BN_DATA_EWRAM_BSS valid_face_info _valid_faces_info[_max_faces];
    static BN_DATA_EWRAM_BSS int _visible_face_projected_zs[_max_faces];
    static BN_DATA_EWRAM_BSS uint16_t _visible_face_indexes[_max_faces];

    point_3d camera_position = camera.position();
    bn::fixed camera_phi = camera.phi();
    bn::fixed camera_u_x = camera.u().x();
    bn::fixed camera_u_z = camera.u().z();
    bn::fixed camera_v_x = camera.v().x();
    bn::fixed camera_v_z = camera.v().z();
    visible_face_info* visible_faces = _visible_faces_info;
    int visible_faces_count = 0;
    bool geometry_cache_hit = false;

    if(_geometry_cache_valid && _cached_static_model_items_ptr == _static_model_items_ptr &&
       _cached_static_models_count == _static_models_count &&
       _cached_dynamic_models_generation == _dynamic_models_generation &&
       _cached_camera_position == camera_position && _cached_camera_phi == camera_phi)
    {
        int cached_dynamic_index = 0;
        geometry_cache_hit = true;

        for(const model_3d& model : _dynamic_models_list)
        {
            if(cached_dynamic_index >= _cached_dynamic_models_count ||
               _cached_dynamic_models[cached_dynamic_index] != &model ||
               _cached_dynamic_model_versions[cached_dynamic_index] != model.version())
            {
                geometry_cache_hit = false;
                break;
            }

            ++cached_dynamic_index;
        }

        if(geometry_cache_hit && cached_dynamic_index != _cached_dynamic_models_count)
        {
            geometry_cache_hit = false;
        }
    }

    if(! geometry_cache_hit)
    {
        int global_vertex_index = 0;
        int valid_faces_count = 0;

        // Project static models:

        FR_PROFILER_START("static_project");

        for(int static_model_index = _static_models_count - 1; static_model_index >= 0; --static_model_index)
        {
            const model_3d_item* model_item = _static_model_items_ptr[static_model_index];
            const vertex_3d* model_vertices = model_item->vertices().data();
            point_2d* projected_vertices = _projected_vertices + global_vertex_index;
            bool* projected_vertices_valid = _projected_vertices_valid + global_vertex_index;
            int model_vertices_count = model_item->vertices().size();

            for(int index = 0; index < model_vertices_count; ++index)
            {
                const point_3d& model_point = model_vertices[index].point();
                bn::fixed vry = model_point.y() - camera_position.y();
                int vcz = -vry.data();
                int div_lut_index = vcz >> 10;
                bool vertex_valid = near_plane <= vcz && div_lut_index <= div_lut_max_index;
                projected_vertices_valid[index] = vertex_valid;

                if(vertex_valid) [[likely]]
                {
                    bn::fixed vrx = (model_point.x() - camera_position.x()) / 16;
                    bn::fixed vrz = (model_point.z() - camera_position.z()) / 16;
                    int vcx = (vrx.unsafe_multiplication(camera_u_x) + vrz.unsafe_multiplication(camera_u_z)).data();
                    int vcy = -(vrx.unsafe_multiplication(camera_v_x) + vrz.unsafe_multiplication(camera_v_z)).data();

                    // int scale = (1 << (focal_length_shift + 16 + 4)) / vcz;
                    auto scale = int((div_lut_ptr[div_lut_index] << (focal_length_shift - 8)) >> 6);

                    projected_vertices[index] = {
                        int16_t(((vcx * scale) >> 16) + (display_width / 2)),
                        int16_t(((vcy * scale) >> 16) + (display_height / 2))
                    };
                }
            }

            const face_3d* model_faces = model_item->faces().data();
            int model_faces_count = model_item->faces().size();

            for(int index = model_faces_count - 1; index >= 0; --index)
            {
                const face_3d& face = model_faces[index];
                int first_vertex_index = face.first_vertex_index();
                int second_vertex_index = face.second_vertex_index();
                int third_vertex_index = face.third_vertex_index();
                int fourth_vertex_index = face.fourth_vertex_index();

                if(projected_vertices_valid[first_vertex_index] &&
                   projected_vertices_valid[second_vertex_index] &&
                   projected_vertices_valid[third_vertex_index] &&
                   projected_vertices_valid[fourth_vertex_index])
                {
                    const point_3d& centroid = face.centroid().point();
                    const point_3d& normal = face.normal().point();
                    point_3d vr = centroid - camera_position;

                    if(vr.safe_dot_product(normal) < 0) [[likely]]
                    {
                        int projected_z = -vr.y().data();

                        _valid_faces_info[valid_faces_count] = {
                            &face, projected_vertices, projected_z
                        };

                        ++valid_faces_count;
                    }
                }
            }

            global_vertex_index += model_vertices_count;
        }

        FR_PROFILER_STOP();

        // Project dynamic models:

        FR_PROFILER_START("dynamic_project");

        for(model_3d& model : _dynamic_models_list)
        {
            const model_3d_item& model_item = model.item();
            const vertex_3d* model_vertices = model_item.vertices().data();
            point_2d* projected_vertices = _projected_vertices + global_vertex_index;
            bool* projected_vertices_valid = _projected_vertices_valid + global_vertex_index;
            int model_vertices_count = model_item.vertices().size();
            bool model_double_sided = model.double_sided();
            model.update();

            for(int index = 0; index < model_vertices_count; ++index)
            {
                point_3d model_point = model.transform(model_vertices[index]);
                bn::fixed vry = model_point.y() - camera_position.y();
                int vcz = -vry.data();
                int div_lut_index = vcz >> 10;
                bool vertex_valid = near_plane <= vcz && div_lut_index <= div_lut_max_index;
                projected_vertices_valid[index] = vertex_valid;

                if(vertex_valid) [[likely]]
                {
                    bn::fixed vrx = (model_point.x() - camera_position.x()) / 16;
                    bn::fixed vrz = (model_point.z() - camera_position.z()) / 16;
                    int vcx = (vrx.unsafe_multiplication(camera_u_x) + vrz.unsafe_multiplication(camera_u_z)).data();
                    int vcy = -(vrx.unsafe_multiplication(camera_v_x) + vrz.unsafe_multiplication(camera_v_z)).data();

                    // int scale = (1 << (focal_length_shift + 16 + 4)) / vcz;
                    auto scale = int((div_lut_ptr[div_lut_index] << (focal_length_shift - 8)) >> 6);

                    projected_vertices[index] = {
                        int16_t(((vcx * scale) >> 16) + (display_width / 2)),
                        int16_t(((vcy * scale) >> 16) + (display_height / 2))
                    };
                }
            }

            const face_3d* model_faces = model_item.faces().data();
            int model_faces_count = model_item.faces().size();

            for(int index = model_faces_count - 1; index >= 0; --index)
            {
                const face_3d& face = model_faces[index];
                int first_vertex_index = face.first_vertex_index();
                int second_vertex_index = face.second_vertex_index();
                int third_vertex_index = face.third_vertex_index();
                int fourth_vertex_index = face.fourth_vertex_index();

                if(projected_vertices_valid[first_vertex_index] &&
                   projected_vertices_valid[second_vertex_index] &&
                   projected_vertices_valid[third_vertex_index] &&
                   projected_vertices_valid[fourth_vertex_index])
                {
                    point_3d centroid = model.transform(face.centroid());
                    point_3d normal = model.rotate(face.normal());
                    point_3d vr = centroid - camera_position;
                    bool front_facing = vr.safe_dot_product(normal) < 0;
                    int color_index = face.color_index();
                    bool room_floor_surface = color_index >= 0 && color_index <= 5;
                    bool room_shell_surface = color_index >= 6 && color_index <= 8;
                    bool room_main_wall_surface = color_index == 6;
                    bool room_perspective_mode = model.mode() == model_3d::layering_mode::room_perspective;
                    bool room_floor_only_mode = model.mode() == model_3d::layering_mode::room_floor_only;
                    bool near_shell_surface = false;
                    bool render_face = false;

                    if(room_perspective_mode && room_shell_surface)
                    {
                        near_shell_surface = normal.y() < room_near_wall_cull_normal_y_max;
                    }

                    if(room_floor_surface)
                    {
                        // Room floor surfaces are visible in perspective and floor-only modes.
                        render_face = (room_perspective_mode || room_floor_only_mode) && front_facing;
                    }
                    else if(room_shell_surface && room_floor_only_mode)
                    {
                        // Floor-only preview mode hides room walls/windows/door frames.
                        render_face = false;
                    }
                    else if(near_shell_surface)
                    {
                        // Hide shell surfaces whose outward normal points toward the camera.
                        render_face = false;
                    }
                    else
                    {
                        bool allow_double_sided = model_double_sided;

                        if(room_shell_surface)
                        {
                            // Keep only the main room shell double-sided so thin door-frame
                            // faces don't leak through walls as horizontal/diagonal bands.
                            allow_double_sided = room_main_wall_surface;
                        }

                        render_face = front_facing || allow_double_sided;
                    }

                    if(render_face) [[likely]]
                    {
                        int projected_z = -vr.y().data() + model.depth_bias();

                        if(room_perspective_mode)
                        {
                            // Keep room shell generally behind entities, but allow the
                            // remaining near-half shell to overlay the player if shown.
                            projected_z += near_shell_surface ? room_front_layer_bias : room_back_layer_bias;
                        }

                        _valid_faces_info[valid_faces_count] = {
                            &face, projected_vertices, projected_z
                        };

                        ++valid_faces_count;
                    }
                }
            }

            global_vertex_index += model_vertices_count;
        }

        FR_PROFILER_STOP();

        // Cull valid faces:

        FR_PROFILER_START("cull_valid_faces");

        for(int face_index = valid_faces_count - 1; face_index >= 0; --face_index)
        {
            const valid_face_info& valid_face = _valid_faces_info[face_index];
            const face_3d* face = valid_face.face;
            const point_2d* projected_vertices = valid_face.projected_vertices;
            const point_2d& pv0 = projected_vertices[face->first_vertex_index()];
            const point_2d& pv1 = projected_vertices[face->second_vertex_index()];
            const point_2d& pv2 = projected_vertices[face->third_vertex_index()];
            const point_2d& pv3 = projected_vertices[face->fourth_vertex_index()];
            int16_t minimum_x = pv0.x;
            int16_t maximum_x = minimum_x;

            auto min_max_x = [&minimum_x, &maximum_x](int16_t value)
            {
                if(value < minimum_x)
                {
                    minimum_x = value;
                }
                else if(value > maximum_x)
                {
                    maximum_x = value;
                }
            };

            min_max_x(pv1.x);
            min_max_x(pv2.x);
            min_max_x(pv3.x);

            if(minimum_x < display_width && maximum_x >= 0) [[likely]]
            {
                auto tri_area2 = [](const point_2d& a, const point_2d& b, const point_2d& c) {
                    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
                };
                auto same_sign = [](int a, int b) {
                    return (a > 0 && b > 0) || (a < 0 && b < 0);
                };

                bool stable_projection = true;

                if(face->triangle())
                {
                    int area2 = tri_area2(pv0, pv1, pv2);
                    stable_projection = bn::abs(area2) >= projected_face_min_area2;
                }
                else
                {
                    int area012 = tri_area2(pv0, pv1, pv2);
                    int area023 = tri_area2(pv0, pv2, pv3);
                    int edge01 = tri_area2(pv0, pv1, pv2);
                    int edge12 = tri_area2(pv1, pv2, pv3);
                    int edge23 = tri_area2(pv2, pv3, pv0);
                    int edge30 = tri_area2(pv3, pv0, pv1);

                    stable_projection =
                        bn::abs(area012) >= projected_face_min_area2 &&
                        bn::abs(area023) >= projected_face_min_area2 &&
                        same_sign(area012, area023) &&
                        same_sign(edge01, edge12) &&
                        same_sign(edge12, edge23) &&
                        same_sign(edge23, edge30);
                }

                if(! stable_projection) [[unlikely]]
                {
                    continue;
                }

                int16_t minimum_y = pv0.y;
                int16_t maximum_y = minimum_y;
                int top_index = 0;

                auto min_max_y = [&minimum_y, &maximum_y, &top_index](int index, int16_t value)
                {
                    if(value < minimum_y)
                    {
                        top_index = index;
                        minimum_y = value;
                    }
                    else if(value > maximum_y)
                    {
                        maximum_y = value;
                    }
                };

                min_max_y(1, pv1.y);
                min_max_y(2, pv2.y);
                min_max_y(3, pv3.y);

                if(minimum_y < display_height && maximum_y >= 0)
                {
                    visible_faces[visible_faces_count] = {
                        &valid_face, top_index, minimum_x, maximum_x, minimum_y, maximum_y
                    };

                    _visible_face_projected_zs[visible_faces_count] = valid_face.projected_z;
                    _visible_face_indexes[visible_faces_count] = visible_faces_count;
                    ++visible_faces_count;
                }
            }
        }

        FR_PROFILER_STOP();

        _cached_camera_position = camera_position;
        _cached_camera_phi = camera_phi;
        _cached_static_model_items_ptr = _static_model_items_ptr;
        _cached_static_models_count = _static_models_count;
        _cached_dynamic_models_generation = _dynamic_models_generation;
        _cached_geometry_visible_faces_count = visible_faces_count;

        int cached_dynamic_index = 0;
        for(const model_3d& model : _dynamic_models_list)
        {
            _cached_dynamic_models[cached_dynamic_index] = &model;
            _cached_dynamic_model_versions[cached_dynamic_index] = model.version();
            ++cached_dynamic_index;
        }

        _cached_dynamic_models_count = cached_dynamic_index;
        _geometry_cache_valid = true;
    }
    else
    {
        visible_faces_count = _cached_geometry_visible_faces_count;

        for(int visible_face_index = 0; visible_face_index < visible_faces_count; ++visible_face_index)
        {
            _visible_face_indexes[visible_face_index] = visible_face_index;
        }
    }

    // Project and cull sprites:

    FR_PROFILER_START("sprites");

    for(sprite_3d& sprite : _sprites_list)
    {
        const point_3d& sprite_position = sprite.position();
        bn::fixed vry = sprite_position.y() - camera_position.y();
        int vcz = -vry.data();

        int div_lut_index = vcz >> 10;

        if(near_plane <= vcz && div_lut_index <= div_lut_max_index) [[likely]]
        {
            sprite_3d_item& sprite_item = sprite.item();
            int px_size = sprite_item.pixel_size();
            int canvas_size = px_size * 2;

            bn::fixed vrx = (sprite_position.x() - camera_position.x()) / 16;
            bn::fixed vrz = (sprite_position.z() - camera_position.z()) / 16;
            int vcx = (vrx.unsafe_multiplication(camera_u_x) + vrz.unsafe_multiplication(camera_u_z)).data();

            auto sprite_scale = int((div_lut_ptr[div_lut_index] << (focal_length_shift - 8)) >> 3);
            auto scale = sprite_scale >> 3;
            int sprite_x = ((vcx * scale) >> 16) + (display_width / 2) - px_size;

            if(sprite_x < display_width && sprite_x + canvas_size > 0) [[likely]]
            {
                int vcy = -(vrx.unsafe_multiplication(camera_v_x) + vrz.unsafe_multiplication(camera_v_z)).data();
                int sprite_y = ((vcy * scale) >> 16) + (display_height / 2) - px_size;

                if(sprite_y < display_height && sprite_y + canvas_size > 0) [[likely]]
                {
                    bn::fixed affine_scale = bn::fixed::from_data(sprite_scale).unsafe_multiplication(sprite.scale());

                    if(affine_scale > 0) [[likely]]
                    {
                        int degrees = (camera_phi + sprite.theta()).shift_integer() * 360;
                        bn::fixed rotation_angle = bn::fixed::from_data(degrees >> 4);

                        if(rotation_angle >= 360)
                        {
                            rotation_angle -= 360;
                        }

                        bn::sprite_affine_mat_ptr& affine_mat = sprite_item.affine_mat();
                        affine_mat.set_scale(affine_scale);
                        affine_mat.set_rotation_angle(rotation_angle);
                        affine_mat.set_horizontal_flip(sprite.horizontal_flip());

                        int attr0 = bn::hw::sprites::first_attributes(
                                    sprite_y, bn::sprite_shape::SQUARE, bn::bpp_mode::BPP_4, 3 << 8,
                                    false, false, false, false);
                        int attr1 = bn::hw::sprites::second_attributes(
                                    sprite_x, sprite_item.sprite_size(), sprite_item.affine_mat_id());
                        int attr2 = bn::hw::sprites::third_attributes(
                                    sprite_item.tiles_id(), sprite_item.palette_id(), 3);

                        visible_faces[visible_faces_count] = {
                            nullptr, sprite_y, int16_t(attr0), int16_t(attr1), int16_t(attr2), int16_t(canvas_size)
                        };

                        _visible_face_projected_zs[visible_faces_count] = vcz;
                        _visible_face_indexes[visible_faces_count] = visible_faces_count;
                        ++visible_faces_count;
                    }
                }
            }
        }
    }

    FR_PROFILER_STOP();

    if(! visible_faces_count) [[unlikely]]
    {
        return;
    }

    // Sort visible faces:

    FR_PROFILER_START("sort_visible_faces");

    const int* projected_zs = _visible_face_projected_zs;

    bn::sort(_visible_face_indexes, _visible_face_indexes + visible_faces_count, [projected_zs](uint16_t a, uint16_t b)
    {
        return projected_zs[a] > projected_zs[b];
    });

    FR_PROFILER_STOP();

    // Render visible faces:

    FR_PROFILER_START("render_visible_faces");

    _shape_groups.enable_drawing();

    int faces_to_render = visible_faces_count;

    if(FR_MAX_RENDER_VISIBLE_FACES > 0 && faces_to_render > FR_MAX_RENDER_VISIBLE_FACES)
    {
        faces_to_render = FR_MAX_RENDER_VISIBLE_FACES;
    }

    for(int rendered_faces = 0, visible_face_index = visible_faces_count - 1;
        rendered_faces < faces_to_render;
        ++rendered_faces, --visible_face_index)
    {
        const visible_face_info& visible_face = visible_faces[_visible_face_indexes[visible_face_index]];

        if(const valid_face_info* valid_face = visible_face.valid_face) [[likely]]
        {
            const face_3d* face = valid_face->face;
            int minimum_x = visible_face.minimum_x;
            int maximum_x = visible_face.maximum_x;
            int minimum_y = visible_face.minimum_y;
            int maximum_y = visible_face.maximum_y;

            shape_groups::hline hlines[bn::display::height()];
            bool x_outside = false;

            if(minimum_x < 0)
            {
                minimum_x = 0;
                x_outside = true;
            }

            if(maximum_x > display_width - 1)
            {
                maximum_x = display_width - 1;
                x_outside = true;
            }

            if(minimum_y != maximum_y) [[likely]]
            {
                int y = minimum_y;

                if(minimum_y < 0)
                {
                    minimum_y = 0;
                }

                if(maximum_y > display_height - 1)
                {
                    maximum_y = display_height - 1;
                }

                vertex_2d vertices[4];
                const point_2d* projected_vertices = valid_face->projected_vertices;
                const point_2d& pv0 = projected_vertices[face->first_vertex_index()];
                vertices[0].x = pv0.x;
                vertices[0].y = pv0.y;
                vertices[0].next = &vertices[1];

                const point_2d& pv1 = projected_vertices[face->second_vertex_index()];
                vertices[1].x = pv1.x;
                vertices[1].y = pv1.y;
                vertices[1].prev = &vertices[0];
                vertices[1].next = &vertices[2];

                const point_2d& pv2 = projected_vertices[face->third_vertex_index()];
                vertices[2].x = pv2.x;
                vertices[2].y = pv2.y;
                vertices[2].prev = &vertices[1];

                if(face->triangle())
                {
                    vertices[0].prev = &vertices[2];
                    vertices[2].next = &vertices[0];
                }
                else
                {
                    vertices[0].prev = &vertices[3];
                    vertices[2].next = &vertices[3];

                    const point_2d& pv3 = projected_vertices[face->fourth_vertex_index()];
                    vertices[3].x = pv3.x;
                    vertices[3].y = pv3.y;
                    vertices[3].prev = &vertices[2];
                    vertices[3].next = &vertices[0];
                }

                vertex_2d& top_vertex = vertices[visible_face.top_index];
                vertex_2d* left_top = &top_vertex;
                vertex_2d* right_top = &top_vertex;
                vertex_2d* left_bottom = top_vertex.next;
                vertex_2d* right_bottom = top_vertex.prev;

                while(left_top->y == left_bottom->y) [[unlikely]]
                {
                    left_top = left_bottom;
                    left_bottom = left_bottom->next;
                }

                while(right_top->y == right_bottom->y) [[unlikely]]
                {
                    right_top = right_bottom;
                    right_bottom = right_bottom->prev;
                }

                fixed xl = left_top->x;
                fixed xr = right_top->x;

                fixed left_delta = unsafe_unsigned_lut_division<fixed_precision>(
                            left_bottom->x - left_top->x, left_bottom->y - left_top->y);
                fixed right_delta = unsafe_unsigned_lut_division<fixed_precision>(
                            right_bottom->x - right_top->x, right_bottom->y - right_top->y);

                while(true)
                {
                    int left_bottom_y = left_bottom->y;
                    int right_bottom_y = right_bottom->y;
                    int bottom_y = left_bottom_y;

                    if(right_bottom_y < bottom_y)
                    {
                        bottom_y = right_bottom_y;
                    }

                    if(maximum_y < bottom_y)
                    {
                        bottom_y = maximum_y;
                    }

                    if(y < 0)
                    {
                        int invalid_bottom_y = bottom_y;

                        if(invalid_bottom_y > -1)
                        {
                            invalid_bottom_y = -1;
                        }

                        while(y <= invalid_bottom_y)
                        {
                            xl += left_delta;
                            xr += right_delta;
                            ++y;
                        }
                    }

                    while(y <= bottom_y)
                    {
                        int hline_xl = xl.shift_integer();
                        int hline_xr = xr.shift_integer();

                        if(hline_xl > hline_xr)
                        {
                            int temp = hline_xl;
                            hline_xl = hline_xr;
                            hline_xr = temp;
                        }

                        hlines[y] = { hline_xl, hline_xr };
                        xl += left_delta;
                        xr += right_delta;
                        ++y;
                    }

                    if(y <= maximum_y)
                    {
                        if(bottom_y == left_bottom_y)
                        {
                            left_top = left_bottom;
                            left_bottom = left_bottom->next;

                            int delta_y = left_bottom->y - left_top->y;

                            if(delta_y <= 0) [[unlikely]]
                            {
                                left_top = left_bottom;
                                left_bottom = left_bottom->next;
                                delta_y = left_bottom->y - left_top->y;
                            }

                            left_delta = unsafe_unsigned_lut_division<fixed_precision>(
                                        left_bottom->x - left_top->x, delta_y);
                            xl = left_top->x + left_delta;
                        }

                        if(bottom_y == right_bottom_y)
                        {
                            right_top = right_bottom;
                            right_bottom = right_bottom->prev;

                            int delta_y = right_bottom->y - right_top->y;

                            if(delta_y <= 0) [[unlikely]]
                            {
                                right_top = right_bottom;
                                right_bottom = right_bottom->prev;
                                delta_y = right_bottom->y - right_top->y;
                            }

                            right_delta = unsafe_unsigned_lut_division<fixed_precision>(
                                        right_bottom->x - right_top->x, delta_y);
                            xr = right_top->x + right_delta;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else
            {
                int hline_xl = minimum_x;
                int hline_xr = maximum_x;

                if(hline_xl > hline_xr)
                {
                    int temp = hline_xl;
                    hline_xl = hline_xr;
                    hline_xr = temp;
                }

                hlines[minimum_y] = { hline_xl, hline_xr };
            }

            int width = maximum_x - minimum_x + 1;
            _shape_groups.add_hlines(unsigned(minimum_y), unsigned(maximum_y), width, x_outside,
                                     face->color_index(), face->shading(), hlines);
        }
        else
        {
            int sprite_px_size = visible_face.maximum_y ? visible_face.maximum_y : 64;
            int minimum_y = visible_face.top_index;
            int maximum_y = minimum_y + sprite_px_size;

            if(minimum_y < 0)
            {
                minimum_y = 0;
            }
            else if(maximum_y > display_height - 1)
            {
                maximum_y = display_height - 1;
            }

            uint16_t attr0 = visible_face.minimum_x;
            uint16_t attr1 = visible_face.maximum_x;
            uint16_t attr2 = visible_face.minimum_y;
            _shape_groups.add_sprite(unsigned(minimum_y), unsigned(maximum_y), attr0, attr1, attr2);
        }
    }

    FR_PROFILER_STOP();
}

}
