#include "str_scene_room_viewer.h"

#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_sprites.h"
#include "bn_sprite_text_generator.h"
#include "bn_sprite_affine_mat_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_affine_mat_attributes.h"
#include "bn_fixed.h"
#include "bn_math.h"
#include "bn_algorithm.h"
#include "bn_assert.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_log.h"
#include "bn_profiler.h"
#include "bn_point.h"
#include "bn_sprite_items_eris.h"
#include "bn_sprite_items_escaping_criticism_wall_bottom.h"
#include "bn_sprite_items_escaping_criticism_wall_top.h"
#include "bn_sprite_items_mr_and_mrs_andrews_wall_bottom.h"
#include "bn_sprite_items_mr_and_mrs_andrews_wall_top.h"

#include "common_variable_8x8_sprite_font.h"

#include "fr_sin_cos.h"
#include "fr_div_lut.h"
#include "fr_constants_3d.h"
#include "models/str_model_3d_items_room.h"
#include "models/str_model_3d_items_table.h"
#include "models/str_model_3d_items_chair.h"
#include "../butano/butano/hw/include/bn_hw_sprites.h"

namespace {
    constexpr int iso_phi = 6400;
    constexpr int iso_theta = 59904;
    constexpr int iso_psi = 6400;
    constexpr int NUM_ROOMS = 6;
    constexpr int QUARTER_TURN_ANGLE = 16384;
    constexpr int CORNER_TURN_DURATION_FRAMES = 20;
    constexpr bn::fixed AUTO_CORNER_AXIS_THRESHOLD = 28;
    constexpr bn::fixed AUTO_CORNER_SWITCH_ADVANTAGE = 10;
    constexpr int START_RECENTER_AUTO_PAUSE_FRAMES = 30;
    constexpr int ADJACENT_ROOM_DEPTH_BIAS = 1500000;
    constexpr int TRANSITION_DECOR_DEPTH_BIAS = ADJACENT_ROOM_DEPTH_BIAS;
    constexpr bool ENABLE_PAINTING_QUADS = true;

    struct corner_matrix
    {
        bn::fixed r00, r01, r02;
        bn::fixed r10, r11, r12;
        bn::fixed r20, r21, r22;
    };

    int normalize_angle(int angle)
    {
        return int(unsigned(angle) & 0xFFFF);
    }

    corner_matrix rotate_corner_matrix(const corner_matrix& base, int angle)
    {
        int normalized_angle = normalize_angle(angle);
        bn::fixed s = fr::sin(normalized_angle);
        bn::fixed c = fr::cos(normalized_angle);

        return {
            base.r00 * c - base.r01 * s, base.r00 * s + base.r01 * c, base.r02,
            base.r10 * c - base.r11 * s, base.r10 * s + base.r11 * c, base.r12,
            base.r20 * c - base.r21 * s, base.r20 * s + base.r21 * c, base.r22
        };
    }

    void compute_corner_matrices(corner_matrix out[4])
    {
        bn::fixed sp = fr::sin(iso_phi), cp = fr::cos(iso_phi);
        bn::fixed st = fr::sin(iso_theta), ct = fr::cos(iso_theta);
        bn::fixed ss = fr::sin(iso_psi),  cs = fr::cos(iso_psi);

        bn::fixed c0x = cp * ct;
        bn::fixed c0y = cp * st * ss - sp * cs;
        bn::fixed c0z = cp * st * cs + sp * ss;
        bn::fixed c1x = sp * ct;
        bn::fixed c1y = sp * st * ss + cp * cs;
        bn::fixed c1z = sp * st * cs - cp * ss;
        bn::fixed c2x = -st;
        bn::fixed c2y = ct * ss;
        bn::fixed c2z = ct * cs;

        out[0] = { c0x, c0y, c0z,  c1x, c1y, c1z,  c2x, c2y, c2z };
        out[1] = { c0y, -c0x, c0z,  c1y, -c1x, c1z,  c2y, -c2x, c2z };
        out[2] = { -c0x, -c0y, c0z,  -c1x, -c1y, c1z,  -c2x, -c2y, c2z };
        out[3] = { -c0y, c0x, c0z,  -c1y, c1x, c1z,  -c2y, c2x, c2z };
    }

    constexpr bn::fixed TABLE_FX = 10;
    constexpr bn::fixed TABLE_FY = 0;
    constexpr bn::fixed CHAIR_FX = 10;
    constexpr bn::fixed CHAIR_FY = 16;
    // Textured-polygon painting quads (room-local coordinates):
    // A on back wall (Y = +58), B on right wall (X = +58).
    // A (back wall): mr_and_mrs_andrews (192x112, landscape)
    // B (right wall): escaping_criticism (80x100, portrait)
    constexpr bn::fixed PAINTING_A_HALF_WIDTH = bn::fixed(14.4);
    constexpr bn::fixed PAINTING_A_HALF_HEIGHT = bn::fixed(8.4);
    constexpr bn::fixed PAINTING_B_HALF_WIDTH = bn::fixed(8.0);
    constexpr bn::fixed PAINTING_B_HALF_HEIGHT = bn::fixed(10.0);
    constexpr bn::fixed PAINTING_Z_CENTER = -24;
    constexpr bn::fixed PAINTING_WALL_INSET = bn::fixed(0.2);
    constexpr bn::fixed PAINTING_A_CENTER_X = -38;
    constexpr bn::fixed PAINTING_B_CENTER_Y = -34;

    constexpr bn::fixed TABLE_HW = 12;
    constexpr bn::fixed TABLE_HD = 8;
    constexpr bn::fixed CHAIR_HW = 6;
    constexpr bn::fixed CHAIR_HD = 6;

    constexpr bn::fixed FLOOR_MIN = -55;
    constexpr bn::fixed FLOOR_MAX = 55;
    constexpr bn::fixed DOOR_HALF_WIDTH = 10;
    // Movement is frame-compensated, so keep a lower per-step speed target.
    constexpr bn::fixed MOVE_SPEED = bn::fixed(0.5);
    constexpr int DOOR_TRANSITION_DURATION_FRAMES = 16;
    constexpr int SPAWN_CORNER_INDEX = 2;
    constexpr int SPAWN_PLAYER_DIR = 3;
    constexpr bool SPAWN_PLAYER_FACING_LEFT = false;
    constexpr int SPAWN_ROOM_ID = 1;
    constexpr int SHAPE_OAM_START_INDEX = 64;
    constexpr int SHAPE_RESERVED_HANDLES = fr::models_3d::required_reserved_sprite_handles();
    constexpr int ROOM_VIEWER_RESERVED_HANDLES = 0;
    static_assert(ROOM_VIEWER_RESERVED_HANDLES <= bn::hw::sprites::count(),
                  "Room viewer reserved handles overflow OAM");

    class textured_triangle
    {
    public:
        explicit textured_triangle(const bn::sprite_item& sprite_item) :
            _sprite(sprite_item.create_sprite(0, 0)),
            _affine_mat(bn::sprite_affine_mat_ptr::create()),
            _half_size(sprite_item.shape_size().width() / 2)
        {
            _sprite.set_affine_mat(_affine_mat);
            // Keep paintings above room HDMA sprites without needing manual OAM writes.
            _sprite.set_bg_priority(1);
            _sprite.set_visible(false);
        }

        void set_visible(bool visible)
        {
            _sprite.set_visible(visible);
        }

        void set_points(const bn::point& p0, const bn::point& p1, const bn::point& p2)
        {
            _p0 = p0;
            _p1 = p1;
            _p2 = p2;
            _update();
        }

    private:
        bn::sprite_ptr _sprite;
        bn::sprite_affine_mat_ptr _affine_mat;
        bn::point _p0;
        bn::point _p1;
        bn::point _p2;
        int _half_size;

