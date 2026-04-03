#include "room_renderer.h"

#include "bn_profiler.h"
#include "../../butano/butano/hw/include/bn_hw_sprites.h"

#include "fr_div_lut.h"

#if true
    #define RV_PROFILER_START BN_PROFILER_START
    #define RV_PROFILER_STOP BN_PROFILER_STOP
#else
    #define RV_PROFILER_START(id) \
        do \
        { \
        } while(false)

    #define RV_PROFILER_STOP() \
        do \
        { \
        } while(false)
#endif

namespace str::viewer
{

namespace
{
    constexpr int fixed_precision = 18;
    using fixed = bn::fixed_t<fixed_precision>;
    constexpr int split_length = 64 - 2;
    constexpr int room_back_layer_bias = 1000000;
    constexpr int room_front_layer_bias = -1000000;
    constexpr bn::fixed room_near_wall_cull_normal_y_max = bn::fixed(-0.2);
    constexpr int projected_face_min_area2 = 8;
    constexpr int div_lut_max_index = 1024 * 4 - 1;
}

auto ScanlineRenderer::_scanline_sprite_attributes(int width, int color_index, unsigned shading) const
        -> ScanlineSpriteAttributes
{
    const ColorTileIds& tile_ids = _color_tile_ids[color_index];
    uint8_t palette_id = _palette_ids[shading];

    if(width < 8)
    {
        return {
            bn::hw::sprites::second_attributes(0, bn::sprite_size::SMALL, false, false),
            bn::hw::sprites::third_attributes(tile_ids.small_tiles_id, palette_id, 3),
            1,
            bn::display::width()
        };
    }

    if(width < 16)
    {
        return {
            bn::hw::sprites::second_attributes(0, bn::sprite_size::NORMAL, false, false),
            bn::hw::sprites::third_attributes(tile_ids.normal_tiles_id, palette_id, 3),
            1,
            bn::display::width()
        };
    }

    if(width < 32)
    {
        return {
            bn::hw::sprites::second_attributes(0, bn::sprite_size::BIG, false, false),
            bn::hw::sprites::third_attributes(tile_ids.big_tiles_id, palette_id, 3),
            1,
            bn::display::width()
        };
    }

    int clipped_width = bn::min(width, bn::display::width());
    int segment_count = width > split_length ? 1 + ((clipped_width - 1) / split_length) : 1;

    return {
        bn::hw::sprites::second_attributes(0, bn::sprite_size::HUGE, false, false),
        bn::hw::sprites::third_attributes(tile_ids.huge_tiles_id, palette_id, 3),
        segment_count,
        segment_count > 1 ? split_length : bn::display::width()
    };
}

bool ScanlineRenderer::_clip_span_to_screen(int& left_x, int& right_x) const
{
    if(left_x >= bn::display::width() || right_x < 0)
    {
        return false;
    }

    if(left_x < 0)
    {
        left_x = 0;
    }

    if(right_x > bn::display::width() - 1)
    {
        right_x = bn::display::width() - 1;
    }

    return true;
}

bool ScanlineRenderer::_reserve_scanline_slots(unsigned y, int slot_count, uint16_t*& sprite_hdma_source)
{
    int used_slots = _scanline_sprite_counts[y];

    if(used_slots + slot_count > _max_hdma_sprites)
    {
        return false;
    }

    _scanline_sprite_counts[y] = used_slots + slot_count;
    sprite_hdma_source = _hdma_source + (y * _max_hdma_sprites * 4) + (used_slots * 4);
    return true;
}

void ScanlineRenderer::_write_scanline_sprite(
        uint16_t*& sprite_hdma_source, unsigned y, int attr1, int attr2, int left_x, int length)
{
    sprite_hdma_source[0] = bn::hw::sprites::first_attributes(
            int(y) - length, bn::sprite_shape::SQUARE, bn::bpp_mode::BPP_4, 0,
            true, false, false, false);
    sprite_hdma_source[1] = attr1 + left_x;
    sprite_hdma_source[2] = attr2;
}

void ScanlineRenderer::_write_hidden_scanline_sprite(uint16_t*& sprite_hdma_source)
{
    sprite_hdma_source[0] = ATTR0_HIDE;
    sprite_hdma_source[1] = 0;
    sprite_hdma_source[2] = 0;
}

void ScanlineRenderer::add_scanline_spans(
        unsigned minimum_y, unsigned maximum_y, int width, bool x_outside, int color_index,
        unsigned shading, const ScanlineSpan* scanline_spans)
{
    ScanlineSpriteAttributes sprite_attributes = _scanline_sprite_attributes(width, color_index, shading);

    for(unsigned y = minimum_y; y <= maximum_y; ++y)
    {
        int left_x = scanline_spans[y].left_x;
        int right_x = scanline_spans[y].right_x;

        if(x_outside && ! _clip_span_to_screen(left_x, right_x)) [[unlikely]]
        {
            continue;
        }

        uint16_t* sprite_hdma_source = nullptr;

        if(! _reserve_scanline_slots(y, sprite_attributes.segment_count, sprite_hdma_source)) [[unlikely]]
        {
            continue;
        }

        int segment_left_x = left_x;

        for(int segment_index = 0; segment_index < sprite_attributes.segment_count; ++segment_index)
        {
            if(segment_left_x <= right_x) [[likely]]
            {
                int length = right_x - segment_left_x;

                if(length <= 0) [[unlikely]]
                {
                    length = 1;
                }
                else if(length > sprite_attributes.segment_length_limit)
                {
                    length = sprite_attributes.segment_length_limit;
                }

                _write_scanline_sprite(
                        sprite_hdma_source, y, sprite_attributes.attr1, sprite_attributes.attr2,
                        segment_left_x, length);
            }
            else
            {
                _write_hidden_scanline_sprite(sprite_hdma_source);
            }

            segment_left_x += sprite_attributes.segment_length_limit;
            sprite_hdma_source += 4;
        }
    }
}

void ScanlineRenderer::add_sprite(unsigned minimum_y, unsigned maximum_y, uint16_t attr0, uint16_t attr1, uint16_t attr2)
{
    for(unsigned y = minimum_y; y <= maximum_y; ++y)
    {
        uint16_t* sprite_hdma_source = nullptr;

        if(! _reserve_scanline_slots(y, 1, sprite_hdma_source)) [[unlikely]]
        {
            continue;
        }

        sprite_hdma_source[0] = attr0;
        sprite_hdma_source[1] = attr1;
        sprite_hdma_source[2] = attr2;
    }
}

void ScanlineRenderer::_hide_left_scanline_sprites(const uint8_t* previous_scanline_sprite_counts)
{
    int screen_line_elements = _max_hdma_sprites * 4;

    for(int y = 0; y < bn::display::height(); ++y)
    {
        uint16_t* sprite_hdma_source = _hdma_source + (y * screen_line_elements);
        int used_slots = _scanline_sprite_counts[y];
        int previous_slots = previous_scanline_sprite_counts[y];

        for(int slot_index = used_slots; slot_index < previous_slots; ++slot_index)
        {
            sprite_hdma_source[slot_index * 4] = ATTR0_HIDE;
        }
    }
}

void Renderer::_render_frame(const Camera& camera)
{
    constexpr int display_width = bn::display::width();
    constexpr int display_height = bn::display::height();
    constexpr int near_plane = 24 * 256 * 16;

    static BN_DATA_EWRAM_BSS ScreenPoint projected_vertices[_max_vertices];
    static BN_DATA_EWRAM_BSS bool projected_vertices_valid[_max_vertices];
    static BN_DATA_EWRAM_BSS ProjectedFace valid_faces_info[_max_faces];
    static BN_DATA_EWRAM_BSS int visible_face_projected_zs[_max_faces];
    static BN_DATA_EWRAM_BSS uint8_t visible_face_indexes[_max_faces];

    fr::point_3d camera_position = camera.position();
    bn::fixed camera_yaw = camera.yaw();
    bn::fixed camera_u_x = camera.right_axis().x();
    bn::fixed camera_u_z = camera.right_axis().z();
    bn::fixed camera_v_x = camera.up_axis().x();
    bn::fixed camera_v_z = camera.up_axis().z();
    VisibleRenderItem* visible_faces = _visible_render_items;
    int visible_faces_count = 0;
    bool geometry_cache_hit = false;

    auto project_screen_point = [&](const fr::point_3d& world_point, ScreenPoint& projected_point)
    {
        bn::fixed view_y = world_point.y() - camera_position.y();
        int camera_depth = -view_y.data();
        int div_lut_index = camera_depth >> 10;

        if(camera_depth < near_plane || div_lut_index > div_lut_max_index)
        {
            return false;
        }

        bn::fixed view_x = (world_point.x() - camera_position.x()) / 16;
        bn::fixed view_z = (world_point.z() - camera_position.z()) / 16;
        int projected_x = (view_x.unsafe_multiplication(camera_u_x) + view_z.unsafe_multiplication(camera_u_z)).data();
        int projected_y = -(view_x.unsafe_multiplication(camera_v_x) + view_z.unsafe_multiplication(camera_v_z)).data();
        int scale = int((fr::div_lut_ptr[div_lut_index] << (focal_length_shift - 8)) >> 6);

        projected_point = {
            int16_t(((projected_x * scale) >> 16) + (display_width / 2)),
            int16_t(((projected_y * scale) >> 16) + (display_height / 2))
        };
        return true;
    };

    auto face_vertices_are_visible = [](const fr::face_3d& face, const bool* vertices_valid)
    {
        return vertices_valid[face.first_vertex_index()] &&
               vertices_valid[face.second_vertex_index()] &&
               vertices_valid[face.third_vertex_index()] &&
               vertices_valid[face.fourth_vertex_index()];
    };

    if(_geometry_cache_valid && _cached_models_revision == _models_revision &&
       _cached_camera_position == camera_position && _cached_camera_yaw == camera_yaw)
    {
        int cached_model_index = 0;
        geometry_cache_hit = true;

        for(const Model& model : _models_list)
        {
            if(cached_model_index >= _cached_models_count ||
               _cached_models[cached_model_index] != &model ||
               _cached_model_versions[cached_model_index] != model.version())
            {
                geometry_cache_hit = false;
                break;
            }

            ++cached_model_index;
        }

        if(geometry_cache_hit && cached_model_index != _cached_models_count)
        {
            geometry_cache_hit = false;
        }
    }

    if(! geometry_cache_hit)
    {
        int global_vertex_index = 0;
        int valid_faces_count = 0;

        RV_PROFILER_START("dynamic_project");

        for(Model& model : _models_list)
        {
            const fr::model_3d_item& model_item = model.item();
            const fr::vertex_3d* model_vertices = model_item.vertices().data();
            ScreenPoint* model_projected_vertices = projected_vertices + global_vertex_index;
            bool* model_projected_vertices_valid = projected_vertices_valid + global_vertex_index;
            int model_vertices_count = model_item.vertices().size();
            bool model_double_sided = model.double_sided();

            for(int index = 0; index < model_vertices_count; ++index)
            {
                fr::point_3d model_point = model.transform(model_vertices[index]);
                model_projected_vertices_valid[index] =
                    project_screen_point(model_point, model_projected_vertices[index]);
            }

            const fr::face_3d* model_faces = model_item.faces().data();
            int model_faces_count = model_item.faces().size();

            for(int index = model_faces_count - 1; index >= 0; --index)
            {
                const fr::face_3d& face = model_faces[index];

                if(! face_vertices_are_visible(face, model_projected_vertices_valid))
                {
                    continue;
                }

                fr::point_3d centroid = model.transform(face.centroid());
                fr::point_3d normal = model.rotate(face.normal());
                fr::point_3d vr = centroid - camera_position;
                bool front_facing = vr.safe_dot_product(normal) < 0;
                int color_index = face.color_index();
                bool room_floor_surface = color_index >= 0 && color_index <= 5;
                bool room_shell_surface = color_index >= 6 && color_index <= 8;
                bool room_main_wall_surface = color_index == 6;
                bool room_perspective_mode = model.layering_mode() == Model::LayeringMode::room_perspective;
                bool room_floor_only_mode = model.layering_mode() == Model::LayeringMode::room_floor_only;
                bool near_shell_surface = room_perspective_mode && room_shell_surface &&
                                          normal.y() < room_near_wall_cull_normal_y_max;
                bool render_face = false;

                if(room_floor_surface)
                {
                    render_face = (room_perspective_mode || room_floor_only_mode) && front_facing;
                }
                else if(room_shell_surface && room_floor_only_mode)
                {
                    render_face = false;
                }
                else if(near_shell_surface)
                {
                    render_face = false;
                }
                else
                {
                    bool allow_double_sided = model_double_sided;

                    if(room_shell_surface)
                    {
                        allow_double_sided = room_main_wall_surface;
                    }

                    render_face = front_facing || allow_double_sided;
                }

                if(! render_face)
                {
                    continue;
                }

                int projected_depth = -vr.y().data() + model.depth_bias();

                if(room_perspective_mode)
                {
                    projected_depth += near_shell_surface ? room_front_layer_bias : room_back_layer_bias;
                }

                valid_faces_info[valid_faces_count] = {
                    &face, model_projected_vertices, projected_depth
                };
                ++valid_faces_count;
            }

            global_vertex_index += model_vertices_count;
        }

        RV_PROFILER_STOP();

        RV_PROFILER_START("cull_valid_faces");

        for(int face_index = valid_faces_count - 1; face_index >= 0; --face_index)
        {
            const ProjectedFace& projected_face = valid_faces_info[face_index];
            const fr::face_3d* face = projected_face.face;
            const ScreenPoint* model_projected_vertices = projected_face.projected_vertices;
            const ScreenPoint& pv0 = model_projected_vertices[face->first_vertex_index()];
            const ScreenPoint& pv1 = model_projected_vertices[face->second_vertex_index()];
            const ScreenPoint& pv2 = model_projected_vertices[face->third_vertex_index()];
            const ScreenPoint& pv3 = model_projected_vertices[face->fourth_vertex_index()];
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

            if(minimum_x >= display_width || maximum_x < 0)
            {
                continue;
            }

            auto tri_area2 = [](const ScreenPoint& a, const ScreenPoint& b, const ScreenPoint& c) {
                return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
            };
            auto same_sign = [](int a, int b) {
                return (a > 0 && b > 0) || (a < 0 && b < 0);
            };

            bool stable_projection = true;

            if(face->triangle())
            {
                stable_projection = bn::abs(tri_area2(pv0, pv1, pv2)) >= projected_face_min_area2;
            }
            else
            {
                int area012 = tri_area2(pv0, pv1, pv2);
                int area023 = tri_area2(pv0, pv2, pv3);
                int edge12 = tri_area2(pv1, pv2, pv3);
                int edge23 = tri_area2(pv2, pv3, pv0);
                int edge30 = tri_area2(pv3, pv0, pv1);

                stable_projection =
                    bn::abs(area012) >= projected_face_min_area2 &&
                    bn::abs(area023) >= projected_face_min_area2 &&
                    same_sign(area012, area023) &&
                    same_sign(area012, edge12) &&
                    same_sign(edge12, edge23) &&
                    same_sign(edge23, edge30);
            }

            if(! stable_projection)
            {
                continue;
            }

            int16_t minimum_y = pv0.y;
            int16_t maximum_y = minimum_y;
            int top_vertex_index = 0;

            auto min_max_y = [&minimum_y, &maximum_y, &top_vertex_index](int index, int16_t value)
            {
                if(value < minimum_y)
                {
                    top_vertex_index = index;
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

            if(minimum_y >= display_height || maximum_y < 0)
            {
                continue;
            }

            visible_faces[visible_faces_count] = {
                &projected_face, top_vertex_index, minimum_x, maximum_x, minimum_y, maximum_y
            };
            visible_face_projected_zs[visible_faces_count] = projected_face.projected_depth;
            visible_face_indexes[visible_faces_count] = uint8_t(visible_faces_count);
            ++visible_faces_count;
        }

        RV_PROFILER_STOP();

        _cached_camera_position = camera_position;
        _cached_camera_yaw = camera_yaw;
        _cached_models_revision = _models_revision;
        _cached_geometry_visible_item_count = visible_faces_count;

        int cached_model_index = 0;
        for(const Model& model : _models_list)
        {
            _cached_models[cached_model_index] = &model;
            _cached_model_versions[cached_model_index] = model.version();
            ++cached_model_index;
        }

        _cached_models_count = cached_model_index;
        _geometry_cache_valid = true;
    }
    else
    {
        visible_faces_count = _cached_geometry_visible_item_count;

        if(! _sprites_list.empty())
        {
            for(int visible_face_index = 0; visible_face_index < visible_faces_count; ++visible_face_index)
            {
                visible_face_indexes[visible_face_index] = uint8_t(visible_face_index);
            }
        }
    }

    RV_PROFILER_START("sprites");

    for(Sprite& sprite : _sprites_list)
    {
        const fr::point_3d& sprite_position = sprite.position();
        bn::fixed vry = sprite_position.y() - camera_position.y();
        int vcz = -vry.data();
        int div_lut_index = vcz >> 10;

        if(near_plane > vcz || div_lut_index > div_lut_max_index)
        {
            continue;
        }

        SpriteItem& sprite_item = sprite.item();
        int sprite_px_size = sprite_item.width();
        int canvas_size = sprite_px_size * 2;
        bn::fixed vrx = (sprite_position.x() - camera_position.x()) / 16;
        bn::fixed vrz = (sprite_position.z() - camera_position.z()) / 16;
        int vcx = (vrx.unsafe_multiplication(camera_u_x) + vrz.unsafe_multiplication(camera_u_z)).data();
        int sprite_scale = int((fr::div_lut_ptr[div_lut_index] << (focal_length_shift - 8)) >> 3);
        int scale = sprite_scale >> 3;
        int sprite_x = ((vcx * scale) >> 16) + (display_width / 2) - sprite_px_size;

        if(sprite_x >= display_width || sprite_x + canvas_size <= 0)
        {
            continue;
        }

        int vcy = -(vrx.unsafe_multiplication(camera_v_x) + vrz.unsafe_multiplication(camera_v_z)).data();
        int sprite_y = ((vcy * scale) >> 16) + (display_height / 2) - sprite_px_size;

        if(sprite_y >= display_height || sprite_y + canvas_size <= 0)
        {
            continue;
        }

        bn::fixed affine_scale = bn::fixed::from_data(sprite_scale).unsafe_multiplication(sprite.scale());

        if(affine_scale <= 0)
        {
            continue;
        }

        int degrees = camera_yaw.shift_integer() * 360;
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
                    sprite_x, sprite_item.size(), sprite_item.affine_mat_id());
        int attr2 = bn::hw::sprites::third_attributes(
                    sprite_item.tiles_id(), sprite_item.palette_id(), 3);

        visible_faces[visible_faces_count] = {
            nullptr, sprite_y, int16_t(attr0), int16_t(attr1), int16_t(attr2), int16_t(canvas_size)
        };
        visible_face_projected_zs[visible_faces_count] = vcz;
        visible_face_indexes[visible_faces_count] = uint8_t(visible_faces_count);
        ++visible_faces_count;
    }

    RV_PROFILER_STOP();

    if(! visible_faces_count)
    {
        return;
    }

    if(! geometry_cache_hit || ! _sprites_list.empty())
    {
        RV_PROFILER_START("sort_visible_faces");

        const int* projected_zs = visible_face_projected_zs;

        bn::sort(visible_face_indexes, visible_face_indexes + visible_faces_count, [projected_zs](uint8_t a, uint8_t b)
        {
            return projected_zs[a] > projected_zs[b];
        });

        RV_PROFILER_STOP();
    }

    RV_PROFILER_START("render_visible_faces");

    _scanline_renderer.begin_frame();

    for(int visible_face_index = visible_faces_count - 1; visible_face_index >= 0; --visible_face_index)
    {
        const VisibleRenderItem& visible_face = visible_faces[visible_face_indexes[visible_face_index]];

        if(const ProjectedFace* projected_face = visible_face.projected_face)
        {
            const fr::face_3d* face = projected_face->face;
            int minimum_x = visible_face.minimum_x;
            int maximum_x = visible_face.maximum_x;
            int minimum_y = visible_face.minimum_y;
            int maximum_y = visible_face.maximum_y;

            ScanlineRenderer::ScanlineSpan hlines[bn::display::height()];
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

            if(minimum_x > maximum_x)
            {
                continue;
            }

            if(minimum_y != maximum_y)
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

                PolygonVertex vertices[4];
                const ScreenPoint* model_projected_vertices = projected_face->projected_vertices;
                const ScreenPoint& pv0 = model_projected_vertices[face->first_vertex_index()];
                vertices[0].x = pv0.x;
                vertices[0].y = pv0.y;
                vertices[0].next = &vertices[1];

                const ScreenPoint& pv1 = model_projected_vertices[face->second_vertex_index()];
                vertices[1].x = pv1.x;
                vertices[1].y = pv1.y;
                vertices[1].prev = &vertices[0];
                vertices[1].next = &vertices[2];

                const ScreenPoint& pv2 = model_projected_vertices[face->third_vertex_index()];
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

                    const ScreenPoint& pv3 = model_projected_vertices[face->fourth_vertex_index()];
                    vertices[3].x = pv3.x;
                    vertices[3].y = pv3.y;
                    vertices[3].prev = &vertices[2];
                    vertices[3].next = &vertices[0];
                }

                PolygonVertex& top_vertex = vertices[visible_face.top_vertex_index];
                PolygonVertex* left_top = &top_vertex;
                PolygonVertex* right_top = &top_vertex;
                PolygonVertex* left_bottom = top_vertex.next;
                PolygonVertex* right_bottom = top_vertex.prev;

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

                fixed left_delta = fr::unsafe_unsigned_lut_division<fixed_precision>(
                            left_bottom->x - left_top->x, left_bottom->y - left_top->y);
                fixed right_delta = fr::unsafe_unsigned_lut_division<fixed_precision>(
                            right_bottom->x - right_top->x, right_bottom->y - right_top->y);

                while(true)
                {
                    int left_bottom_y = left_bottom->y;
                    int right_bottom_y = right_bottom->y;
                    int bottom_y = bn::min(left_bottom_y, right_bottom_y);

                    if(maximum_y < bottom_y)
                    {
                        bottom_y = maximum_y;
                    }

                    if(y < 0)
                    {
                        int invalid_bottom_y = bn::min(bottom_y, -1);

                        while(y <= invalid_bottom_y)
                        {
                            xl += left_delta;
                            xr += right_delta;
                            ++y;
                        }
                    }

                    while(y <= bottom_y)
                    {
                        if(y == left_bottom->y)
                        {
                            xl = left_bottom->x;
                        }

                        if(y == right_bottom->y)
                        {
                            xr = right_bottom->x;
                        }

                        int hline_xl = xl.shift_integer();
                        int hline_xr = xr.shift_integer();

                        if(hline_xl > hline_xr)
                        {
                            bn::swap(hline_xl, hline_xr);
                        }

                        --hline_xl;
                        ++hline_xr;
                        hlines[y] = { hline_xl, hline_xr };
                        xl += left_delta;
                        xr += right_delta;
                        ++y;
                    }

                    if(y > maximum_y)
                    {
                        break;
                    }

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

                        left_delta = fr::unsafe_unsigned_lut_division<fixed_precision>(
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

                        right_delta = fr::unsafe_unsigned_lut_division<fixed_precision>(
                                    right_bottom->x - right_top->x, delta_y);
                        xr = right_top->x + right_delta;
                    }
                }
            }
            else
            {
                int hline_xl = minimum_x;
                int hline_xr = maximum_x;

                if(hline_xl > hline_xr)
                {
                    bn::swap(hline_xl, hline_xr);
                }

                --hline_xl;
                ++hline_xr;
                hlines[minimum_y] = { hline_xl, hline_xr };
            }

            int width = maximum_x - minimum_x + 3;
            _scanline_renderer.add_scanline_spans(unsigned(minimum_y), unsigned(maximum_y), width, x_outside,
                                                  face->color_index(), face->shading(), hlines);
        }
        else
        {
            int minimum_y = visible_face.top_vertex_index;
            int maximum_y = minimum_y + visible_face.maximum_y - 1;

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
            _scanline_renderer.add_sprite(unsigned(minimum_y), unsigned(maximum_y), attr0, attr1, attr2);
        }
    }

    RV_PROFILER_STOP();
}

}
