#include "private/viewer/str_room_renderer.h"
#include "bn_assert.h"
#include "bn_hdma.h"
#include "bn_math.h"
#include "bn_memory.h"
#include "bn_profiler.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprites.h"
#include "bn_hw_sprites.h"
#include "fr_sin_cos.h"
#include "bn_sprite_tiles_items_shape_group_texture_1_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_1_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_1_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_1_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_2_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_2_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_2_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_2_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_3_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_3_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_3_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_3_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_4_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_4_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_4_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_4_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_5_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_5_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_5_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_5_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_6_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_6_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_6_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_6_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_7_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_7_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_7_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_7_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_8_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_8_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_8_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_8_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_9_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_9_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_9_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_9_64.h"
#include "bn_sprite_tiles_items_shape_group_texture_10_8.h"
#include "bn_sprite_tiles_items_shape_group_texture_10_16.h"
#include "bn_sprite_tiles_items_shape_group_texture_10_32.h"
#include "bn_sprite_tiles_items_shape_group_texture_10_64.h"
namespace str::viewer
{
namespace
{
    struct ColorTileItemSet
    {
        const bn::sprite_tiles_item* small;
        const bn::sprite_tiles_item* normal;
        const bn::sprite_tiles_item* big;
        const bn::sprite_tiles_item* huge;
    };
    constexpr ColorTileItemSet color_tile_item_sets[] = {
        {
            &bn::sprite_tiles_items::shape_group_texture_1_8,
            &bn::sprite_tiles_items::shape_group_texture_1_16,
            &bn::sprite_tiles_items::shape_group_texture_1_32,
            &bn::sprite_tiles_items::shape_group_texture_1_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_2_8,
            &bn::sprite_tiles_items::shape_group_texture_2_16,
            &bn::sprite_tiles_items::shape_group_texture_2_32,
            &bn::sprite_tiles_items::shape_group_texture_2_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_3_8,
            &bn::sprite_tiles_items::shape_group_texture_3_16,
            &bn::sprite_tiles_items::shape_group_texture_3_32,
            &bn::sprite_tiles_items::shape_group_texture_3_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_4_8,
            &bn::sprite_tiles_items::shape_group_texture_4_16,
            &bn::sprite_tiles_items::shape_group_texture_4_32,
            &bn::sprite_tiles_items::shape_group_texture_4_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_5_8,
            &bn::sprite_tiles_items::shape_group_texture_5_16,
            &bn::sprite_tiles_items::shape_group_texture_5_32,
            &bn::sprite_tiles_items::shape_group_texture_5_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_6_8,
            &bn::sprite_tiles_items::shape_group_texture_6_16,
            &bn::sprite_tiles_items::shape_group_texture_6_32,
            &bn::sprite_tiles_items::shape_group_texture_6_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_7_8,
            &bn::sprite_tiles_items::shape_group_texture_7_16,
            &bn::sprite_tiles_items::shape_group_texture_7_32,
            &bn::sprite_tiles_items::shape_group_texture_7_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_8_8,
            &bn::sprite_tiles_items::shape_group_texture_8_16,
            &bn::sprite_tiles_items::shape_group_texture_8_32,
            &bn::sprite_tiles_items::shape_group_texture_8_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_9_8,
            &bn::sprite_tiles_items::shape_group_texture_9_16,
            &bn::sprite_tiles_items::shape_group_texture_9_32,
            &bn::sprite_tiles_items::shape_group_texture_9_64
        },
        {
            &bn::sprite_tiles_items::shape_group_texture_10_8,
            &bn::sprite_tiles_items::shape_group_texture_10_16,
            &bn::sprite_tiles_items::shape_group_texture_10_32,
            &bn::sprite_tiles_items::shape_group_texture_10_64
        }
    };
    constexpr bn::color brightness_color(bn::color color, int brightness)
    {
        int red = (color.red() * brightness) / 32;
        int green = (color.green() * brightness) / 32;
        int blue = (color.blue() * brightness) / 32;
        return bn::color(red + (green << 5) + (blue << 10));
    }
}
Camera::Camera() :
    _position(0, 256, 0)
{
    set_yaw(0);
}
void Camera::set_position(const fr::point_3d& position)
{
    BN_ASSERT(position.y() >= 2, "Invalid y: ", position.y());
    _position = position;
}
void Camera::set_yaw(bn::fixed yaw)
{
    if(yaw >= 0x10000)
    {
        yaw -= 0x10000;
    }
    else if(yaw < 0)
    {
        yaw += 0x10000;
    }
    BN_ASSERT(yaw >= 0 && yaw < 0x10000, "Invalid yaw: ", yaw);
    int angle = yaw.shift_integer();
    bn::fixed s = fr::sin(angle);
    bn::fixed c = fr::cos(angle);
    _yaw = yaw;
    _right_axis.set_x(c);
    _right_axis.set_z(s);
    _up_axis.set_x(s);
    _up_axis.set_z(-c);
}
void Model::set_rotation_matrix(
    bn::fixed xx, bn::fixed xy, bn::fixed xz,
    bn::fixed yx, bn::fixed yy, bn::fixed yz,
    bn::fixed zx, bn::fixed zy, bn::fixed zz)
{
    _xx = xx;
    _xy = xy;
    _xz = xz;
    _yx = yx;
    _yy = yy;
    _yz = yz;
    _zx = zx;
    _zy = zy;
    _zz = zz;
    _xx_xy = xx.unsafe_multiplication(xy);
    _yx_yy = yx.unsafe_multiplication(yy);
    _zx_zy = zx.unsafe_multiplication(zy);
    _touch();
}
SpriteItem::SpriteItem(const bn::sprite_item& item, int graphics_index) :
    _shape_size(item.shape_size()),
    _tiles(item.tiles_item().create_tiles(graphics_index)),
    _palette(item.palette_item().create_palette()),
    _affine_mat(bn::sprite_affine_mat_ptr::create())
{
    BN_ASSERT(_shape_size.shape() == bn::sprite_shape::SQUARE, "Invalid shape");
    BN_ASSERT((_shape_size.width() == 32 || _shape_size.width() == 64) &&
              _shape_size.width() == item.shape_size().height(), "Invalid shape size");
    _tiles_id = _tiles.id();
    _palette_id = _palette.id();
    _affine_mat_id = _affine_mat.id();
}
ScanlineRenderer::ScanlineRenderer()
{
    for(int index = 0; index < _hdma_source_size; index += 4)
    {
        bn::hw::sprites::hide(_hdma_source_a[index]);
        bn::hw::sprites::hide(_hdma_source_b[index]);
    }
}
void ScanlineRenderer::load_colors(const bn::span<const bn::color>& colors)
{
    int colors_count = colors.size();
    BN_ASSERT(colors_count <= fr::face_3d::max_colors, "Invalid colors count: ", colors_count);
    if(! colors_count)
    {
        _color_tiles.clear();
        _palettes.clear();
        return;
    }
    int current_color_tiles_count = _color_tiles.size();
    bool reload_palettes = false;
    if(current_color_tiles_count < colors_count)
    {
        reload_palettes = true;
        for(int index = current_color_tiles_count; index < colors_count; ++index)
        {
            const ColorTileItemSet& tile_items = color_tile_item_sets[index];
            _color_tiles.emplace_back(*tile_items.small, *tile_items.normal, *tile_items.big, *tile_items.huge);
            _color_tile_ids[index].load(_color_tiles.back());
        }
    }
    else
    {
        if(current_color_tiles_count > colors_count)
        {
            _color_tiles.shrink(colors_count);
        }
        reload_palettes = colors != bn::span<const bn::color>(_colors, colors_count);
    }
    if(! reload_palettes)
    {
        return;
    }
    bn::color palettes_colors[_max_palettes][16] = {};
    for(int color_index = 0; color_index < colors_count; ++color_index)
    {
        _colors[color_index] = colors[color_index];
        int palette_color_index = color_index + 1;
        int brightness = 25;
        for(bn::color* palette_colors : palettes_colors)
        {
            palette_colors[palette_color_index] = brightness_color(colors[color_index], brightness);
            ++brightness;
        }
    }
    if(_palettes.empty())
    {
        for(int palette_index = 0; palette_index < _max_palettes; ++palette_index)
        {
            bn::sprite_palette_item palette_item(palettes_colors[palette_index], bn::bpp_mode::BPP_4);
            bn::sprite_palette_ptr palette = palette_item.create_new_palette();
            _palette_ids[palette_index] = uint8_t(palette.id());
            _palettes.push_back(bn::move(palette));
        }
    }
    else
    {
        for(int palette_index = 0; palette_index < _max_palettes; ++palette_index)
        {
            bn::sprite_palette_item palette_item(palettes_colors[palette_index], bn::bpp_mode::BPP_4);
            _palettes[palette_index].set_colors(palette_item);
        }
    }
}
void ScanlineRenderer::commit_frame()
{
    if(! _frame_active)
    {
        _stop_hdma();
        return;
    }
    uint16_t* hdma_source = _hdma_source;
    _frame_active = false;
    if(hdma_source == _hdma_source_a)
    {
        _hide_left_scanline_sprites(_previous_scanline_sprite_counts_a);
    }
    else
    {
        _hide_left_scanline_sprites(_previous_scanline_sprite_counts_b);
    }
    int scanline_elements = _max_hdma_sprites * 4;
    int hdma_source_size = bn::display::height() * scanline_elements;
    bn::memory::copy(hdma_source[0], scanline_elements, hdma_source[hdma_source_size]);
    bn::span<const uint16_t> hdma_source_ref(hdma_source + scanline_elements, hdma_source_size);
    bn::hdma::start(hdma_source_ref, *bn::hw::sprites::first_attributes_register(_oam_start_index));
    if(hdma_source == _hdma_source_a)
    {
        bn::memory::copy(*_scanline_sprite_counts, bn::display::height(), *_previous_scanline_sprite_counts_a);
        _hdma_source = _hdma_source_b;
    }
    else
    {
        bn::memory::copy(*_scanline_sprite_counts, bn::display::height(), *_previous_scanline_sprite_counts_b);
        _hdma_source = _hdma_source_a;
    }
    bn::memory::clear(bn::display::height(), *_scanline_sprite_counts);
}
void ScanlineRenderer::_stop_hdma()
{
    if(bn::hdma::running())
    {
        bn::hdma::stop();
        bn::sprites::reload();
    }
}
ScanlineRenderer::ColorTiles::ColorTiles(
        const bn::sprite_tiles_item& small_item, const bn::sprite_tiles_item& normal_item,
        const bn::sprite_tiles_item& big_item, const bn::sprite_tiles_item& huge_item) :
    small_tiles(small_item.create_tiles()),
    normal_tiles(normal_item.create_tiles()),
    big_tiles(big_item.create_tiles()),
    huge_tiles(huge_item.create_tiles())
{
}
void ScanlineRenderer::ColorTileIds::load(const ColorTiles& color_tiles)
{
    small_tiles_id = uint16_t(color_tiles.small_tiles.id());
    normal_tiles_id = uint16_t(color_tiles.normal_tiles.id());
    big_tiles_id = uint16_t(color_tiles.big_tiles.id());
    huge_tiles_id = uint16_t(color_tiles.huge_tiles.id());
}
Model& Renderer::create_model(const fr::model_3d_item& model_item)
{
    int model_vertices_count = model_item.vertices().size();
    int model_faces_count = model_item.faces().size();
    BN_ASSERT(! _models_pool.full(), "There's no space for more dynamic models");
    BN_ASSERT(model_vertices_count + _vertices_count <= _max_vertices, "There's no space for more vertices");
    BN_ASSERT(model_faces_count + _faces_count <= _max_faces, "There's no space for more faces");
    Model& result = _models_pool.create(model_item);
    _models_list.push_back(result);
    _vertices_count += model_vertices_count;
    _faces_count += model_faces_count;
    ++_models_revision;
    _geometry_cache_valid = false;
    return result;
}
void Renderer::destroy_model(Model& model)
{
    const fr::model_3d_item& model_item = model.item();
    _vertices_count -= model_item.vertices().size();
    _faces_count -= model_item.faces().size();
    _models_list.erase(model);
    _models_pool.destroy(model);
    ++_models_revision;
    _geometry_cache_valid = false;
}
Sprite& Renderer::create_sprite(SpriteItem& sprite_item)
{
    BN_ASSERT(! _sprites_pool.full(), "There's no space for more dynamic sprites");
    BN_ASSERT(1 + _vertices_count <= _max_vertices, "There's no space for more vertices");
    BN_ASSERT(1 + _faces_count <= _max_faces, "There's no space for more faces");
    Sprite& result = _sprites_pool.create(sprite_item);
    _sprites_list.push_back(result);
    ++_vertices_count;
    ++_faces_count;
    return result;
}
void Renderer::destroy_sprite(Sprite& sprite)
{
    --_vertices_count;
    --_faces_count;
    _sprites_list.erase(sprite);
    _sprites_pool.destroy(sprite);
}
void Renderer::render(const Camera& camera)
{
    _render_frame(camera);
    _scanline_renderer.commit_frame();
}
}