        void _update()
        {
            switch(_half_size)
            {
                case 4:
                    _update_impl<4>();
                    break;

                case 8:
                    _update_impl<8>();
                    break;

                case 16:
                    _update_impl<16>();
                    break;

                case 32:
                    _update_impl<32>();
                    break;

                default:
                    BN_ERROR("Invalid textured triangle half size: ", _half_size);
                    break;
            }
        }

        template<int half_size>
        void _update_impl()
        {
            int x0 = _p0.x();
            int y0 = _p0.y();
            int x1 = _p1.x();
            int y1 = _p1.y();
            int x2 = _p2.x();
            int y2 = _p2.y();
            int affine_divisor = x0 * y1 - x0 * y2 - x1 * y0 + x1 * y2 + x2 * y0 - x2 * y1;

            // Keep a stable triangle winding so the transparent half of the
            // texture doesn't flip into view when projected winding changes.
            if(affine_divisor > 0)
            {
                bn::swap(x1, x2);
                bn::swap(y1, y2);
                affine_divisor = x0 * y1 - x0 * y2 - x1 * y0 + x1 * y2 + x2 * y0 - x2 * y1;
            }

            // Very thin/skewed screen-space triangles explode affine params and cause scanline artifacts.
            int abs_affine_divisor = affine_divisor >= 0 ? affine_divisor : -affine_divisor;

            if(! affine_divisor || abs_affine_divisor < 32)
            {
                set_visible(false);
                return;
            }

            int pa = (-512 * half_size * y1 + 512 * half_size * y2 + 256 * y1 - 256 * y2) / affine_divisor;
            int pb = (256 * (2 * half_size * x1 - 2 * half_size * x2 - x1 + x2)) / affine_divisor;
            int pc = (-512 * half_size * y0 + 512 * half_size * y1 + 256 * y0 - 256 * y1) / affine_divisor;
            int pd = (256 * (2 * half_size * x0 - 2 * half_size * x1 - x0 + x1)) / affine_divisor;
            bn::affine_mat_attributes attributes;
            attributes.unsafe_set_register_values(pa, pb, pc, pd);
            _affine_mat.set_attributes(attributes);

            int u0 = -half_size;
            int v0 = half_size;
            int u1 = half_size;
            int v1 = half_size;
            int u2 = half_size;
            int v2 = -half_size;

            int u0v1 = u0 * v1;
            int u0v2 = u0 * v2;
            int u1v0 = u1 * v0;
            int u1v2 = u1 * v2;
            int u2v0 = u2 * v0;
            int u2v1 = u2 * v1;

            int delta_x = (u0v1 * x2) + (u2v0 * x1) + (x0 * u1v2) - (x0 * u2v1) - (u1v0 * x2) - (u0v2 * x1);
            int delta_y = (u0v1 * y2) + (y1 * u2v0) + (y0 * u1v2) - (y0 * u2v1) - (u1v0 * y2) - (u0v2 * y1);
            int position_divisor = u0v1 + u2v0 + u1v2 - u2v1 - u1v0 - u0v2;
            _sprite.set_position(delta_x / position_divisor, delta_y / position_divisor);
            set_visible(true);
        }
    };

    class textured_quad
    {
    public:
        textured_quad(const bn::sprite_item& top_item, const bn::sprite_item& bottom_item) :
            _top(top_item),
            _bottom(bottom_item)
        {
        }

        void set_visible(bool visible)
        {
            _top.set_visible(visible);
            _bottom.set_visible(visible);
        }

        void set_points(const bn::point& p0, const bn::point& p1, const bn::point& p2, const bn::point& p3)
        {
            _top.set_points(p2, p3, p0);
            _bottom.set_points(p0, p1, p2);
        }

    private:
        textured_triangle _top;
        textured_triangle _bottom;
    };

    enum decor_flags
    {
        decor_none = 0,
        decor_table = 1 << 0,
        decor_chair = 1 << 1,
    };

    constexpr int room_decor_flags[NUM_ROOMS] = {
        decor_table | decor_chair, // Room 0: TC
        decor_none,                // Room 1: W
        decor_table,               // Room 2: T
        decor_chair,               // Room 3: CW
        decor_table,               // Room 4: T
        decor_chair,               // Room 5: C
    };

    bool overlaps_any_furniture(int room_id, bn::fixed px, bn::fixed py)
    {
        int decor = room_decor_flags[room_id];

        if(decor & decor_table)
        {
            if(bn::abs(px - TABLE_FX) < TABLE_HW && bn::abs(py - TABLE_FY) < TABLE_HD)
                return true;
        }

        if(decor & decor_chair)
        {
            if(bn::abs(px - CHAIR_FX) < CHAIR_HW && bn::abs(py - CHAIR_FY) < CHAIR_HD)
                return true;
        }

        return false;
    }

    // Building layout: 2 columns x 3 rows
    //
    //   col 0   col 1
    //  +------+------+
    //  |  R0  |  R1  |  row 0
    //  +--||--+--||--+
    //  |  R2  |  R3  |  row 1
    //  +--||--+--||--+
    //  |  R4  |  R5  |  row 2
    //  +------+------+
    //
    // Each room is 120x120, player confined to [-55,55] (FLOOR_MIN/MAX).
    // Doors trigger when player crosses ±55 near center of a wall edge.

    constexpr inline bn::color room_viewer_colors[10] = {
        bn::color(22, 16, 8),   // 0 room0_floor
        bn::color(12, 14, 22),  // 1 room1_floor
        bn::color(10, 20, 10),  // 2 room2_floor
        bn::color(22, 10, 8),   // 3 room3_floor
        bn::color(16, 10, 20),  // 4 room4_floor
        bn::color(8, 18, 20),   // 5 room5_floor
        bn::color(28, 26, 22),  // 6 wall
        bn::color(16, 22, 28),  // 7 window
        bn::color(10, 8, 6),    // 8 door_frame
        bn::color(18, 12, 6)    // 9 furniture
    };

    // Door transition info: which direction, which neighbor room
    // Doors are at the edges of local room space:
    //   North door (y near -55): go to row-1 (same column)
    //   South door (y near +55): go to row+1 (same column)
    //   West door (x near -55):  go to col-1 (same row) -- but only col0->col1 via center wall
    //   East door (x near +55):  go to col+1 (same row) -- but only col0->col1 via center wall
    //
    // In our 2x3 grid (col, row): R0=(0,0), R1=(1,0), R2=(0,1), R3=(1,1), R4=(0,2), R5=(1,2)

    int room_col(int room_id) { return room_id % 2; }
    int room_row(int room_id) { return room_id / 2; }
    int room_id_from_grid(int col, int row) { return row * 2 + col; }

    int int_abs(int value)
    {
        return value >= 0 ? value : -value;
    }

    // Runtime room vertices are emitted with ROOM_SCALE=2 (generate_level_headers.py).
    // Keep placement centers in the same scaled engine-space units.
    constexpr bn::fixed room_center_x_values[NUM_ROOMS] = {
        bn::fixed(-60), bn::fixed(75), bn::fixed(-60), bn::fixed(60), bn::fixed(-60), bn::fixed(75)
    };
    constexpr bn::fixed room_center_y_values[NUM_ROOMS] = {
        bn::fixed(-120), bn::fixed(-135), bn::fixed(0), bn::fixed(0), bn::fixed(120), bn::fixed(135)
    };
    constexpr bn::fixed room_half_extent_x_values[NUM_ROOMS] = {
        bn::fixed(60), bn::fixed(75), bn::fixed(60), bn::fixed(60), bn::fixed(60), bn::fixed(75)
    };
    constexpr bn::fixed room_half_extent_y_values[NUM_ROOMS] = {
        bn::fixed(60), bn::fixed(75), bn::fixed(60), bn::fixed(60), bn::fixed(60), bn::fixed(75)
    };

