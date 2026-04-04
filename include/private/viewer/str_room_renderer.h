#ifndef STR_ROOM_RENDERER_H
#define STR_ROOM_RENDERER_H
#include <cstdint>
#include "bn_color.h"
#include "bn_display.h"
#include "bn_fixed.h"
#include "bn_intrusive_list.h"
#include "bn_limits.h"
#include "bn_pool.h"
#include "bn_span.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_item.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_vector.h"
#include "fr_model_3d_item.h"
namespace str::viewer
{
inline constexpr int focal_length_shift = 8;
inline constexpr int max_dynamic_models = 3;
inline constexpr int max_sprites = 3;
class Camera
{
public:
    Camera();
    [[nodiscard]] const fr::point_3d& position() const { return _position; }
    void set_position(const fr::point_3d& position);
    [[nodiscard]] bn::fixed yaw() const { return _yaw; }
    void set_yaw(bn::fixed yaw);
    [[nodiscard]] const fr::point_3d& right_axis() const { return _right_axis; }
    [[nodiscard]] const fr::point_3d& up_axis() const { return _up_axis; }
private:
    fr::point_3d _position;
    bn::fixed _yaw;
    fr::point_3d _right_axis;
    fr::point_3d _up_axis;
};
class Model : public bn::intrusive_list_node_type
{
public:
    enum class LayeringMode
    {
        none,
        room_perspective,
        room_floor_only
    };
    explicit Model(const fr::model_3d_item& item) : _item(item) {}
    [[nodiscard]] const fr::model_3d_item& item() const { return _item; }
    [[nodiscard]] const fr::point_3d& position() const { return _position; }
    void set_position(const fr::point_3d& position)
    {
        if(_position != position)
        {
            _position = position;
            _touch();
        }
    }
    [[nodiscard]] bn::fixed scale() const { return _scale; }
    void set_scale(bn::fixed scale)
    {
        BN_ASSERT(scale > 0, "Invalid scale: ", scale);
        if(_scale != scale)
        {
            _scale = scale;
            _touch();
        }
    }
    [[nodiscard]] fr::point_3d rotate(const fr::vertex_3d& vertex) const
    {
        bn::fixed vx = vertex.point().x();
        bn::fixed vy = vertex.point().y();
        bn::fixed vz = vertex.point().z();
        bn::fixed vxy = vertex.xy();
        bn::fixed rx = (_xx + vy).safe_multiplication(_xy + vx) + vz.unsafe_multiplication(_xz) - _xx_xy - vxy;
        bn::fixed ry = (_yx + vy).safe_multiplication(_yy + vx) + vz.unsafe_multiplication(_yz) - _yx_yy - vxy;
        bn::fixed rz = (_zx + vy).safe_multiplication(_zy + vx) + vz.unsafe_multiplication(_zz) - _zx_zy - vxy;
        return fr::point_3d(rx, ry, rz);
    }
    [[nodiscard]] fr::point_3d transform(const fr::vertex_3d& vertex) const
    {
        fr::point_3d result = rotate(vertex);
        if(_scale != 1)
        {
            result.set_x(result.x().unsafe_multiplication(_scale));
            result.set_y(result.y().unsafe_multiplication(_scale));
            result.set_z(result.z().unsafe_multiplication(_scale));
        }
        return result + _position;
    }
    void set_rotation_matrix(
        bn::fixed xx, bn::fixed xy, bn::fixed xz,
        bn::fixed yx, bn::fixed yy, bn::fixed yz,
        bn::fixed zx, bn::fixed zy, bn::fixed zz);
    [[nodiscard]] int depth_bias() const { return _depth_bias; }
    void set_depth_bias(int bias)
    {
        if(_depth_bias != bias)
        {
            _depth_bias = bias;
            _touch();
        }
    }
    [[nodiscard]] LayeringMode layering_mode() const { return _layering_mode; }
    void set_layering_mode(LayeringMode layering_mode)
    {
        if(_layering_mode != layering_mode)
        {
            _layering_mode = layering_mode;
            _touch();
        }
    }
    [[nodiscard]] bool double_sided() const { return _double_sided; }
    void set_double_sided(bool double_sided)
    {
        if(_double_sided != double_sided)
        {
            _double_sided = double_sided;
            _touch();
        }
    }
    [[nodiscard]] uint16_t version() const { return _version; }
private:
    const fr::model_3d_item& _item;
    fr::point_3d _position;
    bn::fixed _scale = 1;
    bn::fixed _xx = 1;
    bn::fixed _xy;
    bn::fixed _xz;
    bn::fixed _yx;
    bn::fixed _yy = 1;
    bn::fixed _yz;
    bn::fixed _zx;
    bn::fixed _zy;
    bn::fixed _zz = 1;
    bn::fixed _xx_xy;
    bn::fixed _yx_yy;
    bn::fixed _zx_zy;
    int _depth_bias = 0;
    LayeringMode _layering_mode = LayeringMode::none;
    bool _double_sided = false;
    uint16_t _version = 1;
    void _touch()
    {
        ++_version;
        if(! _version)
        {
            _version = 1;
        }
    }
};
class SpriteItem
{
public:
    SpriteItem(const bn::sprite_item& item, int graphics_index);
    [[nodiscard]] int width() const { return _shape_size.width(); }
    [[nodiscard]] bn::sprite_size size() const { return _shape_size.size(); }
    [[nodiscard]] const bn::sprite_tiles_ptr& tiles() const { return _tiles; }
    [[nodiscard]] bn::sprite_tiles_ptr& tiles() { return _tiles; }
    [[nodiscard]] int tiles_id() const { return _tiles_id; }
    [[nodiscard]] const bn::sprite_palette_ptr& palette() const { return _palette; }
    [[nodiscard]] bn::sprite_palette_ptr& palette() { return _palette; }
    [[nodiscard]] int palette_id() const { return _palette_id; }
    [[nodiscard]] const bn::sprite_affine_mat_ptr& affine_mat() const { return _affine_mat; }
    [[nodiscard]] bn::sprite_affine_mat_ptr& affine_mat() { return _affine_mat; }
    [[nodiscard]] int affine_mat_id() const { return _affine_mat_id; }
private:
    bn::sprite_shape_size _shape_size;
    int _tiles_id;
    int _palette_id;
    int _affine_mat_id;
    bn::sprite_tiles_ptr _tiles;
    bn::sprite_palette_ptr _palette;
    bn::sprite_affine_mat_ptr _affine_mat;
};
class Sprite : public bn::intrusive_list_node_type
{
public:
    explicit Sprite(SpriteItem& item) : _item(item) {}
    [[nodiscard]] const SpriteItem& item() const { return _item; }
    [[nodiscard]] SpriteItem& item() { return _item; }
    [[nodiscard]] const fr::point_3d& position() const { return _position; }
    void set_position(const fr::point_3d& position)
    {
        _position = position;
    }
    [[nodiscard]] bn::fixed scale() const { return _scale; }
    void set_scale(bn::fixed scale)
    {
        BN_ASSERT(scale > 0, "Invalid scale: ", scale);
        _scale = scale;
    }
    [[nodiscard]] bool horizontal_flip() const { return _horizontal_flip; }
    void set_horizontal_flip(bool horizontal_flip)
    {
        _horizontal_flip = horizontal_flip;
    }
private:
    SpriteItem& _item;
    fr::point_3d _position;
    bn::fixed _scale = 1;
    bool _horizontal_flip = false;
};
class ScanlineRenderer
{
public:
    struct ScanlineSpan { int left_x; int right_x; };
    ScanlineRenderer();
    ~ScanlineRenderer() { _stop_hdma(); }
    void load_colors(const bn::span<const bn::color>& colors);
    void begin_frame() { _frame_active = true; }
    BN_CODE_IWRAM void add_scanline_spans(unsigned minimum_y, unsigned maximum_y, int width, bool x_outside,
                                          int color_index, unsigned shading, const ScanlineSpan* scanline_spans);
    BN_CODE_IWRAM void add_sprite(unsigned minimum_y, unsigned maximum_y,
                                  uint16_t attr0, uint16_t attr1, uint16_t attr2);
    void commit_frame();
private:
    struct ScanlineSpriteAttributes { int attr1; int attr2; int segment_count; int segment_length_limit; };
    static constexpr int _max_palettes = 8;
    static constexpr int _oam_start_index = 64;
    static constexpr int _max_hdma_sprites = 32;
    static constexpr int _hdma_source_size = (bn::display::height() + 1) * 4 * _max_hdma_sprites;
    class ColorTiles
    {
    public:
        bn::sprite_tiles_ptr small_tiles;
        bn::sprite_tiles_ptr normal_tiles;
        bn::sprite_tiles_ptr big_tiles;
        bn::sprite_tiles_ptr huge_tiles;
        ColorTiles(const bn::sprite_tiles_item& small_item, const bn::sprite_tiles_item& normal_item,
                   const bn::sprite_tiles_item& big_item, const bn::sprite_tiles_item& huge_item);
    };
    class ColorTileIds
    {
    public:
        uint16_t small_tiles_id;
        uint16_t normal_tiles_id;
        uint16_t big_tiles_id;
        uint16_t huge_tiles_id;
        void load(const ColorTiles& color_tiles);
    };
    alignas(int) bn::vector<ColorTiles, fr::face_3d::max_colors> _color_tiles;
    alignas(int) ColorTileIds _color_tile_ids[fr::face_3d::max_colors];
    alignas(int) bn::color _colors[fr::face_3d::max_colors];
    alignas(int) bn::vector<bn::sprite_palette_ptr, _max_palettes> _palettes;
    alignas(int) uint8_t _palette_ids[_max_palettes];
    alignas(int) uint8_t _scanline_sprite_counts[bn::display::height()] = {};
    alignas(int) uint8_t _previous_scanline_sprite_counts_a[bn::display::height()] = {};
    alignas(int) uint8_t _previous_scanline_sprite_counts_b[bn::display::height()] = {};
    alignas(int) uint16_t _hdma_source_a[_hdma_source_size];
    alignas(int) uint16_t _hdma_source_b[_hdma_source_size];
    uint16_t* _hdma_source = _hdma_source_a;
    bool _frame_active = false;
    [[nodiscard]] BN_CODE_IWRAM ScanlineSpriteAttributes _scanline_sprite_attributes(
        int width, int color_index, unsigned shading) const;
    [[nodiscard]] BN_CODE_IWRAM bool _clip_span_to_screen(int& left_x, int& right_x) const;
    [[nodiscard]] BN_CODE_IWRAM bool _reserve_scanline_slots(
        unsigned y, int slot_count, uint16_t*& sprite_hdma_source);
    BN_CODE_IWRAM void _write_scanline_sprite(
        uint16_t*& sprite_hdma_source, unsigned y, int attr1, int attr2, int left_x, int length);
    BN_CODE_IWRAM void _write_hidden_scanline_sprite(uint16_t*& sprite_hdma_source);
    BN_CODE_IWRAM void _hide_left_scanline_sprites(const uint8_t* previous_scanline_sprite_counts);
    void _stop_hdma();
};
class Renderer
{
public:
    void load_colors(const bn::span<const bn::color>& colors) { _scanline_renderer.load_colors(colors); }
    [[nodiscard]] Model& create_model(const fr::model_3d_item& model_item);
    void destroy_model(Model& model);
    [[nodiscard]] Sprite& create_sprite(SpriteItem& sprite_item);
    void destroy_sprite(Sprite& sprite);
    void render(const Camera& camera);
private:
    static constexpr int _max_vertices = 240;
    static constexpr int _max_faces = 192;
    static_assert(_max_faces <= bn::numeric_limits<uint8_t>::max());
    struct ScreenPoint { int16_t x; int16_t y; };
    struct PolygonVertex
    {
        int x;
        int y;
        PolygonVertex* prev;
        PolygonVertex* next;
    };
    struct ProjectedFace { const fr::face_3d* face; const ScreenPoint* projected_vertices; int projected_depth; };
    struct VisibleRenderItem
    {
        const ProjectedFace* projected_face;
        int top_vertex_index;
        int16_t minimum_x;
        int16_t maximum_x;
        int16_t minimum_y;
        int16_t maximum_y;
    };
    bn::pool<Model, max_dynamic_models> _models_pool;
    bn::intrusive_list<Model> _models_list;
    uint16_t _models_revision = 0;
    bn::pool<Sprite, max_sprites> _sprites_pool;
    bn::intrusive_list<Sprite> _sprites_list;
    VisibleRenderItem _visible_render_items[_max_faces];
    ScanlineRenderer _scanline_renderer;
    int _vertices_count = 0;
    int _faces_count = 0;
    bool _geometry_cache_valid = false;
    fr::point_3d _cached_camera_position;
    bn::fixed _cached_camera_yaw;
    uint16_t _cached_models_revision = 0;
    const Model* _cached_models[max_dynamic_models] = {};
    uint16_t _cached_model_versions[max_dynamic_models] = {};
    int _cached_models_count = 0;
    int _cached_geometry_visible_item_count = 0;
    BN_CODE_IWRAM void _render_frame(const Camera& camera);
};
}
#endif