    bn::fixed room_center_x(int room_id)
    {
        return room_center_x_values[room_id];
    }

    bn::fixed room_center_y(int room_id)
    {
        return room_center_y_values[room_id];
    }

    bn::fixed room_half_extent_x(int room_id)
    {
        return room_half_extent_x_values[room_id];
    }

    bn::fixed room_half_extent_y(int room_id)
    {
        return room_half_extent_y_values[room_id];
    }

    enum class door_direction
    {
        east,
        west,
        south,
        north
    };

    int neighbor_room_for_door(int room_id, door_direction direction)
    {
        int col = room_col(room_id);
        int row = room_row(room_id);

        switch(direction)
        {
            case door_direction::east:
                return col < 1 ? room_id_from_grid(col + 1, row) : -1;

            case door_direction::west:
                return col > 0 ? room_id_from_grid(col - 1, row) : -1;

            case door_direction::south:
                return row < 2 ? room_id_from_grid(col, row + 1) : -1;

            case door_direction::north:
                return row > 0 ? room_id_from_grid(col, row - 1) : -1;

            default:
                return -1;
        }
    }

    // Keep doorway centers aligned between rooms with different shell sizes.
    bn::fixed aligned_door_center_offset(int room_id, door_direction direction)
    {
        int neighbor_room = neighbor_room_for_door(room_id, direction);
        if(neighbor_room < 0)
        {
            return 0;
        }

        if(direction == door_direction::east || direction == door_direction::west)
        {
            bn::fixed shared_world_y = (room_center_y(room_id) + room_center_y(neighbor_room)) / 2;
            return shared_world_y - room_center_y(room_id);
        }

        bn::fixed shared_world_x = (room_center_x(room_id) + room_center_x(neighbor_room)) / 2;
        return shared_world_x - room_center_x(room_id);
    }

    // Check if a door transition should occur and return the new room id (-1 if no transition)
    // Also sets new_local_x/y for the player position in the new room
    int check_door_transition(int current_room, bn::fixed local_x, bn::fixed local_y,
                              bn::fixed& new_local_x, bn::fixed& new_local_y)
    {
        int east_room = neighbor_room_for_door(current_room, door_direction::east);
        bn::fixed east_door_center = aligned_door_center_offset(current_room, door_direction::east);
        if(east_room >= 0 && local_x >= FLOOR_MAX && bn::abs(local_y - east_door_center) <= DOOR_HALF_WIDTH)
        {
            new_local_x = FLOOR_MIN;
            bn::fixed shared_world_y = room_center_y(current_room) + local_y;
            new_local_y = bn::clamp(shared_world_y - room_center_y(east_room), FLOOR_MIN, FLOOR_MAX);
            return east_room;
        }

        int west_room = neighbor_room_for_door(current_room, door_direction::west);
        bn::fixed west_door_center = aligned_door_center_offset(current_room, door_direction::west);
        if(west_room >= 0 && local_x <= FLOOR_MIN && bn::abs(local_y - west_door_center) <= DOOR_HALF_WIDTH)
        {
            new_local_x = FLOOR_MAX;
            bn::fixed shared_world_y = room_center_y(current_room) + local_y;
            new_local_y = bn::clamp(shared_world_y - room_center_y(west_room), FLOOR_MIN, FLOOR_MAX);
            return west_room;
        }

        int south_room = neighbor_room_for_door(current_room, door_direction::south);
        bn::fixed south_door_center = aligned_door_center_offset(current_room, door_direction::south);
        if(south_room >= 0 && local_y >= FLOOR_MAX && bn::abs(local_x - south_door_center) <= DOOR_HALF_WIDTH)
        {
            bn::fixed shared_world_x = room_center_x(current_room) + local_x;
            new_local_x = bn::clamp(shared_world_x - room_center_x(south_room), FLOOR_MIN, FLOOR_MAX);
            new_local_y = FLOOR_MIN;
            return south_room;
        }

        int north_room = neighbor_room_for_door(current_room, door_direction::north);
        bn::fixed north_door_center = aligned_door_center_offset(current_room, door_direction::north);
        if(north_room >= 0 && local_y <= FLOOR_MIN && bn::abs(local_x - north_door_center) <= DOOR_HALF_WIDTH)
        {
            bn::fixed shared_world_x = room_center_x(current_room) + local_x;
            new_local_x = bn::clamp(shared_world_x - room_center_x(north_room), FLOOR_MIN, FLOOR_MAX);
            new_local_y = FLOOR_MAX;
            return north_room;
        }

        return -1;
    }

    const fr::model_3d_item& get_room_model(int room_id)
    {
        switch(room_id)
        {
            case 0:  return str::model_3d_items::room_0;
            case 1:  return str::model_3d_items::room_1;
            case 2:  return str::model_3d_items::room_2;
            case 3:  return str::model_3d_items::room_3;
            case 4:  return str::model_3d_items::room_4;
            case 5:  return str::model_3d_items::room_5;
            default: return str::model_3d_items::room_0;
        }
    }

    bn::fixed corner_depth_score(int corner, bn::fixed local_x, bn::fixed local_y)
    {
        switch(corner)
        {
            case 0:
                return -local_x - local_y;

            case 1:
                return -local_x + local_y;

            case 2:
                return local_x + local_y;

            default:
                return local_x - local_y;
        }
    }

    int auto_corner_for_position(bn::fixed local_x, bn::fixed local_y, int current_corner)
    {
        bn::fixed abs_x = bn::abs(local_x);
        bn::fixed abs_y = bn::abs(local_y);

        // Only change view when player commits into a corner on both axes.
        if(abs_x < AUTO_CORNER_AXIS_THRESHOLD || abs_y < AUTO_CORNER_AXIS_THRESHOLD)
        {
            return current_corner;
        }

        int candidate_corner;
        if(local_x <= 0)
        {
            candidate_corner = local_y <= 0 ? 0 : 1;
        }
        else
        {
            candidate_corner = local_y <= 0 ? 3 : 2;
        }

        if(candidate_corner == current_corner)
        {
            return current_corner;
        }

        bn::fixed candidate_depth = corner_depth_score(candidate_corner, local_x, local_y);
        bn::fixed current_depth = corner_depth_score(current_corner, local_x, local_y);

        if(candidate_depth >= current_depth + AUTO_CORNER_SWITCH_ADVANTAGE)
        {
            return candidate_corner;
        }

        return current_corner;
    }

    int corner_turn_step_toward(int from_corner, int to_corner)
    {
        int clockwise_steps = (to_corner - from_corner + 4) % 4;
        int counterclockwise_steps = (from_corner - to_corner + 4) % 4;

        if(clockwise_steps == 0)
        {
            return 0;
        }

        return clockwise_steps <= counterclockwise_steps ? 1 : -1;
    }

    int shortest_corner_turns(int from_corner, int to_corner)
    {
        int clockwise_steps = (to_corner - from_corner + 4) % 4;

        if(clockwise_steps > 2)
        {
            return clockwise_steps - 4;
        }

        return clockwise_steps;
    }

    int eight_dir_from_player_facing(int player_dir, bool player_facing_left)
    {
        if(player_facing_left && player_dir >= 1 && player_dir <= 3)
        {
            return 8 - player_dir;
        }

        return player_dir;
    }

    int north_aligned_corner_from_facing(int current_corner, int player_dir, bool player_facing_left)
    {
        static constexpr int dir_x_from_8[] = {0, 1, 1, 1, 0, -1, -1, -1};
        static constexpr int dir_y_from_8[] = {1, 1, 0, -1, -1, -1, 0, 1};

        int eight_dir = eight_dir_from_player_facing(player_dir, player_facing_left);
        int best_corner = current_corner;
        int best_score = -9999;
        int best_turn_distance = 9999;

        for(int corner = 0; corner < 4; ++corner)
        {
            int quarter_turns = shortest_corner_turns(current_corner, corner);
            int rotated_eight_dir = int(unsigned(eight_dir - 2 * quarter_turns) & 7);
            int score = -dir_y_from_8[rotated_eight_dir] * 10 - int_abs(dir_x_from_8[rotated_eight_dir]);
            int turn_distance = int_abs(quarter_turns);

            if(score > best_score || (score == best_score && turn_distance < best_turn_distance))
            {
                best_score = score;
                best_turn_distance = turn_distance;
                best_corner = corner;
            }
        }

        return best_corner;
    }
}

namespace str
{

RoomViewer::RoomViewer() {}

void RoomViewer::_update_player_anim_tiles(fr::sprite_3d_item& item, bool moving, int dir, int elapsed_frames)
{
    constexpr int idle_bases[] = {8, 12, 16, 20, 24};
    constexpr int walk_bases[] = {28, 32, 36, 40, 44};
    constexpr int frames_per_anim = 4;
    constexpr int anim_speed = 8;

    if(_player_moving != moving || _player_dir != dir)
    {
        _player_moving = moving;
        _player_dir = dir;
        _anim_frame_counter = 0;
    }

    int base_frame = moving ? walk_bases[dir] : idle_bases[dir];
    int frame_in_anim = (_anim_frame_counter / anim_speed) % frames_per_anim;
    int tile_index = base_frame + frame_in_anim;

    item.tiles().set_tiles_ref(bn::sprite_items::eris.tiles_item(), tile_index);

    _anim_frame_counter += elapsed_frames;
}

void RoomViewer::_rotate_player_dir(fr::sprite_3d& sprite, int quarter_turns)
{
    int eight_dir = int(unsigned(eight_dir_from_player_facing(_player_dir, _player_facing_left) - 2 * quarter_turns) & 7);

    static constexpr int dir_from_8[] = {0, 1, 2, 3, 4, 3, 2, 1};
    static constexpr bool flip_from_8[] = {false, false, false, false, false, true, true, true};

    _player_dir = dir_from_8[eight_dir];
    _player_facing_left = flip_from_8[eight_dir];

    sprite.set_horizontal_flip(_player_facing_left);
}

str::Scene RoomViewer::execute()
{
    bn::bg_palettes::set_transparent_color(bn::color(2, 2, 4));
    _models.set_shape_oam_start_index(SHAPE_OAM_START_INDEX);
    int previous_reserved_sprite_handles = bn::sprites::reserved_handles_count();
    int required_reserved_sprite_handles = ROOM_VIEWER_RESERVED_HANDLES;

    if(previous_reserved_sprite_handles < required_reserved_sprite_handles)
    {
        bn::sprites::set_reserved_handles_count(required_reserved_sprite_handles);
    }

    // Keep spawn deterministic for debugging: start from the painting-facing view.
    _corner_index = SPAWN_CORNER_INDEX;
    _player_dir = SPAWN_PLAYER_DIR;
    _player_facing_left = SPAWN_PLAYER_FACING_LEFT;
    _player_moving = false;

    int current_room = SPAWN_ROOM_ID;
    _models.load_colors(bn::span<const bn::color>(room_viewer_colors, 10));

    fr::model_3d* room_models[NUM_ROOMS] = {};
    fr::model_3d* table_ptr = nullptr;
    fr::model_3d* chair_ptr = nullptr;
    fr::model_3d* transition_table_ptr = nullptr;
    fr::model_3d* transition_chair_ptr = nullptr;

    fr::point_3d room_base_pos(0, 96, 16);
    bool player_sprite_created = false;

    corner_matrix all_corners[4];
    compute_corner_matrices(all_corners);
    const corner_matrix base_corner = all_corners[0];

    int current_view_angle = -_corner_index * QUARTER_TURN_ANGLE;
    bool corner_transition_active = false;
    int corner_transition_elapsed = 0;
    int corner_transition_turns = 0;
    int corner_transition_duration_frames = CORNER_TURN_DURATION_FRAMES;
    int auto_corner_pause_frames = 0;
    int corner_transition_start_angle = current_view_angle;
    int corner_transition_target_angle = current_view_angle;
    bn::fixed world_anchor_x = room_center_x(current_room);
    bn::fixed world_anchor_y = room_center_y(current_room);
    bool door_transition_active = false;
    int door_transition_elapsed = 0;
    int door_transition_target_room = current_room;
    int door_transition_furniture_room = -1;
    bn::fixed door_transition_start_global_x = 0;
    bn::fixed door_transition_start_global_y = 0;
    bn::fixed door_transition_target_global_x = 0;
    bn::fixed door_transition_target_global_y = 0;
    bn::fixed door_transition_current_global_x = 0;
    bn::fixed door_transition_current_global_y = 0;
    bn::fixed door_transition_start_anchor_x = world_anchor_x;
    bn::fixed door_transition_start_anchor_y = world_anchor_y;
    bn::fixed door_transition_target_anchor_x = world_anchor_x;
    bn::fixed door_transition_target_anchor_y = world_anchor_y;
    bn::fixed door_transition_target_local_x = 0;
    bn::fixed door_transition_target_local_y = 0;

    auto set_model_rotation = [](fr::model_3d& m, const corner_matrix& cm) {
        m.set_rotation_matrix(
            cm.r00, cm.r01, cm.r02,
            cm.r10, cm.r11, cm.r12,
            cm.r20, cm.r21, cm.r22);
    };

    auto transform_global_point = [&](const corner_matrix& cm, bn::fixed gx, bn::fixed gy, bn::fixed gz) {
        bn::fixed rx = gx.unsafe_multiplication(cm.r00) +
                       gy.unsafe_multiplication(cm.r01) +
                       gz.unsafe_multiplication(cm.r02);
        bn::fixed ry = gx.unsafe_multiplication(cm.r10) +
                       gy.unsafe_multiplication(cm.r11) +
                       gz.unsafe_multiplication(cm.r12);
        bn::fixed rz = gx.unsafe_multiplication(cm.r20) +
                       gy.unsafe_multiplication(cm.r21) +
                       gz.unsafe_multiplication(cm.r22);
        return fr::point_3d(
            room_base_pos.x() + rx,
            room_base_pos.y() + ry,
            room_base_pos.z() + rz);
    };

    auto try_create_dynamic_model = [&](const fr::model_3d_item& model_item) -> fr::model_3d* {
        int reserved_vertices = player_sprite_created ? 0 : 1;
        int model_vertices_count = model_item.vertices().size();

        if(_models.vertices_count() + model_vertices_count + reserved_vertices > fr::models_3d::max_vertices())
        {
            return nullptr;
        }

        return &_models.create_dynamic_model(model_item);
    };

    auto sync_room_models = [&]() {
        bool changed = false;
        bool should_exist[NUM_ROOMS] = {};
        auto preferred_neighbor_room = [&](int room_id) {
            switch(_corner_index)
            {
                case 0:
                    return neighbor_room_for_door(room_id, door_direction::north);

                case 1:
                    return neighbor_room_for_door(room_id, door_direction::west);

                case 2:
                    return neighbor_room_for_door(room_id, door_direction::south);

                default:
                    return neighbor_room_for_door(room_id, door_direction::east);
            }
        };
        int current_preferred_neighbor = preferred_neighbor_room(current_room);

        for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
        {
            bool around_current = room_id == current_room || room_id == current_preferred_neighbor;
            bool around_transition_target = false;

            if(door_transition_active)
            {
                int transition_preferred_neighbor = preferred_neighbor_room(door_transition_target_room);
                around_transition_target = room_id == door_transition_target_room ||
                                           room_id == transition_preferred_neighbor;
            }

            should_exist[room_id] = around_current || around_transition_target;

            if(!should_exist[room_id] && room_models[room_id])
            {
                _models.destroy_dynamic_model(*room_models[room_id]);
                room_models[room_id] = nullptr;
                changed = true;
            }
        }

        auto ensure_room_model = [&](int room_id) {
            if(!should_exist[room_id])
            {
                return;
            }

            fr::model_3d* room_model = room_models[room_id];

            if(!room_model)
            {
                room_model = try_create_dynamic_model(get_room_model(room_id));
                room_models[room_id] = room_model;

                if(room_model)
                {
                    changed = true;
                }
                else
                {
                    return;
                }
            }

            room_model->set_mode(
                room_id == current_room ? fr::model_3d::layering_mode::room_perspective :
                                          fr::model_3d::layering_mode::none);
            room_model->set_depth_bias(
                room_id == current_room ? 0 : ADJACENT_ROOM_DEPTH_BIAS);
            room_model->set_double_sided(false);
        };

        // Current room first, then adjacent rooms while budget allows.
        ensure_room_model(current_room);

        for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
        {
            if(room_id != current_room)
            {
                ensure_room_model(room_id);
            }
        }

        return changed;
    };

    auto ensure_room_decor_models =
        [&](int room_id, int depth_bias, fr::model_3d*& room_table_ptr, fr::model_3d*& room_chair_ptr) {
        int decor = room_decor_flags[room_id];

        bool needs_table = decor & decor_table;
        if(needs_table)
        {
            if(!room_table_ptr)
            {
                room_table_ptr = try_create_dynamic_model(str::model_3d_items::table);
            }

            if(room_table_ptr)
            {
                room_table_ptr->set_mode(fr::model_3d::layering_mode::none);
                room_table_ptr->set_depth_bias(depth_bias);
                room_table_ptr->set_double_sided(false);
            }
        }
        else if(room_table_ptr)
        {
            _models.destroy_dynamic_model(*room_table_ptr);
            room_table_ptr = nullptr;
        }

        bool needs_chair = decor & decor_chair;
        if(needs_chair)
        {
            if(!room_chair_ptr)
            {
                room_chair_ptr = try_create_dynamic_model(str::model_3d_items::chair);
            }

            if(room_chair_ptr)
            {
                room_chair_ptr->set_mode(fr::model_3d::layering_mode::none);
                room_chair_ptr->set_depth_bias(depth_bias);
                room_chair_ptr->set_double_sided(false);
            }
        }
        else if(room_chair_ptr)
        {
            _models.destroy_dynamic_model(*room_chair_ptr);
            room_chair_ptr = nullptr;
        }
    };

    auto clear_room_decor_models = [&](fr::model_3d*& room_table_ptr, fr::model_3d*& room_chair_ptr) {
        if(room_chair_ptr)
        {
            _models.destroy_dynamic_model(*room_chair_ptr);
            room_chair_ptr = nullptr;
        }

        if(room_table_ptr)
        {
            _models.destroy_dynamic_model(*room_table_ptr);
            room_table_ptr = nullptr;
        }
    };

    auto ensure_decor_models = [&]() {
        ensure_room_decor_models(current_room, 0, table_ptr, chair_ptr);

        if(door_transition_furniture_room >= 0 && door_transition_furniture_room != current_room)
        {
            ensure_room_decor_models(
                door_transition_furniture_room,
                TRANSITION_DECOR_DEPTH_BIAS,
                transition_table_ptr,
                transition_chair_ptr);
        }
        else
        {
            clear_room_decor_models(transition_table_ptr, transition_chair_ptr);
            door_transition_furniture_room = -1;
        }
    };

    auto update_all_orientations = [&]() {
        corner_matrix cm = rotate_corner_matrix(base_corner, current_view_angle);

        for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
        {
            fr::model_3d* room_model = room_models[room_id];

            if(room_model)
            {
                set_model_rotation(*room_model, cm);
                room_model->set_position(
                    transform_global_point(cm,
                                           room_center_x(room_id) - world_anchor_x,
                                           room_center_y(room_id) - world_anchor_y,
                                           0));
            }
        }

        if(table_ptr)
        {
            set_model_rotation(*table_ptr, cm);
            table_ptr->set_position(
                transform_global_point(cm,
                                       room_center_x(current_room) + TABLE_FX - world_anchor_x,
                                       room_center_y(current_room) + TABLE_FY - world_anchor_y,
                                       0));
        }

        if(chair_ptr)
        {
            set_model_rotation(*chair_ptr, cm);
            chair_ptr->set_position(
                transform_global_point(cm,
                                       room_center_x(current_room) + CHAIR_FX - world_anchor_x,
                                       room_center_y(current_room) + CHAIR_FY - world_anchor_y,
                                       0));
        }

        if(door_transition_furniture_room >= 0)
        {
            if(transition_table_ptr)
            {
                set_model_rotation(*transition_table_ptr, cm);
                transition_table_ptr->set_position(
                    transform_global_point(cm,
                                           room_center_x(door_transition_furniture_room) + TABLE_FX - world_anchor_x,
                                           room_center_y(door_transition_furniture_room) + TABLE_FY - world_anchor_y,
                                           0));
            }

            if(transition_chair_ptr)
            {
                set_model_rotation(*transition_chair_ptr, cm);
                transition_chair_ptr->set_position(
                    transform_global_point(cm,
                                           room_center_x(door_transition_furniture_room) + CHAIR_FX - world_anchor_x,
                                           room_center_y(door_transition_furniture_room) + CHAIR_FY - world_anchor_y,
                                           0));
            }
        }
    };

    bool paintings_need_update = ENABLE_PAINTING_QUADS;
    auto update_orientations_and_paintings = [&]() {
        update_all_orientations();
        paintings_need_update = ENABLE_PAINTING_QUADS;
    };

    sync_room_models();
    ensure_decor_models();
    update_orientations_and_paintings();

    bn::fixed cam_dist = 274;

    auto update_camera = [&]() {
        _camera.set_position(fr::point_3d(0, cam_dist, 0));
        paintings_need_update = ENABLE_PAINTING_QUADS;
    };

    _camera.set_phi(0);
    update_camera();

    _player_fx = -20;
    _player_fy = 20;
    _player_fz = -10;

    fr::sprite_3d_item player_sprite_item(bn::sprite_items::eris, 8);
    fr::sprite_3d& player_sprite = _models.create_sprite(player_sprite_item);
    player_sprite_created = true;
    player_sprite.set_scale(2);

    update_orientations_and_paintings();

    auto update_player_sprite_position = [&]() {
        corner_matrix cm = rotate_corner_matrix(base_corner, current_view_angle);
        bn::fixed player_global_x = room_center_x(current_room) + _player_fx - world_anchor_x;
        bn::fixed player_global_y = room_center_y(current_room) + _player_fy - world_anchor_y;

        if(door_transition_active)
        {
            player_global_x = door_transition_current_global_x - world_anchor_x;
            player_global_y = door_transition_current_global_y - world_anchor_y;
        }

        player_sprite.set_position(
            transform_global_point(cm, player_global_x, player_global_y, _player_fz));
    };

    textured_quad painting_a_quad(
        bn::sprite_items::mr_and_mrs_andrews_wall_top,
        bn::sprite_items::mr_and_mrs_andrews_wall_bottom);
    textured_quad painting_b_quad(
        bn::sprite_items::escaping_criticism_wall_top,
        bn::sprite_items::escaping_criticism_wall_bottom);

    auto project_point_to_screen = [&](const fr::point_3d& point, bn::point& output) {
        constexpr int focal_length_shift = fr::constants_3d::focal_length_shift;
        constexpr int near_plane = 24 * 256 * 16;

        const fr::point_3d& camera_position = _camera.position();
        bn::fixed vry = point.y() - camera_position.y();
        int vcz = -vry.data();

        if(vcz < near_plane)
        {
            return false;
        }

        bn::fixed vrx = (point.x() - camera_position.x()) / 16;
        bn::fixed vrz = (point.z() - camera_position.z()) / 16;
        int vcx = (vrx.unsafe_multiplication(_camera.u().x()) + vrz.unsafe_multiplication(_camera.u().z())).data();
        int vcy = -(vrx.unsafe_multiplication(_camera.v().x()) + vrz.unsafe_multiplication(_camera.v().z())).data();

        int scale = int((fr::div_lut_ptr[vcz >> 10] << (focal_length_shift - 8)) >> 6);
        output.set_x((vcx * scale) >> 16);
        output.set_y((vcy * scale) >> 16);
        return true;
    };

    auto update_painting_quads = [&]() {
        corner_matrix cm = rotate_corner_matrix(base_corner, current_view_angle);
        bn::fixed room_half_x = room_half_extent_x(current_room);
        bn::fixed room_half_y = room_half_extent_y(current_room);
        bn::fixed a_center_x = PAINTING_A_CENTER_X;
        bn::fixed b_center_y = PAINTING_B_CENTER_Y;
        bn::fixed a_wall_y = room_half_y - PAINTING_WALL_INSET;
        bn::fixed b_wall_x = room_half_x - PAINTING_WALL_INSET;

        auto local_point_to_view = [&](bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) {
            return transform_global_point(cm,
                                          room_center_x(current_room) + local_x - world_anchor_x,
                                          room_center_y(current_room) + local_y - world_anchor_y,
                                          local_z);
        };

        auto local_vector_to_view = [&](bn::fixed local_x, bn::fixed local_y, bn::fixed local_z) {
            return fr::point_3d(local_x * cm.r00 + local_y * cm.r01 + local_z * cm.r02,
                                local_x * cm.r10 + local_y * cm.r11 + local_z * cm.r12,
                                local_x * cm.r20 + local_y * cm.r21 + local_z * cm.r22);
        };

        auto is_front_facing = [&](const fr::point_3d& center, const fr::point_3d& normal) {
            fr::point_3d to_camera(_camera.position().x() - center.x(),
                                   _camera.position().y() - center.y(),
                                   _camera.position().z() - center.z());
            bn::fixed facing_dot = normal.x() * to_camera.x() +
                                   normal.y() * to_camera.y() +
                                   normal.z() * to_camera.z();
            // Require a small positive margin so near-grazing views don't leak through double walls.
            return facing_dot > bn::fixed(2);
        };

        auto quad_projection_is_stable = [&](const bn::point& p0, const bn::point& p1,
                                             const bn::point& p2, const bn::point& p3) {
            constexpr int max_abs_x = 168;
            constexpr int max_abs_y = 128;
            auto point_in_safe_range = [&](const bn::point& p) {
                return int_abs(p.x()) <= max_abs_x && int_abs(p.y()) <= max_abs_y;
            };

            if(! point_in_safe_range(p0) || ! point_in_safe_range(p1) ||
               ! point_in_safe_range(p2) || ! point_in_safe_range(p3))
            {
                return false;
            }

            auto tri_area2 = [](const bn::point& a, const bn::point& b, const bn::point& c) {
                return (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
            };

            int area_bottom = tri_area2(p0, p1, p2);
            int area_top = tri_area2(p0, p2, p3);

            if(int_abs(area_bottom) < 120 || int_abs(area_top) < 120)
            {
                return false;
            }

            return (area_bottom > 0) == (area_top > 0);
        };

        bn::fixed a_z_top = PAINTING_Z_CENTER + PAINTING_A_HALF_HEIGHT;
        bn::fixed a_z_bottom = PAINTING_Z_CENTER - PAINTING_A_HALF_HEIGHT;
        bn::fixed b_z_top = PAINTING_Z_CENTER - PAINTING_B_HALF_HEIGHT;
        bn::fixed b_z_bottom = PAINTING_Z_CENTER + PAINTING_B_HALF_HEIGHT;

        // Painting A: on back wall (constant Y).
        bn::fixed a_left_x = a_center_x - PAINTING_A_HALF_WIDTH;
        bn::fixed a_right_x = a_center_x + PAINTING_A_HALF_WIDTH;

        fr::point_3d a0 = local_point_to_view(a_left_x, a_wall_y, a_z_bottom);
        fr::point_3d a1 = local_point_to_view(a_right_x, a_wall_y, a_z_bottom);
        fr::point_3d a2 = local_point_to_view(a_right_x, a_wall_y, a_z_top);
        fr::point_3d a3 = local_point_to_view(a_left_x, a_wall_y, a_z_top);

        fr::point_3d a_center = local_point_to_view(a_center_x, a_wall_y, PAINTING_Z_CENTER);
        fr::point_3d a_normal = local_vector_to_view(0, -1, 0);  // south wall interior normal

        bn::point a0_screen, a1_screen, a2_screen, a3_screen;
        bool a_visible = is_front_facing(a_center, a_normal) &&
                         project_point_to_screen(a0, a0_screen) &&
                         project_point_to_screen(a1, a1_screen) &&
                         project_point_to_screen(a2, a2_screen) &&
                         project_point_to_screen(a3, a3_screen) &&
                         quad_projection_is_stable(a0_screen, a1_screen, a2_screen, a3_screen);

        if(a_visible)
        {
            painting_a_quad.set_points(a2_screen, a3_screen, a0_screen, a1_screen);
        }
        else
        {
            painting_a_quad.set_visible(false);
        }

        // Painting B: on right wall (constant X).
        bn::fixed b_low_y = b_center_y - PAINTING_B_HALF_WIDTH;
        bn::fixed b_high_y = b_center_y + PAINTING_B_HALF_WIDTH;

        fr::point_3d b0 = local_point_to_view(b_wall_x, b_low_y, b_z_bottom);
        fr::point_3d b1 = local_point_to_view(b_wall_x, b_high_y, b_z_bottom);
        fr::point_3d b2 = local_point_to_view(b_wall_x, b_high_y, b_z_top);
        fr::point_3d b3 = local_point_to_view(b_wall_x, b_low_y, b_z_top);

        fr::point_3d b_center = local_point_to_view(b_wall_x, b_center_y, PAINTING_Z_CENTER);
        fr::point_3d b_normal = local_vector_to_view(-1, 0, 0);  // east wall interior normal

        bn::point b0_screen, b1_screen, b2_screen, b3_screen;
        bool b_visible = is_front_facing(b_center, b_normal) &&
                         project_point_to_screen(b0, b0_screen) &&
                         project_point_to_screen(b1, b1_screen) &&
                         project_point_to_screen(b2, b2_screen) &&
                         project_point_to_screen(b3, b3_screen) &&
                         quad_projection_is_stable(b0_screen, b1_screen, b2_screen, b3_screen);

        if(b_visible)
        {
            painting_b_quad.set_points(b0_screen, b1_screen, b2_screen, b3_screen);
        }
        else
        {
            painting_b_quad.set_visible(false);
        }
    };

    update_player_sprite_position();

    if(paintings_need_update)
    {
        update_painting_quads();
        paintings_need_update = false;
    }

    bn::sprite_text_generator tg(common::variable_8x8_sprite_font);
    tg.set_center_alignment();
    tg.set_bg_priority(0);

    bool text_debug_mode = !_debug_mode;
    int text_fx = 0;
    int text_fy = 0;
    int text_dir = 0;
    int text_room = -1;
    int text_corner = -1;
    int text_fps = -1;
    int text_vertices = -1;
    int current_fps = 60;
    int fps_sample_updates = 0;
    int fps_sample_refreshes = 0;

    auto refresh_overlay_text = [&](int room_id, int fps, int vertices_count) {
        _text_sprites.clear();

        auto generation_failed = [&]() {
            // Avoid hard asserts from sprite tile exhaustion when toggling overlay.
            _text_sprites.clear();
            text_debug_mode = _debug_mode;
            text_fx = _player_fx.integer();
            text_fy = _player_fy.integer();
            text_dir = _player_dir;
            text_room = room_id;
            text_corner = _corner_index;
            text_fps = fps;
            text_vertices = vertices_count;
        };

        if(_debug_mode)
        {
            int current_fx = _player_fx.integer();
            int current_fy = _player_fy.integer();

            bn::string<32> line1;
            bn::ostringstream stream1(line1);
            stream1 << "F:" << current_fx << "," << current_fy << " D:" << _player_dir;
            if(! tg.generate_optional(0, -72, line1, _text_sprites))
            {
                generation_failed();
                return;
            }

            bn::string<32> line2;
            bn::ostringstream stream2(line2);
            stream2 << "Room:" << room_id << " C:" << _corner_index;
            if(! tg.generate_optional(0, -60, line2, _text_sprites))
            {
                generation_failed();
                return;
            }

            bn::string<32> line3;
            bn::ostringstream stream3(line3);
            stream3 << "FPS:" << fps << " V:" << vertices_count << "/" << fr::models_3d::max_vertices();
            if(! tg.generate_optional(0, -48, line3, _text_sprites))
            {
                generation_failed();
                return;
            }

            #if BN_CFG_PROFILER_ENABLED
                if(! tg.generate_optional(0, 72, "SEL+START:Profiler B:Exit", _text_sprites))
                {
                    generation_failed();
                    return;
                }
            #else
                if(! tg.generate_optional(0, 72, "Profiler:OFF B:Exit", _text_sprites))
                {
                    generation_failed();
                    return;
                }
            #endif

            text_fx = current_fx;
            text_fy = current_fy;
            text_dir = _player_dir;
            text_room = room_id;
            text_corner = _corner_index;
            text_fps = fps;
            text_vertices = vertices_count;
        }
        else
        {
            if(! tg.generate_optional(0, -72, "ROOM VIEWER", _text_sprites))
            {
                generation_failed();
                return;
            }

            if(! tg.generate_optional(0, 72, "L/R:Zoom START:North B:Exit", _text_sprites))
            {
                generation_failed();
                return;
            }

            text_fps = fps;
            text_vertices = vertices_count;
        }

        text_debug_mode = _debug_mode;
    };

    BN_PROFILER_RESET();
    refresh_overlay_text(current_room, current_fps, _models.vertices_count());

    while(true)
    {
        int frame_cost = bn::core::last_missed_frames() + 1;
        fps_sample_updates += 1;
        fps_sample_refreshes += frame_cost;

        if(fps_sample_refreshes >= 60)
        {
            current_fps = (60 * fps_sample_updates + (fps_sample_refreshes / 2)) / fps_sample_refreshes;
            fps_sample_updates = 0;
            fps_sample_refreshes = 0;
        }

        int elapsed_frames = bn::clamp(frame_cost, 1, 4);

        if(bn::keypad::b_pressed())
        {
            clear_room_decor_models(table_ptr, chair_ptr);
            clear_room_decor_models(transition_table_ptr, transition_chair_ptr);

            _models.destroy_sprite(player_sprite);

            for(int room_id = 0; room_id < NUM_ROOMS; ++room_id)
            {
                if(room_models[room_id])
                {
                    _models.destroy_dynamic_model(*room_models[room_id]);
                    room_models[room_id] = nullptr;
                }
            }

            BN_PROFILER_START("room_models_update");
            _models.update(_camera);
            BN_PROFILER_STOP();

            if(bn::sprites::reserved_handles_count() != previous_reserved_sprite_handles)
            {
                bn::sprites::set_reserved_handles_count(previous_reserved_sprite_handles);
            }

            bn::core::update();
            return str::Scene::START;
        }

        #if BN_CFG_PROFILER_ENABLED
            if(_debug_mode && bn::keypad::select_held() && bn::keypad::start_pressed())
            {
                bn::profiler::show();
            }
        #endif

        if(auto_corner_pause_frames > 0)
        {
            --auto_corner_pause_frames;
        }

        if(!corner_transition_active && !door_transition_active)
        {
            if(bn::keypad::start_pressed() && !bn::keypad::select_held())
            {
                int north_corner = north_aligned_corner_from_facing(_corner_index, _player_dir, _player_facing_left);
                int turn_steps = shortest_corner_turns(_corner_index, north_corner);

                if(turn_steps != 0)
                {
                    corner_transition_active = true;
                    corner_transition_elapsed = 0;
                    corner_transition_turns = turn_steps;
                    corner_transition_duration_frames = CORNER_TURN_DURATION_FRAMES * int_abs(turn_steps);
                    corner_transition_start_angle = current_view_angle;
                    corner_transition_target_angle = current_view_angle - QUARTER_TURN_ANGLE * corner_transition_turns;
                    auto_corner_pause_frames = START_RECENTER_AUTO_PAUSE_FRAMES;
                }
            }
            else if(auto_corner_pause_frames == 0)
            {
                int desired_corner = auto_corner_for_position(_player_fx, _player_fy, _corner_index);
                int turn_step = corner_turn_step_toward(_corner_index, desired_corner);

                if(turn_step != 0)
                {
                    corner_transition_active = true;
                    corner_transition_elapsed = 0;
                    corner_transition_turns = turn_step;
                    corner_transition_duration_frames = CORNER_TURN_DURATION_FRAMES;
                    corner_transition_start_angle = current_view_angle;
                    corner_transition_target_angle = current_view_angle - QUARTER_TURN_ANGLE * corner_transition_turns;
                }
            }
        }

        {
            bn::fixed old_dist = cam_dist;
            if(bn::keypad::l_held()) cam_dist = bn::max(bn::fixed(100), cam_dist - 3);
            else if(bn::keypad::r_held()) cam_dist = bn::min(bn::fixed(500), cam_dist + 3);
            if(cam_dist != old_dist)
            {
                update_camera();
            }
        }

        if(bn::keypad::select_pressed())
        {
            _debug_mode = !_debug_mode;
        }

        if(corner_transition_active)
        {
            corner_transition_elapsed += elapsed_frames;

            if(corner_transition_elapsed >= corner_transition_duration_frames)
            {
                corner_transition_active = false;
                _corner_index = (_corner_index + corner_transition_turns + 8) % 4;
                current_view_angle = -_corner_index * QUARTER_TURN_ANGLE;
                _rotate_player_dir(player_sprite, corner_transition_turns);
            }
            else
            {
                bn::fixed transition_progress = bn::fixed(corner_transition_elapsed) / corner_transition_duration_frames;
                bn::fixed eased_progress = transition_progress * transition_progress *
                                           (bn::fixed(3) - bn::fixed(2) * transition_progress);
                bn::fixed interpolated_angle = bn::fixed(corner_transition_start_angle) +
                                               bn::fixed(corner_transition_target_angle - corner_transition_start_angle) *
                                               eased_progress;
                current_view_angle = interpolated_angle.round_integer();
            }

            update_orientations_and_paintings();
        }

        if(door_transition_active)
        {
            door_transition_elapsed += elapsed_frames;

            if(door_transition_elapsed >= DOOR_TRANSITION_DURATION_FRAMES)
            {
                door_transition_active = false;
                current_room = door_transition_target_room;
                _player_fx = door_transition_target_local_x;
                _player_fy = door_transition_target_local_y;
                world_anchor_x = door_transition_target_anchor_x;
                world_anchor_y = door_transition_target_anchor_y;
                clear_room_decor_models(table_ptr, chair_ptr);
                table_ptr = transition_table_ptr;
                chair_ptr = transition_chair_ptr;
                transition_table_ptr = nullptr;
                transition_chair_ptr = nullptr;
                door_transition_furniture_room = -1;
                sync_room_models();
                ensure_decor_models();
                update_orientations_and_paintings();
            }
            else
            {
                bn::fixed transition_progress = bn::fixed(door_transition_elapsed) / DOOR_TRANSITION_DURATION_FRAMES;
                bn::fixed eased_progress = transition_progress * transition_progress *
                                           (bn::fixed(3) - bn::fixed(2) * transition_progress);
                door_transition_current_global_x = door_transition_start_global_x +
                    (door_transition_target_global_x - door_transition_start_global_x) * eased_progress;
                door_transition_current_global_y = door_transition_start_global_y +
                    (door_transition_target_global_y - door_transition_start_global_y) * eased_progress;
                world_anchor_x = door_transition_start_anchor_x +
                    (door_transition_target_anchor_x - door_transition_start_anchor_x) * eased_progress;
                world_anchor_y = door_transition_start_anchor_y +
                    (door_transition_target_anchor_y - door_transition_start_anchor_y) * eased_progress;
                update_orientations_and_paintings();
            }
        }

        bool moving = false;
        int dir = _player_dir;
        bool facing_left = _player_facing_left;
        bn::fixed dfx = 0, dfy = 0;
        bn::fixed screen_dx = 0, screen_dy = 0;

        if(!corner_transition_active && !door_transition_active && bn::keypad::up_held())
        {
            screen_dy = -1;
            moving = true;
        }
        else if(!corner_transition_active && !door_transition_active && bn::keypad::down_held())
        {
            screen_dy = 1;
            moving = true;
        }

        if(!corner_transition_active && !door_transition_active && bn::keypad::left_held())
        {
            screen_dx = -1;
            moving = true;
        }
        else if(!corner_transition_active && !door_transition_active && bn::keypad::right_held())
        {
            screen_dx = 1;
            moving = true;
        }

        if(moving)
        {
            if(screen_dx == 0 && screen_dy == 1)       { dir = 0; facing_left = false; }
            else if(screen_dx == 0 && screen_dy == -1)  { dir = 4; facing_left = false; }
            else if(screen_dx == 1 && screen_dy == 0)   { dir = 2; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == 0)  { dir = 2; facing_left = true;  }
            else if(screen_dx == 1 && screen_dy == 1)   { dir = 1; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == 1)  { dir = 1; facing_left = true;  }
            else if(screen_dx == 1 && screen_dy == -1)  { dir = 3; facing_left = false; }
            else if(screen_dx == -1 && screen_dy == -1) { dir = 3; facing_left = true;  }

            bn::fixed speed_factor = 1;
            if(screen_dx != 0 && screen_dy != 0)
            {
                speed_factor = bn::fixed(0.707);
            }
            screen_dx *= speed_factor;
            screen_dy *= speed_factor;

            bn::fixed base_dx = screen_dx + screen_dy;
            bn::fixed base_dy = screen_dy - screen_dx;

            if(_corner_index == 0)      { dfx = base_dx;  dfy = base_dy;  }
            else if(_corner_index == 1) { dfx = base_dy;  dfy = -base_dx; }
            else if(_corner_index == 2) { dfx = -base_dx; dfy = -base_dy; }
            else if(_corner_index == 3) { dfx = -base_dy; dfy = base_dx;  }

            dfx *= MOVE_SPEED;
            dfy *= MOVE_SPEED;

            for(int step = 0; step < elapsed_frames; ++step)
            {
                bn::fixed new_fx = bn::clamp(_player_fx + dfx, FLOOR_MIN, FLOOR_MAX);
                bn::fixed new_fy = bn::clamp(_player_fy + dfy, FLOOR_MIN, FLOOR_MAX);

                if(!overlaps_any_furniture(current_room, new_fx, new_fy))
                {
                    _player_fx = new_fx;
                    _player_fy = new_fy;
                }
                else
                {
                    bn::fixed slide_fx = bn::clamp(_player_fx + dfx, FLOOR_MIN, FLOOR_MAX);
                    bn::fixed slide_fy = _player_fy;
                    if(!overlaps_any_furniture(current_room, slide_fx, slide_fy))
                    {
                        _player_fx = slide_fx;
                    }
                    else
                    {
                        slide_fx = _player_fx;
                        slide_fy = bn::clamp(_player_fy + dfy, FLOOR_MIN, FLOOR_MAX);
                        if(!overlaps_any_furniture(current_room, slide_fx, slide_fy))
                        {
                            _player_fy = slide_fy;
                        }
                    }
                }

                // Check for room transition via doors.
                bn::fixed new_local_x, new_local_y;
                int next_room = check_door_transition(current_room, _player_fx, _player_fy,
                                                      new_local_x, new_local_y);
                if(next_room >= 0 && next_room != current_room)
                {
                    door_transition_furniture_room = next_room;
                    // Preload destination-room decor before camera interpolation starts.
                    ensure_decor_models();
                    // Avoid seeing current-room decor through walls during camera interpolation.
                    clear_room_decor_models(table_ptr, chair_ptr);

                    update_orientations_and_paintings();

                    door_transition_active = true;
                    door_transition_elapsed = 0;
                    door_transition_target_room = next_room;
                    door_transition_start_global_x = room_center_x(current_room) + _player_fx;
                    door_transition_start_global_y = room_center_y(current_room) + _player_fy;
                    door_transition_target_global_x = room_center_x(next_room) + new_local_x;
                    door_transition_target_global_y = room_center_y(next_room) + new_local_y;
                    door_transition_current_global_x = door_transition_start_global_x;
                    door_transition_current_global_y = door_transition_start_global_y;
                    door_transition_start_anchor_x = world_anchor_x;
                    door_transition_start_anchor_y = world_anchor_y;
                    door_transition_target_anchor_x = room_center_x(next_room);
                    door_transition_target_anchor_y = room_center_y(next_room);
                    door_transition_target_local_x = new_local_x;
                    door_transition_target_local_y = new_local_y;
                    // Keep destination neighborhood loaded during interpolation.
                    sync_room_models();
                    break;
                }
            }
        }

        update_player_sprite_position();

        if(paintings_need_update)
        {
            update_painting_quads();
            paintings_need_update = false;
        }

        player_sprite.set_horizontal_flip(facing_left);
        _player_facing_left = facing_left;
        _update_player_anim_tiles(player_sprite_item, moving || door_transition_active, dir, elapsed_frames);

        bool text_dirty = _debug_mode != text_debug_mode;
        int current_vertices = _models.vertices_count();

        if(_debug_mode)
        {
            int current_fx = _player_fx.integer();
            int current_fy = _player_fy.integer();

            if(current_fx != text_fx || current_fy != text_fy ||
               _player_dir != text_dir || current_room != text_room || _corner_index != text_corner ||
               current_fps != text_fps || current_vertices != text_vertices)
            {
                text_dirty = true;
            }
        }

        if(text_dirty)
        {
            refresh_overlay_text(current_room, current_fps, current_vertices);
        }

        BN_PROFILER_START("room_models_update");
        _models.update(_camera);
        BN_PROFILER_STOP();
        bn::core::update();
    }
}

}
